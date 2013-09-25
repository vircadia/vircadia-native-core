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

ProgramObject BlendFace::_eyeProgram;
GLuint BlendFace::_eyeTextureID;

void BlendFace::init() {
    if (!_eyeProgram.isLinked()) {
        switchToResourcesParentIfRequired();
        _eyeProgram.addShaderFromSourceFile(QGLShader::Vertex, "resources/shaders/eye.vert");
        _eyeProgram.addShaderFromSourceFile(QGLShader::Fragment, "resources/shaders/iris.frag");
        _eyeProgram.link();
        
        _eyeProgram.bind();
        _eyeProgram.setUniformValue("texture", 0);
        _eyeProgram.release();
        
        _eyeTextureID = Application::getInstance()->getTextureCache()->getFileTextureID("resources/images/eye.png");
    }
}

const glm::vec3 MODEL_TRANSLATION(0.0f, -0.025f, -0.025f); // temporary fudge factor
const float MODEL_SCALE = 0.0006f;

bool BlendFace::render(float alpha) {
    if (_meshIDs.isEmpty()) {
        return false;
    }

    glPushMatrix();
    glTranslatef(_owningHead->getPosition().x, _owningHead->getPosition().y, _owningHead->getPosition().z);
    glm::quat orientation = _owningHead->getOrientation();
    glm::vec3 axis = glm::axis(orientation);
    glRotatef(glm::angle(orientation), axis.x, axis.y, axis.z);    
    glTranslatef(MODEL_TRANSLATION.x, MODEL_TRANSLATION.y, MODEL_TRANSLATION.z); 
    glm::vec3 scale(-_owningHead->getScale() * MODEL_SCALE, _owningHead->getScale() * MODEL_SCALE,
        -_owningHead->getScale() * MODEL_SCALE);
    glScalef(scale.x, scale.y, scale.z);

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
    
    // enable normalization under the expectation that the GPU can do it faster
    glEnable(GL_NORMALIZE); 
    
    glColor4f(_owningHead->getSkinColor().r, _owningHead->getSkinColor().g, _owningHead->getSkinColor().b, alpha);
    
    for (int i = 0; i < _meshIDs.size(); i++) {
        const VerticesIndices& ids = _meshIDs.at(i);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ids.first);
        glBindBuffer(GL_ARRAY_BUFFER, ids.second);
        
        const FBXMesh& mesh = _geometry.meshes.at(i);    
        int vertexCount = mesh.vertices.size();
        
        // apply eye rotation if appropriate
        if (mesh.isEye) {
            glPushMatrix();
            glTranslatef(mesh.pivot.x, mesh.pivot.y, mesh.pivot.z);
            glm::quat rotation = glm::inverse(orientation) * _owningHead->getEyeRotation(orientation *
                (mesh.pivot * scale + MODEL_TRANSLATION) + _owningHead->getPosition());
            glm::vec3 rotationAxis = glm::axis(rotation);
            glRotatef(glm::angle(rotation), -rotationAxis.x, rotationAxis.y, -rotationAxis.z);
            glTranslatef(-mesh.pivot.x, -mesh.pivot.y, -mesh.pivot.z);
        
            // use texture coordinates only for the eye, for now
            glEnableClientState(GL_TEXTURE_COORD_ARRAY);
            
            glBindTexture(GL_TEXTURE_2D, _eyeTextureID);
            glEnable(GL_TEXTURE_2D);
            
            _eyeProgram.bind();
        }
        
        // all meshes after the first are white
        if (i == 1) {
            glColor4f(1.0f, 1.0f, 1.0f, alpha);
        }
        
        if (!mesh.blendshapes.isEmpty()) {
            _blendedVertices.resize(max(_blendedVertices.size(), vertexCount));
            _blendedNormals.resize(_blendedVertices.size());
            memcpy(_blendedVertices.data(), mesh.vertices.constData(), vertexCount * sizeof(glm::vec3));
            memcpy(_blendedNormals.data(), mesh.normals.constData(), vertexCount * sizeof(glm::vec3));
            
            // blend in each coefficient
            const vector<float>& coefficients = _owningHead->getBlendshapeCoefficients();
            for (int i = 0; i < coefficients.size(); i++) {
                float coefficient = coefficients[i];
                if (coefficient == 0.0f || i >= mesh.blendshapes.size() || mesh.blendshapes[i].vertices.isEmpty()) {
                    continue;
                }
                const float NORMAL_COEFFICIENT_SCALE = 0.01f;
                float normalCoefficient = coefficient * NORMAL_COEFFICIENT_SCALE;
                const glm::vec3* vertex = mesh.blendshapes[i].vertices.constData();
                const glm::vec3* normal = mesh.blendshapes[i].normals.constData();
                for (const int* index = mesh.blendshapes[i].indices.constData(),
                        *end = index + mesh.blendshapes[i].indices.size(); index != end; index++, vertex++, normal++) {
                    _blendedVertices[*index] += *vertex * coefficient;
                    _blendedNormals[*index] += *normal * normalCoefficient;
                }
            }
    
            glBufferSubData(GL_ARRAY_BUFFER, 0, vertexCount * sizeof(glm::vec3), _blendedVertices.constData());
            glBufferSubData(GL_ARRAY_BUFFER, vertexCount * sizeof(glm::vec3),
                vertexCount * sizeof(glm::vec3), _blendedNormals.constData());
        }
        
        glVertexPointer(3, GL_FLOAT, 0, 0);
        glNormalPointer(GL_FLOAT, 0, (void*)(vertexCount * sizeof(glm::vec3)));
        glTexCoordPointer(2, GL_FLOAT, 0, (void*)(vertexCount * 2 * sizeof(glm::vec3)));
        glDrawRangeElementsEXT(GL_QUADS, 0, vertexCount - 1, mesh.quadIndices.size(), GL_UNSIGNED_INT, 0);
        glDrawRangeElementsEXT(GL_TRIANGLES, 0, vertexCount - 1, mesh.triangleIndices.size(),
            GL_UNSIGNED_INT, (void*)(mesh.quadIndices.size() * sizeof(int)));
            
        if (mesh.isEye) {
            _eyeProgram.release();
            glBindTexture(GL_TEXTURE_2D, 0);
            glDisable(GL_TEXTURE_2D);
            glDisableClientState(GL_TEXTURE_COORD_ARRAY);
            glPopMatrix();
        }
    }
    
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

void BlendFace::getEyePositions(glm::vec3& firstEyePosition, glm::vec3& secondEyePosition) const {
    glm::quat orientation = _owningHead->getOrientation();
    glm::vec3 scale(-_owningHead->getScale() * MODEL_SCALE, _owningHead->getScale() * MODEL_SCALE,
        -_owningHead->getScale() * MODEL_SCALE);
    bool foundFirst = false;
    
    foreach (const FBXMesh& mesh, _geometry.meshes) {
        if (mesh.isEye) {
            glm::vec3 position = orientation * (mesh.pivot * scale + MODEL_TRANSLATION) + _owningHead->getPosition();
            if (foundFirst) {
                secondEyePosition = position;
                return;
            }
            firstEyePosition = position;
            foundFirst = true;
        }
    }
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
    // clear any existing geometry
    deleteGeometry();
    
    if (geometry.meshes.isEmpty()) {
        return;
    }
    foreach (const FBXMesh& mesh, geometry.meshes) {
        VerticesIndices ids;
        glGenBuffers(1, &ids.first);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ids.first);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, (mesh.quadIndices.size() + mesh.triangleIndices.size()) * sizeof(int),
            NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, mesh.quadIndices.size() * sizeof(int), mesh.quadIndices.constData());
        glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, mesh.quadIndices.size() * sizeof(int),
            mesh.triangleIndices.size() * sizeof(int), mesh.triangleIndices.constData());
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        
        glGenBuffers(1, &ids.second);
        glBindBuffer(GL_ARRAY_BUFFER, ids.second);
        
        if (mesh.blendshapes.isEmpty()) {
            glBufferData(GL_ARRAY_BUFFER, (mesh.vertices.size() + mesh.normals.size()) * sizeof(glm::vec3) +
                mesh.texCoords.size() * sizeof(glm::vec2), NULL, GL_STATIC_DRAW);
            glBufferSubData(GL_ARRAY_BUFFER, 0, mesh.vertices.size() * sizeof(glm::vec3), mesh.vertices.constData());
            glBufferSubData(GL_ARRAY_BUFFER, mesh.vertices.size() * sizeof(glm::vec3),
                mesh.normals.size() * sizeof(glm::vec3), mesh.normals.constData());
            glBufferSubData(GL_ARRAY_BUFFER, (mesh.vertices.size() + mesh.normals.size()) * sizeof(glm::vec3),
                mesh.texCoords.size() * sizeof(glm::vec2), mesh.texCoords.constData());    
                
        } else {
            glBufferData(GL_ARRAY_BUFFER, (mesh.vertices.size() + mesh.normals.size()) * sizeof(glm::vec3),
                NULL, GL_DYNAMIC_DRAW);
        }
        
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        
        _meshIDs.append(ids);
    }
    
    _geometry = geometry;
}

void BlendFace::deleteGeometry() {
    foreach (const VerticesIndices& meshIDs, _meshIDs) {
        glDeleteBuffers(1, &meshIDs.first);
        glDeleteBuffers(1, &meshIDs.second);
    }
    _meshIDs.clear();
}
