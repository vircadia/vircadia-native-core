//
//  ModelCache.h
//  libraries/model-networking
//
//  Created by Zach Pomerantz on 3/15/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ModelCache_h
#define hifi_ModelCache_h

#include <DependencyManager.h>
#include <ResourceCache.h>

#include <graphics/Asset.h>

#include "FBXSerializer.h"
#include <procedural/ProceduralMaterialCache.h>
#include <material-networking/TextureCache.h>
#include "ModelLoader.h"

using GeometryMappingPair = std::pair<QUrl, QVariantHash>;
Q_DECLARE_METATYPE(GeometryMappingPair)

class NetworkModel {
public:
    using Pointer = std::shared_ptr<NetworkModel>;
    using WeakPointer = std::weak_ptr<NetworkModel>;

    NetworkModel() = default;
    NetworkModel(const NetworkModel& geometry);
    virtual ~NetworkModel() = default;

    // Immutable over lifetime
    using GeometryMeshes = std::vector<std::shared_ptr<const graphics::Mesh>>;

    // Mutable, but must retain structure of vector
    using NetworkMaterials = std::vector<std::shared_ptr<NetworkMaterial>>;

    bool isHFMModelLoaded() const { return (bool)_hfmModel; }

    const HFMModel& getHFMModel() const { return *_hfmModel; }
    const HFMModel::ConstPointer& getConstHFMModelPointer() const { return _hfmModel; }
    const MaterialMapping& getMaterialMapping() const { return _materialMapping; }
    const GeometryMeshes& getMeshes() const { return *_meshes; }
    const std::shared_ptr<NetworkMaterial> getShapeMaterial(int shapeID) const;

    const QVariantMap getTextures() const;
    void setTextures(const QVariantMap& textureMap);

    virtual bool areTexturesLoaded() const;
    const QUrl& getAnimGraphOverrideUrl() const { return _animGraphOverrideUrl; }
    const QVariantHash& getMapping() const { return _mapping; }

protected:
    // Shared across all geometries, constant throughout lifetime
    HFMModel::ConstPointer _hfmModel;
    MaterialMapping _materialMapping;
    std::shared_ptr<const GeometryMeshes> _meshes;

    // Copied to each geometry, mutable throughout lifetime via setTextures
    NetworkMaterials _materials;

    QUrl _animGraphOverrideUrl;
    QVariantHash _mapping;  // parsed contents of FST file.

private:
    mutable bool _areTexturesLoaded { false };
};

/// A geometry loaded from the network.
class ModelResource : public Resource, public NetworkModel {
    Q_OBJECT
public:
    using Pointer = QSharedPointer<ModelResource>;

    ModelResource(const QUrl& url, const ModelLoader& modelLoader) : Resource(url), _modelLoader(modelLoader) {}
    ModelResource(const ModelResource& other);

    QString getType() const override { return "Model"; }

    virtual void deleter() override;

    virtual void downloadFinished(const QByteArray& data) override;
    void setExtra(void* extra) override;

    virtual bool areTexturesLoaded() const override { return isLoaded() && NetworkModel::areTexturesLoaded(); }

private slots:
    void onGeometryMappingLoaded(bool success);

protected:
    friend class ModelCache;

    Q_INVOKABLE void setGeometryDefinition(HFMModel::Pointer hfmModel, const MaterialMapping& materialMapping);

    // Geometries may not hold onto textures while cached - that is for the texture cache
    // Instead, these methods clear and reset textures from the geometry when caching/loading
    bool shouldSetTextures() const { return _hfmModel && _materials.empty(); }
    void setTextures();
    void resetTextures();

    virtual bool isCacheable() const override { return _loaded && _isCacheable; }

private:
    ModelLoader _modelLoader;
    GeometryMappingPair _mappingPair;
    QUrl _textureBaseURL;
    bool _combineParts;

    ModelResource::Pointer _modelResource;
    QMetaObject::Connection _connection;

    bool _isCacheable{ true };
};

class ModelResourceWatcher : public QObject {
    Q_OBJECT
public:
    using Pointer = std::shared_ptr<ModelResourceWatcher>;

    ModelResourceWatcher() = delete;
    ModelResourceWatcher(NetworkModel::Pointer& geometryPtr) : _networkModelRef(geometryPtr) {}

    void setResource(ModelResource::Pointer resource);

    QUrl getURL() const { return (bool)_resource ? _resource->getURL() : QUrl(); }
    int getResourceDownloadAttempts() { return _resource ? _resource->getDownloadAttempts() : 0; }
    int getResourceDownloadAttemptsRemaining() { return _resource ? _resource->getDownloadAttemptsRemaining() : 0; }

private:
    void startWatching();
    void stopWatching();

signals:
    void finished(bool success);

private slots:
    void resourceFinished(bool success);
    void resourceRefreshed();

private:
    ModelResource::Pointer _resource;
    NetworkModel::Pointer& _networkModelRef;
};

/// Stores cached model geometries.
class ModelCache : public ResourceCache, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY

public:

    ModelResource::Pointer getModelResource(const QUrl& url,
                                                  const GeometryMappingPair& mapping =
                                                        GeometryMappingPair(QUrl(), QVariantHash()),
                                                  const QUrl& textureBaseUrl = QUrl());

    ModelResource::Pointer getCollisionModelResource(const QUrl& url,
                                                           const GeometryMappingPair& mapping =
                                                                 GeometryMappingPair(QUrl(), QVariantHash()),
                                                           const QUrl& textureBaseUrl = QUrl());

protected:
    friend class ModelResource;

    virtual QSharedPointer<Resource> createResource(const QUrl& url) override;
    QSharedPointer<Resource> createResourceCopy(const QSharedPointer<Resource>& resource) override;

private:
    ModelCache();
    virtual ~ModelCache() = default;
    ModelLoader _modelLoader;
};

#endif // hifi_ModelCache_h
