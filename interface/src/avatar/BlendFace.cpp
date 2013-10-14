//
//  BlendFace.cpp
//  interface
//
//  Created by Andrzej Kapolka on 9/16/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include <QNetworkReply>

#include <glm/gtx/transform.hpp>

#include "Application.h"
#include "BlendFace.h"
#include "Head.h"

using namespace fs;
using namespace std;

BlendFace::BlendFace(Head* owningHead) :
    _owningHead(owningHead)
{
    // we may have been created in the network thread, but we live in the main thread
    moveToThread(Application::getInstance()->thread());
}

BlendFace::~BlendFace() {
    deleteGeometry();
}

ProgramObject BlendFace::_eyeProgram;

void BlendFace::init() {
    if (!_eyeProgram.isLinked()) {
        switchToResourcesParentIfRequired();
        _eyeProgram.addShaderFromSourceFile(QGLShader::Vertex, "resources/shaders/eye.vert");
        _eyeProgram.addShaderFromSourceFile(QGLShader::Fragment, "resources/shaders/iris.frag");
        _eyeProgram.link();
        
        _eyeProgram.bind();
        _eyeProgram.setUniformValue("texture", 0);
        _eyeProgram.release();
    }
}

void BlendFace::reset() {
    _resetStates = true;
}

const glm::vec3 MODEL_TRANSLATION(0.0f, -120.0f, 40.0f); // temporary fudge factor
const float MODEL_SCALE = 0.0006f;

void BlendFace::simulate(float deltaTime) {
    if (!isActive()) {
        return;
    }
    
    // set up world vertices on first simulate after load
    const FBXGeometry& geometry = _geometry->getFBXGeometry();
    if (_meshStates.isEmpty()) {
        QVector<glm::vec3> vertices;
        foreach (const FBXJoint& joint, geometry.joints) {
            JointState state;
            state.rotation = joint.rotation;
            _jointStates.append(state);
        }
        foreach (const FBXMesh& mesh, geometry.meshes) {
            MeshState state;
            state.jointMatrices.resize(mesh.clusters.size());
            if (mesh.springiness > 0.0f) {
                state.worldSpaceVertices.resize(mesh.vertices.size());
                state.vertexVelocities.resize(mesh.vertices.size());
                state.worldSpaceNormals.resize(mesh.vertices.size());
            }
            _meshStates.append(state);    
        }
        _resetStates = true;
    }
    
    glm::quat orientation = static_cast<Avatar*>(_owningHead->_owningAvatar)->getOrientation();
    glm::vec3 scale = glm::vec3(-1.0f, 1.0f, -1.0f) * _owningHead->getScale() * MODEL_SCALE;
    glm::vec3 offset = MODEL_TRANSLATION - _geometry->getFBXGeometry().neckPivot;
    glm::mat4 baseTransform = glm::translate(_owningHead->getPosition()) * glm::mat4_cast(orientation) *
        glm::scale(scale) * glm::translate(offset);
    
    // apply the neck rotation
    if (geometry.neckJointIndex != -1) {
        _jointStates[geometry.neckJointIndex].rotation = glm::quat(glm::radians(glm::vec3(
            _owningHead->getPitch(), _owningHead->getYaw(), _owningHead->getRoll())));
    }
    
    // update the world space transforms for all joints
    for (int i = 0; i < _jointStates.size(); i++) {
        JointState& state = _jointStates[i];
        const FBXJoint& joint = geometry.joints.at(i);
        if (joint.parentIndex == -1) {
            state.transform = baseTransform * geometry.offset * joint.preRotation *
                glm::mat4_cast(state.rotation) * joint.postRotation;
            
        } else {
            state.transform = _jointStates[joint.parentIndex].transform * joint.preRotation;
            if (i == geometry.leftEyeJointIndex || i == geometry.rightEyeJointIndex) {
                // extract the translation component of the matrix
                state.rotation = _owningHead->getEyeRotation(glm::vec3(
                    state.transform[3][0], state.transform[3][1], state.transform[3][2]));
            }
            state.transform = state.transform * glm::mat4_cast(state.rotation) * joint.postRotation;
        }
    }
    
    for (int i = 0; i < _meshStates.size(); i++) {
        MeshState& state = _meshStates[i];
        const FBXMesh& mesh = geometry.meshes.at(i);
        for (int j = 0; j < mesh.clusters.size(); j++) {
            const FBXCluster& cluster = mesh.clusters.at(j);
            state.jointMatrices[j] = _jointStates[cluster.jointIndex].transform * cluster.inverseBindMatrix;
        }
        int vertexCount = state.worldSpaceVertices.size();
        if (vertexCount == 0) {
            continue;
        }
        glm::vec3* destVertices = state.worldSpaceVertices.data();
        glm::vec3* destVelocities = state.vertexVelocities.data();
        glm::vec3* destNormals = state.worldSpaceNormals.data();
        
        const glm::vec3* sourceVertices = mesh.vertices.constData();
        if (!mesh.blendshapes.isEmpty()) {
            _blendedVertices.resize(max(_blendedVertices.size(), vertexCount));
            memcpy(_blendedVertices.data(), mesh.vertices.constData(), vertexCount * sizeof(glm::vec3));
            
            // blend in each coefficient
            const vector<float>& coefficients = _owningHead->getBlendshapeCoefficients();
            for (int j = 0; j < coefficients.size(); j++) {
                float coefficient = coefficients[j];
                if (coefficient == 0.0f || j >= mesh.blendshapes.size() || mesh.blendshapes[j].vertices.isEmpty()) {
                    continue;
                }
                const glm::vec3* vertex = mesh.blendshapes[j].vertices.constData();
                for (const int* index = mesh.blendshapes[j].indices.constData(),
                        *end = index + mesh.blendshapes[j].indices.size(); index != end; index++, vertex++) {
                    _blendedVertices[*index] += *vertex * coefficient;
                }
            }
            
            sourceVertices = _blendedVertices.constData();
        }
        glm::mat4 transform;
        if (mesh.clusters.size() > 1) {
            _blendedVertices.resize(max(_blendedVertices.size(), vertexCount));

            // skin each vertex
            const glm::vec4* clusterIndices = mesh.clusterIndices.constData();
            const glm::vec4* clusterWeights = mesh.clusterWeights.constData();
            for (int j = 0; j < vertexCount; j++) {
                _blendedVertices[j] =
                    glm::vec3(state.jointMatrices[clusterIndices[j][0]] *
                        glm::vec4(sourceVertices[j], 1.0f)) * clusterWeights[j][0] +
                    glm::vec3(state.jointMatrices[clusterIndices[j][1]] *
                        glm::vec4(sourceVertices[j], 1.0f)) * clusterWeights[j][1] +
                    glm::vec3(state.jointMatrices[clusterIndices[j][2]] *
                        glm::vec4(sourceVertices[j], 1.0f)) * clusterWeights[j][2] +
                    glm::vec3(state.jointMatrices[clusterIndices[j][3]] *
                        glm::vec4(sourceVertices[j], 1.0f)) * clusterWeights[j][3];
            }
            
            sourceVertices = _blendedVertices.constData();
            
        } else {
            transform = state.jointMatrices[0];
        }
        if (_resetStates) {
            for (int j = 0; j < vertexCount; j++) {
                destVertices[j] = glm::vec3(transform * glm::vec4(sourceVertices[j], 1.0f));        
                destVelocities[j] = glm::vec3();
            }        
        } else {
            const float SPRINGINESS_MULTIPLIER = 200.0f;
            const float DAMPING = 5.0f;
            for (int j = 0; j < vertexCount; j++) {
                destVelocities[j] += ((glm::vec3(transform * glm::vec4(sourceVertices[j], 1.0f)) - destVertices[j]) *
                    mesh.springiness * SPRINGINESS_MULTIPLIER - destVelocities[j] * DAMPING) * deltaTime;
                destVertices[j] += destVelocities[j] * deltaTime;
            }
        }
        for (int j = 0; j < vertexCount; j++) {
            destNormals[j] = glm::vec3();
            
            const glm::vec3& middle = destVertices[j];
            for (QVarLengthArray<QPair<int, int>, 4>::const_iterator connection = mesh.vertexConnections.at(j).constBegin(); 
                    connection != mesh.vertexConnections.at(j).constEnd(); connection++) {
                destNormals[j] += glm::normalize(glm::cross(destVertices[connection->second] - middle,
                    destVertices[connection->first] - middle));
            }
        }
    }
    _resetStates = false;
}

bool BlendFace::render(float alpha) {
    if (_meshStates.isEmpty()) {
        return false;
    }

    // set up blended buffer ids on first render after load/simulate
    const FBXGeometry& geometry = _geometry->getFBXGeometry();
    const QVector<NetworkMesh>& networkMeshes = _geometry->getMeshes();
    if (_blendedVertexBufferIDs.isEmpty()) {
        foreach (const FBXMesh& mesh, geometry.meshes) {
            GLuint id = 0;
            if (!mesh.blendshapes.isEmpty() || mesh.springiness > 0.0f) {
                glGenBuffers(1, &id);
                glBindBuffer(GL_ARRAY_BUFFER, id);
                glBufferData(GL_ARRAY_BUFFER, (mesh.vertices.size() + mesh.normals.size()) * sizeof(glm::vec3),
                    NULL, GL_DYNAMIC_DRAW);
                glBindBuffer(GL_ARRAY_BUFFER, 0);
            }
            _blendedVertexBufferIDs.append(id);
        }
        
        // make sure we have the right number of dilated texture pointers
        _dilatedTextures.resize(geometry.meshes.size());
    }

    glm::quat orientation = _owningHead->getOrientation();
    glm::vec3 scale(-_owningHead->getScale() * MODEL_SCALE, _owningHead->getScale() * MODEL_SCALE,
        -_owningHead->getScale() * MODEL_SCALE);
    glm::vec3 offset = MODEL_TRANSLATION - geometry.neckPivot;
    
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    
    // enable normalization under the expectation that the GPU can do it faster
    glEnable(GL_NORMALIZE); 
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_COLOR_MATERIAL);
    
    // the eye shader uses the color state even though color material is disabled
    glColor4f(1.0f, 1.0f, 1.0f, alpha);
    
    for (int i = 0; i < networkMeshes.size(); i++) {
        const NetworkMesh& networkMesh = networkMeshes.at(i);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, networkMesh.indexBufferID);
        
        const FBXMesh& mesh = geometry.meshes.at(i);    
        int vertexCount = mesh.vertices.size();
        
        glPushMatrix();
        
        // apply eye rotation if appropriate
        Texture* texture = networkMesh.diffuseTexture.data();
        if (mesh.isEye) {
            _eyeProgram.bind();
            
            if (texture != NULL) {
                texture = (_dilatedTextures[i] = static_cast<DilatableNetworkTexture*>(texture)->getDilatedTexture(
                    _owningHead->getPupilDilation())).data();
            }
        }
        
        // apply material properties
        glm::vec4 diffuse = glm::vec4(mesh.diffuseColor, alpha);
        glm::vec4 specular = glm::vec4(mesh.specularColor, alpha);
        glMaterialfv(GL_FRONT, GL_AMBIENT, (const float*)&diffuse);
        glMaterialfv(GL_FRONT, GL_DIFFUSE, (const float*)&diffuse);
        glMaterialfv(GL_FRONT, GL_SPECULAR, (const float*)&specular);
        glMaterialf(GL_FRONT, GL_SHININESS, mesh.shininess);
        
        const MeshState& state = _meshStates.at(i);
        if (state.worldSpaceVertices.isEmpty()) {
            glMultMatrixf((const GLfloat*)&state.jointMatrices[0]);
        }
        
        glBindTexture(GL_TEXTURE_2D, texture == NULL ? 0 : texture->getID());
        
        glBindBuffer(GL_ARRAY_BUFFER, networkMesh.vertexBufferID);
        if (mesh.blendshapes.isEmpty() && mesh.springiness == 0.0f) {
            glTexCoordPointer(2, GL_FLOAT, 0, (void*)(vertexCount * 2 * sizeof(glm::vec3)));    
        
        } else {
            glTexCoordPointer(2, GL_FLOAT, 0, 0);
            glBindBuffer(GL_ARRAY_BUFFER, _blendedVertexBufferIDs.at(i));
            
            if (!state.worldSpaceVertices.isEmpty()) {
                glBufferSubData(GL_ARRAY_BUFFER, 0, vertexCount * sizeof(glm::vec3), state.worldSpaceVertices.constData());
                glBufferSubData(GL_ARRAY_BUFFER, vertexCount * sizeof(glm::vec3),
                    vertexCount * sizeof(glm::vec3), state.worldSpaceNormals.constData());
                
            } else {
                _blendedVertices.resize(max(_blendedVertices.size(), vertexCount));
                _blendedNormals.resize(_blendedVertices.size());
                memcpy(_blendedVertices.data(), mesh.vertices.constData(), vertexCount * sizeof(glm::vec3));
                memcpy(_blendedNormals.data(), mesh.normals.constData(), vertexCount * sizeof(glm::vec3));
                
                // blend in each coefficient
                const vector<float>& coefficients = _owningHead->getBlendshapeCoefficients();
                for (int j = 0; j < coefficients.size(); j++) {
                    float coefficient = coefficients[j];
                    if (coefficient == 0.0f || j >= mesh.blendshapes.size() || mesh.blendshapes[j].vertices.isEmpty()) {
                        continue;
                    }
                    const float NORMAL_COEFFICIENT_SCALE = 0.01f;
                    float normalCoefficient = coefficient * NORMAL_COEFFICIENT_SCALE;
                    const glm::vec3* vertex = mesh.blendshapes[j].vertices.constData();
                    const glm::vec3* normal = mesh.blendshapes[j].normals.constData();
                    for (const int* index = mesh.blendshapes[j].indices.constData(),
                            *end = index + mesh.blendshapes[j].indices.size(); index != end; index++, vertex++, normal++) {
                        _blendedVertices[*index] += *vertex * coefficient;
                        _blendedNormals[*index] += *normal * normalCoefficient;
                    }
                }
        
                glBufferSubData(GL_ARRAY_BUFFER, 0, vertexCount * sizeof(glm::vec3), _blendedVertices.constData());
                glBufferSubData(GL_ARRAY_BUFFER, vertexCount * sizeof(glm::vec3),
                    vertexCount * sizeof(glm::vec3), _blendedNormals.constData());
            }
        }
        glVertexPointer(3, GL_FLOAT, 0, 0);
        glNormalPointer(GL_FLOAT, 0, (void*)(vertexCount * sizeof(glm::vec3)));
        
        glDrawRangeElementsEXT(GL_QUADS, 0, vertexCount - 1, mesh.quadIndices.size(), GL_UNSIGNED_INT, 0);
        glDrawRangeElementsEXT(GL_TRIANGLES, 0, vertexCount - 1, mesh.triangleIndices.size(),
            GL_UNSIGNED_INT, (void*)(mesh.quadIndices.size() * sizeof(int)));
            
        if (mesh.isEye) {
            _eyeProgram.release();
        }
        
        glPopMatrix();
    }
    
    glDisable(GL_NORMALIZE); 
    glDisable(GL_TEXTURE_2D);
    
    // deactivate vertex arrays after drawing
    glDisableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    
    // bind with 0 to switch back to normal operation
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    // restore all the default material settings
    Application::getInstance()->setupWorldLight(*Application::getInstance()->getCamera());

    return true;
}

bool BlendFace::getEyePositions(glm::vec3& firstEyePosition, glm::vec3& secondEyePosition, bool upright) const {
    if (!isActive() || _jointStates.isEmpty()) {
        return false;
    }
    const FBXGeometry& geometry = _geometry->getFBXGeometry();
    if (geometry.leftEyeJointIndex != -1) {
        const glm::mat4& transform = _jointStates[geometry.leftEyeJointIndex].transform;
        firstEyePosition = glm::vec3(transform[3][0], transform[3][1], transform[3][2]);
    }
    if (geometry.rightEyeJointIndex != -1) {
        const glm::mat4& transform = _jointStates[geometry.rightEyeJointIndex].transform;
        secondEyePosition = glm::vec3(transform[3][0], transform[3][1], transform[3][2]);
    }
    return geometry.leftEyeJointIndex != -1 && geometry.rightEyeJointIndex != -1;
}

glm::vec4 BlendFace::computeAverageColor() const {
    return _geometry ? _geometry->computeAverageColor() : glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
}

void BlendFace::setModelURL(const QUrl& url) {
    // don't recreate the geometry if it's the same URL
    if (_modelURL == url) {
        return;
    }
    _modelURL = url;

    // delete our local geometry and custom textures
    deleteGeometry();
    _dilatedTextures.clear();
    
    _geometry = Application::getInstance()->getGeometryCache()->getGeometry(url);
}

void BlendFace::deleteGeometry() {
    foreach (GLuint id, _blendedVertexBufferIDs) {
        glDeleteBuffers(1, &id);
    }
    _blendedVertexBufferIDs.clear();
    _jointStates.clear();
    _meshStates.clear();
}
