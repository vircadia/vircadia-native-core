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

class GeometryExtra {
public:
    const GeometryMappingPair& mapping;
    const QUrl& textureBaseUrl;
    bool combineParts;
};

int geometryMappingPairTypeId = qRegisterMetaType<GeometryMappingPair>("GeometryMappingPair");

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
        size_t operator()(const GeometryExtra& geometryExtra) const {
            size_t result = 0;
            hash_combine(result, geometryExtra.mapping.first, geometryExtra.mapping.second, geometryExtra.textureBaseUrl,
                geometryExtra.combineParts);
            return result;
        }
    };
}

class GeometryReader : public QRunnable {
public:
    GeometryReader(const ModelLoader& modelLoader, QWeakPointer<Resource>& resource, const QUrl& url, const GeometryMappingPair& mapping,
                   const QByteArray& data, bool combineParts, const QString& webMediaType) :
        _modelLoader(modelLoader), _resource(resource), _url(url), _mapping(mapping), _data(data), _combineParts(combineParts), _webMediaType(webMediaType) {

        DependencyManager::get<StatTracker>()->incrementStat("PendingProcessing");
    }

    virtual void run() override;

private:
    ModelLoader _modelLoader;
    QWeakPointer<Resource> _resource;
    QUrl _url;
    GeometryMappingPair _mapping;
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
        QVariantHash serializerMapping = _mapping.second;
        serializerMapping["combineParts"] = _combineParts;
        serializerMapping["deduplicateIndices"] = true;

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
        if (!serializerMapping.value(SCRIPT_FIELD).isNull()) {
            QVariantList scripts = serializerMapping.values(SCRIPT_FIELD);
            for (auto &script : scripts) {
                hfmModel->scripts.push_back(script.toString());
            }
        }
        QMetaObject::invokeMethod(resource.data(), "setGeometryDefinition",
                Q_ARG(HFMModel::Pointer, hfmModel), Q_ARG(GeometryMappingPair, _mapping));
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

QUrl resolveTextureBaseUrl(const QUrl& url, const QUrl& textureBaseUrl) {
    return textureBaseUrl.isValid() ? textureBaseUrl : url;
}

GeometryResource::GeometryResource(const GeometryResource& other) :
    Resource(other),
    Geometry(other),
    _modelLoader(other._modelLoader),
    _mappingPair(other._mappingPair),
    _textureBaseURL(other._textureBaseURL),
    _combineParts(other._combineParts),
    _isCacheable(other._isCacheable)
{
    if (other._geometryResource) {
        _startedLoading = false;
    }
}

void GeometryResource::downloadFinished(const QByteArray& data) {
    if (_activeUrl.fileName().toLower().endsWith(".fst")) {
        PROFILE_ASYNC_BEGIN(resource_parse_geometry, "GeometryResource::downloadFinished", _url.toString(), { { "url", _url.toString() } });

        // store parsed contents of FST file
        _mapping = FSTReader::readMapping(data);

        QString filename = _mapping.value("filename").toString();

        if (filename.isNull()) {
            finishedLoading(false);
        } else {
            const QString baseURL = _mapping.value("baseURL").toString();
            const QUrl base = _effectiveBaseURL.resolved(baseURL);
            QUrl url = base.resolved(filename);

            QString texdir = _mapping.value(TEXDIR_FIELD).toString();
            if (!texdir.isNull()) {
                if (!texdir.endsWith('/')) {
                    texdir += '/';
                }
                _textureBaseURL = resolveTextureBaseUrl(url, base.resolved(texdir));
            } else {
                _textureBaseURL = url.resolved(QUrl("."));
            }

            auto scripts = FSTReader::getScripts(base, _mapping);
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
                    _animGraphOverrideUrl = base.resolved(fstUrl);
                } else {
                    _animGraphOverrideUrl = QUrl();
                }
            } else {
                _animGraphOverrideUrl = QUrl();
            }

            auto modelCache = DependencyManager::get<ModelCache>();
            GeometryExtra extra { GeometryMappingPair(base, _mapping), _textureBaseURL, false };

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

                _connection = connect(_geometryResource.data(), &Resource::finished, this, &GeometryResource::onGeometryMappingLoaded);
            }
        }
    } else {
        if (_url != _effectiveBaseURL) {
            _url = _effectiveBaseURL;
            _textureBaseURL = _effectiveBaseURL;
        }
        QThreadPool::globalInstance()->start(new GeometryReader(_modelLoader, _self, _effectiveBaseURL, _mappingPair, data, _combineParts, _request->getWebMediaType()));
    }
}

bool GeometryResource::handleFailedRequest(ResourceRequest::Result result) {
    if (_shouldFailOnRedirect && result == ResourceRequest::Result::RedirectFail) {
        auto newPath = _request->getRelativePathUrl();
        if (newPath.fileName().toLower().endsWith(".fst")) {
            _activeUrl = newPath;
            _shouldFailOnRedirect = false;
            makeRequest();
            return true;
        }
    }
    return Resource::handleFailedRequest(result);
}

void GeometryResource::onGeometryMappingLoaded(bool success) {
    if (success && _geometryResource) {
        _hfmModel = _geometryResource->_hfmModel;
        _materialMapping = _geometryResource->_materialMapping;
        _meshParts = _geometryResource->_meshParts;
        _meshes = _geometryResource->_meshes;
        _materials = _geometryResource->_materials;

        // Avoid holding onto extra references
        _geometryResource.reset();
        // Make sure connection will not trigger again
        disconnect(_connection); // FIXME Should not have to do this
    }

    PROFILE_ASYNC_END(resource_parse_geometry, "GeometryResource::downloadFinished", _url.toString());
    finishedLoading(success);
}

void GeometryResource::setExtra(void* extra) {
    const GeometryExtra* geometryExtra = static_cast<const GeometryExtra*>(extra);
    _mappingPair = geometryExtra ? geometryExtra->mapping : GeometryMappingPair(QUrl(), QVariantHash());
    _textureBaseURL = geometryExtra ? resolveTextureBaseUrl(_url, geometryExtra->textureBaseUrl) : QUrl();
    _combineParts = geometryExtra ? geometryExtra->combineParts : true;
}

void GeometryResource::setGeometryDefinition(HFMModel::Pointer hfmModel, const GeometryMappingPair& mapping) {
    // Do processing on the model
    baker::Baker modelBaker(hfmModel, mapping.second, mapping.first);
    modelBaker.run();

    // Assume ownership of the processed HFMModel
    _hfmModel = modelBaker.getHFMModel();
    _materialMapping = modelBaker.getMaterialMapping();

    // Copy materials
    QHash<QString, size_t> materialIDAtlas;
    for (const HFMMaterial& material : _hfmModel->materials) {
        materialIDAtlas[material.materialID] = _materials.size();
        _materials.push_back(std::make_shared<NetworkMaterial>(material, _textureBaseURL));
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

void GeometryResource::deleter() {
    resetTextures();
    Resource::deleter();
}

void GeometryResource::setTextures() {
    if (_hfmModel) {
        for (const HFMMaterial& material : _hfmModel->materials) {
            _materials.push_back(std::make_shared<NetworkMaterial>(material, _textureBaseURL));
        }
    }
}

void GeometryResource::resetTextures() {
    _materials.clear();
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
    return QSharedPointer<Resource>(new GeometryResource(url, _modelLoader), &GeometryResource::deleter);
}

QSharedPointer<Resource> ModelCache::createResourceCopy(const QSharedPointer<Resource>& resource) {
    return QSharedPointer<Resource>(new GeometryResource(*resource.staticCast<GeometryResource>()), &GeometryResource::deleter);
}

GeometryResource::Pointer ModelCache::getGeometryResource(const QUrl& url, const GeometryMappingPair& mapping, const QUrl& textureBaseUrl) {
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
                                                                   const GeometryMappingPair& mapping,
                                                                   const QUrl& textureBaseUrl) {
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
            if (texture.second.texture) {
                textures[texture.second.name] = texture.second.texture->getURL();
            }
        }
    }

    return textures;
}

// FIXME: The materials should only be copied when modified, but the Model currently caches the original
Geometry::Geometry(const Geometry& geometry) {
    _hfmModel = geometry._hfmModel;
    _materialMapping = geometry._materialMapping;
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
                [&textureMap](const NetworkMaterial::Textures::value_type& it) { return it.second.texture && textureMap.contains(it.second.name); })) {

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

#include "ModelCache.moc"
