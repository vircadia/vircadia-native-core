//
//  GeometryCache.cpp
//  interface
//
//  Created by Andrzej Kapolka on 6/21/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.

#include <cmath>

#include <QNetworkReply>

#include "Application.h"
#include "GeometryCache.h"
#include "world.h"

GeometryCache::~GeometryCache() {
    foreach (const VerticesIndices& vbo, _hemisphereVBOs) {
        glDeleteBuffers(1, &vbo.first);
        glDeleteBuffers(1, &vbo.second);
    }
}

void GeometryCache::renderHemisphere(int slices, int stacks) {
    VerticesIndices& vbo = _hemisphereVBOs[IntPair(slices, stacks)];
    int vertices = slices * (stacks - 1) + 1;
    int indices = slices * 2 * 3 * (stacks - 2) + slices * 3;
    if (vbo.first == 0) {    
        GLfloat* vertexData = new GLfloat[vertices * 3];
        GLfloat* vertex = vertexData;
        for (int i = 0; i < stacks - 1; i++) {
            float phi = PIf * 0.5f * i / (stacks - 1);
            float z = sinf(phi), radius = cosf(phi);
            
            for (int j = 0; j < slices; j++) {
                float theta = PIf * 2.0f * j / slices;

                *(vertex++) = sinf(theta) * radius;
                *(vertex++) = cosf(theta) * radius;
                *(vertex++) = z;
            }
        }
        *(vertex++) = 0.0f;
        *(vertex++) = 0.0f;
        *(vertex++) = 1.0f;
        
        glGenBuffers(1, &vbo.first);
        glBindBuffer(GL_ARRAY_BUFFER, vbo.first);
        const int BYTES_PER_VERTEX = 3 * sizeof(GLfloat);
        glBufferData(GL_ARRAY_BUFFER, vertices * BYTES_PER_VERTEX, vertexData, GL_STATIC_DRAW);
        delete[] vertexData;
        
        GLushort* indexData = new GLushort[indices];
        GLushort* index = indexData;
        for (int i = 0; i < stacks - 2; i++) {
            GLushort bottom = i * slices;
            GLushort top = bottom + slices;
            for (int j = 0; j < slices; j++) {
                int next = (j + 1) % slices;
                
                *(index++) = bottom + j;
                *(index++) = top + next;
                *(index++) = top + j;
                
                *(index++) = bottom + j;
                *(index++) = bottom + next;
                *(index++) = top + next;
            }
        }
        GLushort bottom = (stacks - 2) * slices;
        GLushort top = bottom + slices;
        for (int i = 0; i < slices; i++) {    
            *(index++) = bottom + i;
            *(index++) = bottom + (i + 1) % slices;
            *(index++) = top;
        }
        
        glGenBuffers(1, &vbo.second);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo.second);
        const int BYTES_PER_INDEX = sizeof(GLushort);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices * BYTES_PER_INDEX, indexData, GL_STATIC_DRAW);
        delete[] indexData;
    
    } else {
        glBindBuffer(GL_ARRAY_BUFFER, vbo.first);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo.second);
    }
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
 
    glVertexPointer(3, GL_FLOAT, 0, 0);
    glNormalPointer(GL_FLOAT, 0, 0);
        
    glDrawRangeElementsEXT(GL_TRIANGLES, 0, vertices - 1, indices, GL_UNSIGNED_SHORT, 0);
        
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void GeometryCache::renderSquare(int xDivisions, int yDivisions) {
    VerticesIndices& vbo = _squareVBOs[IntPair(xDivisions, yDivisions)];
    int xVertices = xDivisions + 1;
    int yVertices = yDivisions + 1;
    int vertices = xVertices * yVertices;
    int indices = 2 * 3 * xDivisions * yDivisions;
    if (vbo.first == 0) {
        GLfloat* vertexData = new GLfloat[vertices * 3];
        GLfloat* vertex = vertexData;
        for (int i = 0; i <= yDivisions; i++) {
            float y = (float)i / yDivisions;
            
            for (int j = 0; j <= xDivisions; j++) {
                *(vertex++) = (float)j / xDivisions;
                *(vertex++) = y;
                *(vertex++) = 0.0f;
            }
        }
        
        glGenBuffers(1, &vbo.first);
        glBindBuffer(GL_ARRAY_BUFFER, vbo.first);
        const int BYTES_PER_VERTEX = 3 * sizeof(GLfloat);
        glBufferData(GL_ARRAY_BUFFER, vertices * BYTES_PER_VERTEX, vertexData, GL_STATIC_DRAW);
        delete[] vertexData;
        
        GLushort* indexData = new GLushort[indices];
        GLushort* index = indexData;
        for (int i = 0; i < yDivisions; i++) {
            GLushort bottom = i * xVertices;
            GLushort top = bottom + xVertices;
            for (int j = 0; j < xDivisions; j++) {
                int next = j + 1;
                
                *(index++) = bottom + j;
                *(index++) = top + next;
                *(index++) = top + j;
                
                *(index++) = bottom + j;
                *(index++) = bottom + next;
                *(index++) = top + next;
            }
        }
        
        glGenBuffers(1, &vbo.second);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo.second);
        const int BYTES_PER_INDEX = sizeof(GLushort);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices * BYTES_PER_INDEX, indexData, GL_STATIC_DRAW);
        delete[] indexData;
        
    } else {
        glBindBuffer(GL_ARRAY_BUFFER, vbo.first);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo.second);
    }
    glEnableClientState(GL_VERTEX_ARRAY);
 
    // all vertices have the same normal
    glNormal3f(0.0f, 0.0f, 1.0f);
    
    glVertexPointer(3, GL_FLOAT, 0, 0);
        
    glDrawRangeElementsEXT(GL_TRIANGLES, 0, vertices - 1, indices, GL_UNSIGNED_SHORT, 0);
        
    glDisableClientState(GL_VERTEX_ARRAY);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void GeometryCache::renderHalfCylinder(int slices, int stacks) {
    VerticesIndices& vbo = _halfCylinderVBOs[IntPair(slices, stacks)];
    int vertices = (slices + 1) * stacks;
    int indices = 2 * 3 * slices * (stacks - 1);
    if (vbo.first == 0) {    
        GLfloat* vertexData = new GLfloat[vertices * 2 * 3];
        GLfloat* vertex = vertexData;
        for (int i = 0; i <= (stacks - 1); i++) {
            float y = (float)i / (stacks - 1);
            
            for (int j = 0; j <= slices; j++) {
                float theta = 3 * PIf / 2 + PIf * j / slices;

                //normals
                *(vertex++) = sinf(theta);
                *(vertex++) = 0;
                *(vertex++) = cosf(theta);

                // vertices
                *(vertex++) = sinf(theta);
                *(vertex++) = y;
                *(vertex++) = cosf(theta);
            }
        }
        
        glGenBuffers(1, &vbo.first);
        glBindBuffer(GL_ARRAY_BUFFER, vbo.first);
        const int BYTES_PER_VERTEX = 3 * sizeof(GLfloat);
        glBufferData(GL_ARRAY_BUFFER, 2 * vertices * BYTES_PER_VERTEX, vertexData, GL_STATIC_DRAW);
        delete[] vertexData;
        
        GLushort* indexData = new GLushort[indices];
        GLushort* index = indexData;
        for (int i = 0; i < stacks - 1; i++) {
            GLushort bottom = i * (slices + 1);
            GLushort top = bottom + slices + 1;
            for (int j = 0; j < slices; j++) {
                int next = j + 1;
                
                *(index++) = bottom + j;
                *(index++) = top + next;
                *(index++) = top + j;
                
                *(index++) = bottom + j;
                *(index++) = bottom + next;
                *(index++) = top + next;
            }
        }
        
        glGenBuffers(1, &vbo.second);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo.second);
        const int BYTES_PER_INDEX = sizeof(GLushort);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices * BYTES_PER_INDEX, indexData, GL_STATIC_DRAW);
        delete[] indexData;
    
    } else {
        glBindBuffer(GL_ARRAY_BUFFER, vbo.first);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo.second);
    }
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);

    glNormalPointer(GL_FLOAT, 6 * sizeof(float), 0);
    glVertexPointer(3, GL_FLOAT, (6 * sizeof(float)), (const void *)(3 * sizeof(float)));
        
    glDrawRangeElementsEXT(GL_TRIANGLES, 0, vertices - 1, indices, GL_UNSIGNED_SHORT, 0);
        
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

QSharedPointer<NetworkGeometry> GeometryCache::getGeometry(const QUrl& url) {
    QSharedPointer<NetworkGeometry> geometry = _networkGeometry.value(url);
    if (geometry.isNull()) {
        geometry = QSharedPointer<NetworkGeometry>(new NetworkGeometry(url));
        _networkGeometry.insert(url, geometry);
    }
    return geometry;
}

NetworkGeometry::NetworkGeometry(const QUrl& url) :
    _modelReply(NULL),
    _mappingReply(NULL)
{
    if (!url.isValid()) {
        return;
    }
    QNetworkRequest modelRequest(url);
    modelRequest.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);
    _modelReply = Application::getInstance()->getNetworkAccessManager()->get(modelRequest);
    
    connect(_modelReply, SIGNAL(downloadProgress(qint64,qint64)), SLOT(maybeReadModelWithMapping()));
    connect(_modelReply, SIGNAL(error(QNetworkReply::NetworkError)), SLOT(handleModelReplyError()));
    
    QUrl mappingURL = url;
    QString path = url.path();
    mappingURL.setPath(path.left(path.lastIndexOf('.')) + ".fst");
    QNetworkRequest mappingRequest(mappingURL);
    mappingRequest.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);
    _mappingReply = Application::getInstance()->getNetworkAccessManager()->get(mappingRequest);
    
    connect(_mappingReply, SIGNAL(downloadProgress(qint64,qint64)), SLOT(maybeReadModelWithMapping()));
    connect(_mappingReply, SIGNAL(error(QNetworkReply::NetworkError)), SLOT(handleMappingReplyError()));
}

NetworkGeometry::~NetworkGeometry() {
    if (_modelReply != NULL) {
        delete _modelReply;
    }
    if (_mappingReply != NULL) {
        delete _mappingReply;
    }
    foreach (const NetworkMesh& mesh, _meshes) {
        glDeleteBuffers(1, &mesh.indexBufferID);
        glDeleteBuffers(1, &mesh.vertexBufferID);
    }    
}

glm::vec4 NetworkGeometry::computeAverageColor() const {
    glm::vec4 totalColor;
    int totalTriangles = 0;
    for (int i = 0; i < _meshes.size(); i++) {
        const FBXMesh& mesh = _geometry.meshes.at(i);
        if (mesh.isEye) {
            continue; // skip eyes
        }
        const NetworkMesh& networkMesh = _meshes.at(i);
        for (int j = 0; j < mesh.parts.size(); j++) {
            const FBXMeshPart& part = mesh.parts.at(j);
            const NetworkMeshPart& networkPart = networkMesh.parts.at(j);
            glm::vec4 color = glm::vec4(part.diffuseColor, 1.0f);
            if (networkPart.diffuseTexture) {
                color *= networkPart.diffuseTexture->getAverageColor();
            }
            int triangles = part.quadIndices.size() * 2 + part.triangleIndices.size();
            totalColor += color * triangles;
            totalTriangles += triangles;
        }
    }
    return (totalTriangles == 0) ? glm::vec4(1.0f, 1.0f, 1.0f, 1.0f) : totalColor / totalTriangles;
}

void NetworkGeometry::handleModelReplyError() {
    qDebug() << _modelReply->errorString() << "\n";
    
    _modelReply->disconnect(this);
    _modelReply->deleteLater();
    _modelReply = NULL;
}

void NetworkGeometry::handleMappingReplyError() {
    _mappingReply->disconnect(this);
    _mappingReply->deleteLater();
    _mappingReply = NULL;
    
    maybeReadModelWithMapping();
}

void NetworkGeometry::maybeReadModelWithMapping() {
    if (_modelReply == NULL || !_modelReply->isFinished() || (_mappingReply != NULL && !_mappingReply->isFinished())) {
        return;
    }
    
    QUrl url = _modelReply->url();
    QByteArray model = _modelReply->readAll();
    _modelReply->disconnect(this);
    _modelReply->deleteLater();
    _modelReply = NULL;
    
    QByteArray mapping;
    if (_mappingReply != NULL) {
        mapping = _mappingReply->readAll();
        _mappingReply->disconnect(this);
        _mappingReply->deleteLater();
        _mappingReply = NULL;
    }
    
    try {
        _geometry = url.path().toLower().endsWith(".svo") ? readSVO(model) : readFBX(model, mapping);
        
    } catch (const QString& error) {
        qDebug() << "Error reading " << url << ": " << error << "\n";
        return;
    }
    
    foreach (const FBXMesh& mesh, _geometry.meshes) {
        NetworkMesh networkMesh;
        
        int totalIndices = 0;
        foreach (const FBXMeshPart& part, mesh.parts) {
            NetworkMeshPart networkPart;
            QString basePath = url.path();
            basePath = basePath.left(basePath.lastIndexOf('/') + 1);
            if (!part.diffuseFilename.isEmpty()) {
                url.setPath(basePath + part.diffuseFilename);
                networkPart.diffuseTexture = Application::getInstance()->getTextureCache()->getTexture(url, false, mesh.isEye);
            }
            if (!part.normalFilename.isEmpty()) {
                url.setPath(basePath + part.normalFilename);
                networkPart.normalTexture = Application::getInstance()->getTextureCache()->getTexture(url, true);
            }
            networkMesh.parts.append(networkPart);
                        
            totalIndices += (part.quadIndices.size() + part.triangleIndices.size());
        }
                        
        glGenBuffers(1, &networkMesh.indexBufferID);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, networkMesh.indexBufferID);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, totalIndices * sizeof(int), NULL, GL_STATIC_DRAW);
        int offset = 0;
        foreach (const FBXMeshPart& part, mesh.parts) {
            glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, offset, part.quadIndices.size() * sizeof(int),
                part.quadIndices.constData());
            offset += part.quadIndices.size() * sizeof(int);
            glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, offset, part.triangleIndices.size() * sizeof(int),
                part.triangleIndices.constData());
            offset += part.triangleIndices.size() * sizeof(int);
        }
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        
        glGenBuffers(1, &networkMesh.vertexBufferID);
        glBindBuffer(GL_ARRAY_BUFFER, networkMesh.vertexBufferID);
            
        // if we don't need to do any blending or springing, then the positions/normals can be static
        if (mesh.blendshapes.isEmpty() && mesh.springiness == 0.0f) {
            int normalsOffset = mesh.vertices.size() * sizeof(glm::vec3);
            int tangentsOffset = normalsOffset + mesh.normals.size() * sizeof(glm::vec3);
            int colorsOffset = tangentsOffset + mesh.tangents.size() * sizeof(glm::vec3);
            int texCoordsOffset = colorsOffset + mesh.colors.size() * sizeof(glm::vec3);
            int clusterIndicesOffset = texCoordsOffset + mesh.texCoords.size() * sizeof(glm::vec2);
            int clusterWeightsOffset = clusterIndicesOffset + mesh.clusterIndices.size() * sizeof(glm::vec4);
            glBufferData(GL_ARRAY_BUFFER, clusterWeightsOffset + mesh.clusterWeights.size() * sizeof(glm::vec4),
                NULL, GL_STATIC_DRAW);
            glBufferSubData(GL_ARRAY_BUFFER, 0, mesh.vertices.size() * sizeof(glm::vec3), mesh.vertices.constData());
            glBufferSubData(GL_ARRAY_BUFFER, normalsOffset, mesh.normals.size() * sizeof(glm::vec3), mesh.normals.constData());
            glBufferSubData(GL_ARRAY_BUFFER, tangentsOffset, mesh.tangents.size() * sizeof(glm::vec3), mesh.tangents.constData());
            glBufferSubData(GL_ARRAY_BUFFER, colorsOffset, mesh.colors.size() * sizeof(glm::vec3), mesh.colors.constData());
            glBufferSubData(GL_ARRAY_BUFFER, texCoordsOffset, mesh.texCoords.size() * sizeof(glm::vec2),
                mesh.texCoords.constData());
            glBufferSubData(GL_ARRAY_BUFFER, clusterIndicesOffset, mesh.clusterIndices.size() * sizeof(glm::vec4),
                mesh.clusterIndices.constData());
            glBufferSubData(GL_ARRAY_BUFFER, clusterWeightsOffset, mesh.clusterWeights.size() * sizeof(glm::vec4),
                mesh.clusterWeights.constData());
        
        // if there's no springiness, then the cluster indices/weights can be static
        } else if (mesh.springiness == 0.0f) {
            int colorsOffset = mesh.tangents.size() * sizeof(glm::vec3);
            int texCoordsOffset = colorsOffset + mesh.colors.size() * sizeof(glm::vec3);
            int clusterIndicesOffset = texCoordsOffset + mesh.texCoords.size() * sizeof(glm::vec2);
            int clusterWeightsOffset = clusterIndicesOffset + mesh.clusterIndices.size() * sizeof(glm::vec4);
            glBufferData(GL_ARRAY_BUFFER, clusterWeightsOffset + mesh.clusterWeights.size() * sizeof(glm::vec4),
                NULL, GL_STATIC_DRAW);
            glBufferSubData(GL_ARRAY_BUFFER, 0, mesh.tangents.size() * sizeof(glm::vec3), mesh.tangents.constData());        
            glBufferSubData(GL_ARRAY_BUFFER, colorsOffset, mesh.colors.size() * sizeof(glm::vec3), mesh.colors.constData());    
            glBufferSubData(GL_ARRAY_BUFFER, texCoordsOffset, mesh.texCoords.size() * sizeof(glm::vec2), mesh.texCoords.constData());
            glBufferSubData(GL_ARRAY_BUFFER, clusterIndicesOffset, mesh.clusterIndices.size() * sizeof(glm::vec4),
                mesh.clusterIndices.constData());
            glBufferSubData(GL_ARRAY_BUFFER, clusterWeightsOffset, mesh.clusterWeights.size() * sizeof(glm::vec4),
                mesh.clusterWeights.constData());
            
        } else {
            int colorsOffset = mesh.tangents.size() * sizeof(glm::vec3);
            int texCoordsOffset = colorsOffset + mesh.colors.size() * sizeof(glm::vec3);
            glBufferData(GL_ARRAY_BUFFER, texCoordsOffset + mesh.texCoords.size() * sizeof(glm::vec2), NULL, GL_STATIC_DRAW);
            glBufferSubData(GL_ARRAY_BUFFER, 0, mesh.tangents.size() * sizeof(glm::vec3), mesh.tangents.constData());
            glBufferSubData(GL_ARRAY_BUFFER, colorsOffset, mesh.colors.size() * sizeof(glm::vec3), mesh.colors.constData());
            glBufferSubData(GL_ARRAY_BUFFER, texCoordsOffset, mesh.texCoords.size() * sizeof(glm::vec2),
                mesh.texCoords.constData());
        }
        
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        
        _meshes.append(networkMesh);
    }
}
