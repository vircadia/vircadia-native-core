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
    _modelReply(NULL),
    _iboID(0)
{
    // we may have been created in the network thread, but we live in the main thread
    moveToThread(Application::getInstance()->thread());
}

BlendFace::~BlendFace() {
    if (_iboID != 0) {
        glDeleteBuffers(1, &_iboID);
        glDeleteBuffers(1, &_vboID);
    }
}

bool BlendFace::render(float alpha) {
    if (_iboID == 0) {
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

    glColor4f(1.0f, 1.0f, 1.0f, alpha);

    // start with the base
    int vertexCount = _geometry.vertices.size();
    int normalCount = _geometry.normals.size();
    _blendedVertices.resize(vertexCount);
    _blendedNormals.resize(normalCount);
    memcpy(_blendedVertices.data(), _geometry.vertices.constData(), vertexCount * sizeof(glm::vec3));
    memcpy(_blendedNormals.data(), _geometry.normals.constData(), normalCount * sizeof(glm::vec3));
    
    // blend in each coefficient
    const vector<float>& coefficients = _owningHead->getBlendshapeCoefficients();
    for (int i = 0; i < coefficients.size(); i++) {
        float coefficient = coefficients[i];
        if (coefficient == 0.0f || i >= _geometry.blendshapes.size() || _geometry.blendshapes[i].vertices.isEmpty()) {
            continue;
        }
        const glm::vec3* vertex = _geometry.blendshapes[i].vertices.constData();
        const glm::vec3* normal = _geometry.blendshapes[i].normals.constData();
        for (const int* index = _geometry.blendshapes[i].indices.constData(),
                *end = index + _geometry.blendshapes[i].indices.size(); index != end; index++, vertex++, normal++) {
            _blendedVertices[*index] += *vertex * coefficient;
            _blendedNormals[*index] += *normal * coefficient;
        }
    }
    
    // update the blended vertices
    glBindBuffer(GL_ARRAY_BUFFER, _vboID);
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
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _iboID);
    glDrawRangeElementsEXT(GL_QUADS, 0, vertexCount - 1, _geometry.quadIndices.size(), GL_UNSIGNED_INT, 0);
    glDrawRangeElementsEXT(GL_TRIANGLES, 0, vertexCount - 1, _geometry.triangleIndices.size(), GL_UNSIGNED_INT,
        (void*)(_geometry.quadIndices.size() * sizeof(int)));

    glDisable(GL_NORMALIZE); 
    
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

void BlendFace::setGeometry(const FBXGeometry& geometry) {
    if (geometry.vertices.isEmpty()) {
        // clear any existing geometry
        if (_iboID != 0) {
            glDeleteBuffers(1, &_iboID);
            glDeleteBuffers(1, &_vboID);
            _iboID = 0;
        }
        return;
    }
    if (_iboID == 0) {
        glGenBuffers(1, &_iboID);
        glGenBuffers(1, &_vboID);
    }
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _iboID);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, (geometry.quadIndices.size() + geometry.triangleIndices.size()) * sizeof(int),
        NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, geometry.quadIndices.size() * sizeof(int), geometry.quadIndices.constData());
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, geometry.quadIndices.size() * sizeof(int),
        geometry.triangleIndices.size() * sizeof(int), geometry.triangleIndices.constData());
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    
    glBindBuffer(GL_ARRAY_BUFFER, _vboID);
    glBufferData(GL_ARRAY_BUFFER, (geometry.vertices.size() + geometry.normals.size()) * sizeof(glm::vec3),
        NULL, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    
    _geometry = geometry;
}
