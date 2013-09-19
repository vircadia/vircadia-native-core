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
    glTranslatef(0.0f, -0.025f, -0.025f);
    const float MODEL_SCALE = 0.0006f;
    glScalef(_owningHead->getScale() * MODEL_SCALE, _owningHead->getScale() * MODEL_SCALE,
        -_owningHead->getScale() * MODEL_SCALE);

    glColor4f(1.0f, 1.0f, 1.0f, alpha);

    // start with the base
    int vertexCount = _geometry.vertices.size();
    _blendedVertices.resize(vertexCount);
    memcpy(_blendedVertices.data(), _geometry.vertices.constData(), vertexCount * sizeof(glm::vec3));
    
    // blend in each coefficient
    const vector<float>& coefficients = _owningHead->getBlendshapeCoefficients();
    for (int i = 0; i < coefficients.size(); i++) {
        float coefficient = coefficients[i];
        if (coefficient == 0.0f || i >= _geometry.blendshapes.size() || _geometry.blendshapes[i].vertices.isEmpty()) {
            continue;
        }
        const glm::vec3* source = _geometry.blendshapes[i].vertices.constData();
        for (const int* index = _geometry.blendshapes[i].indices.constData(),
                *end = index + _geometry.blendshapes[i].indices.size(); index != end; index++, source++) {
            _blendedVertices[*index] += *source * coefficient;
        }
    }
    
    // update the blended vertices
    glBindBuffer(GL_ARRAY_BUFFER, _vboID);
    glBufferSubData(GL_ARRAY_BUFFER, 0, vertexCount * sizeof(glm::vec3), _blendedVertices.constData());
    
    // tell OpenGL where to find vertex information
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, 0, 0);
    
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _iboID);
    glDrawRangeElementsEXT(GL_QUADS, 0, vertexCount - 1, _geometry.quadIndices.size(), GL_UNSIGNED_INT, 0);
    glDrawRangeElementsEXT(GL_TRIANGLES, 0, vertexCount - 1, _geometry.triangleIndices.size(), GL_UNSIGNED_INT,
        (void*)(_geometry.quadIndices.size() * sizeof(int)));

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    
    // deactivate vertex arrays after drawing
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
    
    // remember the URL
    _modelURL = url;
    
    // load the URL data asynchronously
    if (!url.isValid()) {
        return;
    }
    _modelReply = Application::getInstance()->getNetworkAccessManager()->get(QNetworkRequest(url));
    connect(_modelReply, SIGNAL(downloadProgress(qint64,qint64)), SLOT(handleModelDownloadProgress(qint64,qint64)));
    connect(_modelReply, SIGNAL(error(QNetworkReply::NetworkError)), SLOT(handleModelReplyError()));
}

glm::vec3 createVec3(const fsVector3f& vector) {
    return glm::vec3(vector.x, vector.y, vector.z);
}

void BlendFace::setRig(const fsMsgRig& rig) {
    // convert to FBX geometry
    FBXGeometry geometry;
    
    for (vector<fsVector4i>::const_iterator it = rig.mesh().m_quads.begin(), end = rig.mesh().m_quads.end(); it != end; it++) {
        geometry.quadIndices.append(it->x);
        geometry.quadIndices.append(it->y);
        geometry.quadIndices.append(it->z);
        geometry.quadIndices.append(it->w);
    }
    
    for (vector<fsVector3i>::const_iterator it = rig.mesh().m_tris.begin(), end = rig.mesh().m_tris.end(); it != end; it++) {
        geometry.triangleIndices.append(it->x);
        geometry.triangleIndices.append(it->y);
        geometry.triangleIndices.append(it->z);
    }
    
    for (vector<fsVector3f>::const_iterator it = rig.mesh().m_vertex_data.m_vertices.begin(),
            end = rig.mesh().m_vertex_data.m_vertices.end(); it != end; it++) {
        geometry.vertices.append(glm::vec3(it->x, it->y, it->z));
    }
    
    for (vector<fsVertexData>::const_iterator it = rig.blendshapes().begin(), end = rig.blendshapes().end(); it != end; it++) {
        FBXBlendshape blendshape;
        for (int i = 0, n = it->m_vertices.size(); i < n; i++) {
            // subtract the base vertex position; we want the deltas
            blendshape.vertices.append(createVec3(it->m_vertices[i]) - geometry.vertices[i]);
            blendshape.indices.append(i);
        }
        geometry.blendshapes.append(blendshape);
    }
    
    setGeometry(geometry);
}

void BlendFace::handleModelDownloadProgress(qint64 bytesReceived, qint64 bytesTotal) {
    if (bytesReceived < bytesTotal) {
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
    glBufferData(GL_ARRAY_BUFFER, geometry.vertices.size() * sizeof(glm::vec3), NULL, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    
    _geometry = geometry;
}
