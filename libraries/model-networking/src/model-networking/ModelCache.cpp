//
//  ModelCache.cpp
//  libraries/model-networking
//
//  Created by Zach Pomerantz on 3/15/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ModelCache.h"
#include <Finally.h>
#include <FSTReader.h>
#include "FBXReader.h"
#include "OBJReader.h"

#include <gpu/Batch.h>
#include <gpu/Stream.h>

#include <QThreadPool>

#include "ModelNetworkingLogging.h"
#include <Trace.h>
#include <StatTracker.h>

Q_LOGGING_CATEGORY(trace_resource_parse_geometry, "trace.resource.parse.geometry")

class GeometryReader;

class GeometryExtra {
public:
    const QVariantHash& mapping;
    const QUrl& textureBaseUrl;
};

QUrl resolveTextureBaseUrl(const QUrl& url, const QUrl& textureBaseUrl) {
    return textureBaseUrl.isValid() ? textureBaseUrl : url;
}

class GeometryMappingResource : public GeometryResource {
    Q_OBJECT
public:
    GeometryMappingResource(const QUrl& url) : GeometryResource(url) {};

    QString getType() const override { return "GeometryMapping"; }

    virtual void downloadFinished(const QByteArray& data) override;

private slots:
    void onGeometryMappingLoaded(bool success);

private:
    GeometryResource::Pointer _geometryResource;
    QMetaObject::Connection _connection;
};

void GeometryMappingResource::downloadFinished(const QByteArray& data) {
    PROFILE_ASYNC_BEGIN(resource_parse_geometry, "GeometryMappingResource::downloadFinished", _url.toString(),
                         { { "url", _url.toString() } });

    auto mapping = FSTReader::readMapping(data);

    QString filename = mapping.value("filename").toString();
    if (filename.isNull()) {
        qCDebug(modelnetworking) << "Mapping file" << _url << "has no \"filename\" field";
        finishedLoading(false);
    } else {
        QUrl url = _url.resolved(filename);

        QString texdir = mapping.value("texdir").toString();
        if (!texdir.isNull()) {
            if (!texdir.endsWith('/')) {
                texdir += '/';
            }
            _textureBaseUrl = resolveTextureBaseUrl(url, _url.resolved(texdir));
        }

        auto animGraphVariant = mapping.value("animGraphUrl");
        if (animGraphVariant.isValid()) {
            QUrl fstUrl(animGraphVariant.toString());
            if (fstUrl.isValid()) {
                _animGraphOverrideUrl = _url.resolved(fstUrl);
            } else {
                _animGraphOverrideUrl = QUrl();
            }
        } else {
            _animGraphOverrideUrl = QUrl();
        }

        auto modelCache = DependencyManager::get<ModelCache>();
        GeometryExtra extra{ mapping, _textureBaseUrl };

        // Get the raw GeometryResource
        _geometryResource = modelCache->getResource(url, QUrl(), &extra).staticCast<GeometryResource>();
        // Avoid caching nested resources - their references will be held by the parent
        _geometryResource->_isCacheable = false;

        if (_geometryResource->isLoaded()) {
            onGeometryMappingLoaded(!_geometryResource->getURL().isEmpty());
        } else {
            if (_connection) {
                disconnect(_connection);
            }

            _connection = connect(_geometryResource.data(), &Resource::finished,
                                  this, &GeometryMappingResource::onGeometryMappingLoaded);
        }
    }
}

void GeometryMappingResource::onGeometryMappingLoaded(bool success) {
    if (success && _geometryResource) {
        _fbxGeometry = _geometryResource->_fbxGeometry;
        _meshParts = _geometryResource->_meshParts;
        _meshes = _geometryResource->_meshes;
        _materials = _geometryResource->_materials;

        // Avoid holding onto extra references
        _geometryResource.reset();
        // Make sure connection will not trigger again
        disconnect(_connection); // FIXME Should not have to do this
    }

    PROFILE_ASYNC_END(resource_parse_geometry, "GeometryMappingResource::downloadFinished", _url.toString());
    finishedLoading(success);
}

class GeometryReader : public QRunnable {
public:
    GeometryReader(QWeakPointer<Resource>& resource, const QUrl& url, const QVariantHash& mapping,
        const QByteArray& data) :
        _resource(resource), _url(url), _mapping(mapping), _data(data) {

        DependencyManager::get<StatTracker>()->incrementStat("PendingProcessing");
    }

    virtual void run() override;

private:
    QWeakPointer<Resource> _resource;
    QUrl _url;
    QVariantHash _mapping;
    QByteArray _data;
};

void GeometryReader::run() {
    DependencyManager::get<StatTracker>()->decrementStat("PendingProcessing");
    CounterStat counter("Processing");
    PROFILE_RANGE_EX(resource_parse_geometry, "GeometryReader::run", 0xFF00FF00, 0, { { "url", _url.toString() } });
    auto originalPriority = QThread::currentThread()->priority();
    if (originalPriority == QThread::InheritPriority) {
        originalPriority = QThread::NormalPriority;
    }
    QThread::currentThread()->setPriority(QThread::LowPriority);
    Finally setPriorityBackToNormal([originalPriority]() {
        QThread::currentThread()->setPriority(originalPriority);
    });

    if (!_resource.data()) {
        qCWarning(modelnetworking) << "Abandoning load of" << _url << "; resource was deleted";
        return;
    }

    try {
        if (_data.isEmpty()) {
            throw QString("reply is NULL");
        }

        QString urlname = _url.path().toLower();
        if (!urlname.isEmpty() && !_url.path().isEmpty() &&
            (_url.path().toLower().endsWith(".fbx") || _url.path().toLower().endsWith(".obj"))) {
            FBXGeometry::Pointer fbxGeometry;

            if (_url.path().toLower().endsWith(".fbx")) {
                fbxGeometry.reset(readFBX(_data, _mapping, _url.path()));
                if (fbxGeometry->meshes.size() == 0 && fbxGeometry->joints.size() == 0) {
                    throw QString("empty geometry, possibly due to an unsupported FBX version");
                }
            } else if (_url.path().toLower().endsWith(".obj")) {
                fbxGeometry.reset(OBJReader().readOBJ(_data, _mapping, _url));
            } else {
                throw QString("unsupported format");
            }

            // Ensure the resource has not been deleted
            auto resource = _resource.toStrongRef();
            if (!resource) {
                qCWarning(modelnetworking) << "Abandoning load of" << _url << "; could not get strong ref";
            } else {
                QMetaObject::invokeMethod(resource.data(), "setGeometryDefinition",
                    Q_ARG(FBXGeometry::Pointer, fbxGeometry));
            }
        } else {
            throw QString("url is invalid");
        }
    } catch (const QString& error) {

        qCDebug(modelnetworking) << "Error parsing model for" << _url << ":" << error;

        auto resource = _resource.toStrongRef();
        if (resource) {
            QMetaObject::invokeMethod(resource.data(), "finishedLoading",
                Q_ARG(bool, false));
        }
    }
}

class GeometryDefinitionResource : public GeometryResource {
    Q_OBJECT
public:
    GeometryDefinitionResource(const QUrl& url, const QVariantHash& mapping, const QUrl& textureBaseUrl) :
        GeometryResource(url, resolveTextureBaseUrl(url, textureBaseUrl)), _mapping(mapping) {}

    QString getType() const override { return "GeometryDefinition"; }

    virtual void downloadFinished(const QByteArray& data) override;

protected:
    Q_INVOKABLE void setGeometryDefinition(FBXGeometry::Pointer fbxGeometry);

private:
    QVariantHash _mapping;
};

void GeometryDefinitionResource::downloadFinished(const QByteArray& data) {
    QThreadPool::globalInstance()->start(new GeometryReader(_self, _url, _mapping, data));
}

void GeometryDefinitionResource::setGeometryDefinition(FBXGeometry::Pointer fbxGeometry) {
    // Assume ownership of the geometry pointer
    _fbxGeometry = fbxGeometry;

    // Copy materials
    QHash<QString, size_t> materialIDAtlas;
    for (const FBXMaterial& material : _fbxGeometry->materials) {
        materialIDAtlas[material.materialID] = _materials.size();
        _materials.push_back(std::make_shared<NetworkMaterial>(material, _textureBaseUrl));
    }

    std::shared_ptr<GeometryMeshes> meshes = std::make_shared<GeometryMeshes>();
    std::shared_ptr<GeometryMeshParts> parts = std::make_shared<GeometryMeshParts>();
    int meshID = 0;
    for (const FBXMesh& mesh : _fbxGeometry->meshes) {
        // Copy mesh pointers
        meshes->emplace_back(mesh._mesh);
        int partID = 0;
        for (const FBXMeshPart& part : mesh.parts) {
            // Construct local parts
            parts->push_back(std::make_shared<MeshPart>(meshID, partID, (int)materialIDAtlas[part.materialID]));
            partID++;
        }
        meshID++;
    }
    _meshes = meshes;
    _meshParts = parts;

    finishedLoading(true);
}

ModelCache::ModelCache() {
    const qint64 GEOMETRY_DEFAULT_UNUSED_MAX_SIZE = DEFAULT_UNUSED_MAX_SIZE;
    setUnusedResourceCacheSize(GEOMETRY_DEFAULT_UNUSED_MAX_SIZE);
    setObjectName("ModelCache");
}

QSharedPointer<Resource> ModelCache::createResource(const QUrl& url, const QSharedPointer<Resource>& fallback,
    const void* extra) {
    Resource* resource = nullptr;
    if (url.path().toLower().endsWith(".fst")) {
        resource = new GeometryMappingResource(url);
    } else {
        const GeometryExtra* geometryExtra = static_cast<const GeometryExtra*>(extra);
        auto mapping = geometryExtra ? geometryExtra->mapping : QVariantHash();
        auto textureBaseUrl = geometryExtra ? geometryExtra->textureBaseUrl : QUrl();
        resource = new GeometryDefinitionResource(url, mapping, textureBaseUrl);
    }

    return QSharedPointer<Resource>(resource, &Resource::deleter);
}

GeometryResource::Pointer ModelCache::getGeometryResource(const QUrl& url, const QVariantHash& mapping, const QUrl& textureBaseUrl) {
    GeometryExtra geometryExtra = { mapping, textureBaseUrl };
    GeometryResource::Pointer resource = getResource(url, QUrl(), &geometryExtra).staticCast<GeometryResource>();
    if (resource) {
        if (resource->isLoaded() && resource->shouldSetTextures()) {
            resource->setTextures();
        }
    }
    return resource;
}

const QVariantMap Geometry::getTextures() const {
    QVariantMap textures;
    for (const auto& material : _materials) {
        for (const auto& texture : material->_textures) {
            if (texture.texture) {
                textures[texture.name] = texture.texture->getURL();
            }
        }
    }

    return textures;
}

// FIXME: The materials should only be copied when modified, but the Model currently caches the original
Geometry::Geometry(const Geometry& geometry) {
    _fbxGeometry = geometry._fbxGeometry;
    _meshes = geometry._meshes;
    _meshParts = geometry._meshParts;

    _materials.reserve(geometry._materials.size());
    for (const auto& material : geometry._materials) {
        _materials.push_back(std::make_shared<NetworkMaterial>(*material));
    }

    _animGraphOverrideUrl = geometry._animGraphOverrideUrl;
}

void Geometry::setTextures(const QVariantMap& textureMap) {
    if (_meshes->size() > 0) {
        for (auto& material : _materials) {
            // Check if any material textures actually changed
            if (std::any_of(material->_textures.cbegin(), material->_textures.cend(),
                [&textureMap](const NetworkMaterial::Textures::value_type& it) { return it.texture && textureMap.contains(it.name); })) { 

                // FIXME: The Model currently caches the materials (waste of space!)
                //        so they must be copied in the Geometry copy-ctor
                // if (material->isOriginal()) {
                //    // Copy the material to avoid mutating the cached version
                //    material = std::make_shared<NetworkMaterial>(*material);
                //}

                material->setTextures(textureMap);
                _areTexturesLoaded = false;

                // If we only use cached textures, they should all be loaded
                areTexturesLoaded();
            }
        }
    } else {
        qCWarning(modelnetworking) << "Ignoring setTextures(); geometry not ready";
    }
}

bool Geometry::areTexturesLoaded() const {
    if (!_areTexturesLoaded) {
        for (auto& material : _materials) {
            // Check if material textures are loaded
            bool materialMissingTexture = std::any_of(material->_textures.cbegin(), material->_textures.cend(),
                [](const NetworkMaterial::Textures::value_type& it) { 
                auto texture = it.texture;
                if (!texture) {
                    return false;
                }
                // Failed texture downloads need to be considered as 'loaded' 
                // or the object will never fade in
                bool finished = texture->isLoaded() || texture->isFailed();
                if (!finished) {
                    return true;
                }
                return false;
            });

            if (materialMissingTexture) {
                return false;
            }

            // If material textures are loaded, check the material translucency
            const auto albedoTexture = material->_textures[NetworkMaterial::MapChannel::ALBEDO_MAP];
            if (albedoTexture.texture && albedoTexture.texture->getGPUTexture()) {
                material->resetOpacityMap();
            }
        }

        _areTexturesLoaded = true;
    }
    return true;
}

const std::shared_ptr<const NetworkMaterial> Geometry::getShapeMaterial(int partID) const {
    if ((partID >= 0) && (partID < (int)_meshParts->size())) {
        int materialID = _meshParts->at(partID)->materialID;
        if ((materialID >= 0) && (materialID < (int)_materials.size())) {
            return _materials[materialID];
        }
    }
    return nullptr;
}

void GeometryResource::deleter() {
    resetTextures();
    Resource::deleter();
}

void GeometryResource::setTextures() {
    if (_fbxGeometry) {
        for (const FBXMaterial& material : _fbxGeometry->materials) {
            _materials.push_back(std::make_shared<NetworkMaterial>(material, _textureBaseUrl));
        }
    }
}

void GeometryResource::resetTextures() {
    _materials.clear();
}

void GeometryResourceWatcher::startWatching() {
    connect(_resource.data(), &Resource::finished, this, &GeometryResourceWatcher::resourceFinished);
    connect(_resource.data(), &Resource::onRefresh, this, &GeometryResourceWatcher::resourceRefreshed);
    if (_resource->isLoaded()) {
        resourceFinished(!_resource->getURL().isEmpty());
    }
}

void GeometryResourceWatcher::stopWatching() {
    disconnect(_resource.data(), &Resource::finished, this, &GeometryResourceWatcher::resourceFinished);
    disconnect(_resource.data(), &Resource::onRefresh, this, &GeometryResourceWatcher::resourceRefreshed);
}

void GeometryResourceWatcher::setResource(GeometryResource::Pointer resource) {
    if (_resource) {
        stopWatching();
    }
    _resource = resource;
    if (_resource) {
        if (_resource->isLoaded()) {
            _geometryRef = std::make_shared<Geometry>(*_resource);
        } else {
            startWatching();
        }
    }
}

void GeometryResourceWatcher::resourceFinished(bool success) {
    if (success) {
        _geometryRef = std::make_shared<Geometry>(*_resource);
    }
    emit finished(success);
}

void GeometryResourceWatcher::resourceRefreshed() {
    // FIXME: Model is not set up to handle a refresh
    // _instance.reset();
}

const QString NetworkMaterial::NO_TEXTURE = QString();

const QString& NetworkMaterial::getTextureName(MapChannel channel) {
    if (_textures[channel].texture) {
        return _textures[channel].name;
    }
    return NO_TEXTURE;
}

QUrl NetworkMaterial::getTextureUrl(const QUrl& baseUrl, const FBXTexture& texture) {
    if (texture.content.isEmpty()) {
        // External file: search relative to the baseUrl, in case filename is relative
        return baseUrl.resolved(QUrl(texture.filename));
    } else {
        // Inlined file: cache under the fbx file to avoid namespace clashes
        // NOTE: We cannot resolve the path because filename may be an absolute path
        assert(texture.filename.size() > 0);
        if (texture.filename.at(0) == '/') {
            return baseUrl.toString() + texture.filename;
        } else {
            return baseUrl.toString() + '/' + texture.filename;
        }
    }
}

model::TextureMapPointer NetworkMaterial::fetchTextureMap(const QUrl& baseUrl, const FBXTexture& fbxTexture,
                                                        TextureType type, MapChannel channel) {
    const auto url = getTextureUrl(baseUrl, fbxTexture);
    const auto texture = DependencyManager::get<TextureCache>()->getTexture(url, type, fbxTexture.content);
    _textures[channel] = Texture { fbxTexture.name, texture };

    auto map = std::make_shared<model::TextureMap>();
    if (texture) {
        map->setTextureSource(texture->_textureSource);
    }
    map->setTextureTransform(fbxTexture.transform);

    return map;
}

model::TextureMapPointer NetworkMaterial::fetchTextureMap(const QUrl& url, TextureType type, MapChannel channel) {
    const auto texture = DependencyManager::get<TextureCache>()->getTexture(url, type);
    _textures[channel].texture = texture;

    auto map = std::make_shared<model::TextureMap>();
    map->setTextureSource(texture->_textureSource);

    return map;
}

NetworkMaterial::NetworkMaterial(const FBXMaterial& material, const QUrl& textureBaseUrl) :
    model::Material(*material._material)
{
    _textures = Textures(MapChannel::NUM_MAP_CHANNELS);
    if (!material.albedoTexture.filename.isEmpty()) {
        auto map = fetchTextureMap(textureBaseUrl, material.albedoTexture, NetworkTexture::ALBEDO_TEXTURE, MapChannel::ALBEDO_MAP);
        _albedoTransform = material.albedoTexture.transform;
        map->setTextureTransform(_albedoTransform);

        if (!material.opacityTexture.filename.isEmpty()) {
            if (material.albedoTexture.filename == material.opacityTexture.filename) {
                // Best case scenario, just indicating that the albedo map contains transparency
                // TODO: Different albedo/opacity maps are not currently supported
                map->setUseAlphaChannel(true);
            }
        }

        setTextureMap(MapChannel::ALBEDO_MAP, map);
    }


    if (!material.normalTexture.filename.isEmpty()) {
        auto type = (material.normalTexture.isBumpmap ? NetworkTexture::BUMP_TEXTURE : NetworkTexture::NORMAL_TEXTURE);
        auto map = fetchTextureMap(textureBaseUrl, material.normalTexture, type, MapChannel::NORMAL_MAP);
        setTextureMap(MapChannel::NORMAL_MAP, map);
    }

    if (!material.roughnessTexture.filename.isEmpty()) {
        auto map = fetchTextureMap(textureBaseUrl, material.roughnessTexture, NetworkTexture::ROUGHNESS_TEXTURE, MapChannel::ROUGHNESS_MAP);
        setTextureMap(MapChannel::ROUGHNESS_MAP, map);
    } else if (!material.glossTexture.filename.isEmpty()) {
        auto map = fetchTextureMap(textureBaseUrl, material.glossTexture, NetworkTexture::GLOSS_TEXTURE, MapChannel::ROUGHNESS_MAP);
        setTextureMap(MapChannel::ROUGHNESS_MAP, map);
    }

    if (!material.metallicTexture.filename.isEmpty()) {
        auto map = fetchTextureMap(textureBaseUrl, material.metallicTexture, NetworkTexture::METALLIC_TEXTURE, MapChannel::METALLIC_MAP);
        setTextureMap(MapChannel::METALLIC_MAP, map);
    } else if (!material.specularTexture.filename.isEmpty()) {
        auto map = fetchTextureMap(textureBaseUrl, material.specularTexture, NetworkTexture::SPECULAR_TEXTURE, MapChannel::METALLIC_MAP);
        setTextureMap(MapChannel::METALLIC_MAP, map);
    }

    if (!material.occlusionTexture.filename.isEmpty()) {
        auto map = fetchTextureMap(textureBaseUrl, material.occlusionTexture, NetworkTexture::OCCLUSION_TEXTURE, MapChannel::OCCLUSION_MAP);
        map->setTextureTransform(material.occlusionTexture.transform);
        setTextureMap(MapChannel::OCCLUSION_MAP, map);
    }

    if (!material.emissiveTexture.filename.isEmpty()) {
        auto map = fetchTextureMap(textureBaseUrl, material.emissiveTexture, NetworkTexture::EMISSIVE_TEXTURE, MapChannel::EMISSIVE_MAP);
        setTextureMap(MapChannel::EMISSIVE_MAP, map);
    }

    if (!material.scatteringTexture.filename.isEmpty()) {
        auto map = fetchTextureMap(textureBaseUrl, material.scatteringTexture, NetworkTexture::SCATTERING_TEXTURE, MapChannel::SCATTERING_MAP);
        setTextureMap(MapChannel::SCATTERING_MAP, map);
    }

    if (!material.lightmapTexture.filename.isEmpty()) {
        auto map = fetchTextureMap(textureBaseUrl, material.lightmapTexture, NetworkTexture::LIGHTMAP_TEXTURE, MapChannel::LIGHTMAP_MAP);
        _lightmapTransform = material.lightmapTexture.transform;
        _lightmapParams = material.lightmapParams;
        map->setTextureTransform(_lightmapTransform);
        map->setLightmapOffsetScale(_lightmapParams.x, _lightmapParams.y);
        setTextureMap(MapChannel::LIGHTMAP_MAP, map);
    }
}

void NetworkMaterial::setTextures(const QVariantMap& textureMap) {
    _isOriginal = false;

    const auto& albedoName = getTextureName(MapChannel::ALBEDO_MAP);
    const auto& normalName = getTextureName(MapChannel::NORMAL_MAP);
    const auto& roughnessName = getTextureName(MapChannel::ROUGHNESS_MAP);
    const auto& metallicName = getTextureName(MapChannel::METALLIC_MAP);
    const auto& occlusionName = getTextureName(MapChannel::OCCLUSION_MAP);
    const auto& emissiveName = getTextureName(MapChannel::EMISSIVE_MAP);
    const auto& lightmapName = getTextureName(MapChannel::LIGHTMAP_MAP);
    const auto& scatteringName = getTextureName(MapChannel::SCATTERING_MAP);

    if (!albedoName.isEmpty()) {
        auto url = textureMap.contains(albedoName) ? textureMap[albedoName].toUrl() : QUrl();
        auto map = fetchTextureMap(url, NetworkTexture::ALBEDO_TEXTURE, MapChannel::ALBEDO_MAP);
        map->setTextureTransform(_albedoTransform);
        // when reassigning the albedo texture we also check for the alpha channel used as opacity
        map->setUseAlphaChannel(true);
        setTextureMap(MapChannel::ALBEDO_MAP, map);
    }

    if (!normalName.isEmpty()) {
        auto url = textureMap.contains(normalName) ? textureMap[normalName].toUrl() : QUrl();
        auto map = fetchTextureMap(url, NetworkTexture::NORMAL_TEXTURE, MapChannel::NORMAL_MAP);
        setTextureMap(MapChannel::NORMAL_MAP, map);
    }

    if (!roughnessName.isEmpty()) {
        auto url = textureMap.contains(roughnessName) ? textureMap[roughnessName].toUrl() : QUrl();
        // FIXME: If passing a gloss map instead of a roughmap how do we know?
        auto map = fetchTextureMap(url, NetworkTexture::ROUGHNESS_TEXTURE, MapChannel::ROUGHNESS_MAP);
        setTextureMap(MapChannel::ROUGHNESS_MAP, map);
    }

    if (!metallicName.isEmpty()) {
        auto url = textureMap.contains(metallicName) ? textureMap[metallicName].toUrl() : QUrl();
        // FIXME: If passing a specular map instead of a metallic how do we know?
        auto map = fetchTextureMap(url, NetworkTexture::METALLIC_TEXTURE, MapChannel::METALLIC_MAP);
        setTextureMap(MapChannel::METALLIC_MAP, map);
    }

    if (!occlusionName.isEmpty()) {
        auto url = textureMap.contains(occlusionName) ? textureMap[occlusionName].toUrl() : QUrl();
        auto map = fetchTextureMap(url, NetworkTexture::OCCLUSION_TEXTURE, MapChannel::OCCLUSION_MAP);
        setTextureMap(MapChannel::OCCLUSION_MAP, map);
    }

    if (!emissiveName.isEmpty()) {
        auto url = textureMap.contains(emissiveName) ? textureMap[emissiveName].toUrl() : QUrl();
        auto map = fetchTextureMap(url, NetworkTexture::EMISSIVE_TEXTURE, MapChannel::EMISSIVE_MAP);
        setTextureMap(MapChannel::EMISSIVE_MAP, map);
    }

    if (!scatteringName.isEmpty()) {
        auto url = textureMap.contains(scatteringName) ? textureMap[scatteringName].toUrl() : QUrl();
        auto map = fetchTextureMap(url, NetworkTexture::SCATTERING_TEXTURE, MapChannel::SCATTERING_MAP);
        setTextureMap(MapChannel::SCATTERING_MAP, map);
    }

    if (!lightmapName.isEmpty()) {
        auto url = textureMap.contains(lightmapName) ? textureMap[lightmapName].toUrl() : QUrl();
        auto map = fetchTextureMap(url, NetworkTexture::LIGHTMAP_TEXTURE, MapChannel::LIGHTMAP_MAP);
        map->setTextureTransform(_lightmapTransform);
        map->setLightmapOffsetScale(_lightmapParams.x, _lightmapParams.y);
        setTextureMap(MapChannel::LIGHTMAP_MAP, map);
    }
}

#include "ModelCache.moc"
