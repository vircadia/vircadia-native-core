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

#include <gpu/Batch.h>
#include <gpu/Stream.h>

#include <QThreadPool>

#include <Gzip.h>

#include "ModelNetworkingLogging.h"
#include <Trace.h>
#include <StatTracker.h>
#include <hfm/ModelFormatRegistry.h>
#include <FBXSerializer.h>
#include <OBJSerializer.h>
#include <GLTFSerializer.h>
#include <model-baker/Baker.h>

Q_LOGGING_CATEGORY(trace_resource_parse_geometry, "trace.resource.parse.geometry")

class GeometryReader;

class GeometryExtra {
public:
    const QVariantHash& mapping;
    const QUrl& textureBaseUrl;
    bool combineParts;
};

// From: https://stackoverflow.com/questions/41145012/how-to-hash-qvariant
class QVariantHasher {
public:
    QVariantHasher() : buff(&bb), ds(&buff) {
        bb.reserve(1000);
        buff.open(QIODevice::WriteOnly);
    }
    uint hash(const QVariant& v) {
        buff.seek(0);
        ds << v;
        return qHashBits(bb.constData(), buff.pos());
    }
private:
    QByteArray bb;
    QBuffer buff;
    QDataStream ds;
};

namespace std {
    template <>
    struct hash<QVariantHash> {
        size_t operator()(const QVariantHash& a) const {
            QVariantHasher hasher;
            return hasher.hash(a);
        }
    };

    template <>
    struct hash<QUrl> {
        size_t operator()(const QUrl& a) const {
            return qHash(a);
        }
    };

    template <>
    struct hash<GeometryExtra> {
        size_t operator()(const GeometryExtra& a) const {
            size_t result = 0;
            hash_combine(result, a.mapping, a.textureBaseUrl, a.combineParts);
            return result;
        }
    };
}

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

    // store parsed contents of FST file
    _mapping = FSTReader::readMapping(data);

    QString filename = _mapping.value("filename").toString();

    if (filename.isNull()) {
        finishedLoading(false);
    } else {
        QUrl url = _url.resolved(filename);

        QString texdir = _mapping.value(TEXDIR_FIELD).toString();
        if (!texdir.isNull()) {
            if (!texdir.endsWith('/')) {
                texdir += '/';
            }
            _textureBaseUrl = resolveTextureBaseUrl(url, _url.resolved(texdir));
        } else {
            _textureBaseUrl = url.resolved(QUrl("."));
        }

        auto scripts = FSTReader::getScripts(_url, _mapping);
        if (scripts.size() > 0) {
            _mapping.remove(SCRIPT_FIELD);
            for (auto &scriptPath : scripts) {
                _mapping.insertMulti(SCRIPT_FIELD, scriptPath);
            }
        }

        auto animGraphVariant = _mapping.value("animGraphUrl");

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
        GeometryExtra extra { _mapping, _textureBaseUrl, false };

        // Get the raw GeometryResource
        _geometryResource = modelCache->getResource(url, QUrl(), &extra, std::hash<GeometryExtra>()(extra)).staticCast<GeometryResource>();
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
        _hfmModel = _geometryResource->_hfmModel;
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
    GeometryReader(const ModelLoader& modelLoader, QWeakPointer<Resource>& resource, const QUrl& url, const QVariantHash& mapping,
                   const QByteArray& data, bool combineParts, const QString& webMediaType) :
        _modelLoader(modelLoader), _resource(resource), _url(url), _mapping(mapping), _data(data), _combineParts(combineParts), _webMediaType(webMediaType) {

        DependencyManager::get<StatTracker>()->incrementStat("PendingProcessing");
    }

    virtual void run() override;

private:
    ModelLoader _modelLoader;
    QWeakPointer<Resource> _resource;
    QUrl _url;
    QVariantHash _mapping;
    QByteArray _data;
    bool _combineParts;
    QString _webMediaType;
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
        return;
    }

    try {
        if (_data.isEmpty()) {
            throw QString("reply is NULL");
        }

        // Ensure the resource has not been deleted
        auto resource = _resource.toStrongRef();
        if (!resource) {
            qCWarning(modelnetworking) << "Abandoning load of" << _url << "; could not get strong ref";
            return;
        }

        if (_url.path().isEmpty()) {
            throw QString("url is invalid");
        }

        HFMModel::Pointer hfmModel;
        QVariantHash serializerMapping = _mapping;
        serializerMapping["combineParts"] = _combineParts;

        if (_url.path().toLower().endsWith(".gz")) {
            QByteArray uncompressedData;
            if (!gunzip(_data, uncompressedData)) {
                throw QString("failed to decompress .gz model");
            }
            // Strip the compression extension from the path, so the loader can infer the file type from what remains.
            // This is okay because we don't expect the serializer to be able to read the contents of a compressed model file.
            auto strippedUrl = _url;
            strippedUrl.setPath(_url.path().left(_url.path().size() - 3));
            hfmModel = _modelLoader.load(uncompressedData, serializerMapping, strippedUrl, "");
        } else {
            hfmModel = _modelLoader.load(_data, serializerMapping, _url, _webMediaType.toStdString());
        }

        if (!hfmModel) {
            throw QString("unsupported format");
        }

        if (hfmModel->meshes.empty() || hfmModel->joints.empty()) {
            throw QString("empty geometry, possibly due to an unsupported model version");
        }

        // Add scripts to hfmModel
        if (!_mapping.value(SCRIPT_FIELD).isNull()) {
            QVariantList scripts = _mapping.values(SCRIPT_FIELD);
            for (auto &script : scripts) {
                hfmModel->scripts.push_back(script.toString());
            }
        }

        QMetaObject::invokeMethod(resource.data(), "setGeometryDefinition",
                Q_ARG(HFMModel::Pointer, hfmModel), Q_ARG(QVariantHash, _mapping));
    } catch (const std::exception&) {
        auto resource = _resource.toStrongRef();
        if (resource) {
            QMetaObject::invokeMethod(resource.data(), "finishedLoading",
                Q_ARG(bool, false));
        }
    } catch (QString& e) {
        qCWarning(modelnetworking) << "Exception while loading model --" << e;
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
    GeometryDefinitionResource(const ModelLoader& modelLoader, const QUrl& url) : GeometryResource(url), _modelLoader(modelLoader) {}
    GeometryDefinitionResource(const GeometryDefinitionResource& other) :
        GeometryResource(other),
        _modelLoader(other._modelLoader),
        _mapping(other._mapping),
        _combineParts(other._combineParts) {}

    QString getType() const override { return "GeometryDefinition"; }

    virtual void downloadFinished(const QByteArray& data) override;

    void setExtra(void* extra) override;

protected:
    Q_INVOKABLE void setGeometryDefinition(HFMModel::Pointer hfmModel, QVariantHash mapping);

private:
    ModelLoader _modelLoader;
    QVariantHash _mapping;
    bool _combineParts;
};

void GeometryDefinitionResource::setExtra(void* extra) {
    const GeometryExtra* geometryExtra = static_cast<const GeometryExtra*>(extra);
    _mapping = geometryExtra ? geometryExtra->mapping : QVariantHash();
    _textureBaseUrl = geometryExtra ? resolveTextureBaseUrl(_url, geometryExtra->textureBaseUrl) : QUrl();
    _combineParts = geometryExtra ? geometryExtra->combineParts : true;
}

void GeometryDefinitionResource::downloadFinished(const QByteArray& data) {
    if (_url != _effectiveBaseURL) {
        _url = _effectiveBaseURL;
        _textureBaseUrl = _effectiveBaseURL;
    }
    QThreadPool::globalInstance()->start(new GeometryReader(_modelLoader, _self, _effectiveBaseURL, _mapping, data, _combineParts, _request->getWebMediaType()));
}

void GeometryDefinitionResource::setGeometryDefinition(HFMModel::Pointer hfmModel, QVariantHash mapping) {
    // Do processing on the model
    baker::Baker modelBaker(hfmModel, mapping);
    modelBaker.run();

    // Assume ownership of the processed HFMModel
    _hfmModel = modelBaker.hfmModel;

    // Copy materials
    QHash<QString, size_t> materialIDAtlas;
    for (const HFMMaterial& material : _hfmModel->materials) {
        materialIDAtlas[material.materialID] = _materials.size();
        _materials.push_back(std::make_shared<NetworkMaterial>(material, _textureBaseUrl));
    }

    std::shared_ptr<GeometryMeshes> meshes = std::make_shared<GeometryMeshes>();
    std::shared_ptr<GeometryMeshParts> parts = std::make_shared<GeometryMeshParts>();
    int meshID = 0;
    for (const HFMMesh& mesh : _hfmModel->meshes) {
        // Copy mesh pointers
        meshes->emplace_back(mesh._mesh);
        int partID = 0;
        for (const HFMMeshPart& part : mesh.parts) {
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

    auto modelFormatRegistry = DependencyManager::get<ModelFormatRegistry>();
    modelFormatRegistry->addFormat(FBXSerializer());
    modelFormatRegistry->addFormat(OBJSerializer());
    modelFormatRegistry->addFormat(GLTFSerializer());
}

QSharedPointer<Resource> ModelCache::createResource(const QUrl& url) {
    Resource* resource = nullptr;
    if (url.path().toLower().endsWith(".fst")) {
        resource = new GeometryMappingResource(url);
    } else {
        resource = new GeometryDefinitionResource(_modelLoader, url);
    }

    return QSharedPointer<Resource>(resource, &Resource::deleter);
}

QSharedPointer<Resource> ModelCache::createResourceCopy(const QSharedPointer<Resource>& resource) {
    return QSharedPointer<Resource>(new GeometryDefinitionResource(*resource.staticCast<GeometryDefinitionResource>().data()), &Resource::deleter);
}

GeometryResource::Pointer ModelCache::getGeometryResource(const QUrl& url,
                                                          const QVariantHash& mapping, const QUrl& textureBaseUrl) {
    bool combineParts = true;
    GeometryExtra geometryExtra = { mapping, textureBaseUrl, combineParts };
    GeometryResource::Pointer resource = getResource(url, QUrl(), &geometryExtra, std::hash<GeometryExtra>()(geometryExtra)).staticCast<GeometryResource>();
    if (resource) {
        if (resource->isLoaded() && resource->shouldSetTextures()) {
            resource->setTextures();
        }
    }
    return resource;
}

GeometryResource::Pointer ModelCache::getCollisionGeometryResource(const QUrl& url,
                                                                   const QVariantHash& mapping, const QUrl& textureBaseUrl) {
    bool combineParts = false;
    GeometryExtra geometryExtra = { mapping, textureBaseUrl, combineParts };
    GeometryResource::Pointer resource = getResource(url, QUrl(), &geometryExtra, std::hash<GeometryExtra>()(geometryExtra)).staticCast<GeometryResource>();
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
    _hfmModel = geometry._hfmModel;
    _meshes = geometry._meshes;
    _meshParts = geometry._meshParts;

    _materials.reserve(geometry._materials.size());
    for (const auto& material : geometry._materials) {
        _materials.push_back(std::make_shared<NetworkMaterial>(*material));
    }

    _animGraphOverrideUrl = geometry._animGraphOverrideUrl;
    _mapping = geometry._mapping;
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
            }
        }

        // If we only use cached textures, they should all be loaded
        areTexturesLoaded();
    } else {
        qCWarning(modelnetworking) << "Ignoring setTextures(); geometry not ready";
    }
}

bool Geometry::areTexturesLoaded() const {
    if (!_areTexturesLoaded) {
        for (auto& material : _materials) {
            if (material->isMissingTexture()) {
                return false;
            }

            material->checkResetOpacityMap();
        }

        _areTexturesLoaded = true;
    }
    return true;
}

const std::shared_ptr<NetworkMaterial> Geometry::getShapeMaterial(int partID) const {
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
    if (_hfmModel) {
        for (const HFMMaterial& material : _hfmModel->materials) {
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
            resourceFinished(true);
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

NetworkMaterial::NetworkMaterial(const NetworkMaterial& m) :
    Material(m),
    _textures(m._textures),
    _albedoTransform(m._albedoTransform),
    _lightmapTransform(m._lightmapTransform),
    _lightmapParams(m._lightmapParams),
    _isOriginal(m._isOriginal)
{}

const QString NetworkMaterial::NO_TEXTURE = QString();

const QString& NetworkMaterial::getTextureName(MapChannel channel) {
    if (_textures[channel].texture) {
        return _textures[channel].name;
    }
    return NO_TEXTURE;
}

QUrl NetworkMaterial::getTextureUrl(const QUrl& baseUrl, const HFMTexture& texture) {
    if (texture.content.isEmpty()) {
        // External file: search relative to the baseUrl, in case filename is relative
        return baseUrl.resolved(QUrl(texture.filename));
    } else {
        // Inlined file: cache under the fbx file to avoid namespace clashes
        // NOTE: We cannot resolve the path because filename may be an absolute path
        assert(texture.filename.size() > 0);
        auto baseUrlStripped = baseUrl.toDisplayString(QUrl::RemoveFragment | QUrl::RemoveQuery | QUrl::RemoveUserInfo);
        if (texture.filename.at(0) == '/') {
            return baseUrlStripped + texture.filename;
        } else {
            return baseUrlStripped + '/' + texture.filename;
        }
    }
}

graphics::TextureMapPointer NetworkMaterial::fetchTextureMap(const QUrl& baseUrl, const HFMTexture& hfmTexture,
                                                          image::TextureUsage::Type type, MapChannel channel) {

    if (baseUrl.isEmpty()) {
        return nullptr;
    }

    const auto url = getTextureUrl(baseUrl, hfmTexture);
    const auto texture = DependencyManager::get<TextureCache>()->getTexture(url, type, hfmTexture.content, hfmTexture.maxNumPixels);
    _textures[channel] = Texture { hfmTexture.name, texture };

    auto map = std::make_shared<graphics::TextureMap>();
    if (texture) {
        map->setTextureSource(texture->_textureSource);
    }
    map->setTextureTransform(hfmTexture.transform);

    return map;
}

graphics::TextureMapPointer NetworkMaterial::fetchTextureMap(const QUrl& url, image::TextureUsage::Type type, MapChannel channel) {
    auto textureCache = DependencyManager::get<TextureCache>();
    if (textureCache && !url.isEmpty()) {
        auto texture = textureCache->getTexture(url, type);
        _textures[channel].texture = texture;

        auto map = std::make_shared<graphics::TextureMap>();
        if (texture) {
            map->setTextureSource(texture->_textureSource);
        }

        return map;
    }
    return nullptr;
}

void NetworkMaterial::setAlbedoMap(const QUrl& url, bool useAlphaChannel) {
    auto map = fetchTextureMap(url, image::TextureUsage::ALBEDO_TEXTURE, MapChannel::ALBEDO_MAP);
    if (map) {
        map->setUseAlphaChannel(useAlphaChannel);
        setTextureMap(MapChannel::ALBEDO_MAP, map);
    }
}

void NetworkMaterial::setNormalMap(const QUrl& url, bool isBumpmap) {
    auto map = fetchTextureMap(url, isBumpmap ? image::TextureUsage::BUMP_TEXTURE : image::TextureUsage::NORMAL_TEXTURE, MapChannel::NORMAL_MAP);
    if (map) {
        setTextureMap(MapChannel::NORMAL_MAP, map);
    }
}

void NetworkMaterial::setRoughnessMap(const QUrl& url, bool isGloss) {
    auto map = fetchTextureMap(url, isGloss ? image::TextureUsage::GLOSS_TEXTURE : image::TextureUsage::ROUGHNESS_TEXTURE, MapChannel::ROUGHNESS_MAP);
    if (map) {
        setTextureMap(MapChannel::ROUGHNESS_MAP, map);
    }
}

void NetworkMaterial::setMetallicMap(const QUrl& url, bool isSpecular) {
    auto map = fetchTextureMap(url, isSpecular ? image::TextureUsage::SPECULAR_TEXTURE : image::TextureUsage::METALLIC_TEXTURE, MapChannel::METALLIC_MAP);
    if (map) {
        setTextureMap(MapChannel::METALLIC_MAP, map);
    }
}

void NetworkMaterial::setOcclusionMap(const QUrl& url) {
    auto map = fetchTextureMap(url, image::TextureUsage::OCCLUSION_TEXTURE, MapChannel::OCCLUSION_MAP);
    if (map) {
        setTextureMap(MapChannel::OCCLUSION_MAP, map);
    }
}

void NetworkMaterial::setEmissiveMap(const QUrl& url) {
    auto map = fetchTextureMap(url, image::TextureUsage::EMISSIVE_TEXTURE, MapChannel::EMISSIVE_MAP);
    if (map) {
        setTextureMap(MapChannel::EMISSIVE_MAP, map);
    }
}

void NetworkMaterial::setScatteringMap(const QUrl& url) {
    auto map = fetchTextureMap(url, image::TextureUsage::SCATTERING_TEXTURE, MapChannel::SCATTERING_MAP);
    if (map) {
        setTextureMap(MapChannel::SCATTERING_MAP, map);
    }
}

void NetworkMaterial::setLightmapMap(const QUrl& url) {
    auto map = fetchTextureMap(url, image::TextureUsage::LIGHTMAP_TEXTURE, MapChannel::LIGHTMAP_MAP);
    if (map) {
        //map->setTextureTransform(_lightmapTransform);
        //map->setLightmapOffsetScale(_lightmapParams.x, _lightmapParams.y);
        setTextureMap(MapChannel::LIGHTMAP_MAP, map);
    }
}

NetworkMaterial::NetworkMaterial(const HFMMaterial& material, const QUrl& textureBaseUrl) :
    graphics::Material(*material._material),
    _textures(MapChannel::NUM_MAP_CHANNELS)
{
    _name = material.name.toStdString();
    if (!material.albedoTexture.filename.isEmpty()) {
        auto map = fetchTextureMap(textureBaseUrl, material.albedoTexture, image::TextureUsage::ALBEDO_TEXTURE, MapChannel::ALBEDO_MAP);
        if (map) {
            _albedoTransform = material.albedoTexture.transform;
            map->setTextureTransform(_albedoTransform);

            if (!material.opacityTexture.filename.isEmpty()) {
                if (material.albedoTexture.filename == material.opacityTexture.filename) {
                    // Best case scenario, just indicating that the albedo map contains transparency
                    // TODO: Different albedo/opacity maps are not currently supported
                    map->setUseAlphaChannel(true);
                }
            }
        }

        setTextureMap(MapChannel::ALBEDO_MAP, map);
    }


    if (!material.normalTexture.filename.isEmpty()) {
        auto type = (material.normalTexture.isBumpmap ? image::TextureUsage::BUMP_TEXTURE : image::TextureUsage::NORMAL_TEXTURE);
        auto map = fetchTextureMap(textureBaseUrl, material.normalTexture, type, MapChannel::NORMAL_MAP);
        setTextureMap(MapChannel::NORMAL_MAP, map);
    }

    if (!material.roughnessTexture.filename.isEmpty()) {
        auto map = fetchTextureMap(textureBaseUrl, material.roughnessTexture, image::TextureUsage::ROUGHNESS_TEXTURE, MapChannel::ROUGHNESS_MAP);
        setTextureMap(MapChannel::ROUGHNESS_MAP, map);
    } else if (!material.glossTexture.filename.isEmpty()) {
        auto map = fetchTextureMap(textureBaseUrl, material.glossTexture, image::TextureUsage::GLOSS_TEXTURE, MapChannel::ROUGHNESS_MAP);
        setTextureMap(MapChannel::ROUGHNESS_MAP, map);
    }

    if (!material.metallicTexture.filename.isEmpty()) {
        auto map = fetchTextureMap(textureBaseUrl, material.metallicTexture, image::TextureUsage::METALLIC_TEXTURE, MapChannel::METALLIC_MAP);
        setTextureMap(MapChannel::METALLIC_MAP, map);
    } else if (!material.specularTexture.filename.isEmpty()) {
        auto map = fetchTextureMap(textureBaseUrl, material.specularTexture, image::TextureUsage::SPECULAR_TEXTURE, MapChannel::METALLIC_MAP);
        setTextureMap(MapChannel::METALLIC_MAP, map);
    }

    if (!material.occlusionTexture.filename.isEmpty()) {
        auto map = fetchTextureMap(textureBaseUrl, material.occlusionTexture, image::TextureUsage::OCCLUSION_TEXTURE, MapChannel::OCCLUSION_MAP);
        if (map) {
            map->setTextureTransform(material.occlusionTexture.transform);
        }
        setTextureMap(MapChannel::OCCLUSION_MAP, map);
    }

    if (!material.emissiveTexture.filename.isEmpty()) {
        auto map = fetchTextureMap(textureBaseUrl, material.emissiveTexture, image::TextureUsage::EMISSIVE_TEXTURE, MapChannel::EMISSIVE_MAP);
        setTextureMap(MapChannel::EMISSIVE_MAP, map);
    }

    if (!material.scatteringTexture.filename.isEmpty()) {
        auto map = fetchTextureMap(textureBaseUrl, material.scatteringTexture, image::TextureUsage::SCATTERING_TEXTURE, MapChannel::SCATTERING_MAP);
        setTextureMap(MapChannel::SCATTERING_MAP, map);
    }

    if (!material.lightmapTexture.filename.isEmpty()) {
        auto map = fetchTextureMap(textureBaseUrl, material.lightmapTexture, image::TextureUsage::LIGHTMAP_TEXTURE, MapChannel::LIGHTMAP_MAP);
        if (map) {
            _lightmapTransform = material.lightmapTexture.transform;
            _lightmapParams = material.lightmapParams;
            map->setTextureTransform(_lightmapTransform);
            map->setLightmapOffsetScale(_lightmapParams.x, _lightmapParams.y);
        }
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
        auto map = fetchTextureMap(url, image::TextureUsage::ALBEDO_TEXTURE, MapChannel::ALBEDO_MAP);
        if (map) {
            map->setTextureTransform(_albedoTransform);
            // when reassigning the albedo texture we also check for the alpha channel used as opacity
            map->setUseAlphaChannel(true);
        }
        setTextureMap(MapChannel::ALBEDO_MAP, map);
    }

    if (!normalName.isEmpty()) {
        auto url = textureMap.contains(normalName) ? textureMap[normalName].toUrl() : QUrl();
        auto map = fetchTextureMap(url, image::TextureUsage::NORMAL_TEXTURE, MapChannel::NORMAL_MAP);
        setTextureMap(MapChannel::NORMAL_MAP, map);
    }

    if (!roughnessName.isEmpty()) {
        auto url = textureMap.contains(roughnessName) ? textureMap[roughnessName].toUrl() : QUrl();
        // FIXME: If passing a gloss map instead of a roughmap how do we know?
        auto map = fetchTextureMap(url, image::TextureUsage::ROUGHNESS_TEXTURE, MapChannel::ROUGHNESS_MAP);
        setTextureMap(MapChannel::ROUGHNESS_MAP, map);
    }

    if (!metallicName.isEmpty()) {
        auto url = textureMap.contains(metallicName) ? textureMap[metallicName].toUrl() : QUrl();
        // FIXME: If passing a specular map instead of a metallic how do we know?
        auto map = fetchTextureMap(url, image::TextureUsage::METALLIC_TEXTURE, MapChannel::METALLIC_MAP);
        setTextureMap(MapChannel::METALLIC_MAP, map);
    }

    if (!occlusionName.isEmpty()) {
        auto url = textureMap.contains(occlusionName) ? textureMap[occlusionName].toUrl() : QUrl();
        // FIXME: we need to handle the occlusion map transform here
        auto map = fetchTextureMap(url, image::TextureUsage::OCCLUSION_TEXTURE, MapChannel::OCCLUSION_MAP);
        setTextureMap(MapChannel::OCCLUSION_MAP, map);
    }

    if (!emissiveName.isEmpty()) {
        auto url = textureMap.contains(emissiveName) ? textureMap[emissiveName].toUrl() : QUrl();
        auto map = fetchTextureMap(url, image::TextureUsage::EMISSIVE_TEXTURE, MapChannel::EMISSIVE_MAP);
        setTextureMap(MapChannel::EMISSIVE_MAP, map);
    }

    if (!scatteringName.isEmpty()) {
        auto url = textureMap.contains(scatteringName) ? textureMap[scatteringName].toUrl() : QUrl();
        auto map = fetchTextureMap(url, image::TextureUsage::SCATTERING_TEXTURE, MapChannel::SCATTERING_MAP);
        setTextureMap(MapChannel::SCATTERING_MAP, map);
    }

    if (!lightmapName.isEmpty()) {
        auto url = textureMap.contains(lightmapName) ? textureMap[lightmapName].toUrl() : QUrl();
        auto map = fetchTextureMap(url, image::TextureUsage::LIGHTMAP_TEXTURE, MapChannel::LIGHTMAP_MAP);
        if (map) {
            map->setTextureTransform(_lightmapTransform);
            map->setLightmapOffsetScale(_lightmapParams.x, _lightmapParams.y);
        }
        setTextureMap(MapChannel::LIGHTMAP_MAP, map);
    }
}

bool NetworkMaterial::isMissingTexture() {
    for (auto& networkTexture : _textures) {
        auto& texture = networkTexture.texture;
        if (!texture) {
            continue;
        }
        // Failed texture downloads need to be considered as 'loaded'
        // or the object will never fade in
        bool finished = texture->isFailed() || (texture->isLoaded() && texture->getGPUTexture() && texture->getGPUTexture()->isDefined());
        if (!finished) {
            return true;
        }
    }
    return false;
}

void NetworkMaterial::checkResetOpacityMap() {
    // If material textures are loaded, check the material translucency
    // FIXME: This should not be done here.  The opacity map should already be reset in Material::setTextureMap.
    // However, currently that code can be called before the albedo map is defined, so resetOpacityMap will fail.
    // Geometry::areTexturesLoaded() is called repeatedly until it returns true, so we do the check here for now
    const auto& albedoTexture = _textures[NetworkMaterial::MapChannel::ALBEDO_MAP];
    if (albedoTexture.texture) {
        resetOpacityMap();
    }
}

#include "ModelCache.moc"
