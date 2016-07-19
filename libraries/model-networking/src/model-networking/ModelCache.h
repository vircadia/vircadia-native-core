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

#include <model/Material.h>
#include <model/Asset.h>

#include "FBXReader.h"
#include "TextureCache.h"

// Alias instead of derive to avoid copying

class NetworkTexture;
class NetworkMaterial;
class MeshPart;

class GeometryMappingResource;

class Geometry {
public:
    using Pointer = std::shared_ptr<Geometry>;
    using WeakPointer = std::weak_ptr<Geometry>;

    Geometry() = default;
    Geometry(const Geometry& geometry);

    // Immutable over lifetime
    using GeometryMeshes = std::vector<std::shared_ptr<const model::Mesh>>;
    using GeometryMeshParts = std::vector<std::shared_ptr<const MeshPart>>;

    // Mutable, but must retain structure of vector
    using NetworkMaterials = std::vector<std::shared_ptr<NetworkMaterial>>;

    const FBXGeometry& getFBXGeometry() const { return *_fbxGeometry; }
    const GeometryMeshes& getMeshes() const { return *_meshes; }
    const std::shared_ptr<const NetworkMaterial> getShapeMaterial(int shapeID) const;

    const QVariantMap getTextures() const;
    void setTextures(const QVariantMap& textureMap);

    virtual bool areTexturesLoaded() const;
    const QUrl& getAnimGraphOverrideUrl() const { return _animGraphOverrideUrl; }

protected:
    friend class GeometryMappingResource;

    // Shared across all geometries, constant throughout lifetime
    std::shared_ptr<const FBXGeometry> _fbxGeometry;
    std::shared_ptr<const GeometryMeshes> _meshes;
    std::shared_ptr<const GeometryMeshParts> _meshParts;

    // Copied to each geometry, mutable throughout lifetime via setTextures
    NetworkMaterials _materials;

    QUrl _animGraphOverrideUrl;

private:
    mutable bool _areTexturesLoaded { false };
};

/// A geometry loaded from the network.
class GeometryResource : public Resource, public Geometry {
public:
    using Pointer = QSharedPointer<GeometryResource>;

    GeometryResource(const QUrl& url, const QUrl& textureBaseUrl = QUrl()) :
        Resource(url), _textureBaseUrl(textureBaseUrl) {}

    virtual bool areTexturesLoaded() const override { return isLoaded() && Geometry::areTexturesLoaded(); }

    virtual void deleter() override;

protected:
    friend class ModelCache;
    friend class GeometryMappingResource;

    // Geometries may not hold onto textures while cached - that is for the texture cache
    // Instead, these methods clear and reset textures from the geometry when caching/loading
    bool shouldSetTextures() const { return _fbxGeometry && _materials.empty(); }
    void setTextures();
    void resetTextures();

    QUrl _textureBaseUrl;

    virtual bool isCacheable() const override { return _loaded && _isCacheable; }
    bool _isCacheable { true };
};

class GeometryResourceWatcher : public QObject {
    Q_OBJECT
public:
    using Pointer = std::shared_ptr<GeometryResourceWatcher>;

    GeometryResourceWatcher() = delete;
    GeometryResourceWatcher(Geometry::Pointer& geometryPtr) : _geometryRef(geometryPtr) {}

    void setResource(GeometryResource::Pointer resource);

    QUrl getURL() const { return (bool)_resource ? _resource->getURL() : QUrl(); }

    void startWatching();
    void stopWatching();

signals:
    void finished(bool success);

private slots:
    void resourceFinished(bool success);
    void resourceRefreshed();

private:
    GeometryResource::Pointer _resource;
    Geometry::Pointer& _geometryRef;
};

/// Stores cached model geometries.
class ModelCache : public ResourceCache, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY

public:
    GeometryResource::Pointer getGeometryResource(const QUrl& url,
        const QVariantHash& mapping = QVariantHash(), const QUrl& textureBaseUrl = QUrl());

protected:
    friend class GeometryMappingResource;

    virtual QSharedPointer<Resource> createResource(const QUrl& url, const QSharedPointer<Resource>& fallback,
        const void* extra) override;

private:
    ModelCache();
    virtual ~ModelCache() = default;
};

class NetworkMaterial : public model::Material {
public:
    using MapChannel = model::Material::MapChannel;

    NetworkMaterial(const FBXMaterial& material, const QUrl& textureBaseUrl);

protected:
    friend class Geometry;

    class Texture {
    public:
        QString name;
        QSharedPointer<NetworkTexture> texture;
    };
    using Textures = std::vector<Texture>;

    Textures _textures;

    static const QString NO_TEXTURE;
    const QString& getTextureName(MapChannel channel);

    void setTextures(const QVariantMap& textureMap);

    const bool& isOriginal() const { return _isOriginal; }

private:
    using TextureType = NetworkTexture::Type;

    // Helpers for the ctors
    QUrl getTextureUrl(const QUrl& baseUrl, const FBXTexture& fbxTexture);
    model::TextureMapPointer fetchTextureMap(const QUrl& baseUrl, const FBXTexture& fbxTexture,
        TextureType type, MapChannel channel);
    model::TextureMapPointer fetchTextureMap(const QUrl& url, TextureType type, MapChannel channel);

    Transform _albedoTransform;
    Transform _lightmapTransform;
    vec2 _lightmapParams;

    bool _isOriginal { true };
};

class MeshPart {
public:
    MeshPart(int mesh, int part, int material) : meshID { mesh }, partID { part }, materialID { material } {}
    int meshID { -1 };
    int partID { -1 };
    int materialID { -1 };
};

#endif // hifi_ModelCache_h
