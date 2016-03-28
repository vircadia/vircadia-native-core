//
//  ModelCache.cpp
//  interface/src/renderer
//
//  Created by Andrzej Kapolka on 6/21/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ModelCache.h"

#include <cmath>

#include <QNetworkReply>
#include <QThreadPool>

#include <FSTReader.h>
#include <NumericalConstants.h>

#include "TextureCache.h"
#include "ModelNetworkingLogging.h"

#include "model/TextureMap.h"

//#define WANT_DEBUG

ModelCache::ModelCache()
{
    const qint64 GEOMETRY_DEFAULT_UNUSED_MAX_SIZE = DEFAULT_UNUSED_MAX_SIZE;
    setUnusedResourceCacheSize(GEOMETRY_DEFAULT_UNUSED_MAX_SIZE);
}

ModelCache::~ModelCache() {
}

QSharedPointer<Resource> ModelCache::createResource(const QUrl& url, const QSharedPointer<Resource>& fallback,
                                                        bool delayLoad, const void* extra) {
    // NetworkGeometry is no longer a subclass of Resource, but requires this method because, it is pure virtual.
    assert(false);
    return QSharedPointer<Resource>();
}


GeometryReader::GeometryReader(const QUrl& url, const QByteArray& data, const QVariantHash& mapping) :
    _url(url),
    _data(data),
    _mapping(mapping) {
}

void GeometryReader::run() {
    auto originalPriority = QThread::currentThread()->priority();
    if (originalPriority == QThread::InheritPriority) {
        originalPriority = QThread::NormalPriority;
    }
    QThread::currentThread()->setPriority(QThread::LowPriority);
    try {
        if (_data.isEmpty()) {
            throw QString("Reply is NULL ?!");
        }
        QString urlname = _url.path().toLower();
        bool urlValid = true;
        urlValid &= !urlname.isEmpty();
        urlValid &= !_url.path().isEmpty();
        urlValid &= _url.path().toLower().endsWith(".fbx") || _url.path().toLower().endsWith(".obj");

        if (urlValid) {
            // Let's read the binaries from the network
            FBXGeometry* fbxgeo = nullptr;
            if (_url.path().toLower().endsWith(".fbx")) {
                const bool grabLightmaps = true;
                const float lightmapLevel = 1.0f;
                fbxgeo = readFBX(_data, _mapping, _url.path(), grabLightmaps, lightmapLevel);
                if (fbxgeo->meshes.size() == 0 && fbxgeo->joints.size() == 0) {
                    // empty fbx geometry, indicates error
                    throw QString("empty geometry, possibly due to an unsupported FBX version");
                }
            } else if (_url.path().toLower().endsWith(".obj")) {
                fbxgeo = OBJReader().readOBJ(_data, _mapping, _url);
            } else {
                QString errorStr("unsupported format");
                throw errorStr;
            }
            emit onSuccess(fbxgeo);
        } else {
            throw QString("url is invalid");
        }

    } catch (const QString& error) {
        qCDebug(modelnetworking) << "Error reading " << _url << ": " << error;
        emit onError(NetworkGeometry::ModelParseError, error);
    }

    QThread::currentThread()->setPriority(originalPriority);
}

NetworkGeometry::NetworkGeometry(const QUrl& url, bool delayLoad, const QVariantHash& mapping, const QUrl& textureBaseUrl) :
    _url(url),
    _mapping(mapping),
    _textureBaseUrl(textureBaseUrl.isValid() ? textureBaseUrl : url) {

    if (delayLoad) {
        _state = DelayState;
    } else {
        attemptRequestInternal();
    }
}

NetworkGeometry::~NetworkGeometry() {
    if (_resource) {
        _resource->deleteLater();
    }
}

void NetworkGeometry::attemptRequest() {
    if (_state == DelayState) {
        attemptRequestInternal();
    }
}

void NetworkGeometry::attemptRequestInternal() {
    if (_url.path().toLower().endsWith(".fst")) {
        _mappingUrl = _url;
        requestMapping(_url);
    } else {
        _modelUrl = _url;
        requestModel(_url);
    }
}

bool NetworkGeometry::isLoaded() const {
    return _state == SuccessState;
}

bool NetworkGeometry::isLoadedWithTextures() const {
    if (!isLoaded()) {
        return false;
    }

    if (!_isLoadedWithTextures) {
        _hasTransparentTextures = false;

        for (auto&& material : _materials) {
            if ((material->albedoTexture && !material->albedoTexture->isLoaded()) ||
                (material->normalTexture && !material->normalTexture->isLoaded()) ||
                (material->roughnessTexture && !material->roughnessTexture->isLoaded()) ||
                (material->metallicTexture && !material->metallicTexture->isLoaded()) ||
                (material->occlusionTexture && !material->occlusionTexture->isLoaded()) ||
                (material->emissiveTexture && !material->emissiveTexture->isLoaded()) ||
                (material->lightmapTexture && !material->lightmapTexture->isLoaded())) {
                return false;
            }
            if (material->albedoTexture && material->albedoTexture->getGPUTexture()) {
                // Reassign the texture to make sure that itsalbedo alpha channel material key is detected correctly
                material->_material->setTextureMap(model::MaterialKey::ALBEDO_MAP, material->_material->getTextureMap(model::MaterialKey::ALBEDO_MAP));
                const auto& usage = material->albedoTexture->getGPUTexture()->getUsage();
                bool isTransparentTexture = usage.isAlpha() && !usage.isAlphaMask();
                _hasTransparentTextures |= isTransparentTexture;
            }
        }

        _isLoadedWithTextures = true;
    }
    return true;
}

void NetworkGeometry::setTextureWithNameToURL(const QString& name, const QUrl& url) {
    if (_meshes.size() > 0) {
        auto textureCache = DependencyManager::get<TextureCache>();
        for (auto&& material : _materials) {
            auto networkMaterial = material->_material;
            auto oldTextureMaps = networkMaterial->getTextureMaps();
            if (material->albedoTextureName == name) {
                material->albedoTexture = textureCache->getTexture(url, DEFAULT_TEXTURE);

                auto albedoMap = model::TextureMapPointer(new model::TextureMap());
                albedoMap->setTextureSource(material->albedoTexture->_textureSource);
                albedoMap->setTextureTransform(oldTextureMaps[model::MaterialKey::ALBEDO_MAP]->getTextureTransform());
                // when reassigning the albedo texture we also check for the alpha channel used as opacity
                albedoMap->setUseAlphaChannel(true); 
                networkMaterial->setTextureMap(model::MaterialKey::ALBEDO_MAP, albedoMap);
            } else if (material->normalTextureName == name) {
                material->normalTexture = textureCache->getTexture(url);

                auto normalMap = model::TextureMapPointer(new model::TextureMap());
                normalMap->setTextureSource(material->normalTexture->_textureSource);

                networkMaterial->setTextureMap(model::MaterialKey::NORMAL_MAP, normalMap);
            } else if (material->roughnessTextureName == name) {
                // FIXME: If passing a gloss map instead of a roughmap how to say that ? looking for gloss in the name ?
                material->roughnessTexture = textureCache->getTexture(url, ROUGHNESS_TEXTURE);

                auto roughnessMap = model::TextureMapPointer(new model::TextureMap());
                roughnessMap->setTextureSource(material->roughnessTexture->_textureSource);

                networkMaterial->setTextureMap(model::MaterialKey::ROUGHNESS_MAP, roughnessMap);
            } else if (material->metallicTextureName == name) {
                // FIXME: If passing a specular map instead of a metallic how to say that ? looking for wtf in the name ?
                material->metallicTexture = textureCache->getTexture(url, METALLIC_TEXTURE);

                auto glossMap = model::TextureMapPointer(new model::TextureMap());
                glossMap->setTextureSource(material->metallicTexture->_textureSource);

                networkMaterial->setTextureMap(model::MaterialKey::METALLIC_MAP, glossMap);
            } else if (material->emissiveTextureName == name) {
                material->emissiveTexture = textureCache->getTexture(url, EMISSIVE_TEXTURE);

                auto emissiveMap = model::TextureMapPointer(new model::TextureMap());
                emissiveMap->setTextureSource(material->emissiveTexture->_textureSource);

                networkMaterial->setTextureMap(model::MaterialKey::EMISSIVE_MAP, emissiveMap);
            } else if (material->lightmapTextureName == name) {
                material->lightmapTexture = textureCache->getTexture(url, LIGHTMAP_TEXTURE);

                auto lightmapMap = model::TextureMapPointer(new model::TextureMap());
                lightmapMap->setTextureSource(material->lightmapTexture->_textureSource);
                lightmapMap->setTextureTransform(
                    oldTextureMaps[model::MaterialKey::LIGHTMAP_MAP]->getTextureTransform());
                glm::vec2 oldOffsetScale =
                    oldTextureMaps[model::MaterialKey::LIGHTMAP_MAP]->getLightmapOffsetScale();
                lightmapMap->setLightmapOffsetScale(oldOffsetScale.x, oldOffsetScale.y);

                networkMaterial->setTextureMap(model::MaterialKey::LIGHTMAP_MAP, lightmapMap);
            }
        }
    } else {
        qCWarning(modelnetworking) << "Ignoring setTextureWithNameToURL() geometry not ready." << name << url;
    }
    _isLoadedWithTextures = false;
}

QStringList NetworkGeometry::getTextureNames() const {
    QStringList result;
    for (auto&& material : _materials) {
        if (!material->emissiveTextureName.isEmpty() && material->emissiveTexture) {
            QString textureURL = material->emissiveTexture->getURL().toString();
            result << material->emissiveTextureName + ":\"" + textureURL + "\"";
        }

        if (!material->albedoTextureName.isEmpty() && material->albedoTexture) {
            QString textureURL = material->albedoTexture->getURL().toString();
            result << material->albedoTextureName + ":\"" + textureURL + "\"";
        }

        if (!material->normalTextureName.isEmpty() && material->normalTexture) {
            QString textureURL = material->normalTexture->getURL().toString();
            result << material->normalTextureName + ":\"" + textureURL + "\"";
        }

        if (!material->roughnessTextureName.isEmpty() && material->roughnessTexture) {
            QString textureURL = material->roughnessTexture->getURL().toString();
            result << material->roughnessTextureName + ":\"" + textureURL + "\"";
        }

        if (!material->metallicTextureName.isEmpty() && material->metallicTexture) {
            QString textureURL = material->metallicTexture->getURL().toString();
            result << material->metallicTextureName + ":\"" + textureURL + "\"";
        }

        if (!material->occlusionTextureName.isEmpty() && material->occlusionTexture) {
            QString textureURL = material->occlusionTexture->getURL().toString();
            result << material->occlusionTextureName + ":\"" + textureURL + "\"";
        }

        if (!material->lightmapTextureName.isEmpty() && material->lightmapTexture) {
            QString textureURL = material->lightmapTexture->getURL().toString();
            result << material->lightmapTextureName + ":\"" + textureURL + "\"";
        }
    }

    return result;
}

void NetworkGeometry::requestMapping(const QUrl& url) {
    _state = RequestMappingState;
    if (_resource) {
        _resource->deleteLater();
    }
    _resource = new Resource(url, false);
    connect(_resource, &Resource::loaded, this, &NetworkGeometry::mappingRequestDone);
    connect(_resource, &Resource::failed, this, &NetworkGeometry::mappingRequestError);
}

void NetworkGeometry::requestModel(const QUrl& url) {
    _state = RequestModelState;
    if (_resource) {
        _resource->deleteLater();
    }
    _modelUrl = url;
    _resource = new Resource(url, false);
    connect(_resource, &Resource::loaded, this, &NetworkGeometry::modelRequestDone);
    connect(_resource, &Resource::failed, this, &NetworkGeometry::modelRequestError);
}

void NetworkGeometry::mappingRequestDone(const QByteArray data) {
    assert(_state == RequestMappingState);

    // parse the mapping file
    _mapping = FSTReader::readMapping(data);

    QUrl replyUrl = _mappingUrl;
    QString modelUrlStr = _mapping.value("filename").toString();
    if (modelUrlStr.isNull()) {
        qCDebug(modelnetworking) << "Mapping file " << _url << "has no \"filename\" entry";
        emit onFailure(*this, MissingFilenameInMapping);
    } else {
        // read _textureBase from mapping file, if present
        QString texdir = _mapping.value("texdir").toString();
        if (!texdir.isNull()) {
            if (!texdir.endsWith('/')) {
                texdir += '/';
            }
            _textureBaseUrl = replyUrl.resolved(texdir);
        }

        _modelUrl = replyUrl.resolved(modelUrlStr);
        requestModel(_modelUrl);
    }
}

void NetworkGeometry::mappingRequestError(QNetworkReply::NetworkError error) {
    assert(_state == RequestMappingState);
    _state = ErrorState;
    emit onFailure(*this, MappingRequestError);
}

void NetworkGeometry::modelRequestDone(const QByteArray data) {
    assert(_state == RequestModelState);

    _state = ParsingModelState;

    // asynchronously parse the model file.
    GeometryReader* geometryReader = new GeometryReader(_modelUrl, data, _mapping);
    connect(geometryReader, SIGNAL(onSuccess(FBXGeometry*)), SLOT(modelParseSuccess(FBXGeometry*)));
    connect(geometryReader, SIGNAL(onError(int, QString)), SLOT(modelParseError(int, QString)));

    QThreadPool::globalInstance()->start(geometryReader);
}

void NetworkGeometry::modelRequestError(QNetworkReply::NetworkError error) {
    assert(_state == RequestModelState);
    _state = ErrorState;
    emit onFailure(*this, ModelRequestError);
}

static NetworkMesh* buildNetworkMesh(const FBXMesh& mesh, const QUrl& textureBaseUrl) {
    NetworkMesh* networkMesh = new NetworkMesh();

    networkMesh->_mesh = mesh._mesh;

    return networkMesh;
}


static model::TextureMapPointer setupNetworkTextureMap(NetworkGeometry* geometry, const QUrl& textureBaseUrl,
        const FBXTexture& texture, TextureType type,
        NetworkTexturePointer& networkTexture, QString& networkTextureName) {
    auto textureCache = DependencyManager::get<TextureCache>();

    // If content is inline, cache it under the fbx file, not its base url
    const auto baseUrl = texture.content.isEmpty() ? textureBaseUrl : QUrl(textureBaseUrl.url() + "/");
    const auto filename = baseUrl.resolved(QUrl(texture.filename));

    networkTexture = textureCache->getTexture(filename, type, texture.content);
    QObject::connect(networkTexture.data(), &NetworkTexture::networkTextureCreated, geometry, &NetworkGeometry::textureLoaded);
    networkTextureName = texture.name;

    auto map = std::make_shared<model::TextureMap>();
    map->setTextureSource(networkTexture->_textureSource);
    return map;
}

static NetworkMaterial* buildNetworkMaterial(NetworkGeometry* geometry, const FBXMaterial& material, const QUrl& textureBaseUrl) {
    NetworkMaterial* networkMaterial = new NetworkMaterial();
    networkMaterial->_material = material._material;

    if (!material.albedoTexture.filename.isEmpty()) {
        auto albedoMap = setupNetworkTextureMap(geometry, textureBaseUrl, material.albedoTexture, DEFAULT_TEXTURE,
            networkMaterial->albedoTexture, networkMaterial->albedoTextureName);
        albedoMap->setTextureTransform(material.albedoTexture.transform);

        if (!material.opacityTexture.filename.isEmpty()) {
            if (material.albedoTexture.filename == material.opacityTexture.filename) {
                // Best case scenario, just indicating that the albedo map contains transparency
                albedoMap->setUseAlphaChannel(true);
            } else {
                // Opacity Map is different from the Abledo map, not supported
            }
        }

        material._material->setTextureMap(model::MaterialKey::ALBEDO_MAP, albedoMap);
    }


    if (!material.normalTexture.filename.isEmpty()) {
        auto normalMap = setupNetworkTextureMap(geometry, textureBaseUrl, material.normalTexture,
            (material.normalTexture.isBumpmap ? BUMP_TEXTURE : NORMAL_TEXTURE),
            networkMaterial->normalTexture, networkMaterial->normalTextureName);
        networkMaterial->_material->setTextureMap(model::MaterialKey::NORMAL_MAP, normalMap);
    }

    // Roughness first or gloss maybe
    if (!material.roughnessTexture.filename.isEmpty()) {
        auto roughnessMap = setupNetworkTextureMap(geometry, textureBaseUrl, material.roughnessTexture, ROUGHNESS_TEXTURE,
            networkMaterial->roughnessTexture, networkMaterial->roughnessTextureName);
        material._material->setTextureMap(model::MaterialKey::ROUGHNESS_MAP, roughnessMap);
    } else if (!material.glossTexture.filename.isEmpty()) {
        auto roughnessMap = setupNetworkTextureMap(geometry, textureBaseUrl, material.glossTexture, GLOSS_TEXTURE,
            networkMaterial->roughnessTexture, networkMaterial->roughnessTextureName);
        material._material->setTextureMap(model::MaterialKey::ROUGHNESS_MAP, roughnessMap);
    }

    // Metallic first or specular maybe

    if (!material.metallicTexture.filename.isEmpty()) {
        auto metallicMap = setupNetworkTextureMap(geometry, textureBaseUrl, material.metallicTexture, METALLIC_TEXTURE,
            networkMaterial->metallicTexture, networkMaterial->metallicTextureName);
        material._material->setTextureMap(model::MaterialKey::METALLIC_MAP, metallicMap);
    } else if (!material.specularTexture.filename.isEmpty()) {

        auto metallicMap = setupNetworkTextureMap(geometry, textureBaseUrl, material.specularTexture, SPECULAR_TEXTURE,
            networkMaterial->metallicTexture, networkMaterial->metallicTextureName);
        material._material->setTextureMap(model::MaterialKey::METALLIC_MAP, metallicMap);
    }

    if (!material.occlusionTexture.filename.isEmpty()) {
        auto occlusionMap = setupNetworkTextureMap(geometry, textureBaseUrl, material.occlusionTexture, OCCLUSION_TEXTURE,
            networkMaterial->occlusionTexture, networkMaterial->occlusionTextureName);
        material._material->setTextureMap(model::MaterialKey::OCCLUSION_MAP, occlusionMap);
    }

    if (!material.emissiveTexture.filename.isEmpty()) {
        auto emissiveMap = setupNetworkTextureMap(geometry, textureBaseUrl, material.emissiveTexture, EMISSIVE_TEXTURE,
            networkMaterial->emissiveTexture, networkMaterial->emissiveTextureName);
        material._material->setTextureMap(model::MaterialKey::EMISSIVE_MAP, emissiveMap);
    }

    if (!material.lightmapTexture.filename.isEmpty()) {
        auto lightmapMap = setupNetworkTextureMap(geometry, textureBaseUrl, material.lightmapTexture, LIGHTMAP_TEXTURE,
            networkMaterial->lightmapTexture, networkMaterial->lightmapTextureName);
        lightmapMap->setTextureTransform(material.lightmapTexture.transform);
        lightmapMap->setLightmapOffsetScale(material.lightmapParams.x, material.lightmapParams.y);
        material._material->setTextureMap(model::MaterialKey::LIGHTMAP_MAP, lightmapMap);
    }

    return networkMaterial;
}


void NetworkGeometry::modelParseSuccess(FBXGeometry* geometry) {
    // assume owner ship of geometry pointer
    _geometry.reset(geometry);



    foreach(const FBXMesh& mesh, _geometry->meshes) {
        _meshes.emplace_back(buildNetworkMesh(mesh, _textureBaseUrl));
    }

    QHash<QString, size_t> fbxMatIDToMatID;
    foreach(const FBXMaterial& material, _geometry->materials) {
        fbxMatIDToMatID[material.materialID] = _materials.size();
        _materials.emplace_back(buildNetworkMaterial(this, material, _textureBaseUrl));
    }


    int meshID = 0;
    foreach(const FBXMesh& mesh, _geometry->meshes) {
        int partID = 0;
        foreach (const FBXMeshPart& part, mesh.parts) {
            NetworkShape* networkShape = new NetworkShape();
            networkShape->_meshID = meshID;
            networkShape->_partID = partID;
            networkShape->_materialID = (int)fbxMatIDToMatID[part.materialID];
            _shapes.emplace_back(networkShape);
            partID++;
        }
        meshID++;
    }

    _state = SuccessState;
    emit onSuccess(*this, *_geometry.get());

    delete _resource;
    _resource = nullptr;
}

void NetworkGeometry::modelParseError(int error, QString str) {
    _state = ErrorState;
    emit onFailure(*this, (NetworkGeometry::Error)error);

    delete _resource;
    _resource = nullptr;
}

const NetworkMaterial* NetworkGeometry::getShapeMaterial(int shapeID) {
    if ((shapeID >= 0) && (shapeID < (int)_shapes.size())) {
        int materialID = _shapes[shapeID]->_materialID;
        if ((materialID >= 0) && ((unsigned int)materialID < _materials.size())) {
            return _materials[materialID].get();
        } else {
            return 0;
        }
    } else {
        return 0;
    }
}

void NetworkGeometry::textureLoaded(const QWeakPointer<NetworkTexture>& networkTexture) {
    numTextureLoaded++;
}
