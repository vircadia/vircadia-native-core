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

#include "gpu/StandardShaderLib.h"

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
            } else if (_url.path().toLower().endsWith(".obj")) {
                fbxgeo = OBJReader().readOBJ(_data, _mapping, _url);
            } else {
                QString errorStr("usupported format");
                emit onError(NetworkGeometry::ModelParseError, errorStr);
            }
            emit onSuccess(fbxgeo);
        } else {
            throw QString("url is invalid");
        }

    } catch (const QString& error) {
        qCDebug(modelnetworking) << "Error reading " << _url << ": " << error;
        emit onError(NetworkGeometry::ModelParseError, error);
    }
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
        for (auto&& material : _materials) {
            if ((material->diffuseTexture && !material->diffuseTexture->isLoaded()) ||
                (material->normalTexture && !material->normalTexture->isLoaded()) ||
                (material->specularTexture && !material->specularTexture->isLoaded()) ||
                (material->emissiveTexture && !material->emissiveTexture->isLoaded())) {
                return false;
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
            QSharedPointer<NetworkTexture> matchingTexture = QSharedPointer<NetworkTexture>();
            if (material->diffuseTextureName == name) {
                material->diffuseTexture = textureCache->getTexture(url, DEFAULT_TEXTURE);
            } else if (material->normalTextureName == name) {
                material->normalTexture = textureCache->getTexture(url);
            } else if (material->specularTextureName == name) {
                material->specularTexture = textureCache->getTexture(url);
            } else if (material->emissiveTextureName == name) {
                material->emissiveTexture = textureCache->getTexture(url);
            }
        }
    } else {
        qCWarning(modelnetworking) << "Ignoring setTextureWirthNameToURL() geometry not ready." << name << url;
    }
    _isLoadedWithTextures = false;
}

QStringList NetworkGeometry::getTextureNames() const {
    QStringList result;
    for (auto&& material : _materials) {
        if (!material->diffuseTextureName.isEmpty() && material->diffuseTexture) {
            QString textureURL = material->diffuseTexture->getURL().toString();
            result << material->diffuseTextureName + ":" + textureURL;
        }

        if (!material->normalTextureName.isEmpty() && material->normalTexture) {
            QString textureURL = material->normalTexture->getURL().toString();
            result << material->normalTextureName + ":" + textureURL;
        }

        if (!material->specularTextureName.isEmpty() && material->specularTexture) {
            QString textureURL = material->specularTexture->getURL().toString();
            result << material->specularTextureName + ":" + textureURL;
        }

        if (!material->emissiveTextureName.isEmpty() && material->emissiveTexture) {
            QString textureURL = material->emissiveTexture->getURL().toString();
            result << material->emissiveTextureName + ":" + textureURL;
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

void NetworkGeometry::mappingRequestDone(const QByteArray& data) {
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

void NetworkGeometry::modelRequestDone(const QByteArray& data) {
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
    auto textureCache = DependencyManager::get<TextureCache>();
    NetworkMesh* networkMesh = new NetworkMesh();

    int totalIndices = 0;
    //bool checkForTexcoordLightmap = false;



    // process network parts
    foreach (const FBXMeshPart& part, mesh.parts) {
        totalIndices += (part.quadIndices.size() + part.triangleIndices.size());
    }

    // initialize index buffer
    {
        networkMesh->_indexBuffer = std::make_shared<gpu::Buffer>();
        networkMesh->_indexBuffer->resize(totalIndices * sizeof(int));
        int offset = 0;
        foreach(const FBXMeshPart& part, mesh.parts) {
            networkMesh->_indexBuffer->setSubData(offset, part.quadIndices.size() * sizeof(int),
                                                  (gpu::Byte*) part.quadIndices.constData());
            offset += part.quadIndices.size() * sizeof(int);
            networkMesh->_indexBuffer->setSubData(offset, part.triangleIndices.size() * sizeof(int),
                                                  (gpu::Byte*) part.triangleIndices.constData());
            offset += part.triangleIndices.size() * sizeof(int);
        }
    }

    // initialize vertex buffer
    {
        networkMesh->_vertexBuffer = std::make_shared<gpu::Buffer>();
        // if we don't need to do any blending, the positions/normals can be static
        if (mesh.blendshapes.isEmpty()) {
            int normalsOffset = mesh.vertices.size() * sizeof(glm::vec3);
            int tangentsOffset = normalsOffset + mesh.normals.size() * sizeof(glm::vec3);
            int colorsOffset = tangentsOffset + mesh.tangents.size() * sizeof(glm::vec3);
            int texCoordsOffset = colorsOffset + mesh.colors.size() * sizeof(glm::vec3);
            int texCoords1Offset = texCoordsOffset + mesh.texCoords.size() * sizeof(glm::vec2);
            int clusterIndicesOffset = texCoords1Offset + mesh.texCoords1.size() * sizeof(glm::vec2);
            int clusterWeightsOffset = clusterIndicesOffset + mesh.clusterIndices.size() * sizeof(glm::vec4);

            networkMesh->_vertexBuffer->resize(clusterWeightsOffset + mesh.clusterWeights.size() * sizeof(glm::vec4));

            networkMesh->_vertexBuffer->setSubData(0, mesh.vertices.size() * sizeof(glm::vec3), (gpu::Byte*) mesh.vertices.constData());
            networkMesh->_vertexBuffer->setSubData(normalsOffset, mesh.normals.size() * sizeof(glm::vec3), (gpu::Byte*) mesh.normals.constData());
            networkMesh->_vertexBuffer->setSubData(tangentsOffset,
                                                   mesh.tangents.size() * sizeof(glm::vec3), (gpu::Byte*) mesh.tangents.constData());
            networkMesh->_vertexBuffer->setSubData(colorsOffset, mesh.colors.size() * sizeof(glm::vec3), (gpu::Byte*) mesh.colors.constData());
            networkMesh->_vertexBuffer->setSubData(texCoordsOffset,
                                                   mesh.texCoords.size() * sizeof(glm::vec2), (gpu::Byte*) mesh.texCoords.constData());
            networkMesh->_vertexBuffer->setSubData(texCoords1Offset,
                                                   mesh.texCoords1.size() * sizeof(glm::vec2), (gpu::Byte*) mesh.texCoords1.constData());
            networkMesh->_vertexBuffer->setSubData(clusterIndicesOffset,
                                                   mesh.clusterIndices.size() * sizeof(glm::vec4), (gpu::Byte*) mesh.clusterIndices.constData());
            networkMesh->_vertexBuffer->setSubData(clusterWeightsOffset,
                                                   mesh.clusterWeights.size() * sizeof(glm::vec4), (gpu::Byte*) mesh.clusterWeights.constData());

            // otherwise, at least the cluster indices/weights can be static
            networkMesh->_vertexStream = std::make_shared<gpu::BufferStream>();
            networkMesh->_vertexStream->addBuffer(networkMesh->_vertexBuffer, 0, sizeof(glm::vec3));
            if (mesh.normals.size()) networkMesh->_vertexStream->addBuffer(networkMesh->_vertexBuffer, normalsOffset, sizeof(glm::vec3));
            if (mesh.tangents.size()) networkMesh->_vertexStream->addBuffer(networkMesh->_vertexBuffer, tangentsOffset, sizeof(glm::vec3));
            if (mesh.colors.size()) networkMesh->_vertexStream->addBuffer(networkMesh->_vertexBuffer, colorsOffset, sizeof(glm::vec3));
            if (mesh.texCoords.size()) networkMesh->_vertexStream->addBuffer(networkMesh->_vertexBuffer, texCoordsOffset, sizeof(glm::vec2));
            if (mesh.texCoords1.size()) networkMesh->_vertexStream->addBuffer(networkMesh->_vertexBuffer, texCoords1Offset, sizeof(glm::vec2));
            if (mesh.clusterIndices.size()) networkMesh->_vertexStream->addBuffer(networkMesh->_vertexBuffer, clusterIndicesOffset, sizeof(glm::vec4));
            if (mesh.clusterWeights.size()) networkMesh->_vertexStream->addBuffer(networkMesh->_vertexBuffer, clusterWeightsOffset, sizeof(glm::vec4));

            int channelNum = 0;
            networkMesh->_vertexFormat = std::make_shared<gpu::Stream::Format>();
            networkMesh->_vertexFormat->setAttribute(gpu::Stream::POSITION, channelNum++, gpu::Element(gpu::VEC3, gpu::FLOAT, gpu::XYZ), 0);
            if (mesh.normals.size()) networkMesh->_vertexFormat->setAttribute(gpu::Stream::NORMAL, channelNum++, gpu::Element(gpu::VEC3, gpu::FLOAT, gpu::XYZ));
            if (mesh.tangents.size()) networkMesh->_vertexFormat->setAttribute(gpu::Stream::TANGENT, channelNum++, gpu::Element(gpu::VEC3, gpu::FLOAT, gpu::XYZ));
            if (mesh.colors.size()) networkMesh->_vertexFormat->setAttribute(gpu::Stream::COLOR, channelNum++, gpu::Element(gpu::VEC3, gpu::FLOAT, gpu::RGB));
            if (mesh.texCoords.size()) networkMesh->_vertexFormat->setAttribute(gpu::Stream::TEXCOORD, channelNum++, gpu::Element(gpu::VEC2, gpu::FLOAT, gpu::UV));
            if (mesh.texCoords1.size()) {
                networkMesh->_vertexFormat->setAttribute(gpu::Stream::TEXCOORD1, channelNum++, gpu::Element(gpu::VEC2, gpu::FLOAT, gpu::UV));
          //  } else if (checkForTexcoordLightmap && mesh.texCoords.size()) {
            } else if (mesh.texCoords.size()) {
                // need lightmap texcoord UV but doesn't have uv#1 so just reuse the same channel
                networkMesh->_vertexFormat->setAttribute(gpu::Stream::TEXCOORD1, channelNum - 1, gpu::Element(gpu::VEC2, gpu::FLOAT, gpu::UV));
            }
            if (mesh.clusterIndices.size()) networkMesh->_vertexFormat->setAttribute(gpu::Stream::SKIN_CLUSTER_INDEX, channelNum++, gpu::Element(gpu::VEC4, gpu::FLOAT, gpu::XYZW));
            if (mesh.clusterWeights.size()) networkMesh->_vertexFormat->setAttribute(gpu::Stream::SKIN_CLUSTER_WEIGHT, channelNum++, gpu::Element(gpu::VEC4, gpu::FLOAT, gpu::XYZW));
        }
        else {
            int colorsOffset = mesh.tangents.size() * sizeof(glm::vec3);
            int texCoordsOffset = colorsOffset + mesh.colors.size() * sizeof(glm::vec3);
            int clusterIndicesOffset = texCoordsOffset + mesh.texCoords.size() * sizeof(glm::vec2);
            int clusterWeightsOffset = clusterIndicesOffset + mesh.clusterIndices.size() * sizeof(glm::vec4);

            networkMesh->_vertexBuffer->resize(clusterWeightsOffset + mesh.clusterWeights.size() * sizeof(glm::vec4));
            networkMesh->_vertexBuffer->setSubData(0, mesh.tangents.size() * sizeof(glm::vec3), (gpu::Byte*) mesh.tangents.constData());
            networkMesh->_vertexBuffer->setSubData(colorsOffset, mesh.colors.size() * sizeof(glm::vec3), (gpu::Byte*) mesh.colors.constData());
            networkMesh->_vertexBuffer->setSubData(texCoordsOffset,
                                                   mesh.texCoords.size() * sizeof(glm::vec2), (gpu::Byte*) mesh.texCoords.constData());
            networkMesh->_vertexBuffer->setSubData(clusterIndicesOffset,
                                                   mesh.clusterIndices.size() * sizeof(glm::vec4), (gpu::Byte*) mesh.clusterIndices.constData());
            networkMesh->_vertexBuffer->setSubData(clusterWeightsOffset,
                                                   mesh.clusterWeights.size() * sizeof(glm::vec4), (gpu::Byte*) mesh.clusterWeights.constData());

            networkMesh->_vertexStream = std::make_shared<gpu::BufferStream>();
            if (mesh.tangents.size()) networkMesh->_vertexStream->addBuffer(networkMesh->_vertexBuffer, 0, sizeof(glm::vec3));
            if (mesh.colors.size()) networkMesh->_vertexStream->addBuffer(networkMesh->_vertexBuffer, colorsOffset, sizeof(glm::vec3));
            if (mesh.texCoords.size()) networkMesh->_vertexStream->addBuffer(networkMesh->_vertexBuffer, texCoordsOffset, sizeof(glm::vec2));
            if (mesh.clusterIndices.size()) networkMesh->_vertexStream->addBuffer(networkMesh->_vertexBuffer, clusterIndicesOffset, sizeof(glm::vec4));
            if (mesh.clusterWeights.size()) networkMesh->_vertexStream->addBuffer(networkMesh->_vertexBuffer, clusterWeightsOffset, sizeof(glm::vec4));

            int channelNum = 0;
            networkMesh->_vertexFormat = std::make_shared<gpu::Stream::Format>();
            networkMesh->_vertexFormat->setAttribute(gpu::Stream::POSITION, channelNum++, gpu::Element(gpu::VEC3, gpu::FLOAT, gpu::XYZ));
            if (mesh.normals.size()) networkMesh->_vertexFormat->setAttribute(gpu::Stream::NORMAL, channelNum++, gpu::Element(gpu::VEC3, gpu::FLOAT, gpu::XYZ));
            if (mesh.tangents.size()) networkMesh->_vertexFormat->setAttribute(gpu::Stream::TANGENT, channelNum++, gpu::Element(gpu::VEC3, gpu::FLOAT, gpu::XYZ));
            if (mesh.colors.size()) networkMesh->_vertexFormat->setAttribute(gpu::Stream::COLOR, channelNum++, gpu::Element(gpu::VEC3, gpu::FLOAT, gpu::RGB));
            if (mesh.texCoords.size()) networkMesh->_vertexFormat->setAttribute(gpu::Stream::TEXCOORD, channelNum++, gpu::Element(gpu::VEC2, gpu::FLOAT, gpu::UV));
            if (mesh.clusterIndices.size()) networkMesh->_vertexFormat->setAttribute(gpu::Stream::SKIN_CLUSTER_INDEX, channelNum++, gpu::Element(gpu::VEC4, gpu::FLOAT, gpu::XYZW));
            if (mesh.clusterWeights.size()) networkMesh->_vertexFormat->setAttribute(gpu::Stream::SKIN_CLUSTER_WEIGHT, channelNum++, gpu::Element(gpu::VEC4, gpu::FLOAT, gpu::XYZW));
        }
    }

    return networkMesh;
}

static NetworkMaterial* buildNetworkMaterial(const FBXMaterial& material, const QUrl& textureBaseUrl) {
    auto textureCache = DependencyManager::get<TextureCache>();
    NetworkMaterial* networkMaterial = new NetworkMaterial();

    //bool checkForTexcoordLightmap = false;

    networkMaterial->_material = material._material;

    if (!material.diffuseTexture.filename.isEmpty()) {
        networkMaterial->diffuseTexture = textureCache->getTexture(textureBaseUrl.resolved(QUrl(material.diffuseTexture.filename)), DEFAULT_TEXTURE, material.diffuseTexture.content);
        networkMaterial->diffuseTextureName = material.diffuseTexture.name;

        auto diffuseMap = model::TextureMapPointer(new model::TextureMap());
        diffuseMap->setTextureSource(networkMaterial->diffuseTexture->_textureSource);
        diffuseMap->setTextureTransform(material.diffuseTexture.transform);

        material._material->setTextureMap(model::MaterialKey::DIFFUSE_MAP, diffuseMap);
    }
    if (!material.normalTexture.filename.isEmpty()) {
        networkMaterial->normalTexture = textureCache->getTexture(textureBaseUrl.resolved(QUrl(material.normalTexture.filename)), (material.normalTexture.isBumpmap ? BUMP_TEXTURE : NORMAL_TEXTURE), material.normalTexture.content);
        networkMaterial->normalTextureName = material.normalTexture.name;

        auto normalMap = model::TextureMapPointer(new model::TextureMap());
        normalMap->setTextureSource(networkMaterial->normalTexture->_textureSource);

        material._material->setTextureMap(model::MaterialKey::NORMAL_MAP, normalMap);
    }
    if (!material.specularTexture.filename.isEmpty()) {
        networkMaterial->specularTexture = textureCache->getTexture(textureBaseUrl.resolved(QUrl(material.specularTexture.filename)), SPECULAR_TEXTURE, material.specularTexture.content);
        networkMaterial->specularTextureName = material.specularTexture.name;

        auto glossMap = model::TextureMapPointer(new model::TextureMap());
        glossMap->setTextureSource(networkMaterial->specularTexture->_textureSource);

        material._material->setTextureMap(model::MaterialKey::GLOSS_MAP, glossMap);
    }
    if (!material.emissiveTexture.filename.isEmpty()) {
        networkMaterial->emissiveTexture = textureCache->getTexture(textureBaseUrl.resolved(QUrl(material.emissiveTexture.filename)), EMISSIVE_TEXTURE, material.emissiveTexture.content);
        networkMaterial->emissiveTextureName = material.emissiveTexture.name;

        //checkForTexcoordLightmap = true;

        auto lightmapMap = model::TextureMapPointer(new model::TextureMap());
        lightmapMap->setTextureSource(networkMaterial->emissiveTexture->_textureSource);
        lightmapMap->setTextureTransform(material.emissiveTexture.transform);
        lightmapMap->setLightmapOffsetScale(material.emissiveParams.x, material.emissiveParams.y);

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

    QHash<QString, int> fbxMatIDToMatID;
    foreach(const FBXMaterial& material, _geometry->materials) {
        fbxMatIDToMatID[material.materialID] = _materials.size();
        _materials.emplace_back(buildNetworkMaterial(material, _textureBaseUrl));
    }


    int meshID = 0;
    foreach(const FBXMesh& mesh, _geometry->meshes) {
        int partID = 0;
        foreach (const FBXMeshPart& part, mesh.parts) {
            NetworkShape* networkShape = new NetworkShape();
            networkShape->_meshID = meshID;
            networkShape->_partID = partID;
            networkShape->_materialID = fbxMatIDToMatID[part.materialID];
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

