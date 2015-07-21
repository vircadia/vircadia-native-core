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

#include <QMap>

#include <DependencyManager.h>
#include <ResourceCache.h>

#include "FBXReader.h"
#include "OBJReader.h"

#include <AnimationCache.h>

#include <gpu/Batch.h>
#include <gpu/Stream.h>


class NetworkGeometry;
class NetworkMesh;
class NetworkTexture;


typedef glm::vec3 Vec3Key;

typedef QPair<glm::vec2, glm::vec2> Vec2Pair;
typedef QPair<Vec2Pair, Vec2Pair> Vec2PairPair;
typedef QPair<glm::vec3, glm::vec3> Vec3Pair;
typedef QPair<glm::vec4, glm::vec4> Vec4Pair;
typedef QPair<Vec3Pair, Vec2Pair> Vec3PairVec2Pair;
typedef QPair<Vec3Pair, glm::vec4> Vec3PairVec4;
typedef QPair<Vec3Pair, Vec4Pair> Vec3PairVec4Pair;
typedef QPair<Vec4Pair, glm::vec4> Vec4PairVec4;
typedef QPair<Vec4Pair, Vec4Pair> Vec4PairVec4Pair;

inline uint qHash(const glm::vec2& v, uint seed) {
    // multiply by prime numbers greater than the possible size
    return qHash(v.x + 5009 * v.y, seed);
}

inline uint qHash(const Vec2Pair& v, uint seed) {
    // multiply by prime numbers greater than the possible size
    return qHash(v.first.x + 5009 * v.first.y + 5011 * v.second.x + 5021 * v.second.y, seed);
}

inline uint qHash(const glm::vec4& v, uint seed) {
    // multiply by prime numbers greater than the possible size
    return qHash(v.x + 5009 * v.y + 5011 * v.z + 5021 * v.w, seed);
}

inline uint qHash(const Vec2PairPair& v, uint seed) {
    // multiply by prime numbers greater than the possible size
    return qHash(v.first.first.x + 5009 * v.first.first.y 
                 + 5011 * v.first.second.x + 5021 * v.first.second.y
                 + 5023 * v.second.first.x + 5039 * v.second.first.y
                 + 5051 * v.second.second.x + 5059 * v.second.second.y, seed);
}

inline uint qHash(const Vec3Pair& v, uint seed) {
    // multiply by prime numbers greater than the possible size
    return qHash(v.first.x + 5009 * v.first.y + 5011 * v.first.z 
                 + 5021 * v.second.x + 5023 * v.second.y + 5039 * v.second.z, seed);
}

inline uint qHash(const Vec4Pair& v, uint seed) {
    // multiply by prime numbers greater than the possible size
    return qHash(v.first.x + 5009 * v.first.y + 5011 * v.first.z + 5021 * v.first.w 
                    + 5023 * v.second.x + 5039 * v.second.y + 5051 * v.second.z + 5059 * v.second.w , seed);
}

inline uint qHash(const Vec3PairVec2Pair& v, uint seed) {
    // multiply by prime numbers greater than the possible size
    return qHash(v.first.first.x + 5009 * v.first.first.y + 5011 * v.first.first.z +
                 5021 * v.first.second.x + 5023 * v.first.second.y + 5039 * v.first.second.z +
                 5051 * v.second.first.x + 5059 * v.second.first.y +
                 5077 * v.second.second.x + 5081 * v.second.second.y, seed);
}

inline uint qHash(const Vec3PairVec4& v, uint seed) {
    // multiply by prime numbers greater than the possible size
    return qHash(v.first.first.x + 5009 * v.first.first.y + 5011 * v.first.first.z +
                 5021 * v.first.second.x + 5023 * v.first.second.y + 5039 * v.first.second.z +
                 5051 * v.second.x + 5059 * v.second.y + 5077 * v.second.z + 5081 * v.second.w, seed);
}


inline uint qHash(const Vec3PairVec4Pair& v, uint seed) {
    // multiply by prime numbers greater than the possible size
    return qHash(v.first.first.x + 5009 * v.first.first.y + 5011 * v.first.first.z 
                + 5023 * v.first.second.x + 5039 * v.first.second.y + 5051 * v.first.second.z 
                + 5077 * v.second.first.x + 5081 * v.second.first.y + 5087 * v.second.first.z + 5099 * v.second.first.w 
                + 5101 * v.second.second.x + 5107 * v.second.second.y + 5113 * v.second.second.z + 5119 * v.second.second.w, 
                seed);
}

inline uint qHash(const Vec4PairVec4& v, uint seed) {
    // multiply by prime numbers greater than the possible size
    return qHash(v.first.first.x + 5009 * v.first.first.y + 5011 * v.first.first.z + 5021 * v.first.first.w 
                + 5023 * v.first.second.x + 5039 * v.first.second.y + 5051 * v.first.second.z + 5059 * v.first.second.w 
                + 5077 * v.second.x + 5081 * v.second.y + 5087 * v.second.z + 5099 * v.second.w,
                seed);
}

inline uint qHash(const Vec4PairVec4Pair& v, uint seed) {
    // multiply by prime numbers greater than the possible size
    return qHash(v.first.first.x + 5009 * v.first.first.y + 5011 * v.first.first.z + 5021 * v.first.first.w 
                + 5023 * v.first.second.x + 5039 * v.first.second.y + 5051 * v.first.second.z + 5059 * v.first.second.w 
                + 5077 * v.second.first.x + 5081 * v.second.first.y + 5087 * v.second.first.z + 5099 * v.second.first.w 
                + 5101 * v.second.second.x + 5107 * v.second.second.y + 5113 * v.second.second.z + 5119 * v.second.second.w, 
                seed);
}

/// Stores cached geometry.
class GeometryCache : public ResourceCache, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY

public:
    int allocateID() { return _nextID++; }
    static const int UNKNOWN_ID;

    void renderSphere(float radius, int slices, int stacks, const glm::vec3& color, bool solid = true, int id = UNKNOWN_ID)
                { renderSphere(radius, slices, stacks, glm::vec4(color, 1.0f), solid, id); }
    void renderSphere(gpu::Batch& batch, float radius, int slices, int stacks, const glm::vec3& color, bool solid = true, int id = UNKNOWN_ID) 
                { renderSphere(batch, radius, slices, stacks, glm::vec4(color, 1.0f), solid, id); }
                
    void renderSphere(float radius, int slices, int stacks, const glm::vec4& color, bool solid = true, int id = UNKNOWN_ID);
    void renderSphere(gpu::Batch& batch, float radius, int slices, int stacks, const glm::vec4& color, bool solid = true, int id = UNKNOWN_ID);

    void renderGrid(int xDivisions, int yDivisions, const glm::vec4& color);
    void renderGrid(gpu::Batch& batch, int xDivisions, int yDivisions, const glm::vec4& color);
    void renderGrid(int x, int y, int width, int height, int rows, int cols, const glm::vec4& color, int id = UNKNOWN_ID);
    void renderGrid(gpu::Batch& batch, int x, int y, int width, int height, int rows, int cols, const glm::vec4& color, int id = UNKNOWN_ID);

    void renderSolidCube(float size, const glm::vec4& color);
    void renderSolidCube(gpu::Batch& batch, float size, const glm::vec4& color);
    void renderWireCube(float size, const glm::vec4& color);
    void renderWireCube(gpu::Batch& batch, float size, const glm::vec4& color);
    void renderBevelCornersRect(int x, int y, int width, int height, int bevelDistance, const glm::vec4& color, int id = UNKNOWN_ID);
    void renderBevelCornersRect(gpu::Batch& batch, int x, int y, int width, int height, int bevelDistance, const glm::vec4& color, int id = UNKNOWN_ID);

    void renderUnitCube(gpu::Batch& batch);
    void renderUnitQuad(const glm::vec4& color = glm::vec4(1), int id = UNKNOWN_ID);
    void renderUnitQuad(gpu::Batch& batch, const glm::vec4& color = glm::vec4(1), int id = UNKNOWN_ID);

    void renderQuad(int x, int y, int width, int height, const glm::vec4& color, int id = UNKNOWN_ID)
            { renderQuad(glm::vec2(x,y), glm::vec2(x + width, y + height), color, id); }
    void renderQuad(gpu::Batch& batch, int x, int y, int width, int height, const glm::vec4& color, int id = UNKNOWN_ID)
            { renderQuad(batch, glm::vec2(x,y), glm::vec2(x + width, y + height), color, id); }
            
    // TODO: I think there's a bug in this version of the renderQuad() that's not correctly rebuilding the vbos
    // if the color changes by the corners are the same, as evidenced by the audio meter which should turn white
    // when it's clipping
    void renderQuad(const glm::vec2& minCorner, const glm::vec2& maxCorner, const glm::vec4& color, int id = UNKNOWN_ID);
    void renderQuad(gpu::Batch& batch, const glm::vec2& minCorner, const glm::vec2& maxCorner, const glm::vec4& color, int id = UNKNOWN_ID);

    void renderQuad(const glm::vec2& minCorner, const glm::vec2& maxCorner,
                    const glm::vec2& texCoordMinCorner, const glm::vec2& texCoordMaxCorner, 
                    const glm::vec4& color, int id = UNKNOWN_ID);
    void renderQuad(gpu::Batch& batch, const glm::vec2& minCorner, const glm::vec2& maxCorner,
                    const glm::vec2& texCoordMinCorner, const glm::vec2& texCoordMaxCorner, 
                    const glm::vec4& color, int id = UNKNOWN_ID);

    void renderQuad(const glm::vec3& minCorner, const glm::vec3& maxCorner, const glm::vec4& color, int id = UNKNOWN_ID);
    void renderQuad(gpu::Batch& batch, const glm::vec3& minCorner, const glm::vec3& maxCorner, const glm::vec4& color, int id = UNKNOWN_ID);

    void renderQuad(const glm::vec3& topLeft, const glm::vec3& bottomLeft, 
                    const glm::vec3& bottomRight, const glm::vec3& topRight,
                    const glm::vec2& texCoordTopLeft, const glm::vec2& texCoordBottomLeft,
                    const glm::vec2& texCoordBottomRight, const glm::vec2& texCoordTopRight, 
                    const glm::vec4& color, int id = UNKNOWN_ID);
    void renderQuad(gpu::Batch& batch, const glm::vec3& topLeft, const glm::vec3& bottomLeft, 
                    const glm::vec3& bottomRight, const glm::vec3& topRight,
                    const glm::vec2& texCoordTopLeft, const glm::vec2& texCoordBottomLeft,
                    const glm::vec2& texCoordBottomRight, const glm::vec2& texCoordTopRight, 
                    const glm::vec4& color, int id = UNKNOWN_ID);


    void renderLine(const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& color, int id = UNKNOWN_ID) 
                    { renderLine(p1, p2, color, color, id); }
    void renderLine(gpu::Batch& batch, const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& color, int id = UNKNOWN_ID) 
                    { renderLine(batch, p1, p2, color, color, id); }
    
    void renderLine(const glm::vec3& p1, const glm::vec3& p2, 
                    const glm::vec3& color1, const glm::vec3& color2, int id = UNKNOWN_ID)
                    { renderLine(p1, p2, glm::vec4(color1, 1.0f), glm::vec4(color2, 1.0f), id); }
    void renderLine(gpu::Batch& batch, const glm::vec3& p1, const glm::vec3& p2, 
                    const glm::vec3& color1, const glm::vec3& color2, int id = UNKNOWN_ID)
                    { renderLine(batch, p1, p2, glm::vec4(color1, 1.0f), glm::vec4(color2, 1.0f), id); }

    void renderLine(const glm::vec3& p1, const glm::vec3& p2, 
                    const glm::vec4& color, int id = UNKNOWN_ID)
                    { renderLine(p1, p2, color, color, id); }
    void renderLine(gpu::Batch& batch, const glm::vec3& p1, const glm::vec3& p2, 
                    const glm::vec4& color, int id = UNKNOWN_ID)
                    { renderLine(batch, p1, p2, color, color, id); }

    void renderLine(const glm::vec3& p1, const glm::vec3& p2, 
                    const glm::vec4& color1, const glm::vec4& color2, int id = UNKNOWN_ID);
    void renderLine(gpu::Batch& batch, const glm::vec3& p1, const glm::vec3& p2, 
                    const glm::vec4& color1, const glm::vec4& color2, int id = UNKNOWN_ID);
                    
    void renderDashedLine(const glm::vec3& start, const glm::vec3& end, const glm::vec4& color, int id = UNKNOWN_ID);
    void renderDashedLine(gpu::Batch& batch, const glm::vec3& start, const glm::vec3& end, const glm::vec4& color, int id = UNKNOWN_ID);

    void renderLine(const glm::vec2& p1, const glm::vec2& p2, const glm::vec3& color, int id = UNKNOWN_ID)
                    { renderLine(p1, p2, glm::vec4(color, 1.0f), id); }
    void renderLine(gpu::Batch& batch, const glm::vec2& p1, const glm::vec2& p2, const glm::vec3& color, int id = UNKNOWN_ID)
                    { renderLine(batch, p1, p2, glm::vec4(color, 1.0f), id); }

    void renderLine(const glm::vec2& p1, const glm::vec2& p2, const glm::vec4& color, int id = UNKNOWN_ID)
                    { renderLine(p1, p2, color, color, id); }
    void renderLine(gpu::Batch& batch, const glm::vec2& p1, const glm::vec2& p2, const glm::vec4& color, int id = UNKNOWN_ID)
                    { renderLine(batch, p1, p2, color, color, id); }


    void renderLine(const glm::vec2& p1, const glm::vec2& p2,                                
                                    const glm::vec3& color1, const glm::vec3& color2, int id = UNKNOWN_ID)
                    { renderLine(p1, p2, glm::vec4(color1, 1.0f), glm::vec4(color2, 1.0f), id); }
    void renderLine(gpu::Batch& batch, const glm::vec2& p1, const glm::vec2& p2,                                
                                    const glm::vec3& color1, const glm::vec3& color2, int id = UNKNOWN_ID)
                    { renderLine(batch, p1, p2, glm::vec4(color1, 1.0f), glm::vec4(color2, 1.0f), id); }
                
    void renderLine(const glm::vec2& p1, const glm::vec2& p2,                                
                                    const glm::vec4& color1, const glm::vec4& color2, int id = UNKNOWN_ID);
    void renderLine(gpu::Batch& batch, const glm::vec2& p1, const glm::vec2& p2,                                
                                    const glm::vec4& color1, const glm::vec4& color2, int id = UNKNOWN_ID);

    void updateVertices(int id, const QVector<glm::vec2>& points, const glm::vec4& color);
    void updateVertices(int id, const QVector<glm::vec3>& points, const glm::vec4& color);
    void updateVertices(int id, const QVector<glm::vec3>& points, const QVector<glm::vec2>& texCoords, const glm::vec4& color);
    void renderVertices(gpu::Batch& batch, gpu::Primitive primitiveType, int id);
    void renderVertices(gpu::Primitive primitiveType, int id);

    /// Loads geometry from the specified URL.
    /// \param fallback a fallback URL to load if the desired one is unavailable
    /// \param delayLoad if true, don't load the geometry immediately; wait until load is first requested
    QSharedPointer<NetworkGeometry> getGeometry(const QUrl& url, const QUrl& fallback = QUrl(), bool delayLoad = false);

    /// Set a batch to the simple pipeline, returning the previous pipeline
    void useSimpleDrawPipeline(gpu::Batch& batch);

protected:

    virtual QSharedPointer<Resource> createResource(const QUrl& url,
        const QSharedPointer<Resource>& fallback, bool delayLoad, const void* extra);

private:
    GeometryCache();
    virtual ~GeometryCache();
    
    typedef QPair<int, int> IntPair;
    typedef QPair<unsigned int, unsigned int> VerticesIndices;

    gpu::PipelinePointer _standardDrawPipeline;
    QHash<float, gpu::BufferPointer> _cubeVerticies;
    QHash<Vec2Pair, gpu::BufferPointer> _cubeColors;
    gpu::BufferPointer _wireCubeIndexBuffer;

    QHash<float, gpu::BufferPointer> _solidCubeVertices;
    QHash<Vec2Pair, gpu::BufferPointer> _solidCubeColors;
    gpu::BufferPointer _solidCubeIndexBuffer;

    class BatchItemDetails {
    public:
        static int population;
        gpu::BufferPointer verticesBuffer;
        gpu::BufferPointer colorBuffer;
        gpu::Stream::FormatPointer streamFormat;
        gpu::BufferStreamPointer stream;

        int vertices;
        int vertexSize;
        bool isCreated;
        
        BatchItemDetails();
        BatchItemDetails(const GeometryCache::BatchItemDetails& other);
        ~BatchItemDetails();
        void clear();
    };
    
    QHash<IntPair, VerticesIndices> _coneVBOs;

    int _nextID;

    QHash<int, Vec3PairVec4Pair> _lastRegisteredQuad3DTexture;
    QHash<Vec3PairVec4Pair, BatchItemDetails> _quad3DTextures;
    QHash<int, BatchItemDetails> _registeredQuad3DTextures;

    QHash<int, Vec4PairVec4> _lastRegisteredQuad2DTexture;
    QHash<Vec4PairVec4, BatchItemDetails> _quad2DTextures;
    QHash<int, BatchItemDetails> _registeredQuad2DTextures;

    QHash<int, Vec3PairVec4> _lastRegisteredQuad3D;
    QHash<Vec3PairVec4, BatchItemDetails> _quad3D;
    QHash<int, BatchItemDetails> _registeredQuad3D;

    QHash<int, Vec4Pair> _lastRegisteredQuad2D;
    QHash<Vec4Pair, BatchItemDetails> _quad2D;
    QHash<int, BatchItemDetails> _registeredQuad2D;

    QHash<int, Vec3Pair> _lastRegisteredBevelRects;
    QHash<Vec3Pair, BatchItemDetails> _bevelRects;
    QHash<int, BatchItemDetails> _registeredBevelRects;

    QHash<int, Vec3Pair> _lastRegisteredLine3D;
    QHash<Vec3Pair, BatchItemDetails> _line3DVBOs;
    QHash<int, BatchItemDetails> _registeredLine3DVBOs;

    QHash<int, Vec2Pair> _lastRegisteredLine2D;
    QHash<Vec2Pair, BatchItemDetails> _line2DVBOs;
    QHash<int, BatchItemDetails> _registeredLine2DVBOs;
    
    QHash<int, BatchItemDetails> _registeredVertices;

    QHash<int, Vec3PairVec2Pair> _lastRegisteredDashedLines;
    QHash<Vec3PairVec2Pair, BatchItemDetails> _dashedLines;
    QHash<int, BatchItemDetails> _registeredDashedLines;

    QHash<IntPair, gpu::BufferPointer> _gridBuffers;
    QHash<Vec3Pair, gpu::BufferPointer> _alternateGridBuffers;
    QHash<int, gpu::BufferPointer> _registeredAlternateGridBuffers;
    QHash<int, Vec3Pair> _lastRegisteredAlternateGridBuffers;
    QHash<Vec3Pair, gpu::BufferPointer> _gridColors;

    QHash<Vec2Pair, gpu::BufferPointer> _sphereVertices;
    QHash<int, gpu::BufferPointer> _registeredSphereVertices;
    QHash<int, Vec2Pair> _lastRegisteredSphereVertices;
    QHash<IntPair, gpu::BufferPointer> _sphereIndices;
    QHash<int, gpu::BufferPointer> _registeredSphereIndices;
    QHash<int, IntPair> _lastRegisteredSphereIndices;
    QHash<Vec3Pair, gpu::BufferPointer> _sphereColors;
    QHash<int, gpu::BufferPointer> _registeredSphereColors;
    QHash<int, Vec3Pair> _lastRegisteredSphereColors;

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

    mutable bool _isLoadedWithTextures = false;
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
