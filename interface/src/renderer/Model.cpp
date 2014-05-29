//
//  Model.cpp
//  interface/src/renderer
//
//  Created by Andrzej Kapolka on 10/18/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QMetaType>
#include <QRunnable>
#include <QThreadPool>

#include <glm/gtx/transform.hpp>
#include <glm/gtx/norm.hpp>

#include <GeometryUtil.h>

#include "Application.h"
#include "Model.h"

#include <SphereShape.h>
#include <CapsuleShape.h>
#include <ShapeCollider.h>

using namespace std;

static int modelPointerTypeId = qRegisterMetaType<QPointer<Model> >();
static int weakNetworkGeometryPointerTypeId = qRegisterMetaType<QWeakPointer<NetworkGeometry> >();
static int vec3VectorTypeId = qRegisterMetaType<QVector<glm::vec3> >();

Model::Model(QObject* parent) :
    QObject(parent),
    _scale(1.0f, 1.0f, 1.0f),
    _scaleToFit(false),
    _scaleToFitLargestDimension(0.0f),
    _scaledToFit(false),
    _snapModelToCenter(false),
    _snappedToCenter(false),
    _rootIndex(-1),
    _shapesAreDirty(true),
    _boundingRadius(0.f),
    _boundingShape(), 
    _boundingShapeLocalOffset(0.f),
    _lodDistance(0.0f),
    _pupilDilation(0.0f),
    _url("http://invalid.com") {
    // we may have been created in the network thread, but we live in the main thread
    moveToThread(Application::getInstance()->thread());
}

Model::~Model() {
    deleteGeometry();
}

ProgramObject Model::_program;
ProgramObject Model::_normalMapProgram;
ProgramObject Model::_specularMapProgram;
ProgramObject Model::_normalSpecularMapProgram;

ProgramObject Model::_shadowMapProgram;
ProgramObject Model::_shadowNormalMapProgram;
ProgramObject Model::_shadowSpecularMapProgram;
ProgramObject Model::_shadowNormalSpecularMapProgram;

ProgramObject Model::_shadowProgram;

ProgramObject Model::_skinProgram;
ProgramObject Model::_skinNormalMapProgram;
ProgramObject Model::_skinSpecularMapProgram;
ProgramObject Model::_skinNormalSpecularMapProgram;

ProgramObject Model::_skinShadowMapProgram;
ProgramObject Model::_skinShadowNormalMapProgram;
ProgramObject Model::_skinShadowSpecularMapProgram;
ProgramObject Model::_skinShadowNormalSpecularMapProgram;

ProgramObject Model::_skinShadowProgram;

int Model::_normalMapTangentLocation;
int Model::_normalSpecularMapTangentLocation;
int Model::_shadowNormalMapTangentLocation;
int Model::_shadowNormalSpecularMapTangentLocation;

Model::SkinLocations Model::_skinLocations;
Model::SkinLocations Model::_skinNormalMapLocations;
Model::SkinLocations Model::_skinSpecularMapLocations;
Model::SkinLocations Model::_skinNormalSpecularMapLocations;
Model::SkinLocations Model::_skinShadowMapLocations;
Model::SkinLocations Model::_skinShadowNormalMapLocations;
Model::SkinLocations Model::_skinShadowSpecularMapLocations;
Model::SkinLocations Model::_skinShadowNormalSpecularMapLocations;
Model::SkinLocations Model::_skinShadowLocations;

void Model::setScale(const glm::vec3& scale) {
    setScaleInternal(scale);
    // if anyone sets scale manually, then we are no longer scaled to fit
    _scaleToFit = false;
    _scaledToFit = false;
}

void Model::setScaleInternal(const glm::vec3& scale) {
    float scaleLength = glm::length(_scale);
    float relativeDeltaScale = glm::length(_scale - scale) / scaleLength;
    
    const float ONE_PERCENT = 0.01f;
    if (relativeDeltaScale > ONE_PERCENT || scaleLength < EPSILON) {
        _scale = scale;
        rebuildShapes();
    }
}

void Model::setOffset(const glm::vec3& offset) { 
    _offset = offset; 
    
    // if someone manually sets our offset, then we are no longer snapped to center
    _snapModelToCenter = false; 
    _snappedToCenter = false; 
}


void Model::initSkinProgram(ProgramObject& program, Model::SkinLocations& locations,
        int specularTextureUnit, int shadowTextureUnit) {
    program.bind();
    locations.clusterMatrices = program.uniformLocation("clusterMatrices");
    locations.clusterIndices = program.attributeLocation("clusterIndices");
    locations.clusterWeights = program.attributeLocation("clusterWeights");
    locations.tangent = program.attributeLocation("tangent");
    program.setUniformValue("diffuseMap", 0);
    program.setUniformValue("normalMap", 1);
    program.setUniformValue("specularMap", specularTextureUnit);
    program.setUniformValue("shadowMap", shadowTextureUnit);
    program.release();
}

QVector<JointState> Model::createJointStates(const FBXGeometry& geometry) {
    QVector<JointState> jointStates;
    foreach (const FBXJoint& joint, geometry.joints) {
        JointState state;
        state.setFBXJoint(joint);
        jointStates.append(state);
    }

    // compute transforms
    // Unfortunately, the joints are not neccessarily in order from parents to children, 
    // so we must iterate over the list multiple times until all are set correctly.
    QVector<bool> jointIsSet;
    int numJoints = jointStates.size();
    jointIsSet.fill(false, numJoints);
    int numJointsSet = 0;
    int lastNumJointsSet = -1;
    while (numJointsSet < numJoints && numJointsSet != lastNumJointsSet) {
        lastNumJointsSet = numJointsSet;
        for (int i = 0; i < numJoints; ++i) {
            if (jointIsSet[i]) {
                continue;
            }
            JointState& state = jointStates[i];
            const FBXJoint& joint = state.getFBXJoint();
            int parentIndex = joint.parentIndex;
            if (parentIndex == -1) {
                _rootIndex = i;
                glm::mat4 baseTransform = glm::mat4_cast(_rotation) * glm::scale(_scale) * glm::translate(_offset) * geometry.offset;
                state.updateWorldTransform(baseTransform, _rotation);
                ++numJointsSet;
                jointIsSet[i] = true;
            } else if (jointIsSet[parentIndex]) {
                const JointState& parentState = jointStates.at(parentIndex);
                state.updateWorldTransform(parentState._transform, parentState._combinedRotation);
                ++numJointsSet;
                jointIsSet[i] = true;
            }
        }
    }

    return jointStates;
}

void Model::init() {
    if (!_program.isLinked()) {
        _program.addShaderFromSourceFile(QGLShader::Vertex, Application::resourcesPath() + "shaders/model.vert");
        _program.addShaderFromSourceFile(QGLShader::Fragment, Application::resourcesPath() + "shaders/model.frag");
        _program.link();
        
        _program.bind();
        _program.setUniformValue("diffuseMap", 0);
        _program.release();
        
        _normalMapProgram.addShaderFromSourceFile(QGLShader::Vertex,
            Application::resourcesPath() + "shaders/model_normal_map.vert");
        _normalMapProgram.addShaderFromSourceFile(QGLShader::Fragment,
            Application::resourcesPath() + "shaders/model_normal_map.frag");
        _normalMapProgram.link();
        
        _normalMapProgram.bind();
        _normalMapProgram.setUniformValue("diffuseMap", 0);
        _normalMapProgram.setUniformValue("normalMap", 1);
        _normalMapTangentLocation = _normalMapProgram.attributeLocation("tangent");
        _normalMapProgram.release();
        
        _specularMapProgram.addShaderFromSourceFile(QGLShader::Vertex,
            Application::resourcesPath() + "shaders/model.vert");
        _specularMapProgram.addShaderFromSourceFile(QGLShader::Fragment,
            Application::resourcesPath() + "shaders/model_specular_map.frag");
        _specularMapProgram.link();
        
        _specularMapProgram.bind();
        _specularMapProgram.setUniformValue("diffuseMap", 0);
        _specularMapProgram.setUniformValue("specularMap", 1);
        _specularMapProgram.release();
        
        _normalSpecularMapProgram.addShaderFromSourceFile(QGLShader::Vertex,
            Application::resourcesPath() + "shaders/model_normal_map.vert");
        _normalSpecularMapProgram.addShaderFromSourceFile(QGLShader::Fragment,
            Application::resourcesPath() + "shaders/model_normal_specular_map.frag");
        _normalSpecularMapProgram.link();
        
        _normalSpecularMapProgram.bind();
        _normalSpecularMapProgram.setUniformValue("diffuseMap", 0);
        _normalSpecularMapProgram.setUniformValue("normalMap", 1);
        _normalSpecularMapProgram.setUniformValue("specularMap", 2);
        _normalSpecularMapTangentLocation = _normalMapProgram.attributeLocation("tangent");
        _normalSpecularMapProgram.release();
        
        
        _shadowMapProgram.addShaderFromSourceFile(QGLShader::Vertex, Application::resourcesPath() + "shaders/model.vert");
        _shadowMapProgram.addShaderFromSourceFile(QGLShader::Fragment, Application::resourcesPath() +
            "shaders/model_shadow_map.frag");
        _shadowMapProgram.link();
        
        _shadowMapProgram.bind();
        _shadowMapProgram.setUniformValue("diffuseMap", 0);
        _shadowMapProgram.setUniformValue("shadowMap", 1);
        _shadowMapProgram.release();
        
        _shadowNormalMapProgram.addShaderFromSourceFile(QGLShader::Vertex,
            Application::resourcesPath() + "shaders/model_normal_map.vert");
        _shadowNormalMapProgram.addShaderFromSourceFile(QGLShader::Fragment,
            Application::resourcesPath() + "shaders/model_shadow_normal_map.frag");
        _shadowNormalMapProgram.link();
        
        _shadowNormalMapProgram.bind();
        _shadowNormalMapProgram.setUniformValue("diffuseMap", 0);
        _shadowNormalMapProgram.setUniformValue("normalMap", 1);
        _shadowNormalMapProgram.setUniformValue("shadowMap", 2);
        _shadowNormalMapTangentLocation = _shadowNormalMapProgram.attributeLocation("tangent");
        _shadowNormalMapProgram.release();
        
        _shadowSpecularMapProgram.addShaderFromSourceFile(QGLShader::Vertex,
            Application::resourcesPath() + "shaders/model.vert");
        _shadowSpecularMapProgram.addShaderFromSourceFile(QGLShader::Fragment,
            Application::resourcesPath() + "shaders/model_shadow_specular_map.frag");
        _shadowSpecularMapProgram.link();
        
        _shadowSpecularMapProgram.bind();
        _shadowSpecularMapProgram.setUniformValue("diffuseMap", 0);
        _shadowSpecularMapProgram.setUniformValue("specularMap", 1);
         _shadowSpecularMapProgram.setUniformValue("shadowMap", 2);
        _shadowSpecularMapProgram.release();
        
        _shadowNormalSpecularMapProgram.addShaderFromSourceFile(QGLShader::Vertex,
            Application::resourcesPath() + "shaders/model_normal_map.vert");
        _shadowNormalSpecularMapProgram.addShaderFromSourceFile(QGLShader::Fragment,
            Application::resourcesPath() + "shaders/model_shadow_normal_specular_map.frag");
        _shadowNormalSpecularMapProgram.link();
        
        _shadowNormalSpecularMapProgram.bind();
        _shadowNormalSpecularMapProgram.setUniformValue("diffuseMap", 0);
        _shadowNormalSpecularMapProgram.setUniformValue("normalMap", 1);
        _shadowNormalSpecularMapProgram.setUniformValue("specularMap", 2);
        _shadowNormalSpecularMapProgram.setUniformValue("shadowMap", 3);
        _shadowNormalSpecularMapTangentLocation = _normalMapProgram.attributeLocation("tangent");
        _shadowNormalSpecularMapProgram.release();
        
        
        _shadowProgram.addShaderFromSourceFile(QGLShader::Vertex, Application::resourcesPath() + "shaders/model_shadow.vert");
        _shadowProgram.addShaderFromSourceFile(QGLShader::Fragment,
            Application::resourcesPath() + "shaders/model_shadow.frag");
        _shadowProgram.link();
        
        
        _skinProgram.addShaderFromSourceFile(QGLShader::Vertex, Application::resourcesPath() + "shaders/skin_model.vert");
        _skinProgram.addShaderFromSourceFile(QGLShader::Fragment, Application::resourcesPath() + "shaders/model.frag");
        _skinProgram.link();
        
        initSkinProgram(_skinProgram, _skinLocations);
        
        _skinNormalMapProgram.addShaderFromSourceFile(QGLShader::Vertex,
            Application::resourcesPath() + "shaders/skin_model_normal_map.vert");
        _skinNormalMapProgram.addShaderFromSourceFile(QGLShader::Fragment,
            Application::resourcesPath() + "shaders/model_normal_map.frag");
        _skinNormalMapProgram.link();
        
        initSkinProgram(_skinNormalMapProgram, _skinNormalMapLocations);
        
        _skinSpecularMapProgram.addShaderFromSourceFile(QGLShader::Vertex,
            Application::resourcesPath() + "shaders/skin_model.vert");
        _skinSpecularMapProgram.addShaderFromSourceFile(QGLShader::Fragment,
            Application::resourcesPath() + "shaders/model_specular_map.frag");
        _skinSpecularMapProgram.link();
        
        initSkinProgram(_skinSpecularMapProgram, _skinSpecularMapLocations);
        
        _skinNormalSpecularMapProgram.addShaderFromSourceFile(QGLShader::Vertex,
            Application::resourcesPath() + "shaders/skin_model_normal_map.vert");
        _skinNormalSpecularMapProgram.addShaderFromSourceFile(QGLShader::Fragment,
            Application::resourcesPath() + "shaders/model_normal_specular_map.frag");
        _skinNormalSpecularMapProgram.link();
        
        initSkinProgram(_skinNormalSpecularMapProgram, _skinNormalSpecularMapLocations, 2);
        
        
        _skinShadowMapProgram.addShaderFromSourceFile(QGLShader::Vertex, Application::resourcesPath() +
            "shaders/skin_model.vert");
        _skinShadowMapProgram.addShaderFromSourceFile(QGLShader::Fragment, Application::resourcesPath() +
            "shaders/model_shadow_map.frag");
        _skinShadowMapProgram.link();
        
        initSkinProgram(_skinShadowMapProgram, _skinShadowMapLocations);
        
        _skinShadowNormalMapProgram.addShaderFromSourceFile(QGLShader::Vertex,
            Application::resourcesPath() + "shaders/skin_model_normal_map.vert");
        _skinShadowNormalMapProgram.addShaderFromSourceFile(QGLShader::Fragment,
            Application::resourcesPath() + "shaders/model_shadow_normal_map.frag");
        _skinShadowNormalMapProgram.link();
        
        initSkinProgram(_skinShadowNormalMapProgram, _skinShadowNormalMapLocations, 1, 2);
        
        _skinShadowSpecularMapProgram.addShaderFromSourceFile(QGLShader::Vertex,
            Application::resourcesPath() + "shaders/skin_model.vert");
        _skinShadowSpecularMapProgram.addShaderFromSourceFile(QGLShader::Fragment,
            Application::resourcesPath() + "shaders/model_shadow_specular_map.frag");
        _skinShadowSpecularMapProgram.link();
        
        initSkinProgram(_skinShadowSpecularMapProgram, _skinShadowSpecularMapLocations, 1, 2);
        
        _skinShadowNormalSpecularMapProgram.addShaderFromSourceFile(QGLShader::Vertex,
            Application::resourcesPath() + "shaders/skin_model_normal_map.vert");
        _skinShadowNormalSpecularMapProgram.addShaderFromSourceFile(QGLShader::Fragment,
            Application::resourcesPath() + "shaders/model_shadow_normal_specular_map.frag");
        _skinShadowNormalSpecularMapProgram.link();
        
        initSkinProgram(_skinShadowNormalSpecularMapProgram, _skinShadowNormalSpecularMapLocations, 2, 3);
        
        
        _skinShadowProgram.addShaderFromSourceFile(QGLShader::Vertex,
            Application::resourcesPath() + "shaders/skin_model_shadow.vert");
        _skinShadowProgram.addShaderFromSourceFile(QGLShader::Fragment,
            Application::resourcesPath() + "shaders/model_shadow.frag");
        _skinShadowProgram.link();
        
        initSkinProgram(_skinShadowProgram, _skinShadowLocations);
    }
}

void Model::reset() {
    if (_jointStates.isEmpty()) {
        return;
    }
    foreach (Model* attachment, _attachments) {
        attachment->reset();
    }
    const FBXGeometry& geometry = _geometry->getFBXGeometry();
    for (int i = 0; i < _jointStates.size(); i++) {
        _jointStates[i]._rotation = geometry.joints.at(i).rotation;
    }
}

bool Model::updateGeometry() {
    // NOTE: this is a recursive call that walks all attachments, and their attachments
    bool needFullUpdate = false;
    for (int i = 0; i < _attachments.size(); i++) {
        Model* model = _attachments.at(i);
        if (model->updateGeometry()) {
            needFullUpdate = true;
        }
    }

    bool needToRebuild = false;
    if (_nextGeometry) {
        _nextGeometry = _nextGeometry->getLODOrFallback(_lodDistance, _nextLODHysteresis);
        _nextGeometry->setLoadPriority(this, -_lodDistance);
        _nextGeometry->ensureLoading();
        if (_nextGeometry->isLoaded()) {
            applyNextGeometry();
            needToRebuild = true;
        }
    }
    if (!_geometry) {
        // geometry is not ready
        return false;
    }

    QSharedPointer<NetworkGeometry> geometry = _geometry->getLODOrFallback(_lodDistance, _lodHysteresis);
    if (_geometry != geometry) {
        // NOTE: it is theoretically impossible to reach here after passing through the applyNextGeometry() call above.
        // Which means we don't need to worry about calling deleteGeometry() below immediately after creating new geometry.

        const FBXGeometry& newGeometry = geometry->getFBXGeometry();
        QVector<JointState> newJointStates = createJointStates(newGeometry);
        if (! _jointStates.isEmpty()) {
            // copy the existing joint states
            const FBXGeometry& oldGeometry = _geometry->getFBXGeometry();
            for (QHash<QString, int>::const_iterator it = oldGeometry.jointIndices.constBegin();
                    it != oldGeometry.jointIndices.constEnd(); it++) {
                int oldIndex = it.value() - 1;
                int newIndex = newGeometry.getJointIndex(it.key());
                if (newIndex != -1) {
                    newJointStates[newIndex] = _jointStates.at(oldIndex);
                }
            }
        } 
        deleteGeometry();
        _dilatedTextures.clear();
        _geometry = geometry;
        _jointStates = newJointStates;
        needToRebuild = true;
    } else if (_jointStates.isEmpty()) {
        const FBXGeometry& fbxGeometry = geometry->getFBXGeometry();
        if (fbxGeometry.joints.size() > 0) {
            _jointStates = createJointStates(fbxGeometry);
            needToRebuild = true;
        }
    } else if (!geometry->isLoaded()) {
        deleteGeometry();
        _dilatedTextures.clear();
    }
    _geometry->setLoadPriority(this, -_lodDistance);
    _geometry->ensureLoading();
   
    if (needToRebuild) {
        const FBXGeometry& fbxGeometry = geometry->getFBXGeometry();
        foreach (const FBXMesh& mesh, fbxGeometry.meshes) {
            MeshState state;
            state.clusterMatrices.resize(mesh.clusters.size());
            _meshStates.append(state);    
            
            QOpenGLBuffer buffer;
            if (!mesh.blendshapes.isEmpty()) {
                buffer.setUsagePattern(QOpenGLBuffer::DynamicDraw);
                buffer.create();
                buffer.bind();
                buffer.allocate((mesh.vertices.size() + mesh.normals.size()) * sizeof(glm::vec3));
                buffer.write(0, mesh.vertices.constData(), mesh.vertices.size() * sizeof(glm::vec3));
                buffer.write(mesh.vertices.size() * sizeof(glm::vec3), mesh.normals.constData(),
                    mesh.normals.size() * sizeof(glm::vec3));
                buffer.release();
            }
            _blendedVertexBuffers.append(buffer);
        }
        foreach (const FBXAttachment& attachment, fbxGeometry.attachments) {
            Model* model = new Model(this);
            model->init();
            model->setURL(attachment.url);
            _attachments.append(model);
        }
        rebuildShapes();
        needFullUpdate = true;
    }
    return needFullUpdate;
}

bool Model::render(float alpha, RenderMode mode, bool receiveShadows) {
    // render the attachments
    foreach (Model* attachment, _attachments) {
        attachment->render(alpha, mode);
    }
    if (_meshStates.isEmpty()) {
        return false;
    }
    
    // set up dilated textures on first render after load/simulate
    const FBXGeometry& geometry = _geometry->getFBXGeometry();
    if (_dilatedTextures.isEmpty()) {
        foreach (const FBXMesh& mesh, geometry.meshes) {
            QVector<QSharedPointer<Texture> > dilated;
            dilated.resize(mesh.parts.size());
            _dilatedTextures.append(dilated);
        }
    }

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
    
    glDisable(GL_COLOR_MATERIAL);
    
    if (mode == DIFFUSE_RENDER_MODE || mode == NORMAL_RENDER_MODE) {
        glDisable(GL_CULL_FACE);
    } else {
        glEnable(GL_CULL_FACE);
        if (mode == SHADOW_RENDER_MODE) {
            glCullFace(GL_FRONT);
        }
    }
    
    // render opaque meshes with alpha testing
    
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.5f * alpha);
    
    receiveShadows &= Menu::getInstance()->isOptionChecked(MenuOption::Shadows);
    renderMeshes(alpha, mode, false, receiveShadows);
    
    glDisable(GL_ALPHA_TEST);
    
    // render translucent meshes afterwards
    
    renderMeshes(alpha, mode, true, receiveShadows);
    
    glDisable(GL_CULL_FACE);
    
    if (mode == SHADOW_RENDER_MODE) {
        glCullFace(GL_BACK);
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
    Application::getInstance()->setupWorldLight();

    return true;
}

Extents Model::getBindExtents() const {
    if (!isActive()) {
        return Extents();
    }
    const Extents& bindExtents = _geometry->getFBXGeometry().bindExtents;
    Extents scaledExtents = { bindExtents.minimum * _scale, bindExtents.maximum * _scale };
    return scaledExtents;
}

Extents Model::getMeshExtents() const {
    if (!isActive()) {
        return Extents();
    }
    const Extents& extents = _geometry->getFBXGeometry().meshExtents;

    // even though our caller asked for "unscaled" we need to include any fst scaling, translation, and rotation, which
    // is captured in the offset matrix
    glm::vec3 minimum = glm::vec3(_geometry->getFBXGeometry().offset * glm::vec4(extents.minimum, 1.0));
    glm::vec3 maximum = glm::vec3(_geometry->getFBXGeometry().offset * glm::vec4(extents.maximum, 1.0));
    Extents scaledExtents = { minimum * _scale, maximum * _scale };
    return scaledExtents;
}

Extents Model::getUnscaledMeshExtents() const {
    if (!isActive()) {
        return Extents();
    }
    
    const Extents& extents = _geometry->getFBXGeometry().meshExtents;

    // even though our caller asked for "unscaled" we need to include any fst scaling, translation, and rotation, which
    // is captured in the offset matrix
    glm::vec3 minimum = glm::vec3(_geometry->getFBXGeometry().offset * glm::vec4(extents.minimum, 1.0));
    glm::vec3 maximum = glm::vec3(_geometry->getFBXGeometry().offset * glm::vec4(extents.maximum, 1.0));
    Extents scaledExtents = { minimum, maximum };
        
    return scaledExtents;
}

bool Model::getJointState(int index, glm::quat& rotation) const {
    if (index == -1 || index >= _jointStates.size()) {
        return false;
    }
    rotation = _jointStates.at(index)._rotation;
    const glm::quat& defaultRotation = _geometry->getFBXGeometry().joints.at(index).rotation;
    return glm::abs(rotation.x - defaultRotation.x) >= EPSILON ||
        glm::abs(rotation.y - defaultRotation.y) >= EPSILON ||
        glm::abs(rotation.z - defaultRotation.z) >= EPSILON ||
        glm::abs(rotation.w - defaultRotation.w) >= EPSILON;
}

void Model::setJointState(int index, bool valid, const glm::quat& rotation, float priority) {
    if (index != -1 && index < _jointStates.size()) {
        JointState& state = _jointStates[index];
        if (priority >= state._animationPriority) {
            if (valid) {
                state._rotation = rotation;
                state._animationPriority = priority;
            } else if (priority == state._animationPriority) {
                state._rotation = _geometry->getFBXGeometry().joints.at(index).rotation;
                state._animationPriority = 0.0f;
            }
        }
    }
}

int Model::getParentJointIndex(int jointIndex) const {
    return (isActive() && jointIndex != -1) ? _geometry->getFBXGeometry().joints.at(jointIndex).parentIndex : -1;
}

int Model::getLastFreeJointIndex(int jointIndex) const {
    return (isActive() && jointIndex != -1) ? _geometry->getFBXGeometry().joints.at(jointIndex).freeLineage.last() : -1;
}

void Model::setURL(const QUrl& url, const QUrl& fallback, bool retainCurrent, bool delayLoad) {
    // don't recreate the geometry if it's the same URL
    if (_url == url) {
        return;
    }
    _url = url;

    // if so instructed, keep the current geometry until the new one is loaded 
    _nextBaseGeometry = _nextGeometry = Application::getInstance()->getGeometryCache()->getGeometry(url, fallback, delayLoad);
    _nextLODHysteresis = NetworkGeometry::NO_HYSTERESIS;
    if (!retainCurrent || !isActive() || _nextGeometry->isLoaded()) {
        applyNextGeometry();
    }
}

bool Model::getJointPosition(int jointIndex, glm::vec3& position) const {
    if (jointIndex == -1 || _jointStates.isEmpty()) {
        return false;
    }
    position = _translation + extractTranslation(_jointStates[jointIndex]._transform);
    return true;
}

bool Model::getJointRotation(int jointIndex, glm::quat& rotation, bool fromBind) const {
    if (jointIndex == -1 || _jointStates.isEmpty()) {
        return false;
    }
    rotation = _jointStates[jointIndex]._combinedRotation *
        (fromBind ? _geometry->getFBXGeometry().joints[jointIndex].inverseBindRotation :
            _geometry->getFBXGeometry().joints[jointIndex].inverseDefaultRotation);
    return true;
}

QStringList Model::getJointNames() const {
    if (QThread::currentThread() != thread()) {
        QStringList result;
        QMetaObject::invokeMethod(const_cast<Model*>(this), "getJointNames", Qt::BlockingQueuedConnection,
            Q_RETURN_ARG(QStringList, result));
        return result;
    }
    return isActive() ? _geometry->getFBXGeometry().getJointNames() : QStringList();
}

uint qHash(const WeakAnimationHandlePointer& handle, uint seed) {
    return qHash(handle.data(), seed);
}

AnimationHandlePointer Model::createAnimationHandle() {
    AnimationHandlePointer handle(new AnimationHandle(this));
    handle->_self = handle;
    _animationHandles.insert(handle);
    return handle;
}

void Model::clearShapes() {
    for (int i = 0; i < _jointShapes.size(); ++i) {
        delete _jointShapes[i];
    }
    _jointShapes.clear();
}

void Model::rebuildShapes() {
    clearShapes();
    
    if (!_geometry || _rootIndex == -1) {
        return;
    }
    
    const FBXGeometry& geometry = _geometry->getFBXGeometry();
    if (geometry.joints.isEmpty()) {
        return;
    }

    // We create the shapes with proper dimensions, but we set their transforms later.
    float uniformScale = extractUniformScale(_scale);
    for (int i = 0; i < _jointStates.size(); i++) {
        const FBXJoint& joint = geometry.joints[i];

        float radius = uniformScale * joint.boneRadius;
        float halfHeight = 0.5f * uniformScale * joint.distanceToParent;
        Shape::Type type = joint.shapeType;
        if (type == Shape::CAPSULE_SHAPE && halfHeight < EPSILON) {
            // this capsule is effectively a sphere
            type = Shape::SPHERE_SHAPE;
        }
        if (type == Shape::CAPSULE_SHAPE) {
            CapsuleShape* capsule = new CapsuleShape(radius, halfHeight);
            _jointShapes.push_back(capsule);
        } else if (type == Shape::SPHERE_SHAPE) {
            SphereShape* sphere = new SphereShape(radius, glm::vec3(0.0f));
            _jointShapes.push_back(sphere);
        } else {
            // this shape type is not handled and the joint shouldn't collide, 
            // however we must have a shape for each joint, 
            // so we make a bogus sphere with zero radius.
            // TODO: implement collision groups for more control over what collides with what
            SphereShape* sphere = new SphereShape(0.f, glm::vec3(0.0f)); 
            _jointShapes.push_back(sphere);
        }
    }

    // This method moves the shapes to their default positions in Model frame
    // which is where we compute the bounding shape's parameters.
    computeBoundingShape(geometry);

    // finally sync shapes to joint positions
    _shapesAreDirty = true;
    updateShapePositions();
}

void Model::computeBoundingShape(const FBXGeometry& geometry) {
    // compute default joint transforms and rotations 
    // (in local frame, ignoring Model translation and rotation)
    int numJoints = geometry.joints.size();
    QVector<glm::mat4> transforms;
    transforms.fill(glm::mat4(), numJoints);
    QVector<glm::quat> finalRotations;
    finalRotations.fill(glm::quat(), numJoints);

    QVector<bool> shapeIsSet;
    shapeIsSet.fill(false, numJoints);
    int numShapesSet = 0;
    int lastNumShapesSet = -1;
    while (numShapesSet < numJoints && numShapesSet != lastNumShapesSet) {
        lastNumShapesSet = numShapesSet;
        for (int i = 0; i < numJoints; i++) {
            const FBXJoint& joint = geometry.joints.at(i);
            int parentIndex = joint.parentIndex;
            
            if (parentIndex == -1) {
                glm::mat4 baseTransform = glm::scale(_scale) * glm::translate(_offset);
                glm::quat combinedRotation = joint.preRotation * joint.rotation * joint.postRotation;    
                glm::mat4 rootTransform = baseTransform * geometry.offset * glm::translate(joint.translation) 
                    * joint.preTransform * glm::mat4_cast(combinedRotation) * joint.postTransform;
                // remove the tranlsation part before we save the root transform
                transforms[i] = glm::translate(- extractTranslation(rootTransform)) * rootTransform;

                finalRotations[i] = combinedRotation;
                ++numShapesSet;
                shapeIsSet[i] = true;
            } else if (shapeIsSet[parentIndex]) {
                glm::quat combinedRotation = joint.preRotation * joint.rotation * joint.postRotation;    
                transforms[i] = transforms[parentIndex] * glm::translate(joint.translation) 
                    * joint.preTransform * glm::mat4_cast(combinedRotation) * joint.postTransform;
                finalRotations[i] = finalRotations[parentIndex] * combinedRotation;
                ++numShapesSet;
                shapeIsSet[i] = true;
            }
        }
    }

    // sync shapes to joints
    _boundingRadius = 0.0f;
    float uniformScale = extractUniformScale(_scale);
    for (int i = 0; i < _jointShapes.size(); i++) {
        const FBXJoint& joint = geometry.joints[i];
        glm::vec3 jointToShapeOffset = uniformScale * (finalRotations[i] * joint.shapePosition);
        glm::vec3 localPosition = extractTranslation(transforms[i]) + jointToShapeOffset;
        Shape* shape = _jointShapes[i];
        shape->setPosition(localPosition);
        shape->setRotation(finalRotations[i] * joint.shapeRotation);
        float distance = glm::length(localPosition) + shape->getBoundingRadius();
        if (distance > _boundingRadius) {
            _boundingRadius = distance;
        }
    }

    // compute bounding box
    Extents totalExtents;
    totalExtents.reset();
    for (int i = 0; i < _jointShapes.size(); i++) {
        Extents shapeExtents;
        shapeExtents.reset();

        Shape* shape = _jointShapes[i];
        glm::vec3 localPosition = shape->getPosition();
        int type = shape->getType();
        if (type == Shape::CAPSULE_SHAPE) {
            // add the two furthest surface points of the capsule
            CapsuleShape* capsule = static_cast<CapsuleShape*>(shape);
            glm::vec3 axis;
            capsule->computeNormalizedAxis(axis);
            float radius = capsule->getRadius();
            float halfHeight = capsule->getHalfHeight();
            axis = halfHeight * axis + glm::vec3(radius);

            shapeExtents.addPoint(localPosition + axis);
            shapeExtents.addPoint(localPosition - axis);
            totalExtents.addExtents(shapeExtents);
        } else if (type == Shape::SPHERE_SHAPE) {
            float radius = shape->getBoundingRadius();
            glm::vec3 axis = glm::vec3(radius);
            shapeExtents.addPoint(localPosition + axis);
            shapeExtents.addPoint(localPosition - axis);
            totalExtents.addExtents(shapeExtents);
        }
    }

    // compute bounding shape parameters
    // NOTE: we assume that the longest side of totalExtents is the yAxis...
    glm::vec3 diagonal = totalExtents.maximum - totalExtents.minimum;
    // ... and assume the radius is half the RMS of the X and Z sides:
    float capsuleRadius = 0.5f * sqrtf(0.5f * (diagonal.x * diagonal.x + diagonal.z * diagonal.z));
    _boundingShape.setRadius(capsuleRadius);
    _boundingShape.setHalfHeight(0.5f * diagonal.y - capsuleRadius);
    _boundingShapeLocalOffset = 0.5f * (totalExtents.maximum + totalExtents.minimum);
}

void Model::resetShapePositions() {
    // DEBUG method.
    // Moves shapes to the joint default locations for debug visibility into
    // how the bounding shape is computed.

    if (!_geometry || _rootIndex == -1) {
        // geometry or joints have not yet been created
        return;
    }
    
    const FBXGeometry& geometry = _geometry->getFBXGeometry();
    if (geometry.joints.isEmpty() || _jointShapes.size() != geometry.joints.size()) {
        return;
    }

    // The shapes are moved to their default positions in computeBoundingShape().
    computeBoundingShape(geometry);

    // Then we move them into world frame for rendering at the Model's location.
    for (int i = 0; i < _jointShapes.size(); i++) {
        Shape* shape = _jointShapes[i];
        shape->setPosition(_translation + _rotation * shape->getPosition());
        shape->setRotation(_rotation * shape->getRotation());
    }
    _boundingShape.setPosition(_translation + _rotation * _boundingShapeLocalOffset);
    _boundingShape.setRotation(_rotation);
}

void Model::updateShapePositions() {
    if (_shapesAreDirty && _jointShapes.size() == _jointStates.size()) {
        glm::vec3 rootPosition(0.f);
        _boundingRadius = 0.f;
        float uniformScale = extractUniformScale(_scale);
        const FBXGeometry& geometry = _geometry->getFBXGeometry();
        for (int i = 0; i < _jointStates.size(); i++) {
            const FBXJoint& joint = geometry.joints[i];
            // shape position and rotation need to be in world-frame
            glm::vec3 jointToShapeOffset = uniformScale * (_jointStates[i]._combinedRotation * joint.shapePosition);
            glm::vec3 worldPosition = extractTranslation(_jointStates[i]._transform) + jointToShapeOffset + _translation;
            Shape* shape = _jointShapes[i];
            shape->setPosition(worldPosition);
            shape->setRotation(_jointStates[i]._combinedRotation * joint.shapeRotation);
            float distance = glm::distance(worldPosition, _translation) + shape->getBoundingRadius();
            if (distance > _boundingRadius) {
                _boundingRadius = distance;
            }
            if (joint.parentIndex == -1) {
                rootPosition = worldPosition;
            }
        }
        _shapesAreDirty = false;
        _boundingShape.setPosition(rootPosition + _rotation * _boundingShapeLocalOffset);
        _boundingShape.setRotation(_rotation);
    }
}

bool Model::findRayIntersection(const glm::vec3& origin, const glm::vec3& direction, float& distance) const {
    const glm::vec3 relativeOrigin = origin - _translation;
    const FBXGeometry& geometry = _geometry->getFBXGeometry();
    float minDistance = FLT_MAX;
    float radiusScale = extractUniformScale(_scale);
    for (int i = 0; i < _jointStates.size(); i++) {
        const FBXJoint& joint = geometry.joints[i];
        glm::vec3 end = extractTranslation(_jointStates[i]._transform);
        float endRadius = joint.boneRadius * radiusScale;
        glm::vec3 start = end;
        float startRadius = joint.boneRadius * radiusScale;
        if (joint.parentIndex != -1) {
            start = extractTranslation(_jointStates[joint.parentIndex]._transform);
            startRadius = geometry.joints[joint.parentIndex].boneRadius * radiusScale;
        }
        // for now, use average of start and end radii
        float capsuleDistance;
        if (findRayCapsuleIntersection(relativeOrigin, direction, start, end,
                (startRadius + endRadius) / 2.0f, capsuleDistance)) {
            minDistance = qMin(minDistance, capsuleDistance);
        }
    }
    if (minDistance < FLT_MAX) {
        distance = minDistance;
        return true;
    }
    return false;
}

bool Model::findCollisions(const QVector<const Shape*> shapes, CollisionList& collisions) {
    bool collided = false;
    for (int i = 0; i < shapes.size(); ++i) {
        const Shape* theirShape = shapes[i];
        for (int j = 0; j < _jointShapes.size(); ++j) {
            const Shape* ourShape = _jointShapes[j];
            if (ShapeCollider::collideShapes(theirShape, ourShape, collisions)) {
                collided = true;
            }
        }
    }
    return collided;
}

bool Model::findSphereCollisions(const glm::vec3& sphereCenter, float sphereRadius,
    CollisionList& collisions, int skipIndex) {
    bool collided = false;
    SphereShape sphere(sphereRadius, sphereCenter);
    const FBXGeometry& geometry = _geometry->getFBXGeometry();
    for (int i = 0; i < _jointShapes.size(); i++) {
        const FBXJoint& joint = geometry.joints[i];
        if (joint.parentIndex != -1) {
            if (skipIndex != -1) {
                int ancestorIndex = joint.parentIndex;
                do {
                    if (ancestorIndex == skipIndex) {
                        goto outerContinue;
                    }
                    ancestorIndex = geometry.joints[ancestorIndex].parentIndex;
                    
                } while (ancestorIndex != -1);
            }
        }
        if (ShapeCollider::collideShapes(&sphere, _jointShapes[i], collisions)) {
            CollisionInfo* collision = collisions.getLastCollision();
            collision->_type = COLLISION_TYPE_MODEL;
            collision->_data = (void*)(this);
            collision->_intData = i;
            collided = true;
        }
        outerContinue: ;
    }
    return collided;
}

bool Model::findPlaneCollisions(const glm::vec4& plane, CollisionList& collisions) {
    bool collided = false;
    PlaneShape planeShape(plane);
    for (int i = 0; i < _jointShapes.size(); i++) {
        if (ShapeCollider::collideShapes(&planeShape, _jointShapes[i], collisions)) {
            CollisionInfo* collision = collisions.getLastCollision();
            collision->_type = COLLISION_TYPE_MODEL;
            collision->_data = (void*)(this);
            collision->_intData = i;
            collided = true;
        }
    }
    return collided;
}

class Blender : public QRunnable {
public:

    Blender(Model* model, const QWeakPointer<NetworkGeometry>& geometry,
        const QVector<FBXMesh>& meshes, const QVector<float>& blendshapeCoefficients);
    
    virtual void run();

private:
    
    QPointer<Model> _model;
    QWeakPointer<NetworkGeometry> _geometry;
    QVector<FBXMesh> _meshes;
    QVector<float> _blendshapeCoefficients;
};

Blender::Blender(Model* model, const QWeakPointer<NetworkGeometry>& geometry,
        const QVector<FBXMesh>& meshes, const QVector<float>& blendshapeCoefficients) :
    _model(model),
    _geometry(geometry),
    _meshes(meshes),
    _blendshapeCoefficients(blendshapeCoefficients) {
}

void Blender::run() {
    // make sure the model/geometry still exists
    if (_model.isNull() || _geometry.isNull()) {
        return;
    }
    QVector<glm::vec3> vertices, normals;
    int offset = 0;
    foreach (const FBXMesh& mesh, _meshes) {
        if (mesh.blendshapes.isEmpty()) {
            continue;
        }
        vertices += mesh.vertices;
        normals += mesh.normals;
        glm::vec3* meshVertices = vertices.data() + offset;
        glm::vec3* meshNormals = normals.data() + offset;
        offset += mesh.vertices.size();
        const float NORMAL_COEFFICIENT_SCALE = 0.01f;
        for (int i = 0, n = qMin(_blendshapeCoefficients.size(), mesh.blendshapes.size()); i < n; i++) {
            float vertexCoefficient = _blendshapeCoefficients.at(i);
            if (vertexCoefficient < EPSILON) {
                continue;
            }
            float normalCoefficient = vertexCoefficient * NORMAL_COEFFICIENT_SCALE;
            const FBXBlendshape& blendshape = mesh.blendshapes.at(i);
            for (int j = 0; j < blendshape.indices.size(); j++) {
                int index = blendshape.indices.at(j);
                meshVertices[index] += blendshape.vertices.at(j) * vertexCoefficient;
                meshNormals[index] += blendshape.normals.at(j) * normalCoefficient;
            }
        }
    }
    
    // post the result to the geometry cache, which will dispatch to the model if still alive
    QMetaObject::invokeMethod(Application::getInstance()->getGeometryCache(), "setBlendedVertices",
        Q_ARG(const QPointer<Model>&, _model), Q_ARG(const QWeakPointer<NetworkGeometry>&, _geometry),
        Q_ARG(const QVector<glm::vec3>&, vertices), Q_ARG(const QVector<glm::vec3>&, normals));
}

void Model::setScaleToFit(bool scaleToFit, float largestDimension) {
    if (_scaleToFit != scaleToFit || _scaleToFitLargestDimension != largestDimension) {
        _scaleToFit = scaleToFit;
        _scaleToFitLargestDimension = largestDimension;
        _scaledToFit = false; // force rescaling
    }
}

void Model::scaleToFit() {
    Extents modelMeshExtents = getUnscaledMeshExtents();

    // size is our "target size in world space"
    // we need to set our model scale so that the extents of the mesh, fit in a cube that size...
    float maxDimension = glm::distance(modelMeshExtents.maximum, modelMeshExtents.minimum);
    float maxScale = _scaleToFitLargestDimension / maxDimension;
    glm::vec3 scale(maxScale, maxScale, maxScale);
    setScaleInternal(scale);
    _scaledToFit = true;
}

void Model::setSnapModelToCenter(bool snapModelToCenter) {
    if (_snapModelToCenter != snapModelToCenter) {
        _snapModelToCenter = snapModelToCenter;
        _snappedToCenter = false; // force re-centering
    }
}

void Model::snapToCenter() {
    Extents modelMeshExtents = getUnscaledMeshExtents();
    glm::vec3 halfDimensions = (modelMeshExtents.maximum - modelMeshExtents.minimum) * 0.5f;
    glm::vec3 offset = -modelMeshExtents.minimum - halfDimensions;
    _offset = offset;
    _snappedToCenter = true;
}

void Model::simulate(float deltaTime, bool fullUpdate) {
    fullUpdate = updateGeometry() || fullUpdate || (_scaleToFit && !_scaledToFit) || (_snapModelToCenter && !_snappedToCenter);
    if (isActive() && fullUpdate) {
        // check for scale to fit
        if (_scaleToFit && !_scaledToFit) {
            scaleToFit();
        }
        if (_snapModelToCenter && !_snappedToCenter) {
            snapToCenter();
        }
        simulateInternal(deltaTime);
    }
}

void Model::simulateInternal(float deltaTime) {
    // update animations
    foreach (const AnimationHandlePointer& handle, _runningAnimations) {
        handle->simulate(deltaTime);
    }

    // NOTE: this is a recursive call that walks all attachments, and their attachments
    // update the world space transforms for all joints
    for (int i = 0; i < _jointStates.size(); i++) {
        updateJointState(i);
    }
    _shapesAreDirty = true;
    
    // update the attachment transforms and simulate them
    const FBXGeometry& geometry = _geometry->getFBXGeometry();
    for (int i = 0; i < _attachments.size(); i++) {
        const FBXAttachment& attachment = geometry.attachments.at(i);
        Model* model = _attachments.at(i);
        
        glm::vec3 jointTranslation = _translation;
        glm::quat jointRotation = _rotation;
        getJointPosition(attachment.jointIndex, jointTranslation);
        getJointRotation(attachment.jointIndex, jointRotation);
        
        model->setTranslation(jointTranslation + jointRotation * attachment.translation * _scale);
        model->setRotation(jointRotation * attachment.rotation);
        model->setScale(_scale * attachment.scale);
        
        if (model->isActive()) {
            model->simulateInternal(deltaTime);
        }
    }
    
    for (int i = 0; i < _meshStates.size(); i++) {
        MeshState& state = _meshStates[i];
        const FBXMesh& mesh = geometry.meshes.at(i);
        for (int j = 0; j < mesh.clusters.size(); j++) {
            const FBXCluster& cluster = mesh.clusters.at(j);
            state.clusterMatrices[j] = _jointStates[cluster.jointIndex]._transform * cluster.inverseBindMatrix;
        }
    }
    
    // post the blender
    if (geometry.hasBlendedMeshes()) {
        QThreadPool::globalInstance()->start(new Blender(this, _geometry, geometry.meshes, _blendshapeCoefficients));
    }
}

void Model::updateJointState(int index) {
    JointState& state = _jointStates[index];
    const FBXJoint& joint = state.getFBXJoint();
    
    if (joint.parentIndex == -1) {
        const FBXGeometry& geometry = _geometry->getFBXGeometry();
        glm::mat4 baseTransform = glm::mat4_cast(_rotation) * glm::scale(_scale) * glm::translate(_offset) * geometry.offset;
        state.updateWorldTransform(baseTransform, _rotation);
    } else {
        const JointState& parentState = _jointStates.at(joint.parentIndex);
        state.updateWorldTransform(parentState._transform, parentState._combinedRotation);
    }
}

bool Model::setJointPosition(int jointIndex, const glm::vec3& translation, const glm::quat& rotation, bool useRotation,
       int lastFreeIndex, bool allIntermediatesFree, const glm::vec3& alignment, float priority) {
    if (jointIndex == -1 || _jointStates.isEmpty()) {
        return false;
    }
    glm::vec3 relativePosition = translation - _translation;
    const FBXGeometry& geometry = _geometry->getFBXGeometry();
    const QVector<int>& freeLineage = geometry.joints.at(jointIndex).freeLineage;
    if (freeLineage.isEmpty()) {
        return false;
    }
    if (lastFreeIndex == -1) {
        lastFreeIndex = freeLineage.last();
    }
    
    // this is a cyclic coordinate descent algorithm: see
    // http://www.ryanjuckett.com/programming/animation/21-cyclic-coordinate-descent-in-2d
    const int ITERATION_COUNT = 1;
    glm::vec3 worldAlignment = _rotation * alignment;
    for (int i = 0; i < ITERATION_COUNT; i++) {
        // first, try to rotate the end effector as close as possible to the target rotation, if any
        glm::quat endRotation;
        if (useRotation) {
            getJointRotation(jointIndex, endRotation, true);
            applyRotationDelta(jointIndex, rotation * glm::inverse(endRotation), true, priority);
            getJointRotation(jointIndex, endRotation, true);
        }    
        
        // then, we go from the joint upwards, rotating the end as close as possible to the target
        glm::vec3 endPosition = extractTranslation(_jointStates[jointIndex]._transform);
        for (int j = 1; freeLineage.at(j - 1) != lastFreeIndex; j++) {
            int index = freeLineage.at(j);
            JointState& state = _jointStates[index];
            const FBXJoint& joint = state.getFBXJoint();
            if (!(joint.isFree || allIntermediatesFree)) {
                continue;
            }
            glm::vec3 jointPosition = extractTranslation(state._transform);
            glm::vec3 jointVector = endPosition - jointPosition;
            glm::quat oldCombinedRotation = state._combinedRotation;
            glm::quat combinedDelta;
            float combinedWeight;
            if (useRotation) {
                combinedDelta = safeMix(rotation * glm::inverse(endRotation),
                    rotationBetween(jointVector, relativePosition - jointPosition), 0.5f);
                combinedWeight = 2.0f;
                
            } else {
                combinedDelta = rotationBetween(jointVector, relativePosition - jointPosition);
                combinedWeight = 1.0f;
            }
            if (alignment != glm::vec3() && j > 1) {
                jointVector = endPosition - jointPosition;
                glm::vec3 positionSum;
                for (int k = j - 1; k > 0; k--) {
                    int index = freeLineage.at(k);
                    updateJointState(index);
                    positionSum += extractTranslation(_jointStates.at(index)._transform);
                }
                glm::vec3 projectedCenterOfMass = glm::cross(jointVector,
                    glm::cross(positionSum / (j - 1.0f) - jointPosition, jointVector));
                glm::vec3 projectedAlignment = glm::cross(jointVector, glm::cross(worldAlignment, jointVector));
                const float LENGTH_EPSILON = 0.001f;
                if (glm::length(projectedCenterOfMass) > LENGTH_EPSILON && glm::length(projectedAlignment) > LENGTH_EPSILON) {
                    combinedDelta = safeMix(combinedDelta, rotationBetween(projectedCenterOfMass, projectedAlignment),
                        1.0f / (combinedWeight + 1.0f));
                }
            }
            applyRotationDelta(index, combinedDelta, true, priority);
            glm::quat actualDelta = state._combinedRotation * glm::inverse(oldCombinedRotation);
            endPosition = actualDelta * jointVector + jointPosition;
            if (useRotation) {
                endRotation = actualDelta * endRotation;
            }
        }       
    }
     
    // now update the joint states from the top
    for (int j = freeLineage.size() - 1; j >= 0; j--) {
        updateJointState(freeLineage.at(j));
    }
    _shapesAreDirty = true;
        
    return true;
}

bool Model::setJointRotation(int jointIndex, const glm::quat& rotation, float priority) {
    if (jointIndex == -1 || _jointStates.isEmpty()) {
        return false;
    }
    JointState& state = _jointStates[jointIndex];
    if (priority >= state._animationPriority) {
        state._rotation = state._rotation * glm::inverse(state._combinedRotation) * rotation *
            glm::inverse(_geometry->getFBXGeometry().joints.at(jointIndex).inverseBindRotation);
        state._animationPriority = priority;
    }
    return true;
}

void Model::setJointTranslation(int jointIndex, const glm::vec3& translation) {
    JointState& state = _jointStates[jointIndex];
    const FBXJoint& joint = state.getFBXJoint();
    
    glm::mat4 parentTransform;
    if (joint.parentIndex == -1) {
        const FBXGeometry& geometry = _geometry->getFBXGeometry();
        parentTransform = glm::mat4_cast(_rotation) * glm::scale(_scale) * glm::translate(_offset) * geometry.offset;
        
    } else {
        parentTransform = _jointStates.at(joint.parentIndex)._transform;
    }
    glm::vec3 preTranslation = extractTranslation(joint.preTransform * glm::mat4_cast(joint.preRotation *
        state._rotation * joint.postRotation) * joint.postTransform); 
    state._translation = glm::vec3(glm::inverse(parentTransform) * glm::vec4(translation, 1.0f)) - preTranslation;
}

bool Model::restoreJointPosition(int jointIndex, float percent, float priority) {
    if (jointIndex == -1 || _jointStates.isEmpty()) {
        return false;
    }
    const FBXGeometry& geometry = _geometry->getFBXGeometry();
    const QVector<int>& freeLineage = geometry.joints.at(jointIndex).freeLineage;
    
    foreach (int index, freeLineage) {
        JointState& state = _jointStates[index];
        if (priority == state._animationPriority) {
            const FBXJoint& joint = geometry.joints.at(index);
            state._rotation = safeMix(state._rotation, joint.rotation, percent);
            state._translation = glm::mix(state._translation, joint.translation, percent);
            state._animationPriority = 0.0f;
        }
    }
    return true;
}

float Model::getLimbLength(int jointIndex) const {
    if (jointIndex == -1 || _jointStates.isEmpty()) {
        return 0.0f;
    }
    const FBXGeometry& geometry = _geometry->getFBXGeometry();
    const QVector<int>& freeLineage = geometry.joints.at(jointIndex).freeLineage;
    float length = 0.0f;
    float lengthScale = (_scale.x + _scale.y + _scale.z) / 3.0f;
    for (int i = freeLineage.size() - 2; i >= 0; i--) {
        length += geometry.joints.at(freeLineage.at(i)).distanceToParent * lengthScale;
    }
    return length;
}

void Model::applyRotationDelta(int jointIndex, const glm::quat& delta, bool constrain, float priority) {
    JointState& state = _jointStates[jointIndex];
    if (priority < state._animationPriority) {
        return;
    }
    state._animationPriority = priority;
    const FBXJoint& joint = state.getFBXJoint();
    if (!constrain || (joint.rotationMin == glm::vec3(-PI, -PI, -PI) &&
            joint.rotationMax == glm::vec3(PI, PI, PI))) {
        // no constraints
        state._rotation = state._rotation * glm::inverse(state._combinedRotation) * delta * state._combinedRotation;
        state._combinedRotation = delta * state._combinedRotation;
        return;
    }
    glm::quat targetRotation = delta * state._combinedRotation;
    glm::vec3 eulers = safeEulerAngles(state._rotation * glm::inverse(state._combinedRotation) * targetRotation);
    glm::quat newRotation = glm::quat(glm::clamp(eulers, joint.rotationMin, joint.rotationMax));
    state._combinedRotation = state._combinedRotation * glm::inverse(state._rotation) * newRotation;
    state._rotation = newRotation;
}

const int BALL_SUBDIVISIONS = 10;

void Model::renderJointCollisionShapes(float alpha) {
    glPushMatrix();
    Application::getInstance()->loadTranslatedViewMatrix(_translation);
    for (int i = 0; i < _jointShapes.size(); i++) {
        glPushMatrix();

        Shape* shape = _jointShapes[i];
        
        if (shape->getType() == Shape::SPHERE_SHAPE) {
            // shapes are stored in world-frame, so we have to transform into model frame
            glm::vec3 position = shape->getPosition() - _translation;
            glTranslatef(position.x, position.y, position.z);
            const glm::quat& rotation = shape->getRotation();
            glm::vec3 axis = glm::axis(rotation);
            glRotatef(glm::degrees(glm::angle(rotation)), axis.x, axis.y, axis.z);

            // draw a grey sphere at shape position
            glColor4f(0.75f, 0.75f, 0.75f, alpha);
            glutSolidSphere(shape->getBoundingRadius(), BALL_SUBDIVISIONS, BALL_SUBDIVISIONS);
        } else if (shape->getType() == Shape::CAPSULE_SHAPE) {
            CapsuleShape* capsule = static_cast<CapsuleShape*>(shape);

            // draw a blue sphere at the capsule endpoint
            glm::vec3 endPoint;
            capsule->getEndPoint(endPoint);
            endPoint = endPoint - _translation;
            glTranslatef(endPoint.x, endPoint.y, endPoint.z);
            glColor4f(0.6f, 0.6f, 0.8f, alpha);
            glutSolidSphere(capsule->getRadius(), BALL_SUBDIVISIONS, BALL_SUBDIVISIONS);

            // draw a yellow sphere at the capsule startpoint
            glm::vec3 startPoint;
            capsule->getStartPoint(startPoint);
            startPoint = startPoint - _translation;
            glm::vec3 axis = endPoint - startPoint;
            glTranslatef(-axis.x, -axis.y, -axis.z);
            glColor4f(0.8f, 0.8f, 0.6f, alpha);
            glutSolidSphere(capsule->getRadius(), BALL_SUBDIVISIONS, BALL_SUBDIVISIONS);
            
            // draw a green cylinder between the two points
            glm::vec3 origin(0.f);
            glColor4f(0.6f, 0.8f, 0.6f, alpha);
            Avatar::renderJointConnectingCone( origin, axis, capsule->getRadius(), capsule->getRadius());
        }
        glPopMatrix();
    }
    glPopMatrix();
}

void Model::renderBoundingCollisionShapes(float alpha) {
    glPushMatrix();

    Application::getInstance()->loadTranslatedViewMatrix(_translation);

    // draw a blue sphere at the capsule endpoint
    glm::vec3 endPoint;
    _boundingShape.getEndPoint(endPoint);
    endPoint = endPoint - _translation;
    glTranslatef(endPoint.x, endPoint.y, endPoint.z);
    glColor4f(0.6f, 0.6f, 0.8f, alpha);
    glutSolidSphere(_boundingShape.getRadius(), BALL_SUBDIVISIONS, BALL_SUBDIVISIONS);

    // draw a yellow sphere at the capsule startpoint
    glm::vec3 startPoint;
    _boundingShape.getStartPoint(startPoint);
    startPoint = startPoint - _translation;
    glm::vec3 axis = endPoint - startPoint;
    glTranslatef(-axis.x, -axis.y, -axis.z);
    glColor4f(0.8f, 0.8f, 0.6f, alpha);
    glutSolidSphere(_boundingShape.getRadius(), BALL_SUBDIVISIONS, BALL_SUBDIVISIONS);

    // draw a green cylinder between the two points
    glm::vec3 origin(0.f);
    glColor4f(0.6f, 0.8f, 0.6f, alpha);
    Avatar::renderJointConnectingCone( origin, axis, _boundingShape.getRadius(), _boundingShape.getRadius());

    glPopMatrix();
}

bool Model::collisionHitsMoveableJoint(CollisionInfo& collision) const {
    if (collision._type == COLLISION_TYPE_MODEL) {
        // the joint is pokable by a collision if it exists and is free to move
        const FBXJoint& joint = _geometry->getFBXGeometry().joints[collision._intData];
        if (joint.parentIndex == -1 || _jointStates.isEmpty()) {
            return false;
        }
        // an empty freeLineage means the joint can't move
        const FBXGeometry& geometry = _geometry->getFBXGeometry();
        int jointIndex = collision._intData;
        const QVector<int>& freeLineage = geometry.joints.at(jointIndex).freeLineage;
        return !freeLineage.isEmpty();
    }
    return false;
}

void Model::applyCollision(CollisionInfo& collision) {
    if (collision._type != COLLISION_TYPE_MODEL) {
        return;
    }

    glm::vec3 jointPosition(0.f);
    int jointIndex = collision._intData;
    if (getJointPosition(jointIndex, jointPosition)) {
        const FBXJoint& joint = _geometry->getFBXGeometry().joints[jointIndex];
        if (joint.parentIndex != -1) {
            // compute the approximate distance (travel) that the joint needs to move
            glm::vec3 start;
            getJointPosition(joint.parentIndex, start);
            glm::vec3 contactPoint = collision._contactPoint - start;
            glm::vec3 penetrationEnd = contactPoint + collision._penetration;
            glm::vec3 axis = glm::cross(contactPoint, penetrationEnd);
            float travel = glm::length(axis);
            const float MIN_TRAVEL = 1.0e-8f;
            if (travel > MIN_TRAVEL) {
                // compute the new position of the joint
                float angle = asinf(travel / (glm::length(contactPoint) * glm::length(penetrationEnd)));
                axis = glm::normalize(axis);
                glm::vec3 end;
                getJointPosition(jointIndex, end);
                glm::vec3 newEnd = start + glm::angleAxis(angle, axis) * (end - start);
                // try to move it
                setJointPosition(jointIndex, newEnd, glm::quat(), false, -1, true);
            }
        }
    }
}

void Model::setBlendedVertices(const QVector<glm::vec3>& vertices, const QVector<glm::vec3>& normals) {
    if (_blendedVertexBuffers.isEmpty()) {
        return;
    }
    const FBXGeometry& geometry = _geometry->getFBXGeometry();
    int index = 0;
    for (int i = 0; i < geometry.meshes.size(); i++) {
        const FBXMesh& mesh = geometry.meshes.at(i);
        if (mesh.blendshapes.isEmpty()) {
            continue;
        }
        QOpenGLBuffer& buffer = _blendedVertexBuffers[i];
        buffer.bind();
        buffer.write(0, vertices.constData() + index, mesh.vertices.size() * sizeof(glm::vec3));
        buffer.write(mesh.vertices.size() * sizeof(glm::vec3), normals.constData() + index,
            mesh.normals.size() * sizeof(glm::vec3));
        buffer.release();
        index += mesh.vertices.size();
    }
}

void Model::applyNextGeometry() {
    // delete our local geometry and custom textures
    deleteGeometry();
    _dilatedTextures.clear();
    _lodHysteresis = _nextLODHysteresis;
    
    // we retain a reference to the base geometry so that its reference count doesn't fall to zero
    _baseGeometry = _nextBaseGeometry;
    _geometry = _nextGeometry;
    _nextBaseGeometry.reset();
    _nextGeometry.reset();
}

void Model::deleteGeometry() {
    foreach (Model* attachment, _attachments) {
        delete attachment;
    }
    _attachments.clear();
    _blendedVertexBuffers.clear();
    _jointStates.clear();
    _meshStates.clear();
    clearShapes();
    
    for (QSet<WeakAnimationHandlePointer>::iterator it = _animationHandles.begin(); it != _animationHandles.end(); ) {
        AnimationHandlePointer handle = it->toStrongRef();
        if (handle) {
            handle->_jointMappings.clear();
            it++;
        } else {
            it = _animationHandles.erase(it);
        }
    }
    
    if (_geometry) {
        _geometry->clearLoadPriority(this);
    }
}

void Model::renderMeshes(float alpha, RenderMode mode, bool translucent, bool receiveShadows) {
    const FBXGeometry& geometry = _geometry->getFBXGeometry();
    const QVector<NetworkMesh>& networkMeshes = _geometry->getMeshes();
    
    if (receiveShadows) {
        glTexGenfv(GL_S, GL_EYE_PLANE, (const GLfloat*)&Application::getInstance()->getShadowMatrix()[0]);
        glTexGenfv(GL_T, GL_EYE_PLANE, (const GLfloat*)&Application::getInstance()->getShadowMatrix()[1]);
        glTexGenfv(GL_R, GL_EYE_PLANE, (const GLfloat*)&Application::getInstance()->getShadowMatrix()[2]);
    }
    for (int i = 0; i < networkMeshes.size(); i++) {
        // exit early if the translucency doesn't match what we're drawing
        const NetworkMesh& networkMesh = networkMeshes.at(i);
        if (translucent ? (networkMesh.getTranslucentPartCount() == 0) :
                (networkMesh.getTranslucentPartCount() == networkMesh.parts.size())) {
            continue;
        }
        const_cast<QOpenGLBuffer&>(networkMesh.indexBuffer).bind();

        const FBXMesh& mesh = geometry.meshes.at(i);    
        int vertexCount = mesh.vertices.size();
        if (vertexCount == 0) {
            // sanity check
            continue;
        }
        
        const_cast<QOpenGLBuffer&>(networkMesh.vertexBuffer).bind();
        
        ProgramObject* program = &_program;
        ProgramObject* skinProgram = &_skinProgram;
        SkinLocations* skinLocations = &_skinLocations;
        GLenum specularTextureUnit = 0;
        GLenum shadowTextureUnit = 0;
        if (mode == SHADOW_RENDER_MODE) {
            program = &_shadowProgram;
            skinProgram = &_skinShadowProgram;
            skinLocations = &_skinShadowLocations;
            
        } else if (!mesh.tangents.isEmpty()) {
            if (mesh.hasSpecularTexture()) {
                if (receiveShadows) {
                    program = &_shadowNormalSpecularMapProgram;
                    skinProgram = &_skinShadowNormalSpecularMapProgram;
                    skinLocations = &_skinShadowNormalSpecularMapLocations;
                    shadowTextureUnit = GL_TEXTURE3;
                } else {
                    program = &_normalSpecularMapProgram;
                    skinProgram = &_skinNormalSpecularMapProgram;
                    skinLocations = &_skinNormalSpecularMapLocations;
                }
                specularTextureUnit = GL_TEXTURE2;
                
            } else if (receiveShadows) {
                program = &_shadowNormalMapProgram;
                skinProgram = &_skinShadowNormalMapProgram;
                skinLocations = &_skinShadowNormalMapLocations;
                shadowTextureUnit = GL_TEXTURE2;
            } else {
                program = &_normalMapProgram;
                skinProgram = &_skinNormalMapProgram;
                skinLocations = &_skinNormalMapLocations;
            }
        } else if (mesh.hasSpecularTexture()) {
            if (receiveShadows) {
                program = &_shadowSpecularMapProgram;
                skinProgram = &_skinShadowSpecularMapProgram;
                skinLocations = &_skinShadowSpecularMapLocations;
                shadowTextureUnit = GL_TEXTURE2;
            } else {
                program = &_specularMapProgram;
                skinProgram = &_skinSpecularMapProgram;
                skinLocations = &_skinSpecularMapLocations;
            }
            specularTextureUnit = GL_TEXTURE1;
            
        } else if (receiveShadows) {
            program = &_shadowMapProgram;
            skinProgram = &_skinShadowMapProgram;
            skinLocations = &_skinShadowMapLocations;
            shadowTextureUnit = GL_TEXTURE1;
        }
        
        const MeshState& state = _meshStates.at(i);
        ProgramObject* activeProgram = program;
        int tangentLocation = _normalMapTangentLocation;
        glPushMatrix();
        Application::getInstance()->loadTranslatedViewMatrix(_translation);
        
        if (state.clusterMatrices.size() > 1) {
            skinProgram->bind();
            glUniformMatrix4fvARB(skinLocations->clusterMatrices, state.clusterMatrices.size(), false,
                (const float*)state.clusterMatrices.constData());
            int offset = (mesh.tangents.size() + mesh.colors.size()) * sizeof(glm::vec3) +
                mesh.texCoords.size() * sizeof(glm::vec2) +
                (mesh.blendshapes.isEmpty() ? vertexCount * 2 * sizeof(glm::vec3) : 0);
            skinProgram->setAttributeBuffer(skinLocations->clusterIndices, GL_FLOAT, offset, 4);
            skinProgram->setAttributeBuffer(skinLocations->clusterWeights, GL_FLOAT,
                offset + vertexCount * sizeof(glm::vec4), 4);
            skinProgram->enableAttributeArray(skinLocations->clusterIndices);
            skinProgram->enableAttributeArray(skinLocations->clusterWeights);
            activeProgram = skinProgram;
            tangentLocation = skinLocations->tangent;
     
        } else {    
            glMultMatrixf((const GLfloat*)&state.clusterMatrices[0]);
            program->bind();
        }

        if (mesh.blendshapes.isEmpty()) {
            if (!(mesh.tangents.isEmpty() || mode == SHADOW_RENDER_MODE)) {
                activeProgram->setAttributeBuffer(tangentLocation, GL_FLOAT, vertexCount * 2 * sizeof(glm::vec3), 3);
                activeProgram->enableAttributeArray(tangentLocation);
            }
            glColorPointer(3, GL_FLOAT, 0, (void*)(vertexCount * 2 * sizeof(glm::vec3) +
                mesh.tangents.size() * sizeof(glm::vec3)));
            glTexCoordPointer(2, GL_FLOAT, 0, (void*)(vertexCount * 2 * sizeof(glm::vec3) +
                (mesh.tangents.size() + mesh.colors.size()) * sizeof(glm::vec3)));    
        
        } else {
            if (!(mesh.tangents.isEmpty() || mode == SHADOW_RENDER_MODE)) {
                activeProgram->setAttributeBuffer(tangentLocation, GL_FLOAT, 0, 3);
                activeProgram->enableAttributeArray(tangentLocation);
            }
            glColorPointer(3, GL_FLOAT, 0, (void*)(mesh.tangents.size() * sizeof(glm::vec3)));
            glTexCoordPointer(2, GL_FLOAT, 0, (void*)((mesh.tangents.size() + mesh.colors.size()) * sizeof(glm::vec3)));
            _blendedVertexBuffers[i].bind();
        }
        glVertexPointer(3, GL_FLOAT, 0, 0);
        glNormalPointer(GL_FLOAT, 0, (void*)(vertexCount * sizeof(glm::vec3)));
        
        if (!mesh.colors.isEmpty()) {
            glEnableClientState(GL_COLOR_ARRAY);
        } else {
            glColor4f(1.0f, 1.0f, 1.0f, alpha);
        }
        if (!mesh.texCoords.isEmpty()) {
            glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        }
        
        qint64 offset = 0;
        for (int j = 0; j < networkMesh.parts.size(); j++) {
            const NetworkMeshPart& networkPart = networkMesh.parts.at(j);
            const FBXMeshPart& part = mesh.parts.at(j);
            if (networkPart.isTranslucent() != translucent) {
                offset += (part.quadIndices.size() + part.triangleIndices.size()) * sizeof(int);
                continue;
            }
            // apply material properties
            if (mode == SHADOW_RENDER_MODE) {
                glBindTexture(GL_TEXTURE_2D, 0);
                
            } else {
                glm::vec4 diffuse = glm::vec4(part.diffuseColor, alpha);
                glm::vec4 specular = glm::vec4(part.specularColor, alpha);
                glMaterialfv(GL_FRONT, GL_AMBIENT, (const float*)&diffuse);
                glMaterialfv(GL_FRONT, GL_DIFFUSE, (const float*)&diffuse);
                glMaterialfv(GL_FRONT, GL_SPECULAR, (const float*)&specular);
                glMaterialf(GL_FRONT, GL_SHININESS, part.shininess);
            
                Texture* diffuseMap = networkPart.diffuseTexture.data();
                if (mesh.isEye && diffuseMap) {
                    diffuseMap = (_dilatedTextures[i][j] =
                        static_cast<DilatableNetworkTexture*>(diffuseMap)->getDilatedTexture(_pupilDilation)).data();
                }
                glBindTexture(GL_TEXTURE_2D, !diffuseMap ?
                    Application::getInstance()->getTextureCache()->getWhiteTextureID() : diffuseMap->getID());
                
                
                if (!mesh.tangents.isEmpty()) {                 
                    glActiveTexture(GL_TEXTURE1);                
                    Texture* normalMap = networkPart.normalTexture.data();
                    glBindTexture(GL_TEXTURE_2D, !normalMap ?
                        Application::getInstance()->getTextureCache()->getBlueTextureID() : normalMap->getID());
                    glActiveTexture(GL_TEXTURE0);
                }
                
                if (specularTextureUnit) {
                    glActiveTexture(specularTextureUnit);
                    Texture* specularMap = networkPart.specularTexture.data();
                    glBindTexture(GL_TEXTURE_2D, !specularMap ?
                        Application::getInstance()->getTextureCache()->getWhiteTextureID() : specularMap->getID());
                    glActiveTexture(GL_TEXTURE0);
                }
                
                if (shadowTextureUnit) {
                    glActiveTexture(shadowTextureUnit);
                    glBindTexture(GL_TEXTURE_2D, Application::getInstance()->getTextureCache()->getShadowDepthTextureID());
                    glActiveTexture(GL_TEXTURE0);
                }
            }
            glDrawRangeElementsEXT(GL_QUADS, 0, vertexCount - 1, part.quadIndices.size(), GL_UNSIGNED_INT, (void*)offset);
            offset += part.quadIndices.size() * sizeof(int);
            glDrawRangeElementsEXT(GL_TRIANGLES, 0, vertexCount - 1, part.triangleIndices.size(),
                GL_UNSIGNED_INT, (void*)offset);
            offset += part.triangleIndices.size() * sizeof(int);
        }
        
        if (!mesh.colors.isEmpty()) {
            glDisableClientState(GL_COLOR_ARRAY);
        }
        if (!mesh.texCoords.isEmpty()) {
            glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        }
        
        if (!(mesh.tangents.isEmpty() || mode == SHADOW_RENDER_MODE)) {
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, 0);
            glActiveTexture(GL_TEXTURE0);
            
            activeProgram->disableAttributeArray(tangentLocation);
        }
        
        if (specularTextureUnit) {
            glActiveTexture(specularTextureUnit);
            glBindTexture(GL_TEXTURE_2D, 0);
            glActiveTexture(GL_TEXTURE0);
        }
        
        if (shadowTextureUnit) {
            glActiveTexture(shadowTextureUnit);
            glBindTexture(GL_TEXTURE_2D, 0);
            glActiveTexture(GL_TEXTURE0);
        }
        
        if (state.clusterMatrices.size() > 1) {
            skinProgram->disableAttributeArray(skinLocations->clusterIndices);
            skinProgram->disableAttributeArray(skinLocations->clusterWeights);  
        } 
        glPopMatrix();

        activeProgram->release();
    }
}

void AnimationHandle::setURL(const QUrl& url) {
    if (_url != url) {
        _animation = Application::getInstance()->getAnimationCache()->getAnimation(_url = url);
        _jointMappings.clear();
    }
}

static void insertSorted(QList<AnimationHandlePointer>& handles, const AnimationHandlePointer& handle) {
    for (QList<AnimationHandlePointer>::iterator it = handles.begin(); it != handles.end(); it++) {
        if (handle->getPriority() > (*it)->getPriority()) {
            handles.insert(it, handle);
            return;
        } 
    }
    handles.append(handle);
}

void AnimationHandle::setPriority(float priority) {
    if (_priority == priority) {
        return;
    }
    if (_running) {
        _model->_runningAnimations.removeOne(_self);
        if (priority < _priority) {
            replaceMatchingPriorities(priority);
        }
        _priority = priority;
        insertSorted(_model->_runningAnimations, _self);
        
    } else {
        _priority = priority;
    }
}

void AnimationHandle::setStartAutomatically(bool startAutomatically) {
    if ((_startAutomatically = startAutomatically) && !_running) {
        start();
    }
}

void AnimationHandle::setMaskedJoints(const QStringList& maskedJoints) {
    _maskedJoints = maskedJoints;
    _jointMappings.clear();
}

void AnimationHandle::setRunning(bool running) {
    if (_running == running) {
        if (running) {
            // move back to the beginning
            _frameIndex = _firstFrame;
        }
        return;
    }
    if ((_running = running)) {
        if (!_model->_runningAnimations.contains(_self)) {
            insertSorted(_model->_runningAnimations, _self);
        }
        _frameIndex = _firstFrame;
          
    } else {
        _model->_runningAnimations.removeOne(_self);
        replaceMatchingPriorities(0.0f);
    }
    emit runningChanged(_running);
}

AnimationHandle::AnimationHandle(Model* model) :
    QObject(model),
    _model(model),
    _fps(30.0f),
    _priority(1.0f),
    _loop(false),
    _hold(false),
    _startAutomatically(false),
    _firstFrame(0),
    _lastFrame(INT_MAX),
    _running(false) {
}

void AnimationHandle::simulate(float deltaTime) {
    _frameIndex += deltaTime * _fps;
    
    // update the joint mappings if necessary/possible
    if (_jointMappings.isEmpty()) {
        if (_model->isActive()) {
            _jointMappings = _model->getGeometry()->getJointMappings(_animation);
        }
        if (_jointMappings.isEmpty()) {
            return;
        }
        if (!_maskedJoints.isEmpty()) {
            const FBXGeometry& geometry = _model->getGeometry()->getFBXGeometry();
            for (int i = 0; i < _jointMappings.size(); i++) {
                int& mapping = _jointMappings[i];
                if (mapping != -1 && _maskedJoints.contains(geometry.joints.at(mapping).name)) {
                    mapping = -1;
                }
            }
        }
    }
    
    const FBXGeometry& animationGeometry = _animation->getGeometry();
    if (animationGeometry.animationFrames.isEmpty()) {
        stop();
        return;
    }
    int lastFrameIndex = qMin(_lastFrame, animationGeometry.animationFrames.size() - 1);
    int firstFrameIndex = qMin(_firstFrame, lastFrameIndex);
    if ((!_loop && _frameIndex >= lastFrameIndex) || firstFrameIndex == lastFrameIndex) {
        // passed the end; apply the last frame
        const FBXAnimationFrame& frame = animationGeometry.animationFrames.at(lastFrameIndex);
        for (int i = 0; i < _jointMappings.size(); i++) {
            int mapping = _jointMappings.at(i);
            if (mapping != -1) {
                JointState& state = _model->_jointStates[mapping];
                if (_priority >= state._animationPriority) {
                    state._rotation = frame.rotations.at(i);
                    state._animationPriority = _priority;
                }
            }
        }
        if (!_hold) {
            stop();
        }
        return;
    }
    int frameCount = lastFrameIndex - firstFrameIndex + 1;
    _frameIndex = firstFrameIndex + glm::mod(qMax(_frameIndex - firstFrameIndex, 0.0f), (float)frameCount);
    
    // blend between the closest two frames
    const FBXAnimationFrame& ceilFrame = animationGeometry.animationFrames.at(
        firstFrameIndex + ((int)glm::ceil(_frameIndex) - firstFrameIndex) % frameCount);
    const FBXAnimationFrame& floorFrame = animationGeometry.animationFrames.at(
        firstFrameIndex + ((int)glm::floor(_frameIndex) - firstFrameIndex) % frameCount);
    float frameFraction = glm::fract(_frameIndex);
    for (int i = 0; i < _jointMappings.size(); i++) {
        int mapping = _jointMappings.at(i);
        if (mapping != -1) {
            JointState& state = _model->_jointStates[mapping];
            if (_priority >= state._animationPriority) {
                state._rotation = safeMix(floorFrame.rotations.at(i), ceilFrame.rotations.at(i), frameFraction);
                state._animationPriority = _priority;
            }
        }
    }
}

void AnimationHandle::replaceMatchingPriorities(float newPriority) {
    for (int i = 0; i < _jointMappings.size(); i++) {
        int mapping = _jointMappings.at(i);
        if (mapping != -1) {
            JointState& state = _model->_jointStates[mapping];
            if (_priority == state._animationPriority) {
                state._animationPriority = newPriority;
            }
        }
    }
}

// ----------------------------------------------------------------------------
// JointState  TODO: move this class to its own files
// ----------------------------------------------------------------------------
JointState::JointState() :
    _translation(0.0f),
    _animationPriority(0.0f),
    _fbxJoint(NULL) {
}

void JointState::setFBXJoint(const FBXJoint& joint) { 
    assert(&joint != NULL);
    _translation = joint.translation;
    _rotation = joint.rotation;
    _fbxJoint = &joint;
}

void JointState::updateWorldTransform(const glm::mat4& baseTransform, const glm::quat& parentRotation) {
    glm::quat combinedRotation = _fbxJoint->preRotation * _rotation * _fbxJoint->postRotation;    
    _transform = baseTransform * glm::translate(_translation) * _fbxJoint->preTransform * glm::mat4_cast(combinedRotation) * _fbxJoint->postTransform;
    _combinedRotation = parentRotation * combinedRotation;
}
