//
//  GeometryCache.h
//  interface/src/renderer
//
//  Created by Andrzej Kapolka on 6/21/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_GeometryCache_h
#define hifi_GeometryCache_h

// include this before QOpenGLBuffer, which includes an earlier version of OpenGL
#include <gpu/GPUConfig.h>

#include <QMap>
#include <QOpenGLBuffer>

#include <DependencyManager.h>
#include <ResourceCache.h>

#include <FBXReader.h>

#include <AnimationCache.h>

#include "gpu/Stream.h"

class NetworkGeometry;
class NetworkMesh;
class NetworkTexture;


typedef QPair<glm::vec2, glm::vec2> Vec2Pair;
typedef QPair<Vec2Pair, Vec2Pair> Vec2PairPair;
typedef QPair<glm::vec3, glm::vec3> Vec3Pair;
typedef QPair<Vec3Pair, Vec2Pair> Vec3PairVec2Pair;

inline uint qHash(const glm::vec2& v, uint seed) {
    // multiply by prime numbers greater than the possible size
    return qHash(v.x + 5009 * v.y, seed);
}

inline uint qHash(const Vec2Pair& v, uint seed) {
    // multiply by prime numbers greater than the possible size
    return qHash(v.first.x + 5009 * v.first.y + 5011 * v.second.x + 5021 * v.second.y, seed);
}

inline uint qHash(const Vec2PairPair& v, uint seed) {
    // multiply by prime numbers greater than the possible size
    return qHash(v.first.first.x + 5009 * v.first.first.y 
                 + 5011 * v.first.second.x + 5021 * v.first.second.y
                 + 5023 * v.second.first.x + 5039 * v.second.first.y
                 + 5051 * v.second.second.x + 5059 * v.second.second.y, seed);
}

inline uint qHash(const glm::vec3& v, uint seed) {
    // multiply by prime numbers greater than the possible size
    return qHash(v.x + 5009 * v.y + 5011 * v.z,  seed);
}

inline uint qHash(const Vec3Pair& v, uint seed) {
    // multiply by prime numbers greater than the possible size
    return qHash(v.first.x + 5009 * v.first.y + 5011 * v.first.z 
                 + 5021 * v.second.x + 5023 * v.second.y + 5039 * v.second.z, seed);
}

inline uint qHash(const Vec3PairVec2Pair& v, uint seed) {
    // multiply by prime numbers greater than the possible size
    return qHash(v.first.first.x + 5009 * v.first.first.y + 5011 * v.first.first.z +
                 5021 * v.first.second.x + 5023 * v.first.second.y + 5039 * v.first.second.z +
                 5051 * v.second.first.x + 5059 * v.second.first.y +
                 5077 * v.second.second.x + 5081 * v.second.second.y, seed);
}


/// Stores cached geometry.
class GeometryCache : public ResourceCache  {
    Q_OBJECT
    SINGLETON_DEPENDENCY(GeometryCache)

public:
    void renderHemisphere(int slices, int stacks);
    void renderSphere(float radius, int slices, int stacks, bool solid = true);
    void renderSquare(int xDivisions, int yDivisions);
    void renderHalfCylinder(int slices, int stacks);
    void renderCone(float base, float height, int slices, int stacks);
    void renderGrid(int xDivisions, int yDivisions);
    void renderSolidCube(float size);
    void renderWireCube(float size);


    void renderQuad(int x, int y, int width, int height) { renderQuad(glm::vec2(x,y), glm::vec2(x + width, y + height)); }
    void renderQuad(const glm::vec2& topLeft, const glm::vec2& bottomRight);

    void renderQuad(const glm::vec2& topLeft, const glm::vec2& bottomRight,
                    const glm::vec2& texCoordTopLeft, const glm::vec2& texCoordBottomRight);


    void renderQuad(const glm::vec3& topLeft, const glm::vec3& bottomRight);

    //void renderQuad(const glm::vec3& topLeft, const glm::vec3& bottomRight,
    //                const glm::vec2& texCoordTopLeft, const glm::vec2& texCoordBottomRight);

    void renderQuad(const glm::vec3& topLeft, const glm::vec3& bottomLeft, 
                    const glm::vec3& bottomRight, const glm::vec3& topRight,
                    const glm::vec2& texCoordTopLeft, const glm::vec2& texCoordBottomLeft,
                    const glm::vec2& texCoordBottomRight, const glm::vec2& texCoordTopRight);

    /// Loads geometry from the specified URL.
    /// \param fallback a fallback URL to load if the desired one is unavailable
    /// \param delayLoad if true, don't load the geometry immediately; wait until load is first requested
    QSharedPointer<NetworkGeometry> getGeometry(const QUrl& url, const QUrl& fallback = QUrl(), bool delayLoad = false);

protected:

    virtual QSharedPointer<Resource> createResource(const QUrl& url,
        const QSharedPointer<Resource>& fallback, bool delayLoad, const void* extra);
        
private:
    GeometryCache();
    virtual ~GeometryCache();
    
    typedef QPair<int, int> IntPair;
    typedef QPair<GLuint, GLuint> VerticesIndices;
    
    QHash<IntPair, VerticesIndices> _hemisphereVBOs;
    QHash<IntPair, VerticesIndices> _sphereVBOs;
    QHash<IntPair, VerticesIndices> _squareVBOs;
    QHash<IntPair, VerticesIndices> _halfCylinderVBOs;
    QHash<IntPair, VerticesIndices> _coneVBOs;
    QHash<float, VerticesIndices> _wireCubeVBOs;
    QHash<float, VerticesIndices> _solidCubeVBOs;
    QHash<Vec2Pair, VerticesIndices> _quad2DVBOs;
    QHash<Vec2PairPair, VerticesIndices> _quad2DTextureVBOs;
    QHash<Vec3Pair, VerticesIndices> _quad3DVBOs;
    QHash<Vec3PairVec2Pair, VerticesIndices> _quad3DTextureVBOs;

    QHash<IntPair, QOpenGLBuffer> _gridBuffers;
    
    QHash<QUrl, QWeakPointer<NetworkGeometry> > _networkGeometry;
};

/// Geometry loaded from the network.
class NetworkGeometry : public Resource {
    Q_OBJECT

public:
    
    /// A hysteresis value indicating that we have no state memory.
    static const float NO_HYSTERESIS;
    
    NetworkGeometry(const QUrl& url, const QSharedPointer<NetworkGeometry>& fallback, bool delayLoad,
        const QVariantHash& mapping = QVariantHash(), const QUrl& textureBase = QUrl());

    /// Checks whether the geometry and its textures are loaded.
    bool isLoadedWithTextures() const;

    /// Returns a pointer to the geometry appropriate for the specified distance.
    /// \param hysteresis a hysteresis parameter that prevents rapid model switching
    QSharedPointer<NetworkGeometry> getLODOrFallback(float distance, float& hysteresis, bool delayLoad = false) const;

    const FBXGeometry& getFBXGeometry() const { return _geometry; }
    const QVector<NetworkMesh>& getMeshes() const { return _meshes; }

    QVector<int> getJointMappings(const AnimationPointer& animation);

    virtual void setLoadPriority(const QPointer<QObject>& owner, float priority);
    virtual void setLoadPriorities(const QHash<QPointer<QObject>, float>& priorities);
    virtual void clearLoadPriority(const QPointer<QObject>& owner);
    
    void setTextureWithNameToURL(const QString& name, const QUrl& url);
    QStringList getTextureNames() const;
        
protected:

    virtual void init();
    virtual void downloadFinished(QNetworkReply* reply);
    virtual void reinsert();
    
    Q_INVOKABLE void setGeometry(const FBXGeometry& geometry);
    
private slots:
    void replaceTexturesWithPendingChanges();
private:
    
    friend class GeometryCache;
    
    void setLODParent(const QWeakPointer<NetworkGeometry>& lodParent) { _lodParent = lodParent; }
    
    QVariantHash _mapping;
    QUrl _textureBase;
    QSharedPointer<NetworkGeometry> _fallback;
    
    QMap<float, QSharedPointer<NetworkGeometry> > _lods;
    FBXGeometry _geometry;
    QVector<NetworkMesh> _meshes;
    
    QWeakPointer<NetworkGeometry> _lodParent;
    
    QHash<QWeakPointer<Animation>, QVector<int> > _jointMappings;
    
    QHash<QString, QUrl> _pendingTextureChanges;
};

/// The state associated with a single mesh part.
class NetworkMeshPart {
public: 
    
    QString diffuseTextureName;
    QSharedPointer<NetworkTexture> diffuseTexture;
    QString normalTextureName;
    QSharedPointer<NetworkTexture> normalTexture;
    QString specularTextureName;
    QSharedPointer<NetworkTexture> specularTexture;
    QString emissiveTextureName;
    QSharedPointer<NetworkTexture> emissiveTexture;

    bool isTranslucent() const;
};

/// The state associated with a single mesh.
class NetworkMesh {
public:
    gpu::BufferPointer _indexBuffer;
    gpu::BufferPointer _vertexBuffer;

    gpu::BufferStreamPointer _vertexStream;

    gpu::Stream::FormatPointer _vertexFormat;
    
    QVector<NetworkMeshPart> parts;
    
    int getTranslucentPartCount(const FBXMesh& fbxMesh) const;
};

#endif // hifi_GeometryCache_h
