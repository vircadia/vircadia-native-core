//
//  GeometryCache.h
//  interface
//
//  Created by Andrzej Kapolka on 6/21/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__GeometryCache__
#define __interface__GeometryCache__

// include this before QOpenGLBuffer, which includes an earlier version of OpenGL
#include "InterfaceConfig.h"

#include <QHash>
#include <QMap>
#include <QNetworkRequest>
#include <QObject>
#include <QOpenGLBuffer>
#include <QSharedPointer>
#include <QWeakPointer>

#include "FBXReader.h"

class QNetworkReply;

class NetworkGeometry;
class NetworkMesh;
class NetworkTexture;

/// Stores cached geometry.
class GeometryCache {
public:
    
    ~GeometryCache();
    
    void renderHemisphere(int slices, int stacks);
    void renderSquare(int xDivisions, int yDivisions);
    void renderHalfCylinder(int slices, int stacks);
    void renderGrid(int xDivisions, int yDivisions);

    /// Loads geometry from the specified URL.
    /// \param fallback a fallback URL to load if the desired one is unavailable
    /// \param delayLoad if true, don't load the geometry immediately; wait until load is first requested
    QSharedPointer<NetworkGeometry> getGeometry(const QUrl& url, const QUrl& fallback = QUrl(), bool delayLoad = false);
    
private:
    
    typedef QPair<int, int> IntPair;
    typedef QPair<GLuint, GLuint> VerticesIndices;
    
    QHash<IntPair, VerticesIndices> _hemisphereVBOs;
    QHash<IntPair, VerticesIndices> _squareVBOs;
    QHash<IntPair, VerticesIndices> _halfCylinderVBOs;
    QHash<IntPair, QOpenGLBuffer> _gridBuffers;
    
    QHash<QUrl, QWeakPointer<NetworkGeometry> > _networkGeometry;
};

/// Geometry loaded from the network.
class NetworkGeometry : public QObject {
    Q_OBJECT

public:
    
    /// A hysteresis value indicating that we have no state memory.
    static const float NO_HYSTERESIS;
    
    NetworkGeometry(const QUrl& url, const QSharedPointer<NetworkGeometry>& fallback, bool delayLoad,
        const QVariantHash& mapping = QVariantHash(), const QUrl& textureBase = QUrl());
    ~NetworkGeometry();

    /// Checks whether the geometry is fulled loaded.
    bool isLoaded() const { return !_geometry.joints.isEmpty(); }

    /// Makes sure that the geometry has started loading.
    void ensureLoading();

    /// Returns a pointer to the geometry appropriate for the specified distance.
    /// \param hysteresis a hysteresis parameter that prevents rapid model switching
    QSharedPointer<NetworkGeometry> getLODOrFallback(float distance, float& hysteresis, bool delayLoad = false) const;

    const FBXGeometry& getFBXGeometry() const { return _geometry; }
    const QVector<NetworkMesh>& getMeshes() const { return _meshes; }

    /// Returns the average color of all meshes in the geometry.
    glm::vec4 computeAverageColor() const;

private slots:
    
    void makeRequest();
    void handleDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void handleReplyError();
    
private:
    
    friend class GeometryCache;
    
    void setLODParent(const QWeakPointer<NetworkGeometry>& lodParent) { _lodParent = lodParent; }
    
    QNetworkRequest _request;
    QNetworkReply* _reply;
    QVariantHash _mapping;
    QUrl _textureBase;
    QSharedPointer<NetworkGeometry> _fallback;
    bool _startedLoading;
    bool _failedToLoad;
    
    int _attempts;
    QMap<float, QSharedPointer<NetworkGeometry> > _lods;
    FBXGeometry _geometry;
    QVector<NetworkMesh> _meshes;
    
    QWeakPointer<NetworkGeometry> _lodParent;
};

/// The state associated with a single mesh part.
class NetworkMeshPart {
public:
    
    QSharedPointer<NetworkTexture> diffuseTexture;
    QSharedPointer<NetworkTexture> normalTexture;
    
    bool isTranslucent() const;
};

/// The state associated with a single mesh.
class NetworkMesh {
public:
    
    QOpenGLBuffer indexBuffer;
    QOpenGLBuffer vertexBuffer;
    
    QVector<NetworkMeshPart> parts;
    
    int getTranslucentPartCount() const;
};

#endif /* defined(__interface__GeometryCache__) */
