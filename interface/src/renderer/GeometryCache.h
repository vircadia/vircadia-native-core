//
//  GeometryCache.h
//  interface
//
//  Created by Andrzej Kapolka on 6/21/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__GeometryCache__
#define __interface__GeometryCache__

#include <QHash>
#include <QObject>
#include <QSharedPointer>
#include <QWeakPointer>

#include "FBXReader.h"
#include "InterfaceConfig.h"

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

    /// Loads geometry from the specified URL.
    QSharedPointer<NetworkGeometry> getGeometry(const QUrl& url);
    
private:
    
    typedef QPair<int, int> IntPair;
    typedef QPair<GLuint, GLuint> VerticesIndices;
    
    QHash<IntPair, VerticesIndices> _hemisphereVBOs;
    QHash<IntPair, VerticesIndices> _squareVBOs;
    QHash<IntPair, VerticesIndices> _halfCylinderVBOs;
    
    QHash<QUrl, QWeakPointer<NetworkGeometry> > _networkGeometry;
};

/// Geometry loaded from the network.
class NetworkGeometry : public QObject {
    Q_OBJECT

public:
    
    NetworkGeometry(const QUrl& url);
    ~NetworkGeometry();

    bool isLoaded() const { return !_meshes.isEmpty(); }

    const FBXGeometry& getFBXGeometry() const { return _geometry; }
    const QVector<NetworkMesh>& getMeshes() const { return _meshes; }

private slots:
    
    void handleDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void handleReplyError();    
    
private:
    
    QNetworkReply* _reply;
    
    FBXGeometry _geometry;
    QVector<NetworkMesh> _meshes;
};

/// The state associated with a single mesh.
class NetworkMesh {
public:
    
    GLuint indexBufferID;
    GLuint vertexBufferID;
    
    QSharedPointer<NetworkTexture> diffuseTexture;
    QSharedPointer<NetworkTexture> normalTexture;
};

#endif /* defined(__interface__GeometryCache__) */
