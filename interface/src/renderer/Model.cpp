//
//  Model.cpp
//  interface
//
//  Created by Andrzej Kapolka on 10/18/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include <glm/gtx/transform.hpp>

#include "Application.h"
#include "Model.h"

using namespace std;

Model::Model() :
    _pupilDilation(0.0f)
{
    // we may have been created in the network thread, but we live in the main thread
    moveToThread(Application::getInstance()->thread());
}

Model::~Model() {
    deleteGeometry();
}

ProgramObject Model::_program;
ProgramObject Model::_skinProgram;
int Model::_clusterMatricesLocation;
int Model::_clusterIndicesLocation;
int Model::_clusterWeightsLocation;

void Model::init() {
    if (!_program.isLinked()) {
        switchToResourcesParentIfRequired();
        _program.addShaderFromSourceFile(QGLShader::Vertex, "resources/shaders/model.vert");
        _program.addShaderFromSourceFile(QGLShader::Fragment, "resources/shaders/model.frag");
        _program.link();
        
        _program.bind();
        _program.setUniformValue("texture", 0);
        _program.release();
        
        _skinProgram.addShaderFromSourceFile(QGLShader::Vertex, "resources/shaders/skin_model.vert");
        _skinProgram.addShaderFromSourceFile(QGLShader::Fragment, "resources/shaders/model.frag");
        _skinProgram.link();
        
        _skinProgram.bind();
        _clusterMatricesLocation = _skinProgram.uniformLocation("clusterMatrices");
        _clusterIndicesLocation = _skinProgram.attributeLocation("clusterIndices");
        _clusterWeightsLocation = _skinProgram.attributeLocation("clusterWeights");
        _skinProgram.setUniformValue("texture", 0);
        _skinProgram.release();
    }
}

void Model::reset() {
    _resetStates = true;
}

void Model::simulate(float deltaTime) {
    if (!isActive()) {
        return;
    }
    
    // set up world vertices on first simulate after load
    const FBXGeometry& geometry = _geometry->getFBXGeometry();
    if (_jointStates.isEmpty()) {
        QVector<glm::vec3> vertices;
        foreach (const FBXJoint& joint, geometry.joints) {
            JointState state;
            state.rotation = joint.rotation;
            _jointStates.append(state);
        }
        foreach (const FBXMesh& mesh, geometry.meshes) {
            MeshState state;
            state.clusterMatrices.resize(mesh.clusters.size());
            if (mesh.springiness > 0.0f) {
                state.worldSpaceVertices.resize(mesh.vertices.size());
                state.vertexVelocities.resize(mesh.vertices.size());
                state.worldSpaceNormals.resize(mesh.vertices.size());
            }
            _meshStates.append(state);    
        }
        _resetStates = true;
    }
    
    // update the world space transforms for all joints
    for (int i = 0; i < _jointStates.size(); i++) {
        updateJointState(i);
    }
    
    for (int i = 0; i < _meshStates.size(); i++) {
        MeshState& state = _meshStates[i];
        const FBXMesh& mesh = geometry.meshes.at(i);
        for (int j = 0; j < mesh.clusters.size(); j++) {
            const FBXCluster& cluster = mesh.clusters.at(j);
            state.clusterMatrices[j] = _jointStates[cluster.jointIndex].transform * cluster.inverseBindMatrix;
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
            for (int j = 0; j < _blendshapeCoefficients.size(); j++) {
                float coefficient = _blendshapeCoefficients[j];
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
                    glm::vec3(state.clusterMatrices[clusterIndices[j][0]] *
                        glm::vec4(sourceVertices[j], 1.0f)) * clusterWeights[j][0] +
                    glm::vec3(state.clusterMatrices[clusterIndices[j][1]] *
                        glm::vec4(sourceVertices[j], 1.0f)) * clusterWeights[j][1] +
                    glm::vec3(state.clusterMatrices[clusterIndices[j][2]] *
                        glm::vec4(sourceVertices[j], 1.0f)) * clusterWeights[j][2] +
                    glm::vec3(state.clusterMatrices[clusterIndices[j][3]] *
                        glm::vec4(sourceVertices[j], 1.0f)) * clusterWeights[j][3];
            }
            sourceVertices = _blendedVertices.constData();
            
        } else {
            transform = state.clusterMatrices[0];
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

bool Model::render(float alpha) {
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
            
            QVector<QSharedPointer<Texture> > dilated;
            dilated.resize(mesh.parts.size());
            _dilatedTextures.append(dilated);
        }
    }

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    
    glDisable(GL_COLOR_MATERIAL);
    
    for (int i = 0; i < networkMeshes.size(); i++) {
        const NetworkMesh& networkMesh = networkMeshes.at(i);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, networkMesh.indexBufferID);
        
        const FBXMesh& mesh = geometry.meshes.at(i);    
        int vertexCount = mesh.vertices.size();
        
        glBindBuffer(GL_ARRAY_BUFFER, networkMesh.vertexBufferID);
        
        const MeshState& state = _meshStates.at(i);
        if (state.worldSpaceVertices.isEmpty()) {
            if (state.clusterMatrices.size() > 1) {
                _skinProgram.bind();
                glUniformMatrix4fvARB(_clusterMatricesLocation, state.clusterMatrices.size(), false,
                    (const float*)state.clusterMatrices.constData());
                int offset = vertexCount * sizeof(glm::vec2) + (mesh.blendshapes.isEmpty() ?
                    vertexCount * 2 * sizeof(glm::vec3) : 0);
                _skinProgram.setAttributeBuffer(_clusterIndicesLocation, GL_FLOAT, offset, 4);
                _skinProgram.setAttributeBuffer(_clusterWeightsLocation, GL_FLOAT,
                    offset + vertexCount * sizeof(glm::vec4), 4);
                _skinProgram.enableAttributeArray(_clusterIndicesLocation);
                _skinProgram.enableAttributeArray(_clusterWeightsLocation);
                
            } else {
                glPushMatrix();
                glMultMatrixf((const GLfloat*)&state.clusterMatrices[0]);
                _program.bind();
            }
        } else {
            _program.bind();
        }
        
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
                for (int j = 0; j < _blendshapeCoefficients.size(); j++) {
                    float coefficient = _blendshapeCoefficients[j];
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
        
        qint64 offset = 0;
        for (int j = 0; j < networkMesh.parts.size(); j++) {
            const NetworkMeshPart& networkPart = networkMesh.parts.at(j);
            const FBXMeshPart& part = mesh.parts.at(j);
            
            // apply material properties
            glm::vec4 diffuse = glm::vec4(part.diffuseColor, alpha);
            glm::vec4 specular = glm::vec4(part.specularColor, alpha);
            glMaterialfv(GL_FRONT, GL_AMBIENT, (const float*)&diffuse);
            glMaterialfv(GL_FRONT, GL_DIFFUSE, (const float*)&diffuse);
            glMaterialfv(GL_FRONT, GL_SPECULAR, (const float*)&specular);
            glMaterialf(GL_FRONT, GL_SHININESS, part.shininess);
        
            Texture* texture = networkPart.diffuseTexture.data();
            if (mesh.isEye) {
                if (texture != NULL) {
                    texture = (_dilatedTextures[i][j] = static_cast<DilatableNetworkTexture*>(texture)->getDilatedTexture(
                        _pupilDilation)).data();
                }
            }
            glBindTexture(GL_TEXTURE_2D, texture == NULL ? Application::getInstance()->getTextureCache()->getWhiteTextureID() :
                texture->getID());
            
            glDrawRangeElementsEXT(GL_QUADS, 0, vertexCount - 1, part.quadIndices.size(), GL_UNSIGNED_INT, (void*)offset);
            offset += part.quadIndices.size() * sizeof(int);
            glDrawRangeElementsEXT(GL_TRIANGLES, 0, vertexCount - 1, part.triangleIndices.size(),
                GL_UNSIGNED_INT, (void*)offset);
            offset += part.triangleIndices.size() * sizeof(int);
        }
        
        if (state.worldSpaceVertices.isEmpty()) {
            if (state.clusterMatrices.size() > 1) {
                _skinProgram.disableAttributeArray(_clusterIndicesLocation);
                _skinProgram.disableAttributeArray(_clusterWeightsLocation);
                _skinProgram.release();
                           
            } else {
                glPopMatrix();
                _program.release();
            }
        } else {
            _program.release();
        }
    }
    
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

bool Model::getHeadPosition(glm::vec3& headPosition) const {
    return isActive() && getJointPosition(_geometry->getFBXGeometry().headJointIndex, headPosition);
}

bool Model::getNeckPosition(glm::vec3& neckPosition) const {
    return isActive() && getJointPosition(_geometry->getFBXGeometry().neckJointIndex, neckPosition);
}

bool Model::getNeckRotation(glm::quat& neckRotation) const {
    return isActive() && getJointRotation(_geometry->getFBXGeometry().neckJointIndex, neckRotation);
}

bool Model::getEyePositions(glm::vec3& firstEyePosition, glm::vec3& secondEyePosition) const {
    if (!isActive()) {
        return false;
    }
    const FBXGeometry& geometry = _geometry->getFBXGeometry();
    return getJointPosition(geometry.leftEyeJointIndex, firstEyePosition) &&
        getJointPosition(geometry.rightEyeJointIndex, secondEyePosition);
}

void Model::setURL(const QUrl& url) {
    // don't recreate the geometry if it's the same URL
    if (_url == url) {
        return;
    }
    _url = url;

    // delete our local geometry and custom textures
    deleteGeometry();
    _dilatedTextures.clear();
    
    _geometry = Application::getInstance()->getGeometryCache()->getGeometry(url);
}

glm::vec4 Model::computeAverageColor() const {
    return _geometry ? _geometry->computeAverageColor() : glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
}

void Model::updateJointState(int index) {
    JointState& state = _jointStates[index];
    const FBXGeometry& geometry = _geometry->getFBXGeometry();
    const FBXJoint& joint = geometry.joints.at(index);
    
    if (joint.parentIndex == -1) {
        glm::mat4 baseTransform = glm::translate(_translation) * glm::mat4_cast(_rotation) *
            glm::scale(_scale) * glm::translate(_offset);
        state.transform = baseTransform * geometry.offset * joint.preRotation *
            glm::mat4_cast(state.rotation) * joint.postRotation;
    
    } else {
        state.transform = _jointStates[joint.parentIndex].transform * joint.preRotation *
            glm::mat4_cast(state.rotation) * joint.postRotation;
    }
}

bool Model::getJointPosition(int jointIndex, glm::vec3& position) const {
    if (jointIndex == -1 || _jointStates.isEmpty()) {
        return false;
    }
    const glm::mat4& transform = _jointStates[jointIndex].transform;
    position = glm::vec3(transform[3][0], transform[3][1], transform[3][2]);
    return true;
}

bool Model::getJointRotation(int jointIndex, glm::quat& rotation) const {
    if (jointIndex == -1 || _jointStates.isEmpty()) {
        return false;
    }
    const glm::mat4& transform = _jointStates[jointIndex].transform;
    rotation = glm::normalize(glm::quat_cast(transform));
    return true;
}

void Model::deleteGeometry() {
    foreach (GLuint id, _blendedVertexBufferIDs) {
        glDeleteBuffers(1, &id);
    }
    _blendedVertexBufferIDs.clear();
    _jointStates.clear();
    _meshStates.clear();
}
