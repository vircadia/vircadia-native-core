//
//  BlendFace.cpp
//  interface
//
//  Created by Andrzej Kapolka on 9/16/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include <QNetworkReply>

#include "Application.h"
#include "BlendFace.h"
#include "Head.h"

using namespace fs;
using namespace std;

BlendFace::BlendFace(Head* owningHead) :
    _owningHead(owningHead),
    _modelReply(NULL)
{
    // we may have been created in the network thread, but we live in the main thread
    moveToThread(Application::getInstance()->thread());
}

BlendFace::~BlendFace() {
    deleteGeometry();
}

bool BlendFace::render(float alpha) {
    if (_baseMeshIDs.first == 0) {
        return false;
    }

    glPushMatrix();
    glTranslatef(_owningHead->getPosition().x, _owningHead->getPosition().y, _owningHead->getPosition().z);
    glm::quat orientation = _owningHead->getOrientation();
    glm::vec3 axis = glm::axis(orientation);
    glRotatef(glm::angle(orientation), axis.x, axis.y, axis.z);
    glTranslatef(0.0f, -0.025f, -0.025f); // temporary fudge factor until we have a better method of per-model positioning
    const float MODEL_SCALE = 0.0006f;
    glScalef(_owningHead->getScale() * MODEL_SCALE, _owningHead->getScale() * MODEL_SCALE,
        -_owningHead->getScale() * MODEL_SCALE);

    // start with the base
    int vertexCount = _geometry.blendMesh.vertices.size();
    int normalCount = _geometry.blendMesh.normals.size();
    _blendedVertices.resize(vertexCount);
    _blendedNormals.resize(normalCount);
    memcpy(_blendedVertices.data(), _geometry.blendMesh.vertices.constData(), vertexCount * sizeof(glm::vec3));
    memcpy(_blendedNormals.data(), _geometry.blendMesh.normals.constData(), normalCount * sizeof(glm::vec3));
    
    // blend in each coefficient
    const vector<float>& coefficients = _owningHead->getBlendshapeCoefficients();
    for (int i = 0; i < coefficients.size(); i++) {
        float coefficient = coefficients[i];
        if (coefficient == 0.0f || i >= _geometry.blendshapes.size() || _geometry.blendshapes[i].vertices.isEmpty()) {
            continue;
        }
        const float NORMAL_COEFFICIENT_SCALE = 0.01f;
        float normalCoefficient = coefficient * NORMAL_COEFFICIENT_SCALE;
        const glm::vec3* vertex = _geometry.blendshapes[i].vertices.constData();
        const glm::vec3* normal = _geometry.blendshapes[i].normals.constData();
        for (const int* index = _geometry.blendshapes[i].indices.constData(),
                *end = index + _geometry.blendshapes[i].indices.size(); index != end; index++, vertex++, normal++) {
            _blendedVertices[*index] += *vertex * coefficient;
            _blendedNormals[*index] += *normal * normalCoefficient;
        }
    }
    
    // use the head skin color
    glColor4f(_owningHead->getSkinColor().r, _owningHead->getSkinColor().g, _owningHead->getSkinColor().b, alpha);
    
    // update the blended vertices
    glBindBuffer(GL_ARRAY_BUFFER, _baseMeshIDs.second);
    glBufferSubData(GL_ARRAY_BUFFER, 0, vertexCount * sizeof(glm::vec3), _blendedVertices.constData());
    glBufferSubData(GL_ARRAY_BUFFER, vertexCount * sizeof(glm::vec3),
        normalCount * sizeof(glm::vec3), _blendedNormals.constData());
    
    // tell OpenGL where to find vertex information
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, 0, 0);
    glEnableClientState(GL_NORMAL_ARRAY);
    glNormalPointer(GL_FLOAT, 0, (void*)(vertexCount * sizeof(glm::vec3)));
    
    // enable normalization under the expectation that the GPU can do it faster
    glEnable(GL_NORMALIZE); 
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _baseMeshIDs.first);
    glDrawRangeElementsEXT(GL_QUADS, 0, vertexCount - 1, _geometry.blendMesh.quadIndices.size(), GL_UNSIGNED_INT, 0);
    glDrawRangeElementsEXT(GL_TRIANGLES, 0, vertexCount - 1, _geometry.blendMesh.triangleIndices.size(), GL_UNSIGNED_INT,
        (void*)(_geometry.blendMesh.quadIndices.size() * sizeof(int)));

    glDisable(GL_NORMALIZE); 

    // back to white for the other meshes
    glColor4f(1.0f, 1.0f, 1.0f, alpha);
    
    for (int i = 0; i < _geometry.otherMeshes.size(); i++) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _otherMeshIDs.at(i).first);
        glBindBuffer(GL_ARRAY_BUFFER, _otherMeshIDs.at(i).second);
        
        glVertexPointer(3, GL_FLOAT, 0, 0);
        int vertexCount = _geometry.otherMeshes.at(i).vertices.size();
        glNormalPointer(GL_FLOAT, 0, (void*)(vertexCount * sizeof(glm::vec3)));
        
        glDrawRangeElementsEXT(GL_QUADS, 0, vertexCount - 1, _geometry.otherMeshes.at(i).quadIndices.size(),
            GL_UNSIGNED_INT, 0);
        glDrawRangeElementsEXT(GL_TRIANGLES, 0, vertexCount - 1, _geometry.otherMeshes.at(i).triangleIndices.size(),
            GL_UNSIGNED_INT, (void*)(_geometry.otherMeshes.at(i).quadIndices.size() * sizeof(int)));
    }
    
    // deactivate vertex arrays after drawing
    glDisableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    
    // bind with 0 to switch back to normal operation
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    glPopMatrix();

    return true;
}

void BlendFace::setModelURL(const QUrl& url) {
    // don't restart the download if it's the same URL
    if (_modelURL == url) {
        return;
    }

    // cancel any current download
    if (_modelReply != 0) {
        delete _modelReply;
        _modelReply = 0;
    }
    
    // clear the current geometry, if any
    setGeometry(FBXGeometry());
    
    // remember the URL
    _modelURL = url;
    
    // load the URL data asynchronously
    if (!url.isValid()) {
        return;
    }
    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);
    _modelReply = Application::getInstance()->getNetworkAccessManager()->get(request);
    connect(_modelReply, SIGNAL(downloadProgress(qint64,qint64)), SLOT(handleModelDownloadProgress(qint64,qint64)));
    connect(_modelReply, SIGNAL(error(QNetworkReply::NetworkError)), SLOT(handleModelReplyError()));
}

glm::vec3 createVec3(const fsVector3f& vector) {
    return glm::vec3(vector.x, vector.y, vector.z);
}

void BlendFace::handleModelDownloadProgress(qint64 bytesReceived, qint64 bytesTotal) {
    if (bytesReceived < bytesTotal && !_modelReply->isFinished()) {
        return;
    }

    QByteArray entirety = _modelReply->readAll();
    _modelReply->disconnect(this);
    _modelReply->deleteLater();
    _modelReply = 0;
    
    try {
        setGeometry(extractFBXGeometry(parseFBX(entirety)));
    
    } catch (const QString& error) {
        qDebug() << error << "\n";
        return;
    }
}

void BlendFace::handleModelReplyError() {
    qDebug("%s\n", _modelReply->errorString().toLocal8Bit().constData());
    
    _modelReply->disconnect(this);
    _modelReply->deleteLater();
    _modelReply = 0;
}

void createIndexBuffer(const FBXMesh& mesh, GLuint& iboID) {
    glGenBuffers(1, &iboID);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, iboID);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, (mesh.quadIndices.size() + mesh.triangleIndices.size()) * sizeof(int),
        NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, mesh.quadIndices.size() * sizeof(int), mesh.quadIndices.constData());
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, mesh.quadIndices.size() * sizeof(int),
        mesh.triangleIndices.size() * sizeof(int), mesh.triangleIndices.constData());
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void BlendFace::setGeometry(const FBXGeometry& geometry) {
    // clear any existing geometry
    deleteGeometry();
    
    if (geometry.blendMesh.vertices.isEmpty()) {
        return;
    }
    createIndexBuffer(geometry.blendMesh, _baseMeshIDs.first);
    
    glGenBuffers(1, &_baseMeshIDs.second);
    glBindBuffer(GL_ARRAY_BUFFER, _baseMeshIDs.second);
    glBufferData(GL_ARRAY_BUFFER, (geometry.blendMesh.vertices.size() + geometry.blendMesh.normals.size()) * sizeof(glm::vec3),
        NULL, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    
    foreach (const FBXMesh& mesh, geometry.otherMeshes) {
        VerticesIndices ids;
        createIndexBuffer(mesh, ids.first);
        
        glGenBuffers(1, &ids.second);
        glBindBuffer(GL_ARRAY_BUFFER, ids.second);
        glBufferData(GL_ARRAY_BUFFER, (mesh.vertices.size() + mesh.normals.size()) * sizeof(glm::vec3),
            NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, mesh.vertices.size() * sizeof(glm::vec3), mesh.vertices.constData());
        glBufferSubData(GL_ARRAY_BUFFER, mesh.vertices.size() * sizeof(glm::vec3),
            mesh.normals.size() * sizeof(glm::vec3), mesh.normals.constData());
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        
        _otherMeshIDs.append(ids);
    }
    
    _geometry = geometry;
}

void BlendFace::deleteGeometry() {
    if (_baseMeshIDs.first != 0) {
        glDeleteBuffers(1, &_baseMeshIDs.first);
        glDeleteBuffers(1, &_baseMeshIDs.second);
        _baseMeshIDs = VerticesIndices();
    }
    foreach (const VerticesIndices& meshIDs, _otherMeshIDs) {
        glDeleteBuffers(1, &meshIDs.first);
        glDeleteBuffers(1, &meshIDs.second);
    }
    _otherMeshIDs.clear();
}
