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

#include <CapsuleShape.h>
#include <GeometryUtil.h>
#include <PhysicsEntity.h>
#include <ShapeCollider.h>
#include <SphereShape.h>

#include "Application.h"
#include "Model.h"

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
    _showTrueJointTransforms(true),
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

ProgramObject Model::_cascadedShadowMapProgram;
ProgramObject Model::_cascadedShadowNormalMapProgram;
ProgramObject Model::_cascadedShadowSpecularMapProgram;
ProgramObject Model::_cascadedShadowNormalSpecularMapProgram;

ProgramObject Model::_shadowProgram;

ProgramObject Model::_skinProgram;
ProgramObject Model::_skinNormalMapProgram;
ProgramObject Model::_skinSpecularMapProgram;
ProgramObject Model::_skinNormalSpecularMapProgram;

ProgramObject Model::_skinShadowMapProgram;
ProgramObject Model::_skinShadowNormalMapProgram;
ProgramObject Model::_skinShadowSpecularMapProgram;
ProgramObject Model::_skinShadowNormalSpecularMapProgram;

ProgramObject Model::_skinCascadedShadowMapProgram;
ProgramObject Model::_skinCascadedShadowNormalMapProgram;
ProgramObject Model::_skinCascadedShadowSpecularMapProgram;
ProgramObject Model::_skinCascadedShadowNormalSpecularMapProgram;

ProgramObject Model::_skinShadowProgram;

int Model::_normalMapTangentLocation;
int Model::_normalSpecularMapTangentLocation;
int Model::_shadowNormalMapTangentLocation;
int Model::_shadowNormalSpecularMapTangentLocation;
int Model::_cascadedShadowNormalMapTangentLocation;
int Model::_cascadedShadowNormalSpecularMapTangentLocation;

int Model::_cascadedShadowMapDistancesLocation;
int Model::_cascadedShadowNormalMapDistancesLocation;
int Model::_cascadedShadowSpecularMapDistancesLocation;
int Model::_cascadedShadowNormalSpecularMapDistancesLocation;
    
Model::SkinLocations Model::_skinLocations;
Model::SkinLocations Model::_skinNormalMapLocations;
Model::SkinLocations Model::_skinSpecularMapLocations;
Model::SkinLocations Model::_skinNormalSpecularMapLocations;
Model::SkinLocations Model::_skinShadowMapLocations;
Model::SkinLocations Model::_skinShadowNormalMapLocations;
Model::SkinLocations Model::_skinShadowSpecularMapLocations;
Model::SkinLocations Model::_skinShadowNormalSpecularMapLocations;
Model::SkinLocations Model::_skinCascadedShadowMapLocations;
Model::SkinLocations Model::_skinCascadedShadowNormalMapLocations;
Model::SkinLocations Model::_skinCascadedShadowSpecularMapLocations;
Model::SkinLocations Model::_skinCascadedShadowNormalSpecularMapLocations;
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
        initJointTransforms();
        if (_shapes.size() > 0) {
            clearShapes();
            buildShapes();
        }
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
    locations.shadowDistances = program.uniformLocation("shadowDistances");
    program.setUniformValue("diffuseMap", 0);
    program.setUniformValue("normalMap", 1);
    program.setUniformValue("specularMap", specularTextureUnit);
    program.setUniformValue("shadowMap", shadowTextureUnit);
    program.release();
}

QVector<JointState> Model::createJointStates(const FBXGeometry& geometry) {
    QVector<JointState> jointStates;
    foreach (const FBXJoint& joint, geometry.joints) {
        // NOTE: the state keeps a pointer to an FBXJoint
        JointState state;
        state.setFBXJoint(&joint);
        jointStates.append(state);
    }
    return jointStates;
};

void Model::initJointTransforms() {
    // compute model transforms
    int numJoints = _jointStates.size();
    for (int i = 0; i < numJoints; ++i) {
        JointState& state = _jointStates[i];
        const FBXJoint& joint = state.getFBXJoint();
        int parentIndex = joint.parentIndex;
        if (parentIndex == -1) {
            const FBXGeometry& geometry = _geometry->getFBXGeometry();
            // NOTE: in practice geometry.offset has a non-unity scale (rather than a translation)
            glm::mat4 parentTransform = glm::scale(_scale) * glm::translate(_offset) * geometry.offset;
            state.initTransform(parentTransform);
        } else {
            const JointState& parentState = _jointStates.at(parentIndex);
            state.initTransform(parentState.getTransform());
        }
    }
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
        _normalSpecularMapTangentLocation = _normalSpecularMapProgram.attributeLocation("tangent");
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
        _shadowNormalSpecularMapTangentLocation = _shadowNormalSpecularMapProgram.attributeLocation("tangent");
        _shadowNormalSpecularMapProgram.release();
        
        
        _cascadedShadowMapProgram.addShaderFromSourceFile(QGLShader::Vertex, Application::resourcesPath() +
            "shaders/model.vert");
        _cascadedShadowMapProgram.addShaderFromSourceFile(QGLShader::Fragment, Application::resourcesPath() +
            "shaders/model_cascaded_shadow_map.frag");
        _cascadedShadowMapProgram.link();
        
        _cascadedShadowMapProgram.bind();
        _cascadedShadowMapProgram.setUniformValue("diffuseMap", 0);
        _cascadedShadowMapProgram.setUniformValue("shadowMap", 1);
        _cascadedShadowMapDistancesLocation = _cascadedShadowMapProgram.uniformLocation("shadowDistances");
        _cascadedShadowMapProgram.release();
        
        _cascadedShadowNormalMapProgram.addShaderFromSourceFile(QGLShader::Vertex,
            Application::resourcesPath() + "shaders/model_normal_map.vert");
        _cascadedShadowNormalMapProgram.addShaderFromSourceFile(QGLShader::Fragment,
            Application::resourcesPath() + "shaders/model_cascaded_shadow_normal_map.frag");
        _cascadedShadowNormalMapProgram.link();
        
        _cascadedShadowNormalMapProgram.bind();
        _cascadedShadowNormalMapProgram.setUniformValue("diffuseMap", 0);
        _cascadedShadowNormalMapProgram.setUniformValue("normalMap", 1);
        _cascadedShadowNormalMapProgram.setUniformValue("shadowMap", 2);
        _cascadedShadowNormalMapDistancesLocation = _cascadedShadowNormalMapProgram.uniformLocation("shadowDistances");
        _cascadedShadowNormalMapTangentLocation = _cascadedShadowNormalMapProgram.attributeLocation("tangent");
        _cascadedShadowNormalMapProgram.release();
        
        _cascadedShadowSpecularMapProgram.addShaderFromSourceFile(QGLShader::Vertex,
            Application::resourcesPath() + "shaders/model.vert");
        _cascadedShadowSpecularMapProgram.addShaderFromSourceFile(QGLShader::Fragment,
            Application::resourcesPath() + "shaders/model_cascaded_shadow_specular_map.frag");
        _cascadedShadowSpecularMapProgram.link();
        
        _cascadedShadowSpecularMapProgram.bind();
        _cascadedShadowSpecularMapProgram.setUniformValue("diffuseMap", 0);
        _cascadedShadowSpecularMapProgram.setUniformValue("specularMap", 1);
        _cascadedShadowSpecularMapProgram.setUniformValue("shadowMap", 2);
        _cascadedShadowSpecularMapDistancesLocation = _cascadedShadowSpecularMapProgram.uniformLocation("shadowDistances");
        _cascadedShadowSpecularMapProgram.release();
        
        _cascadedShadowNormalSpecularMapProgram.addShaderFromSourceFile(QGLShader::Vertex,
            Application::resourcesPath() + "shaders/model_normal_map.vert");
        _cascadedShadowNormalSpecularMapProgram.addShaderFromSourceFile(QGLShader::Fragment,
            Application::resourcesPath() + "shaders/model_cascaded_shadow_normal_specular_map.frag");
        _cascadedShadowNormalSpecularMapProgram.link();
        
        _cascadedShadowNormalSpecularMapProgram.bind();
        _cascadedShadowNormalSpecularMapProgram.setUniformValue("diffuseMap", 0);
        _cascadedShadowNormalSpecularMapProgram.setUniformValue("normalMap", 1);
        _cascadedShadowNormalSpecularMapProgram.setUniformValue("specularMap", 2);
        _cascadedShadowNormalSpecularMapProgram.setUniformValue("shadowMap", 3);
        _cascadedShadowNormalSpecularMapDistancesLocation =
            _cascadedShadowNormalSpecularMapProgram.uniformLocation("shadowDistances");
        _cascadedShadowNormalSpecularMapTangentLocation = _cascadedShadowNormalSpecularMapProgram.attributeLocation("tangent");
        _cascadedShadowNormalSpecularMapProgram.release();
        
        
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
        
        
        _skinCascadedShadowMapProgram.addShaderFromSourceFile(QGLShader::Vertex, Application::resourcesPath() +
            "shaders/skin_model.vert");
        _skinCascadedShadowMapProgram.addShaderFromSourceFile(QGLShader::Fragment, Application::resourcesPath() +
            "shaders/model_cascaded_shadow_map.frag");
        _skinCascadedShadowMapProgram.link();
        
        initSkinProgram(_skinCascadedShadowMapProgram, _skinCascadedShadowMapLocations);
        
        _skinCascadedShadowNormalMapProgram.addShaderFromSourceFile(QGLShader::Vertex,
            Application::resourcesPath() + "shaders/skin_model_normal_map.vert");
        _skinCascadedShadowNormalMapProgram.addShaderFromSourceFile(QGLShader::Fragment,
            Application::resourcesPath() + "shaders/model_cascaded_shadow_normal_map.frag");
        _skinCascadedShadowNormalMapProgram.link();
        
        initSkinProgram(_skinCascadedShadowNormalMapProgram, _skinCascadedShadowNormalMapLocations, 1, 2);
        
        _skinCascadedShadowSpecularMapProgram.addShaderFromSourceFile(QGLShader::Vertex,
            Application::resourcesPath() + "shaders/skin_model.vert");
        _skinCascadedShadowSpecularMapProgram.addShaderFromSourceFile(QGLShader::Fragment,
            Application::resourcesPath() + "shaders/model_cascaded_shadow_specular_map.frag");
        _skinCascadedShadowSpecularMapProgram.link();
        
        initSkinProgram(_skinCascadedShadowSpecularMapProgram, _skinCascadedShadowSpecularMapLocations, 1, 2);
        
        _skinCascadedShadowNormalSpecularMapProgram.addShaderFromSourceFile(QGLShader::Vertex,
            Application::resourcesPath() + "shaders/skin_model_normal_map.vert");
        _skinCascadedShadowNormalSpecularMapProgram.addShaderFromSourceFile(QGLShader::Fragment,
            Application::resourcesPath() + "shaders/model_cascaded_shadow_normal_specular_map.frag");
        _skinCascadedShadowNormalSpecularMapProgram.link();
        
        initSkinProgram(_skinCascadedShadowNormalSpecularMapProgram, _skinCascadedShadowNormalSpecularMapLocations, 2, 3);
        
        
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
        _jointStates[i].setRotationInConstrainedFrame(geometry.joints.at(i).rotation);
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
                    newJointStates[newIndex].copyState(_jointStates[oldIndex]);
                }
            }
        } 
        deleteGeometry();
        _dilatedTextures.clear();
        _geometry = geometry;
        setJointStates(newJointStates);
        needToRebuild = true;
    } else if (_jointStates.isEmpty()) {
        const FBXGeometry& fbxGeometry = geometry->getFBXGeometry();
        if (fbxGeometry.joints.size() > 0) {
            setJointStates(createJointStates(fbxGeometry));
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
        needFullUpdate = true;
    }
    return needFullUpdate;
}

// virtual
void Model::setJointStates(QVector<JointState> states) {
    _jointStates = states;
    initJointTransforms();

    int numJoints = _jointStates.size();
    float radius = 0.0f;
    for (int i = 0; i < numJoints; ++i) {
        float distance = glm::length(_jointStates[i].getPosition());
        if (distance > radius) {
            radius = distance;
        }
        _jointStates[i].updateConstraint();
    }
    for (int i = 0; i < _jointStates.size(); i++) {
        _jointStates[i].slaveVisibleTransform();
    }
    _boundingRadius = radius;
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
    
    receiveShadows &= Menu::getInstance()->getShadowsEnabled();
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
    glm::vec3 minimum = glm::vec3(_geometry->getFBXGeometry().offset * glm::vec4(extents.minimum, 1.0f));
    glm::vec3 maximum = glm::vec3(_geometry->getFBXGeometry().offset * glm::vec4(extents.maximum, 1.0f));
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
    glm::vec3 minimum = glm::vec3(_geometry->getFBXGeometry().offset * glm::vec4(extents.minimum, 1.0f));
    glm::vec3 maximum = glm::vec3(_geometry->getFBXGeometry().offset * glm::vec4(extents.maximum, 1.0f));
    Extents scaledExtents = { minimum, maximum };
        
    return scaledExtents;
}

bool Model::getJointState(int index, glm::quat& rotation) const {
    if (index == -1 || index >= _jointStates.size()) {
        return false;
    }
    rotation = _jointStates.at(index).getRotationInConstrainedFrame();
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
                state.setRotationInConstrainedFrame(rotation);
                state._animationPriority = priority;
            } else {
                state.restoreRotation(1.0f, priority);
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

bool Model::getJointPositionInWorldFrame(int jointIndex, glm::vec3& position) const {
    if (jointIndex == -1 || jointIndex >= _jointStates.size()) {
        return false;
    }
    // position is in world-frame
    position = _translation + _rotation * _jointStates[jointIndex].getPosition();
    return true;
}

bool Model::getJointPosition(int jointIndex, glm::vec3& position) const {
    if (jointIndex == -1 || jointIndex >= _jointStates.size()) {
        return false;
    }
    // position is in model-frame
    position = extractTranslation(_jointStates[jointIndex].getTransform());
    return true;
}

bool Model::getJointRotationInWorldFrame(int jointIndex, glm::quat& rotation) const {
    if (jointIndex == -1 || jointIndex >= _jointStates.size()) {
        return false;
    }
    rotation = _rotation * _jointStates[jointIndex].getRotation();
    return true;
}

bool Model::getJointCombinedRotation(int jointIndex, glm::quat& rotation) const {
    if (jointIndex == -1 || jointIndex >= _jointStates.size()) {
        return false;
    }
    rotation = _rotation * _jointStates[jointIndex].getRotation();
    return true;
}

bool Model::getVisibleJointPositionInWorldFrame(int jointIndex, glm::vec3& position) const {
    if (jointIndex == -1 || jointIndex >= _jointStates.size()) {
        return false;
    }
    // position is in world-frame
    position = _translation + _rotation * _jointStates[jointIndex].getVisiblePosition();
    return true;
}

bool Model::getVisibleJointRotationInWorldFrame(int jointIndex, glm::quat& rotation) const {
    if (jointIndex == -1 || jointIndex >= _jointStates.size()) {
        return false;
    }
    rotation = _rotation * _jointStates[jointIndex].getVisibleRotation();
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

// virtual override from PhysicsEntity
void Model::buildShapes() {
    // TODO: figure out how to load/build collision shapes for general models
}

void Model::updateShapePositions() {
    // TODO: implement this when we know how to build shapes for regular Models
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
    // NOTE: this is a recursive call that walks all attachments, and their attachments
    // update the world space transforms for all joints

    // update animations
    foreach (const AnimationHandlePointer& handle, _runningAnimations) {
        handle->simulate(deltaTime);
    }

    for (int i = 0; i < _jointStates.size(); i++) {
        updateJointState(i);
    }

    _shapesAreDirty = ! _shapes.isEmpty();
    
    // update the attachment transforms and simulate them
    const FBXGeometry& geometry = _geometry->getFBXGeometry();
    for (int i = 0; i < _attachments.size(); i++) {
        const FBXAttachment& attachment = geometry.attachments.at(i);
        Model* model = _attachments.at(i);
        
        glm::vec3 jointTranslation = _translation;
        glm::quat jointRotation = _rotation;
        if (_showTrueJointTransforms) {
            getJointPositionInWorldFrame(attachment.jointIndex, jointTranslation);
            getJointRotationInWorldFrame(attachment.jointIndex, jointRotation);
        } else {
            getVisibleJointPositionInWorldFrame(attachment.jointIndex, jointTranslation);
            getVisibleJointRotationInWorldFrame(attachment.jointIndex, jointRotation);
        }
        
        model->setTranslation(jointTranslation + jointRotation * attachment.translation * _scale);
        model->setRotation(jointRotation * attachment.rotation);
        model->setScale(_scale * attachment.scale);
        
        if (model->isActive()) {
            model->simulateInternal(deltaTime);
        }
    }
    
    glm::mat4 modelToWorld = glm::mat4_cast(_rotation);
    for (int i = 0; i < _meshStates.size(); i++) {
        MeshState& state = _meshStates[i];
        const FBXMesh& mesh = geometry.meshes.at(i);
        if (_showTrueJointTransforms) {
            for (int j = 0; j < mesh.clusters.size(); j++) {
                const FBXCluster& cluster = mesh.clusters.at(j);
                state.clusterMatrices[j] = modelToWorld * _jointStates[cluster.jointIndex].getTransform() * cluster.inverseBindMatrix;
            }
        } else {
            for (int j = 0; j < mesh.clusters.size(); j++) {
                const FBXCluster& cluster = mesh.clusters.at(j);
                state.clusterMatrices[j] = modelToWorld * _jointStates[cluster.jointIndex].getVisibleTransform() * cluster.inverseBindMatrix;
            }
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
    
    // compute model transforms
    int parentIndex = joint.parentIndex;
    if (parentIndex == -1) {
        const FBXGeometry& geometry = _geometry->getFBXGeometry();
        glm::mat4 parentTransform = glm::scale(_scale) * glm::translate(_offset) * geometry.offset;
        state.computeTransform(parentTransform);
    } else {
        const JointState& parentState = _jointStates.at(parentIndex);
        state.computeTransform(parentState.getTransform());
    }
}

void Model::updateVisibleJointStates() {
    if (_showTrueJointTransforms) {
        // no need to update visible transforms
        return;
    }
    for (int i = 0; i < _jointStates.size(); i++) {
        _jointStates[i].slaveVisibleTransform();
    }
}

bool Model::setJointPosition(int jointIndex, const glm::vec3& position, const glm::quat& rotation, bool useRotation,
       int lastFreeIndex, bool allIntermediatesFree, const glm::vec3& alignment, float priority) {
    if (jointIndex == -1 || _jointStates.isEmpty()) {
        return false;
    }
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
    glm::vec3 worldAlignment = alignment;
    for (int i = 0; i < ITERATION_COUNT; i++) {
        // first, try to rotate the end effector as close as possible to the target rotation, if any
        glm::quat endRotation;
        if (useRotation) {
            JointState& state = _jointStates[jointIndex];

            state.setRotationInBindFrame(rotation, priority);
            endRotation = state.getRotationInBindFrame();
        }    
        
        // then, we go from the joint upwards, rotating the end as close as possible to the target
        glm::vec3 endPosition = extractTranslation(_jointStates[jointIndex].getTransform());
        for (int j = 1; freeLineage.at(j - 1) != lastFreeIndex; j++) {
            int index = freeLineage.at(j);
            JointState& state = _jointStates[index];
            const FBXJoint& joint = state.getFBXJoint();
            if (!(joint.isFree || allIntermediatesFree)) {
                continue;
            }
            glm::vec3 jointPosition = extractTranslation(state.getTransform());
            glm::vec3 jointVector = endPosition - jointPosition;
            glm::quat oldCombinedRotation = state.getRotation();
            glm::quat combinedDelta;
            float combinedWeight;
            if (useRotation) {
                combinedDelta = safeMix(rotation * glm::inverse(endRotation),
                    rotationBetween(jointVector, position - jointPosition), 0.5f);
                combinedWeight = 2.0f;
                
            } else {
                combinedDelta = rotationBetween(jointVector, position - jointPosition);
                combinedWeight = 1.0f;
            }
            if (alignment != glm::vec3() && j > 1) {
                jointVector = endPosition - jointPosition;
                glm::vec3 positionSum;
                for (int k = j - 1; k > 0; k--) {
                    int index = freeLineage.at(k);
                    updateJointState(index);
                    positionSum += extractTranslation(_jointStates.at(index).getTransform());
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
            state.applyRotationDelta(combinedDelta, true, priority);
            glm::quat actualDelta = state.getRotation() * glm::inverse(oldCombinedRotation);
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
    _shapesAreDirty = !_shapes.isEmpty();
        
    return true;
}

void Model::inverseKinematics(int endIndex, glm::vec3 targetPosition, const glm::quat& targetRotation, float priority) {
    // NOTE: targetRotation is from bind- to model-frame

    if (endIndex == -1 || _jointStates.isEmpty()) {
        return;
    }

    const FBXGeometry& geometry = _geometry->getFBXGeometry();
    const QVector<int>& freeLineage = geometry.joints.at(endIndex).freeLineage;
    if (freeLineage.isEmpty()) {
        return;
    }
    int numFree = freeLineage.size();

    // store and remember topmost parent transform
    glm::mat4 topParentTransform;
    {
        int index = freeLineage.last();
        const JointState& state = _jointStates.at(index);
        const FBXJoint& joint = state.getFBXJoint();
        int parentIndex = joint.parentIndex;
        if (parentIndex == -1) {
            const FBXGeometry& geometry = _geometry->getFBXGeometry();
            topParentTransform = glm::scale(_scale) * glm::translate(_offset) * geometry.offset;
        } else {
            topParentTransform = _jointStates[parentIndex].getTransform();
        }
    }

    // this is a cyclic coordinate descent algorithm: see
    // http://www.ryanjuckett.com/programming/animation/21-cyclic-coordinate-descent-in-2d

    // keep track of the position of the end-effector
    JointState& endState = _jointStates[endIndex];
    glm::vec3 endPosition = endState.getPosition();
    float distanceToGo = glm::distance(targetPosition, endPosition);

    const int MAX_ITERATION_COUNT = 2;
    const float ACCEPTABLE_IK_ERROR = 0.005f; // 5mm
    int numIterations = 0;
    do {
        ++numIterations;
        // moving up, rotate each free joint to get endPosition closer to target
        for (int j = 1; j < numFree; j++) {
            int nextIndex = freeLineage.at(j);
            JointState& nextState = _jointStates[nextIndex];
            FBXJoint nextJoint = nextState.getFBXJoint();
            if (! nextJoint.isFree) {
                continue;
            }

            glm::vec3 pivot = nextState.getPosition();
            glm::vec3 leverArm = endPosition - pivot;
            float leverLength = glm::length(leverArm);
            if (leverLength < EPSILON) {
                continue;
            }
            glm::quat deltaRotation = rotationBetween(leverArm, targetPosition - pivot);

            // We want to mix the shortest rotation with one that will pull the system down with gravity
            // so that limbs don't float unrealistically.  To do this we compute a simplified center of mass
            // where each joint has unit mass and we don't bother averaging it because we only need direction.
            if (j > 1) {

                glm::vec3 centerOfMass(0.0f);
                for (int k = 0; k < j; ++k) {
                    int massIndex = freeLineage.at(k);
                    centerOfMass += _jointStates[massIndex].getPosition() - pivot;
                }
                // the gravitational effect is a rotation that tends to align the two cross products
                const glm::vec3 worldAlignment = glm::vec3(0.0f, -1.f, 0.0f);
                glm::quat gravityDelta = rotationBetween(glm::cross(centerOfMass, leverArm),
                    glm::cross(worldAlignment, leverArm));

                float gravityAngle = glm::angle(gravityDelta);
                const float MIN_GRAVITY_ANGLE = 0.1f;
                float mixFactor = 0.5f;
                if (gravityAngle < MIN_GRAVITY_ANGLE) {
                    // the final rotation is a mix of the two
                    mixFactor = 0.5f * gravityAngle / MIN_GRAVITY_ANGLE;
                }
                deltaRotation = safeMix(deltaRotation, gravityDelta, mixFactor);
            }

            // Apply the rotation, but use mixRotationDelta() which blends a bit of the default pose
            // at in the process.  This provides stability to the IK solution for most models.
            glm::quat oldNextRotation = nextState.getRotation();
            float mixFactor = 0.03f;
            nextState.mixRotationDelta(deltaRotation, mixFactor, priority);

            // measure the result of the rotation which may have been modified by
            // blending and constraints
            glm::quat actualDelta = nextState.getRotation() * glm::inverse(oldNextRotation);
            endPosition = pivot + actualDelta * leverArm;
        }

        // recompute transforms from the top down
        glm::mat4 parentTransform = topParentTransform;
        for (int j = numFree - 1; j >= 0; --j) {
            JointState& freeState = _jointStates[freeLineage.at(j)];
            freeState.computeTransform(parentTransform);
            parentTransform = freeState.getTransform();
        }

        // measure our success
        endPosition = endState.getPosition();
        distanceToGo = glm::distance(targetPosition, endPosition);
    } while (numIterations < MAX_ITERATION_COUNT && distanceToGo < ACCEPTABLE_IK_ERROR);

    // set final rotation of the end joint
    endState.setRotationInBindFrame(targetRotation, priority, true);
     
    _shapesAreDirty = !_shapes.isEmpty();
}

bool Model::restoreJointPosition(int jointIndex, float fraction, float priority) {
    if (jointIndex == -1 || _jointStates.isEmpty()) {
        return false;
    }
    const FBXGeometry& geometry = _geometry->getFBXGeometry();
    const QVector<int>& freeLineage = geometry.joints.at(jointIndex).freeLineage;
   
    foreach (int index, freeLineage) {
        JointState& state = _jointStates[index];
        state.restoreRotation(fraction, priority);
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

const int BALL_SUBDIVISIONS = 10;

void Model::renderJointCollisionShapes(float alpha) {
    glPushMatrix();
    Application::getInstance()->loadTranslatedViewMatrix(_translation);
    for (int i = 0; i < _shapes.size(); i++) {
        Shape* shape = _shapes[i];
        if (!shape) {
            continue;
        }

        glPushMatrix();
        // NOTE: the shapes are in the avatar local-frame
        if (shape->getType() == Shape::SPHERE_SHAPE) {
            // shapes are stored in world-frame, so we have to transform into model frame
            glm::vec3 position = _rotation * shape->getTranslation();
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
            endPoint = _rotation * endPoint;
            glTranslatef(endPoint.x, endPoint.y, endPoint.z);
            glColor4f(0.6f, 0.6f, 0.8f, alpha);
            glutSolidSphere(capsule->getRadius(), BALL_SUBDIVISIONS, BALL_SUBDIVISIONS);

            // draw a yellow sphere at the capsule startpoint
            glm::vec3 startPoint;
            capsule->getStartPoint(startPoint);
            startPoint = _rotation * startPoint;
            glm::vec3 axis = endPoint - startPoint;
            glTranslatef(-axis.x, -axis.y, -axis.z);
            glColor4f(0.8f, 0.8f, 0.6f, alpha);
            glutSolidSphere(capsule->getRadius(), BALL_SUBDIVISIONS, BALL_SUBDIVISIONS);
            
            // draw a green cylinder between the two points
            glm::vec3 origin(0.0f);
            glColor4f(0.6f, 0.8f, 0.6f, alpha);
            Avatar::renderJointConnectingCone( origin, axis, capsule->getRadius(), capsule->getRadius());
        }
        glPopMatrix();
    }
    glPopMatrix();
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
    updateVisibleJointStates();
    const FBXGeometry& geometry = _geometry->getFBXGeometry();
    const QVector<NetworkMesh>& networkMeshes = _geometry->getMeshes();
    
    bool cascadedShadows = Menu::getInstance()->isOptionChecked(MenuOption::CascadedShadows);
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
        int tangentLocation = _normalMapTangentLocation;
        int shadowDistancesLocation = _cascadedShadowMapDistancesLocation;
        GLenum specularTextureUnit = 0;
        GLenum shadowTextureUnit = 0;
        if (mode == SHADOW_RENDER_MODE) {
            program = &_shadowProgram;
            skinProgram = &_skinShadowProgram;
            skinLocations = &_skinShadowLocations;
            
        } else if (!mesh.tangents.isEmpty()) {
            if (mesh.hasSpecularTexture()) {
                if (receiveShadows) {
                    if (cascadedShadows) {
                        program = &_cascadedShadowNormalSpecularMapProgram;
                        skinProgram = &_skinCascadedShadowNormalSpecularMapProgram;
                        skinLocations = &_skinCascadedShadowNormalSpecularMapLocations;
                        tangentLocation = _cascadedShadowNormalSpecularMapTangentLocation;
                        shadowDistancesLocation = _cascadedShadowNormalSpecularMapDistancesLocation;
                    } else {
                        program = &_shadowNormalSpecularMapProgram;
                        skinProgram = &_skinShadowNormalSpecularMapProgram;
                        skinLocations = &_skinShadowNormalSpecularMapLocations;
                        tangentLocation = _shadowNormalSpecularMapTangentLocation;
                    }
                    shadowTextureUnit = GL_TEXTURE3;
                } else {
                    program = &_normalSpecularMapProgram;
                    skinProgram = &_skinNormalSpecularMapProgram;
                    skinLocations = &_skinNormalSpecularMapLocations;
                    tangentLocation = _normalSpecularMapTangentLocation;
                }
                specularTextureUnit = GL_TEXTURE2;
                
            } else if (receiveShadows) {
                if (cascadedShadows) {
                    program = &_cascadedShadowNormalMapProgram;
                    skinProgram = &_skinCascadedShadowNormalMapProgram;
                    skinLocations = &_skinCascadedShadowNormalMapLocations;
                    tangentLocation = _cascadedShadowNormalMapTangentLocation;
                    shadowDistancesLocation = _cascadedShadowNormalMapDistancesLocation;
                } else {
                    program = &_shadowNormalMapProgram;
                    skinProgram = &_skinShadowNormalMapProgram;
                    skinLocations = &_skinShadowNormalMapLocations;
                    tangentLocation = _shadowNormalMapTangentLocation;
                }
                shadowTextureUnit = GL_TEXTURE2;
            } else {
                program = &_normalMapProgram;
                skinProgram = &_skinNormalMapProgram;
                skinLocations = &_skinNormalMapLocations;
            }
        } else if (mesh.hasSpecularTexture()) {
            if (receiveShadows) {
                if (cascadedShadows) {
                    program = &_cascadedShadowSpecularMapProgram;
                    skinProgram = &_skinCascadedShadowSpecularMapProgram;
                    skinLocations = &_skinCascadedShadowSpecularMapLocations;
                    shadowDistancesLocation = _cascadedShadowSpecularMapDistancesLocation;
                } else {
                    program = &_shadowSpecularMapProgram;
                    skinProgram = &_skinShadowSpecularMapProgram;
                    skinLocations = &_skinShadowSpecularMapLocations;
                }
                shadowTextureUnit = GL_TEXTURE2;
            } else {
                program = &_specularMapProgram;
                skinProgram = &_skinSpecularMapProgram;
                skinLocations = &_skinSpecularMapLocations;
            }
            specularTextureUnit = GL_TEXTURE1;
            
        } else if (receiveShadows) {
            if (cascadedShadows) {
                program = &_cascadedShadowMapProgram;
                skinProgram = &_skinCascadedShadowMapProgram;
                skinLocations = &_skinCascadedShadowMapLocations;
            } else {
                program = &_shadowMapProgram;
                skinProgram = &_skinShadowMapProgram;
                skinLocations = &_skinShadowMapLocations;
            }
            shadowTextureUnit = GL_TEXTURE1;
        }
        
        const MeshState& state = _meshStates.at(i);
        ProgramObject* activeProgram = program;
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
            if (cascadedShadows) {
                program->setUniform(skinLocations->shadowDistances, Application::getInstance()->getShadowDistances());
            }
            
            // local light uniforms
            skinProgram->setUniformValue("numLocalLights", _numLocalLights);
            skinProgram->setUniformArray("localLightDirections", _localLightDirections, MAX_LOCAL_LIGHTS);
            skinProgram->setUniformArray("localLightColors", _localLightColors, MAX_LOCAL_LIGHTS);
        } else {
            glMultMatrixf((const GLfloat*)&state.clusterMatrices[0]);
            program->bind();
            if (cascadedShadows) {
                program->setUniform(shadowDistancesLocation, Application::getInstance()->getShadowDistances());
            }
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

void Model::setLocalLightDirection(const glm::vec3& direction, int lightIndex) {
    assert(lightIndex >= 0 && lightIndex < MAX_LOCAL_LIGHTS);
    _localLightDirections[lightIndex] = direction;
}

void Model::setLocalLightColor(const glm::vec3& color, int lightIndex) {
    assert(lightIndex >= 0 && lightIndex < MAX_LOCAL_LIGHTS);
    _localLightColors[lightIndex] = color;
}

void Model::setNumLocalLights(int numLocalLights) {
    _numLocalLights = numLocalLights;
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
    _firstFrame(0.0f),
    _lastFrame(FLT_MAX),
    _running(false) {
}

AnimationDetails AnimationHandle::getAnimationDetails() const {
    AnimationDetails details(_role, _url, _fps, _priority, _loop, _hold,
                        _startAutomatically, _firstFrame, _lastFrame, _running, _frameIndex);
    return details;
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
    float endFrameIndex = qMin(_lastFrame, animationGeometry.animationFrames.size() - (_loop ? 0.0f : 1.0f));
    float startFrameIndex = qMin(_firstFrame, endFrameIndex);
    if ((!_loop && (_frameIndex < startFrameIndex || _frameIndex > endFrameIndex)) || startFrameIndex == endFrameIndex) {
        // passed the end; apply the last frame
        applyFrame(glm::clamp(_frameIndex, startFrameIndex, endFrameIndex));
        if (!_hold) {
            stop();
        }
        return;
    }
    // wrap within the the desired range
    if (_frameIndex < startFrameIndex) {
        _frameIndex = endFrameIndex - glm::mod(endFrameIndex - _frameIndex, endFrameIndex - startFrameIndex);
    
    } else if (_frameIndex > endFrameIndex) {
        _frameIndex = startFrameIndex + glm::mod(_frameIndex - startFrameIndex, endFrameIndex - startFrameIndex);
    }
    
    // blend between the closest two frames
    applyFrame(_frameIndex);
}

void AnimationHandle::applyFrame(float frameIndex) {
    const FBXGeometry& animationGeometry = _animation->getGeometry();
    int frameCount = animationGeometry.animationFrames.size();
    const FBXAnimationFrame& floorFrame = animationGeometry.animationFrames.at((int)glm::floor(frameIndex) % frameCount);
    const FBXAnimationFrame& ceilFrame = animationGeometry.animationFrames.at((int)glm::ceil(frameIndex) % frameCount);
    float frameFraction = glm::fract(frameIndex);
    for (int i = 0; i < _jointMappings.size(); i++) {
        int mapping = _jointMappings.at(i);
        if (mapping != -1) {
            JointState& state = _model->_jointStates[mapping];
            if (_priority >= state._animationPriority) {
                state.setRotationInConstrainedFrame(safeMix(floorFrame.rotations.at(i), ceilFrame.rotations.at(i), frameFraction));
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

