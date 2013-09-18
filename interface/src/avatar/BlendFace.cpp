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
#include "renderer/FBXReader.h"

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
    int vertexCount = _baseVertices.size();
    _blendedVertices.resize(vertexCount);
    memcpy(_blendedVertices.data(), _baseVertices.data(), vertexCount * sizeof(fsVector3f));
    
    // blend in each coefficient
    const vector<float>& coefficients = _owningHead->getBlendshapeCoefficients();
    for (int i = 0; i < coefficients.size(); i++) {
        float coefficient = coefficients[i];
        if (coefficient == 0.0f || i >= _blendshapes.size()) {
            continue;
        }
        const fsVector3f* source = _blendshapes[i].m_vertices.data();
        fsVector3f* dest = _blendedVertices.data();
        for (int j = 0; j < vertexCount; j++) {
            dest->x += source->x * coefficient;
            dest->y += source->y * coefficient;
            dest->z += source->z * coefficient;
            
            source++;
            dest++;
        }
    }
    
    // update the blended vertices
    glBindBuffer(GL_ARRAY_BUFFER, _vboID);
    glBufferSubData(GL_ARRAY_BUFFER, 0, _blendedVertices.size() * sizeof(fsVector3f), _blendedVertices.data());
    
    // tell OpenGL where to find vertex information
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, 0, 0);
    
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _iboID);
    glDrawRangeElementsEXT(GL_QUADS, 0, _baseVertices.size() - 1, _quadIndexCount, GL_UNSIGNED_INT, 0);
    glDrawRangeElementsEXT(GL_TRIANGLES, 0, _baseVertices.size() - 1, _triangleIndexCount, GL_UNSIGNED_INT,
        (void*)(_quadIndexCount * sizeof(int)));

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

void BlendFace::setRig(const fsMsgRig& rig) {
    if (rig.mesh().m_tris.empty()) {
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
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, rig.mesh().m_quads.size() * sizeof(fsVector4i) +
        rig.mesh().m_tris.size() * sizeof(fsVector3i), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, rig.mesh().m_quads.size() * sizeof(fsVector4i), rig.mesh().m_quads.data());
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, rig.mesh().m_quads.size() * sizeof(fsVector4i),
        rig.mesh().m_tris.size() * sizeof(fsVector3i), rig.mesh().m_tris.data());
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    
    glBindBuffer(GL_ARRAY_BUFFER, _vboID);
    glBufferData(GL_ARRAY_BUFFER, rig.mesh().m_vertex_data.m_vertices.size() * sizeof(fsVector3f), NULL, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    
    _quadIndexCount = rig.mesh().m_quads.size() * 4;
    _triangleIndexCount = rig.mesh().m_tris.size() * 3;
    _baseVertices = rig.mesh().m_vertex_data.m_vertices;
    _blendshapes = rig.blendshapes();
    
    // subtract the neutral locations from the blend shapes; we want the deltas
    int vertexCount = _baseVertices.size();
    for (vector<fsVertexData>::iterator it = _blendshapes.begin(); it != _blendshapes.end(); it++) {
        const fsVector3f* neutral = _baseVertices.data();
        fsVector3f* offset = it->m_vertices.data();
        for (int i = 0; i < vertexCount; i++) {
            offset->x -= neutral->x;
            offset->y -= neutral->y;
            offset->z -= neutral->z;
            
            neutral++;
            offset++;
        }
    }
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
        printNode(parseFBX(entirety));
    
    } catch (const QString& error) {
        qDebug() << error << "\n";
    }
}

void BlendFace::handleModelReplyError() {
    qDebug("%s\n", _modelReply->errorString().toLocal8Bit().constData());
    
    _modelReply->disconnect(this);
    _modelReply->deleteLater();
    _modelReply = 0;
}

