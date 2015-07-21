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
#include <gpu/Batch.h>
#include <gpu/GLBackend.h>
#include <PathUtils.h>
#include <PerfStat.h>
#include "PhysicsEntity.h"
#include <ShapeCollider.h>
#include <SphereShape.h>
#include <ViewFrustum.h>

#include "AbstractViewStateInterface.h"
#include "AnimationHandle.h"
#include "DeferredLightingEffect.h"
#include "Model.h"
#include "RenderUtilsLogging.h"

#include "model_vert.h"
#include "model_shadow_vert.h"
#include "model_normal_map_vert.h"
#include "model_lightmap_vert.h"
#include "model_lightmap_normal_map_vert.h"
#include "skin_model_vert.h"
#include "skin_model_shadow_vert.h"
#include "skin_model_normal_map_vert.h"

#include "model_frag.h"
#include "model_shadow_frag.h"
#include "model_normal_map_frag.h"
#include "model_normal_specular_map_frag.h"
#include "model_specular_map_frag.h"
#include "model_lightmap_frag.h"
#include "model_lightmap_normal_map_frag.h"
#include "model_lightmap_normal_specular_map_frag.h"
#include "model_lightmap_specular_map_frag.h"
#include "model_translucent_frag.h"

using namespace std;

static int modelPointerTypeId = qRegisterMetaType<QPointer<Model> >();
static int weakNetworkGeometryPointerTypeId = qRegisterMetaType<QWeakPointer<NetworkGeometry> >();
static int vec3VectorTypeId = qRegisterMetaType<QVector<glm::vec3> >();
float Model::FAKE_DIMENSION_PLACEHOLDER = -1.0f;

Model::Model(QObject* parent) :
    QObject(parent),
    _scale(1.0f, 1.0f, 1.0f),
    _scaleToFit(false),
    _scaleToFitDimensions(0.0f),
    _scaledToFit(false),
    _snapModelToRegistrationPoint(false),
    _snappedToRegistrationPoint(false),
    _showTrueJointTransforms(true),
    _lodDistance(0.0f),
    _pupilDilation(0.0f),
    _url("http://invalid.com"),
    _isVisible(true),
    _blendNumber(0),
    _appliedBlendNumber(0),
    _calculatedMeshPartOffsetValid(false),
    _calculatedMeshPartBoxesValid(false),
    _calculatedMeshBoxesValid(false),
    _calculatedMeshTrianglesValid(false),
    _meshGroupsKnown(false),
    _isWireframe(false),
    _renderCollisionHull(false) {
    
    // we may have been created in the network thread, but we live in the main thread
    if (_viewState) {
        moveToThread(_viewState->getMainThread());
    }
    
    setSnapModelToRegistrationPoint(true, glm::vec3(0.5f));
}

Model::~Model() {
    deleteGeometry();
}

Model::RenderPipelineLib Model::_renderPipelineLib;
const GLint MATERIAL_GPU_SLOT = 3;

void Model::RenderPipelineLib::addRenderPipeline(Model::RenderKey key,
                                                 gpu::ShaderPointer& vertexShader,
                                                 gpu::ShaderPointer& pixelShader ) {

    gpu::Shader::BindingSet slotBindings;
    slotBindings.insert(gpu::Shader::Binding(std::string("materialBuffer"), MATERIAL_GPU_SLOT));
    slotBindings.insert(gpu::Shader::Binding(std::string("diffuseMap"), 0));
    slotBindings.insert(gpu::Shader::Binding(std::string("normalMap"), 1));
    slotBindings.insert(gpu::Shader::Binding(std::string("specularMap"), 2));
    slotBindings.insert(gpu::Shader::Binding(std::string("emissiveMap"), 3));
    slotBindings.insert(gpu::Shader::Binding(std::string("lightBuffer"), 4));

    gpu::ShaderPointer program = gpu::ShaderPointer(gpu::Shader::createProgram(vertexShader, pixelShader));
    gpu::Shader::makeProgram(*program, slotBindings);
    
    
    auto locations = std::make_shared<Locations>();
    initLocations(program, *locations);

    
    auto state = std::make_shared<gpu::State>();
 
    // Backface on shadow
    if (key.isShadow()) {
        state->setCullMode(gpu::State::CULL_FRONT);
        state->setDepthBias(1.0f);
        state->setDepthBiasSlopeScale(4.0f);
    } else {
        state->setCullMode(gpu::State::CULL_BACK);
    }

    // Z test depends if transparent or not
    state->setDepthTest(true, !key.isTranslucent(), gpu::LESS_EQUAL);

    // Blend on transparent
    state->setBlendFunction(key.isTranslucent(),
        gpu::State::ONE, gpu::State::BLEND_OP_ADD, gpu::State::INV_SRC_ALPHA, // For transparent only, this keep the highlight intensity
        gpu::State::FACTOR_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::ONE);

    // Good to go add the brand new pipeline
    auto pipeline = gpu::PipelinePointer(gpu::Pipeline::create(program, state));
    insert(value_type(key.getRaw(), RenderPipeline(pipeline, locations)));
    
    
    if (!key.isWireFrame()) {
        
        RenderKey wireframeKey(key.getRaw() | RenderKey::IS_WIREFRAME);
        auto wireframeState = std::make_shared<gpu::State>(state->getValues());
        
        wireframeState->setFillMode(gpu::State::FILL_LINE);
        
        // create a new RenderPipeline with the same shader side and the mirrorState
        auto wireframePipeline = gpu::PipelinePointer(gpu::Pipeline::create(program, wireframeState));
        insert(value_type(wireframeKey.getRaw(), RenderPipeline(wireframePipeline, locations)));
    }
    
    // If not a shadow pass, create the mirror version from the same state, just change the FrontFace
    if (!key.isShadow()) {
        
        RenderKey mirrorKey(key.getRaw() | RenderKey::IS_MIRROR);
        auto mirrorState = std::make_shared<gpu::State>(state->getValues());

        mirrorState->setFrontFaceClockwise(true);

        // create a new RenderPipeline with the same shader side and the mirrorState
        auto mirrorPipeline = gpu::PipelinePointer(gpu::Pipeline::create(program, mirrorState));
        insert(value_type(mirrorKey.getRaw(), RenderPipeline(mirrorPipeline, locations)));
        
        if (!key.isWireFrame()) {
            RenderKey wireframeKey(key.getRaw() | RenderKey::IS_MIRROR | RenderKey::IS_WIREFRAME);
            auto wireframeState = std::make_shared<gpu::State>(state->getValues());
            
            wireframeState->setFillMode(gpu::State::FILL_LINE);
            
            // create a new RenderPipeline with the same shader side and the mirrorState
            auto wireframePipeline = gpu::PipelinePointer(gpu::Pipeline::create(program, wireframeState));
            insert(value_type(wireframeKey.getRaw(), RenderPipeline(wireframePipeline, locations)));
        }
    }
}


void Model::RenderPipelineLib::initLocations(gpu::ShaderPointer& program, Model::Locations& locations) {
    locations.alphaThreshold = program->getUniforms().findLocation("alphaThreshold");
    locations.texcoordMatrices = program->getUniforms().findLocation("texcoordMatrices");
    locations.emissiveParams = program->getUniforms().findLocation("emissiveParams");
    locations.glowIntensity = program->getUniforms().findLocation("glowIntensity");

    locations.specularTextureUnit = program->getTextures().findLocation("specularMap");
    locations.emissiveTextureUnit = program->getTextures().findLocation("emissiveMap");

#if (GPU_FEATURE_PROFILE == GPU_CORE)
    locations.materialBufferUnit = program->getBuffers().findLocation("materialBuffer");
    locations.lightBufferUnit = program->getBuffers().findLocation("lightBuffer");
#else
    locations.materialBufferUnit = program->getUniforms().findLocation("materialBuffer");
    locations.lightBufferUnit = program->getUniforms().findLocation("lightBuffer");
#endif
    locations.clusterMatrices = program->getUniforms().findLocation("clusterMatrices");

    locations.clusterIndices = program->getInputs().findLocation("clusterIndices");;
    locations.clusterWeights = program->getInputs().findLocation("clusterWeights");;
    


}

AbstractViewStateInterface* Model::_viewState = NULL;


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
    _snapModelToRegistrationPoint = false; 
    _snappedToRegistrationPoint = false; 
}

QVector<JointState> Model::createJointStates(const FBXGeometry& geometry) {
    QVector<JointState> jointStates;
    for (int i = 0; i < geometry.joints.size(); ++i) {
        const FBXJoint& joint = geometry.joints[i];
        // store a pointer to the FBXJoint in the JointState
        JointState state;
        state.setFBXJoint(&joint);

        jointStates.append(state);
    }
    return jointStates;
};

void Model::initJointTransforms() {
    // compute model transforms
    int numStates = _jointStates.size();
    for (int i = 0; i < numStates; ++i) {
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
    if (_renderPipelineLib.empty()) {
        // Vertex shaders
        auto modelVertex = gpu::ShaderPointer(gpu::Shader::createVertex(std::string(model_vert)));
        auto modelNormalMapVertex = gpu::ShaderPointer(gpu::Shader::createVertex(std::string(model_normal_map_vert)));
        auto modelLightmapVertex = gpu::ShaderPointer(gpu::Shader::createVertex(std::string(model_lightmap_vert)));
        auto modelLightmapNormalMapVertex = gpu::ShaderPointer(gpu::Shader::createVertex(std::string(model_lightmap_normal_map_vert)));
        auto modelShadowVertex = gpu::ShaderPointer(gpu::Shader::createVertex(std::string(model_shadow_vert)));
        auto skinModelVertex = gpu::ShaderPointer(gpu::Shader::createVertex(std::string(skin_model_vert)));
        auto skinModelNormalMapVertex = gpu::ShaderPointer(gpu::Shader::createVertex(std::string(skin_model_normal_map_vert)));
        auto skinModelShadowVertex = gpu::ShaderPointer(gpu::Shader::createVertex(std::string(skin_model_shadow_vert)));

        // Pixel shaders
        auto modelPixel = gpu::ShaderPointer(gpu::Shader::createPixel(std::string(model_frag)));
        auto modelNormalMapPixel = gpu::ShaderPointer(gpu::Shader::createPixel(std::string(model_normal_map_frag)));
        auto modelSpecularMapPixel = gpu::ShaderPointer(gpu::Shader::createPixel(std::string(model_specular_map_frag)));
        auto modelNormalSpecularMapPixel = gpu::ShaderPointer(gpu::Shader::createPixel(std::string(model_normal_specular_map_frag)));
        auto modelTranslucentPixel = gpu::ShaderPointer(gpu::Shader::createPixel(std::string(model_translucent_frag)));
        auto modelShadowPixel = gpu::ShaderPointer(gpu::Shader::createPixel(std::string(model_shadow_frag)));
        auto modelLightmapPixel = gpu::ShaderPointer(gpu::Shader::createPixel(std::string(model_lightmap_frag)));
        auto modelLightmapNormalMapPixel = gpu::ShaderPointer(gpu::Shader::createPixel(std::string(model_lightmap_normal_map_frag)));
        auto modelLightmapSpecularMapPixel = gpu::ShaderPointer(gpu::Shader::createPixel(std::string(model_lightmap_specular_map_frag)));
        auto modelLightmapNormalSpecularMapPixel = gpu::ShaderPointer(gpu::Shader::createPixel(std::string(model_lightmap_normal_specular_map_frag)));

        // Fill the renderPipelineLib
        
        _renderPipelineLib.addRenderPipeline(
            RenderKey(0),
            modelVertex, modelPixel);

        _renderPipelineLib.addRenderPipeline(
            RenderKey(RenderKey::HAS_TANGENTS),
            modelNormalMapVertex, modelNormalMapPixel);

        _renderPipelineLib.addRenderPipeline(
            RenderKey(RenderKey::HAS_SPECULAR),
            modelVertex, modelSpecularMapPixel);

        _renderPipelineLib.addRenderPipeline(
            RenderKey(RenderKey::HAS_TANGENTS | RenderKey::HAS_SPECULAR),
            modelNormalMapVertex, modelNormalSpecularMapPixel);

        
        _renderPipelineLib.addRenderPipeline(
             RenderKey(RenderKey::IS_TRANSLUCENT),
             modelVertex, modelTranslucentPixel);
        // FIXME Ignore lightmap for translucents meshpart
        _renderPipelineLib.addRenderPipeline(
             RenderKey(RenderKey::IS_TRANSLUCENT | RenderKey::HAS_LIGHTMAP),
             modelVertex, modelTranslucentPixel);
 
        _renderPipelineLib.addRenderPipeline(
            RenderKey(RenderKey::HAS_TANGENTS | RenderKey::IS_TRANSLUCENT),
            modelNormalMapVertex, modelTranslucentPixel);

        _renderPipelineLib.addRenderPipeline(
            RenderKey(RenderKey::HAS_SPECULAR | RenderKey::IS_TRANSLUCENT),
            modelVertex, modelTranslucentPixel);

        _renderPipelineLib.addRenderPipeline(
            RenderKey(RenderKey::HAS_TANGENTS | RenderKey::HAS_SPECULAR | RenderKey::IS_TRANSLUCENT),
            modelNormalMapVertex, modelTranslucentPixel);


        _renderPipelineLib.addRenderPipeline(
            RenderKey(RenderKey::HAS_LIGHTMAP),
            modelLightmapVertex, modelLightmapPixel);
        _renderPipelineLib.addRenderPipeline(
            RenderKey(RenderKey::HAS_LIGHTMAP | RenderKey::HAS_TANGENTS),
            modelLightmapNormalMapVertex, modelLightmapNormalMapPixel);

        _renderPipelineLib.addRenderPipeline(
            RenderKey(RenderKey::HAS_LIGHTMAP | RenderKey::HAS_SPECULAR),
            modelLightmapVertex, modelLightmapSpecularMapPixel);

        _renderPipelineLib.addRenderPipeline(
            RenderKey(RenderKey::HAS_LIGHTMAP | RenderKey::HAS_TANGENTS | RenderKey::HAS_SPECULAR),
            modelLightmapNormalMapVertex, modelLightmapNormalSpecularMapPixel);


        _renderPipelineLib.addRenderPipeline(
            RenderKey(RenderKey::IS_SKINNED),
            skinModelVertex, modelPixel);

        _renderPipelineLib.addRenderPipeline(
            RenderKey(RenderKey::IS_SKINNED | RenderKey::HAS_TANGENTS),
            skinModelNormalMapVertex, modelNormalMapPixel);

        _renderPipelineLib.addRenderPipeline(
            RenderKey(RenderKey::IS_SKINNED | RenderKey::HAS_SPECULAR),
            skinModelVertex, modelSpecularMapPixel);

        _renderPipelineLib.addRenderPipeline(
            RenderKey(RenderKey::IS_SKINNED | RenderKey::HAS_TANGENTS | RenderKey::HAS_SPECULAR),
            skinModelNormalMapVertex, modelNormalSpecularMapPixel);


        _renderPipelineLib.addRenderPipeline(
            RenderKey(RenderKey::IS_SKINNED | RenderKey::IS_TRANSLUCENT),
            skinModelVertex, modelTranslucentPixel);

        _renderPipelineLib.addRenderPipeline(
            RenderKey(RenderKey::IS_SKINNED | RenderKey::HAS_TANGENTS | RenderKey::IS_TRANSLUCENT),
            skinModelNormalMapVertex, modelTranslucentPixel);

        _renderPipelineLib.addRenderPipeline(
            RenderKey(RenderKey::IS_SKINNED | RenderKey::HAS_SPECULAR | RenderKey::IS_TRANSLUCENT),
            skinModelVertex, modelTranslucentPixel);

        _renderPipelineLib.addRenderPipeline(
            RenderKey(RenderKey::IS_SKINNED | RenderKey::HAS_TANGENTS | RenderKey::HAS_SPECULAR | RenderKey::IS_TRANSLUCENT),
            skinModelNormalMapVertex, modelTranslucentPixel);


        _renderPipelineLib.addRenderPipeline(
            RenderKey(RenderKey::IS_DEPTH_ONLY | RenderKey::IS_SHADOW),
            modelShadowVertex, modelShadowPixel);


        _renderPipelineLib.addRenderPipeline(
            RenderKey(RenderKey::IS_SKINNED | RenderKey::IS_DEPTH_ONLY | RenderKey::IS_SHADOW),
            skinModelShadowVertex, modelShadowPixel);
    }
}

void Model::reset() {
    if (_jointStates.isEmpty()) {
        return;
    }
    const FBXGeometry& geometry = _geometry->getFBXGeometry();
    for (int i = 0; i < _jointStates.size(); i++) {
        _jointStates[i].setRotationInConstrainedFrame(geometry.joints.at(i).rotation, 0.0f);
    }
    
    _meshGroupsKnown = false;
    _readyWhenAdded = false; // in case any of our users are using scenes
    invalidCalculatedMeshBoxes(); // if we have to reload, we need to assume our mesh boxes are all invalid
}

bool Model::updateGeometry() {
    PROFILE_RANGE(__FUNCTION__);
    bool needFullUpdate = false;

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
        setGeometry(geometry);
        
        _meshGroupsKnown = false;
        _readyWhenAdded = false; // in case any of our users are using scenes
        invalidCalculatedMeshBoxes(); // if we have to reload, we need to assume our mesh boxes are all invalid
        initJointStates(newJointStates);
        needToRebuild = true;
    } else if (_jointStates.isEmpty()) {
        const FBXGeometry& fbxGeometry = geometry->getFBXGeometry();
        if (fbxGeometry.joints.size() > 0) {
            initJointStates(createJointStates(fbxGeometry));
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
            
            auto buffer = std::make_shared<gpu::Buffer>();
            if (!mesh.blendshapes.isEmpty()) {
                buffer->resize((mesh.vertices.size() + mesh.normals.size()) * sizeof(glm::vec3));
                buffer->setSubData(0, mesh.vertices.size() * sizeof(glm::vec3), (gpu::Byte*) mesh.vertices.constData());
                buffer->setSubData(mesh.vertices.size() * sizeof(glm::vec3),
                    mesh.normals.size() * sizeof(glm::vec3), (gpu::Byte*) mesh.normals.constData());
            }
            _blendedVertexBuffers.push_back(buffer);
        }
        needFullUpdate = true;
    }
    return needFullUpdate;
}

// virtual
void Model::initJointStates(QVector<JointState> states) {
    _jointStates = states;
    initJointTransforms();

    int numStates = _jointStates.size();
    float radius = 0.0f;
    for (int i = 0; i < numStates; ++i) {
        float distance = glm::length(_jointStates[i].getPosition());
        if (distance > radius) {
            radius = distance;
        }
        _jointStates[i].buildConstraint();
    }
    for (int i = 0; i < _jointStates.size(); i++) {
        _jointStates[i].slaveVisibleTransform();
    }
    _boundingRadius = radius;
}

bool Model::findRayIntersectionAgainstSubMeshes(const glm::vec3& origin, const glm::vec3& direction, float& distance, 
                                                    BoxFace& face, QString& extraInfo, bool pickAgainstTriangles) {

    bool intersectedSomething = false;

    // if we aren't active, we can't ray pick yet...
    if (!isActive()) {
        return intersectedSomething;
    }
    
    // extents is the entity relative, scaled, centered extents of the entity
    glm::vec3 position = _translation;
    glm::mat4 rotation = glm::mat4_cast(_rotation);
    glm::mat4 translation = glm::translate(position);
    glm::mat4 modelToWorldMatrix = translation * rotation;
    glm::mat4 worldToModelMatrix = glm::inverse(modelToWorldMatrix);

    Extents modelExtents = getMeshExtents(); // NOTE: unrotated
    
    glm::vec3 dimensions = modelExtents.maximum - modelExtents.minimum;
    glm::vec3 corner = -(dimensions * _registrationPoint); // since we're going to do the ray picking in the model frame of reference
    AABox modelFrameBox(corner, dimensions);

    glm::vec3 modelFrameOrigin = glm::vec3(worldToModelMatrix * glm::vec4(origin, 1.0f));
    glm::vec3 modelFrameDirection = glm::vec3(worldToModelMatrix * glm::vec4(direction, 0.0f));

    // we can use the AABox's ray intersection by mapping our origin and direction into the model frame
    // and testing intersection there.
    if (modelFrameBox.findRayIntersection(modelFrameOrigin, modelFrameDirection, distance, face)) {
        float bestDistance = std::numeric_limits<float>::max();

        float distanceToSubMesh;
        BoxFace subMeshFace;
        int subMeshIndex = 0;

        const FBXGeometry& geometry = _geometry->getFBXGeometry();

        // If we hit the models box, then consider the submeshes...
        _mutex.lock();

        if (!_calculatedMeshBoxesValid) {
            recalculateMeshBoxes(pickAgainstTriangles);
        }

        foreach(const AABox& subMeshBox, _calculatedMeshBoxes) {

            if (subMeshBox.findRayIntersection(origin, direction, distanceToSubMesh, subMeshFace)) {
                if (distanceToSubMesh < bestDistance) {
                    if (pickAgainstTriangles) {
                        if (!_calculatedMeshTrianglesValid) {
                            recalculateMeshBoxes(pickAgainstTriangles);
                        }
                        // check our triangles here....
                        const QVector<Triangle>& meshTriangles = _calculatedMeshTriangles[subMeshIndex];
                        int t = 0;
                        foreach (const Triangle& triangle, meshTriangles) {
                            t++;
                        
                            float thisTriangleDistance;
                            if (findRayTriangleIntersection(origin, direction, triangle, thisTriangleDistance)) {
                                if (thisTriangleDistance < bestDistance) {
                                    bestDistance = thisTriangleDistance;
                                    intersectedSomething = true;
                                    face = subMeshFace;
                                    extraInfo = geometry.getModelNameOfMesh(subMeshIndex);
                                }
                            }
                        }
                    } else {
                        // this is the non-triangle picking case...
                        bestDistance = distanceToSubMesh;
                        intersectedSomething = true;
                        face = subMeshFace;
                        extraInfo = geometry.getModelNameOfMesh(subMeshIndex);
                    }
                }
            } 
            subMeshIndex++;
        }
        _mutex.unlock();

        if (intersectedSomething) {
            distance = bestDistance;
        }
        
        return intersectedSomething;
    }

    return intersectedSomething;
}

bool Model::convexHullContains(glm::vec3 point) {
    // if we aren't active, we can't compute that yet...
    if (!isActive()) {
        return false;
    }
    
    // extents is the entity relative, scaled, centered extents of the entity
    glm::vec3 position = _translation;
    glm::mat4 rotation = glm::mat4_cast(_rotation);
    glm::mat4 translation = glm::translate(position);
    glm::mat4 modelToWorldMatrix = translation * rotation;
    glm::mat4 worldToModelMatrix = glm::inverse(modelToWorldMatrix);
    
    Extents modelExtents = getMeshExtents(); // NOTE: unrotated
    
    glm::vec3 dimensions = modelExtents.maximum - modelExtents.minimum;
    glm::vec3 corner = -(dimensions * _registrationPoint);
    AABox modelFrameBox(corner, dimensions);
    
    glm::vec3 modelFramePoint = glm::vec3(worldToModelMatrix * glm::vec4(point, 1.0f));
    
    // we can use the AABox's contains() by mapping our point into the model frame
    // and testing there.
    if (modelFrameBox.contains(modelFramePoint)){
        _mutex.lock();
        if (!_calculatedMeshTrianglesValid) {
            recalculateMeshBoxes(true);
        }
        
        // If we are inside the models box, then consider the submeshes...
        int subMeshIndex = 0;
        foreach(const AABox& subMeshBox, _calculatedMeshBoxes) {
            if (subMeshBox.contains(point)) {
                bool insideMesh = true;
                // To be inside the sub mesh, we need to be behind every triangles' planes
                const QVector<Triangle>& meshTriangles = _calculatedMeshTriangles[subMeshIndex];
                foreach (const Triangle& triangle, meshTriangles) {
                    if (!isPointBehindTrianglesPlane(point, triangle.v0, triangle.v1, triangle.v2)) {
                        // it's not behind at least one so we bail
                        insideMesh = false;
                        break;
                    }
                    
                }
                if (insideMesh) {
                    // It's inside this mesh, return true.
                    _mutex.unlock();
                    return true;
                }
            }
            subMeshIndex++;
        }
        _mutex.unlock();
    }
    // It wasn't in any mesh, return false.
    return false;
}

void Model::recalculateMeshPartOffsets() {
    if (!_calculatedMeshPartOffsetValid) {
        const FBXGeometry& geometry = _geometry->getFBXGeometry();
        int numberOfMeshes = geometry.meshes.size();
        _calculatedMeshPartOffset.clear();
        for (int i = 0; i < numberOfMeshes; i++) {
            const FBXMesh& mesh = geometry.meshes.at(i);
            qint64 partOffset = 0;
            for (int j = 0; j < mesh.parts.size(); j++) {
                const FBXMeshPart& part = mesh.parts.at(j);
                _calculatedMeshPartOffset[QPair<int,int>(i, j)] = partOffset;
                partOffset += part.quadIndices.size() * sizeof(int);
                partOffset += part.triangleIndices.size() * sizeof(int);

            }
        }
        _calculatedMeshPartOffsetValid = true;
    }
}
// TODO: we seem to call this too often when things haven't actually changed... look into optimizing this
// Any script might trigger findRayIntersectionAgainstSubMeshes (and maybe convexHullContains), so these
// can occur multiple times. In addition, rendering does it's own ray picking in order to decide which
// entity-scripts to call.  I think it would be best to do the picking once-per-frame (in cpu, or gpu if possible)
// and then the calls use the most recent such result. 
void Model::recalculateMeshBoxes(bool pickAgainstTriangles) {
    PROFILE_RANGE(__FUNCTION__);
    bool calculatedMeshTrianglesNeeded = pickAgainstTriangles && !_calculatedMeshTrianglesValid;

    if (!_calculatedMeshBoxesValid || calculatedMeshTrianglesNeeded || (!_calculatedMeshPartBoxesValid && pickAgainstTriangles) ) {
        const FBXGeometry& geometry = _geometry->getFBXGeometry();
        int numberOfMeshes = geometry.meshes.size();
        _calculatedMeshBoxes.resize(numberOfMeshes);
        _calculatedMeshTriangles.clear();
        _calculatedMeshTriangles.resize(numberOfMeshes);
        _calculatedMeshPartBoxes.clear();
        _calculatedMeshPartOffset.clear();
        _calculatedMeshPartOffsetValid = false;
        for (int i = 0; i < numberOfMeshes; i++) {
            const FBXMesh& mesh = geometry.meshes.at(i);
            Extents scaledMeshExtents = calculateScaledOffsetExtents(mesh.meshExtents);

            _calculatedMeshBoxes[i] = AABox(scaledMeshExtents);

            if (pickAgainstTriangles) {
                QVector<Triangle> thisMeshTriangles;
                qint64 partOffset = 0;
                for (int j = 0; j < mesh.parts.size(); j++) {
                    const FBXMeshPart& part = mesh.parts.at(j);

                    bool atLeastOnePointInBounds = false;
                    AABox thisPartBounds;

                    const int INDICES_PER_TRIANGLE = 3;
                    const int INDICES_PER_QUAD = 4;

                    if (part.quadIndices.size() > 0) {
                        int numberOfQuads = part.quadIndices.size() / INDICES_PER_QUAD;
                        int vIndex = 0;
                        for (int q = 0; q < numberOfQuads; q++) {
                            int i0 = part.quadIndices[vIndex++];
                            int i1 = part.quadIndices[vIndex++];
                            int i2 = part.quadIndices[vIndex++];
                            int i3 = part.quadIndices[vIndex++];

                            glm::vec3 mv0 = glm::vec3(mesh.modelTransform * glm::vec4(mesh.vertices[i0], 1.0f));
                            glm::vec3 mv1 = glm::vec3(mesh.modelTransform * glm::vec4(mesh.vertices[i1], 1.0f));
                            glm::vec3 mv2 = glm::vec3(mesh.modelTransform * glm::vec4(mesh.vertices[i2], 1.0f));
                            glm::vec3 mv3 = glm::vec3(mesh.modelTransform * glm::vec4(mesh.vertices[i3], 1.0f));
                            
                            // track the mesh parts in model space
                            if (!atLeastOnePointInBounds) {
                                thisPartBounds.setBox(mv0, 0.0f);
                                atLeastOnePointInBounds = true;
                            } else {
                                thisPartBounds += mv0;
                            }
                            thisPartBounds += mv1;
                            thisPartBounds += mv2;
                            thisPartBounds += mv3;

                            glm::vec3 v0 = calculateScaledOffsetPoint(mv0);
                            glm::vec3 v1 = calculateScaledOffsetPoint(mv1);
                            glm::vec3 v2 = calculateScaledOffsetPoint(mv2);
                            glm::vec3 v3 = calculateScaledOffsetPoint(mv3);
                        
                            // Sam's recommended triangle slices
                            Triangle tri1 = { v0, v1, v3 };
                            Triangle tri2 = { v1, v2, v3 };
                        
                            // NOTE: Random guy on the internet's recommended triangle slices
                            //Triangle tri1 = { v0, v1, v2 };
                            //Triangle tri2 = { v2, v3, v0 };
                        
                            thisMeshTriangles.push_back(tri1);
                            thisMeshTriangles.push_back(tri2);
                            
                        }
                    }

                    if (part.triangleIndices.size() > 0) {
                        int numberOfTris = part.triangleIndices.size() / INDICES_PER_TRIANGLE;
                        int vIndex = 0;
                        for (int t = 0; t < numberOfTris; t++) {
                            int i0 = part.triangleIndices[vIndex++];
                            int i1 = part.triangleIndices[vIndex++];
                            int i2 = part.triangleIndices[vIndex++];

                            glm::vec3 mv0 = glm::vec3(mesh.modelTransform * glm::vec4(mesh.vertices[i0], 1.0f));
                            glm::vec3 mv1 = glm::vec3(mesh.modelTransform * glm::vec4(mesh.vertices[i1], 1.0f));
                            glm::vec3 mv2 = glm::vec3(mesh.modelTransform * glm::vec4(mesh.vertices[i2], 1.0f));

                            // track the mesh parts in model space
                            if (!atLeastOnePointInBounds) {
                                thisPartBounds.setBox(mv0, 0.0f);
                                atLeastOnePointInBounds = true;
                            } else {
                                thisPartBounds += mv0;
                            }
                            thisPartBounds += mv1;
                            thisPartBounds += mv2;

                            glm::vec3 v0 = calculateScaledOffsetPoint(mv0);
                            glm::vec3 v1 = calculateScaledOffsetPoint(mv1);
                            glm::vec3 v2 = calculateScaledOffsetPoint(mv2);

                            Triangle tri = { v0, v1, v2 };

                            thisMeshTriangles.push_back(tri);
                        }
                    }
                    _calculatedMeshPartBoxes[QPair<int,int>(i, j)] = thisPartBounds;
                    _calculatedMeshPartOffset[QPair<int,int>(i, j)] = partOffset;

                    partOffset += part.quadIndices.size() * sizeof(int);
                    partOffset += part.triangleIndices.size() * sizeof(int);

                }
                _calculatedMeshTriangles[i] = thisMeshTriangles;
                _calculatedMeshPartBoxesValid = true;
                _calculatedMeshPartOffsetValid = true;
            }
        }
        _calculatedMeshBoxesValid = true;
        _calculatedMeshTrianglesValid = pickAgainstTriangles;
    }
}

void Model::renderSetup(RenderArgs* args) {
    // set up dilated textures on first render after load/simulate
    const FBXGeometry& geometry = _geometry->getFBXGeometry();
    if (_dilatedTextures.isEmpty()) {
        foreach (const FBXMesh& mesh, geometry.meshes) {
            QVector<QSharedPointer<Texture> > dilated;
            dilated.resize(mesh.parts.size());
            _dilatedTextures.append(dilated);
        }
    }
    
    if (!_meshGroupsKnown && isLoaded()) {
        segregateMeshGroups();
    }
}


class MeshPartPayload {
public:
    MeshPartPayload(bool transparent, Model* model, int meshIndex, int partIndex) :
        transparent(transparent), model(model), url(model->getURL()), meshIndex(meshIndex), partIndex(partIndex) { }
    typedef render::Payload<MeshPartPayload> Payload;
    typedef Payload::DataPointer Pointer;
   
    bool transparent;
    Model* model;
    QUrl url;
    int meshIndex;
    int partIndex;
};

namespace render {
    template <> const ItemKey payloadGetKey(const MeshPartPayload::Pointer& payload) { 
        if (!payload->model->isVisible()) {
            return ItemKey::Builder().withInvisible().build();
        }
        return payload->transparent ? ItemKey::Builder::transparentShape() : ItemKey::Builder::opaqueShape();
    }
    
    template <> const Item::Bound payloadGetBound(const MeshPartPayload::Pointer& payload) { 
        if (payload) {
            return payload->model->getPartBounds(payload->meshIndex, payload->partIndex);
        }
        return render::Item::Bound();
    }
    template <> void payloadRender(const MeshPartPayload::Pointer& payload, RenderArgs* args) {
        if (args) {
            return payload->model->renderPart(args, payload->meshIndex, payload->partIndex, payload->transparent);
        }
    }

   /* template <> const model::MaterialKey& shapeGetMaterialKey(const MeshPartPayload::Pointer& payload) {
        return payload->model->getPartMaterial(payload->meshIndex, payload->partIndex);
    }*/
}

void Model::setVisibleInScene(bool newValue, std::shared_ptr<render::Scene> scene) {
    if (_isVisible != newValue) {
        _isVisible = newValue;

        render::PendingChanges pendingChanges;
        foreach (auto item, _renderItems.keys()) {
            pendingChanges.resetItem(item, _renderItems[item]);
        }
        scene->enqueuePendingChanges(pendingChanges);
    }
}


bool Model::addToScene(std::shared_ptr<render::Scene> scene, render::PendingChanges& pendingChanges) {
    if (!_meshGroupsKnown && isLoaded()) {
        segregateMeshGroups();
    }

    bool somethingAdded = false;

    foreach (auto renderItem, _transparentRenderItems) {
        auto item = scene->allocateID();
        auto renderData = MeshPartPayload::Pointer(renderItem);
        auto renderPayload = std::make_shared<MeshPartPayload::Payload>(renderData);
        pendingChanges.resetItem(item, renderPayload);
        _renderItems.insert(item, renderPayload);
        somethingAdded = true;
    }

    foreach (auto renderItem, _opaqueRenderItems) {
        auto item = scene->allocateID();
        auto renderData = MeshPartPayload::Pointer(renderItem);
        auto renderPayload = std::make_shared<MeshPartPayload::Payload>(renderData);
        pendingChanges.resetItem(item, renderPayload);
        _renderItems.insert(item, renderPayload);
        somethingAdded = true;
    }
    
    _readyWhenAdded = readyToAddToScene();

    return somethingAdded;
}

bool Model::addToScene(std::shared_ptr<render::Scene> scene, render::PendingChanges& pendingChanges, render::Item::Status::Getters& statusGetters) {
    if (!_meshGroupsKnown && isLoaded()) {
        segregateMeshGroups();
    }

    bool somethingAdded = false;

    foreach (auto renderItem, _transparentRenderItems) {
        auto item = scene->allocateID();
        auto renderData = MeshPartPayload::Pointer(renderItem);
        auto renderPayload = std::make_shared<MeshPartPayload::Payload>(renderData);
        renderPayload->addStatusGetters(statusGetters);
        pendingChanges.resetItem(item, renderPayload);
        _renderItems.insert(item, renderPayload);
        somethingAdded = true;
    }

    foreach (auto renderItem, _opaqueRenderItems) {
        auto item = scene->allocateID();
        auto renderData = MeshPartPayload::Pointer(renderItem);
        auto renderPayload = std::make_shared<MeshPartPayload::Payload>(renderData);
        renderPayload->addStatusGetters(statusGetters);
        pendingChanges.resetItem(item, renderPayload);
        _renderItems.insert(item, renderPayload);
        somethingAdded = true;
    }
    
    _readyWhenAdded = readyToAddToScene();

    return somethingAdded;
}

void Model::removeFromScene(std::shared_ptr<render::Scene> scene, render::PendingChanges& pendingChanges) {
    foreach (auto item, _renderItems.keys()) {
        pendingChanges.removeItem(item);
    }
    _renderItems.clear();
    _readyWhenAdded = false;
}

void Model::renderDebugMeshBoxes() {
    int colorNdx = 0;
    _mutex.lock();
    foreach(AABox box, _calculatedMeshBoxes) {
        if (_debugMeshBoxesID == GeometryCache::UNKNOWN_ID) {
            _debugMeshBoxesID = DependencyManager::get<GeometryCache>()->allocateID();
        }
        QVector<glm::vec3> points;
        
        glm::vec3 brn = box.getCorner();
        glm::vec3 bln = brn + glm::vec3(box.getDimensions().x, 0, 0);
        glm::vec3 brf = brn + glm::vec3(0, 0, box.getDimensions().z);
        glm::vec3 blf = brn + glm::vec3(box.getDimensions().x, 0, box.getDimensions().z);

        glm::vec3 trn = brn + glm::vec3(0, box.getDimensions().y, 0);
        glm::vec3 tln = bln + glm::vec3(0, box.getDimensions().y, 0);
        glm::vec3 trf = brf + glm::vec3(0, box.getDimensions().y, 0);
        glm::vec3 tlf = blf + glm::vec3(0, box.getDimensions().y, 0);

        points << brn << bln;
        points << brf << blf;
        points << brn << brf;
        points << bln << blf;

        points << trn << tln;
        points << trf << tlf;
        points << trn << trf;
        points << tln << tlf;

        points << brn << trn;
        points << brf << trf;
        points << bln << tln;
        points << blf << tlf;

        glm::vec4 color[] = {
            { 1.0f, 0.0f, 0.0f, 1.0f }, // red
            { 0.0f, 1.0f, 0.0f, 1.0f }, // green
            { 0.0f, 0.0f, 1.0f, 1.0f }, // blue
            { 1.0f, 0.0f, 1.0f, 1.0f }, // purple
            { 1.0f, 1.0f, 0.0f, 1.0f }, // yellow
            { 0.0f, 1.0f, 1.0f, 1.0f }, // cyan
            { 1.0f, 1.0f, 1.0f, 1.0f }, // white
            { 0.0f, 0.5f, 0.0f, 1.0f }, 
            { 0.0f, 0.0f, 0.5f, 1.0f }, 
            { 0.5f, 0.0f, 0.5f, 1.0f }, 
            { 0.5f, 0.5f, 0.0f, 1.0f }, 
            { 0.0f, 0.5f, 0.5f, 1.0f } };
            
        DependencyManager::get<GeometryCache>()->updateVertices(_debugMeshBoxesID, points, color[colorNdx]);
        DependencyManager::get<GeometryCache>()->renderVertices(gpu::LINES, _debugMeshBoxesID);
        colorNdx++;
    }
    _mutex.unlock();
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

Extents Model::calculateScaledOffsetExtents(const Extents& extents) const {
    // we need to include any fst scaling, translation, and rotation, which is captured in the offset matrix
    glm::vec3 minimum = glm::vec3(_geometry->getFBXGeometry().offset * glm::vec4(extents.minimum, 1.0f));
    glm::vec3 maximum = glm::vec3(_geometry->getFBXGeometry().offset * glm::vec4(extents.maximum, 1.0f));

    Extents scaledOffsetExtents = { ((minimum + _offset) * _scale), 
                                    ((maximum + _offset) * _scale) };

    Extents rotatedExtents = scaledOffsetExtents.getRotated(_rotation);

    Extents translatedExtents = { rotatedExtents.minimum + _translation, 
                                  rotatedExtents.maximum + _translation };

    return translatedExtents;
}

/// Returns the world space equivalent of some box in model space.
AABox Model::calculateScaledOffsetAABox(const AABox& box) const {
    return AABox(calculateScaledOffsetExtents(Extents(box)));
}

glm::vec3 Model::calculateScaledOffsetPoint(const glm::vec3& point) const {
    // we need to include any fst scaling, translation, and rotation, which is captured in the offset matrix
    glm::vec3 offsetPoint = glm::vec3(_geometry->getFBXGeometry().offset * glm::vec4(point, 1.0f));
    glm::vec3 scaledPoint = ((offsetPoint + _offset) * _scale);
    glm::vec3 rotatedPoint = _rotation * scaledPoint;
    glm::vec3 translatedPoint = rotatedPoint + _translation;
    return translatedPoint;
}


bool Model::getJointState(int index, glm::quat& rotation) const {
    if (index == -1 || index >= _jointStates.size()) {
        return false;
    }
    const JointState& state = _jointStates.at(index);
    rotation = state.getRotationInConstrainedFrame();
    return !state.rotationIsDefault(rotation);
}

bool Model::getVisibleJointState(int index, glm::quat& rotation) const {
    if (index == -1 || index >= _jointStates.size()) {
        return false;
    }
    const JointState& state = _jointStates.at(index);
    rotation = state.getVisibleRotationInConstrainedFrame();
    return !state.rotationIsDefault(rotation);
}

void Model::clearJointState(int index) {
    if (index != -1 && index < _jointStates.size()) {
        JointState& state = _jointStates[index];
        state.setRotationInConstrainedFrame(glm::quat(), 0.0f);
    }
}

void Model::clearJointAnimationPriority(int index) {
    if (index != -1 && index < _jointStates.size()) {
        _jointStates[index]._animationPriority = 0.0f;
    }
}

void Model::setJointState(int index, bool valid, const glm::quat& rotation, float priority) {
    if (index != -1 && index < _jointStates.size()) {
        JointState& state = _jointStates[index];
        if (valid) {
            state.setRotationInConstrainedFrame(rotation, priority);
        } else {
            state.restoreRotation(1.0f, priority);
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
    if (_url == url && _geometry && _geometry->getURL() == url) {
        return;
    }

    _readyWhenAdded = false; // reset out render items.
    _needsReload = true;
    invalidCalculatedMeshBoxes();

    _url = url;

    onInvalidate();

    // if so instructed, keep the current geometry until the new one is loaded 
    _nextGeometry = DependencyManager::get<GeometryCache>()->getGeometry(url, fallback, delayLoad);
    _nextLODHysteresis = NetworkGeometry::NO_HYSTERESIS;
    if (!retainCurrent || !isActive() || (_nextGeometry && _nextGeometry->isLoaded())) {
        applyNextGeometry();
    }
}

void Model::geometryRefreshed() {
    QObject* sender = QObject::sender();
    
    if (sender == _geometry) {
        _readyWhenAdded = false; // reset out render items.
        _needsReload = true;
        invalidCalculatedMeshBoxes();
        
        onInvalidate();
        
        // if so instructed, keep the current geometry until the new one is loaded
        _nextGeometry = DependencyManager::get<GeometryCache>()->getGeometry(_url);
        _nextLODHysteresis = NetworkGeometry::NO_HYSTERESIS;
        applyNextGeometry();
    } else {
        sender->disconnect(this, SLOT(geometryRefreshed()));
    }
}


const QSharedPointer<NetworkGeometry> Model::getCollisionGeometry(bool delayLoad)
{
    if (_collisionGeometry.isNull() && !_collisionUrl.isEmpty()) {
        _collisionGeometry = DependencyManager::get<GeometryCache>()->getGeometry(_collisionUrl, QUrl(), delayLoad);
    }

    if (_collisionGeometry && _collisionGeometry->isLoaded()) {
        return _collisionGeometry;
    }
    
    return QSharedPointer<NetworkGeometry>();
}

void Model::setCollisionModelURL(const QUrl& url) {
    if (_collisionUrl == url) {
        return;
    }
    _collisionUrl = url;
    _collisionGeometry = DependencyManager::get<GeometryCache>()->getGeometry(url, QUrl(), true);
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

bool Model::getJointRotation(int jointIndex, glm::quat& rotation) const {
    if (jointIndex == -1 || jointIndex >= _jointStates.size()) {
        return false;
    }
    rotation = _jointStates[jointIndex].getRotation();
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

    Blender(Model* model, int blendNumber, const QWeakPointer<NetworkGeometry>& geometry,
        const QVector<FBXMesh>& meshes, const QVector<float>& blendshapeCoefficients);
    
    virtual void run();

private:
    
    QPointer<Model> _model;
    int _blendNumber;
    QWeakPointer<NetworkGeometry> _geometry;
    QVector<FBXMesh> _meshes;
    QVector<float> _blendshapeCoefficients;
};

Blender::Blender(Model* model, int blendNumber, const QWeakPointer<NetworkGeometry>& geometry,
        const QVector<FBXMesh>& meshes, const QVector<float>& blendshapeCoefficients) :
    _model(model),
    _blendNumber(blendNumber),
    _geometry(geometry),
    _meshes(meshes),
    _blendshapeCoefficients(blendshapeCoefficients) {
}

void Blender::run() {
    PROFILE_RANGE(__FUNCTION__);
    QVector<glm::vec3> vertices, normals;
    if (!_model.isNull()) {
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
    }
    // post the result to the geometry cache, which will dispatch to the model if still alive
    QMetaObject::invokeMethod(DependencyManager::get<ModelBlender>().data(), "setBlendedVertices",
        Q_ARG(const QPointer<Model>&, _model), Q_ARG(int, _blendNumber),
        Q_ARG(const QWeakPointer<NetworkGeometry>&, _geometry), Q_ARG(const QVector<glm::vec3>&, vertices),
        Q_ARG(const QVector<glm::vec3>&, normals));
}

void Model::setScaleToFit(bool scaleToFit, const glm::vec3& dimensions) {
    if (_scaleToFit != scaleToFit || _scaleToFitDimensions != dimensions) {
        _scaleToFit = scaleToFit;
        _scaleToFitDimensions = dimensions;
        _scaledToFit = false; // force rescaling
    }
}

void Model::setScaleToFit(bool scaleToFit, float largestDimension, bool forceRescale) {
    // NOTE: if the model is not active, then it means we don't actually know the true/natural dimensions of the
    // mesh, and so we can't do the needed calculations for scaling to fit to a single largest dimension. In this
    // case we will record that we do want to do this, but we will stick our desired single dimension into the
    // first element of the vec3 for the non-fixed aspect ration dimensions
    if (!isActive()) {
        _scaleToFit = scaleToFit;
        if (scaleToFit) {
            _scaleToFitDimensions = glm::vec3(largestDimension, FAKE_DIMENSION_PLACEHOLDER, FAKE_DIMENSION_PLACEHOLDER);
        }
        return;
    }
    
    if (forceRescale || _scaleToFit != scaleToFit || glm::length(_scaleToFitDimensions) != largestDimension) {
        _scaleToFit = scaleToFit;
        
        // we only need to do this work if we're "turning on" scale to fit.
        if (scaleToFit) {
            Extents modelMeshExtents = getUnscaledMeshExtents();
            float maxDimension = glm::distance(modelMeshExtents.maximum, modelMeshExtents.minimum);
            float maxScale = largestDimension / maxDimension;
            glm::vec3 modelMeshDimensions = modelMeshExtents.maximum - modelMeshExtents.minimum;
            glm::vec3 dimensions = modelMeshDimensions * maxScale;

            _scaleToFitDimensions = dimensions;
            _scaledToFit = false; // force rescaling
        }
    }
}

void Model::scaleToFit() {
    // If our _scaleToFitDimensions.y/z are FAKE_DIMENSION_PLACEHOLDER then it means our
    // user asked to scale us in a fixed aspect ratio to a single largest dimension, but
    // we didn't yet have an active mesh. We can only enter this scaleToFit() in this state
    // if we now do have an active mesh, so we take this opportunity to actually determine
    // the correct scale.
    if (_scaleToFit && _scaleToFitDimensions.y == FAKE_DIMENSION_PLACEHOLDER 
            && _scaleToFitDimensions.z == FAKE_DIMENSION_PLACEHOLDER) {
        setScaleToFit(_scaleToFit, _scaleToFitDimensions.x);
    }
    Extents modelMeshExtents = getUnscaledMeshExtents();

    // size is our "target size in world space"
    // we need to set our model scale so that the extents of the mesh, fit in a cube that size...
    glm::vec3 meshDimensions = modelMeshExtents.maximum - modelMeshExtents.minimum;
    glm::vec3 rescaleDimensions = _scaleToFitDimensions / meshDimensions;
    setScaleInternal(rescaleDimensions);
    _scaledToFit = true;
}

void Model::setSnapModelToRegistrationPoint(bool snapModelToRegistrationPoint, const glm::vec3& registrationPoint) {
    glm::vec3 clampedRegistrationPoint = glm::clamp(registrationPoint, 0.0f, 1.0f);
    if (_snapModelToRegistrationPoint != snapModelToRegistrationPoint || _registrationPoint != clampedRegistrationPoint) {
        _snapModelToRegistrationPoint = snapModelToRegistrationPoint;
        _registrationPoint = clampedRegistrationPoint;
        _snappedToRegistrationPoint = false; // force re-centering
    }
}

void Model::snapToRegistrationPoint() {
    Extents modelMeshExtents = getUnscaledMeshExtents();
    glm::vec3 dimensions = (modelMeshExtents.maximum - modelMeshExtents.minimum);
    glm::vec3 offset = -modelMeshExtents.minimum - (dimensions * _registrationPoint);
    _offset = offset;
    _snappedToRegistrationPoint = true;
}

void Model::simulate(float deltaTime, bool fullUpdate) {
    PROFILE_RANGE(__FUNCTION__);
    fullUpdate = updateGeometry() || fullUpdate || (_scaleToFit && !_scaledToFit)
                    || (_snapModelToRegistrationPoint && !_snappedToRegistrationPoint);
                    
    if (isActive() && fullUpdate) {
        // NOTE: This is overly aggressive and we are invalidating the MeshBoxes when in fact they may not be invalid
        //       they really only become invalid if something about the transform to world space has changed. This is
        //       not too bad at this point, because it doesn't impact rendering. However it does slow down ray picking
        //       because ray picking needs valid boxes to work
        _calculatedMeshBoxesValid = false;
        _calculatedMeshTrianglesValid = false;
        onInvalidate();

        // check for scale to fit
        if (_scaleToFit && !_scaledToFit) {
            scaleToFit();
        }
        if (_snapModelToRegistrationPoint && !_snappedToRegistrationPoint) {
            snapToRegistrationPoint();
        }
        simulateInternal(deltaTime);
    }
}

void Model::updateClusterMatrices() {
    const FBXGeometry& geometry = _geometry->getFBXGeometry();
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
}

void Model::simulateInternal(float deltaTime) {
    // update the world space transforms for all joints
    
    // update animations
    foreach (const AnimationHandlePointer& handle, _runningAnimations) {
        handle->simulate(deltaTime);
    }

    for (int i = 0; i < _jointStates.size(); i++) {
        updateJointState(i);
    }
    for (int i = 0; i < _jointStates.size(); i++) {
        _jointStates[i].resetTransformChanged();
    }

    _shapesAreDirty = !_shapes.isEmpty();
    
    const FBXGeometry& geometry = _geometry->getFBXGeometry();
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
    
    // post the blender if we're not currently waiting for one to finish
    if (geometry.hasBlendedMeshes() && _blendshapeCoefficients != _blendedBlendshapeCoefficients) {
        _blendedBlendshapeCoefficients = _blendshapeCoefficients;
        DependencyManager::get<ModelBlender>()->noteRequiresBlend(this);
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
        // guard against out-of-bounds access to _jointStates
        if (joint.parentIndex >= 0 && joint.parentIndex < _jointStates.size()) {
            const JointState& parentState = _jointStates.at(parentIndex);
            state.computeTransform(parentState.getTransform(), parentState.getTransformChanged());
        }
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
                const glm::vec3 worldAlignment = glm::vec3(0.0f, -1.0f, 0.0f);
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
            // in the process.  This provides stability to the IK solution for most models.
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

void Model::renderJointCollisionShapes(float alpha) {
    // implement this when we have shapes for regular models
}

bool Model::maybeStartBlender() {
    const FBXGeometry& fbxGeometry = _geometry->getFBXGeometry();
    if (fbxGeometry.hasBlendedMeshes()) {
        QThreadPool::globalInstance()->start(new Blender(this, ++_blendNumber, _geometry,
            fbxGeometry.meshes, _blendshapeCoefficients));
        return true;
    }
    return false;
}

void Model::setBlendedVertices(int blendNumber, const QWeakPointer<NetworkGeometry>& geometry,
        const QVector<glm::vec3>& vertices, const QVector<glm::vec3>& normals) {
    if (_geometry != geometry || _blendedVertexBuffers.empty() || blendNumber < _appliedBlendNumber) {
        return;
    }
    _appliedBlendNumber = blendNumber;
    const FBXGeometry& fbxGeometry = _geometry->getFBXGeometry();    
    int index = 0;
    for (int i = 0; i < fbxGeometry.meshes.size(); i++) {
        const FBXMesh& mesh = fbxGeometry.meshes.at(i);
        if (mesh.blendshapes.isEmpty()) {
            continue;
        }

        gpu::BufferPointer& buffer = _blendedVertexBuffers[i];
        buffer->setSubData(0, mesh.vertices.size() * sizeof(glm::vec3), (gpu::Byte*) vertices.constData() + index*sizeof(glm::vec3));
        buffer->setSubData(mesh.vertices.size() * sizeof(glm::vec3),
            mesh.normals.size() * sizeof(glm::vec3), (gpu::Byte*) normals.constData() + index*sizeof(glm::vec3));

        index += mesh.vertices.size();
    }
}

void Model::setGeometry(const QSharedPointer<NetworkGeometry>& newGeometry) {
    if (_geometry == newGeometry) {
        return;
    }
    
    if (_geometry) {
        _geometry->disconnect(_geometry.data(), &Resource::onRefresh, this, &Model::geometryRefreshed);
    }
    _geometry = newGeometry;
    QObject::connect(_geometry.data(), &Resource::onRefresh, this, &Model::geometryRefreshed);
}

void Model::applyNextGeometry() {
    // delete our local geometry and custom textures
    deleteGeometry();
    _dilatedTextures.clear();
    _lodHysteresis = _nextLODHysteresis;
    
    // we retain a reference to the base geometry so that its reference count doesn't fall to zero
    setGeometry(_nextGeometry);
    
    _meshGroupsKnown = false;
    _readyWhenAdded = false; // in case any of our users are using scenes
    _needsReload = false; // we are loaded now!
    invalidCalculatedMeshBoxes();
    _nextGeometry.reset();
}

void Model::deleteGeometry() {
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
    
    _blendedBlendshapeCoefficients.clear();
}

AABox Model::getPartBounds(int meshIndex, int partIndex) {

    if (meshIndex < _meshStates.size()) {
        const MeshState& state = _meshStates.at(meshIndex);
        bool isSkinned = state.clusterMatrices.size() > 1;
        if (isSkinned) {
            // if we're skinned return the entire mesh extents because we can't know for sure our clusters don't move us
            return calculateScaledOffsetAABox(_geometry->getFBXGeometry().meshExtents);
        }
    }
    
    if (_geometry->getFBXGeometry().meshes.size() > meshIndex) {
        
        // FIX ME! - This is currently a hack because for some mesh parts our efforts to calculate the bounding
        //           box of the mesh part fails. It seems to create boxes that are not consistent with where the
        //           geometry actually renders. If instead we make all the parts share the bounds of the entire subMesh
        //           things will render properly.
        //
        //    return calculateScaledOffsetAABox(_calculatedMeshPartBoxes[QPair<int,int>(meshIndex, partIndex)]);
        //
        //    NOTE: we also don't want to use the _calculatedMeshBoxes[] because they don't handle avatar moving correctly
        //          without recalculating them...
        //    return _calculatedMeshBoxes[meshIndex];
        //
        // If we not skinned use the bounds of the subMesh for all it's parts
        const FBXMesh& mesh = _geometry->getFBXGeometry().meshes.at(meshIndex);
        return calculateScaledOffsetExtents(mesh.meshExtents);
    }
    return AABox();
}

void Model::renderPart(RenderArgs* args, int meshIndex, int partIndex, bool translucent) {
 //   PROFILE_RANGE(__FUNCTION__);
    PerformanceTimer perfTimer("Model::renderPart");
    if (!_readyWhenAdded) {
        return; // bail asap
    }
    
    // We need to make sure we have valid offsets calculated before we can render
    if (!_calculatedMeshPartOffsetValid) {
        _mutex.lock();
        recalculateMeshPartOffsets();
        _mutex.unlock();
    }
    auto textureCache = DependencyManager::get<TextureCache>();

    gpu::Batch& batch = *(args->_batch);
    auto mode = args->_renderMode;


    // Capture the view matrix once for the rendering of this model
    if (_transforms.empty()) {
        _transforms.push_back(Transform());
    }

    auto alphaThreshold = args->_alphaThreshold; //translucent ? TRANSPARENT_ALPHA_THRESHOLD : OPAQUE_ALPHA_THRESHOLD; // FIX ME

    const FBXGeometry& geometry = _geometry->getFBXGeometry();
    const QVector<NetworkMesh>& networkMeshes = _geometry->getMeshes();

    // guard against partially loaded meshes
    if (meshIndex >= networkMeshes.size() || meshIndex >= geometry.meshes.size() || meshIndex >= _meshStates.size() ) {
        return; 
    }

    const NetworkMesh& networkMesh = networkMeshes.at(meshIndex);
    const FBXMesh& mesh = geometry.meshes.at(meshIndex);
    const MeshState& state = _meshStates.at(meshIndex);
    
    bool translucentMesh = translucent; // networkMesh.getTranslucentPartCount(mesh) == networkMesh.parts.size();
    bool hasTangents = !mesh.tangents.isEmpty();
    bool hasSpecular = mesh.hasSpecularTexture();
    bool hasLightmap = mesh.hasEmissiveTexture();
    bool isSkinned = state.clusterMatrices.size() > 1;
    bool wireframe = isWireframe();

    // render the part bounding box
    #ifdef DEBUG_BOUNDING_PARTS
    {
        AABox partBounds = getPartBounds(meshIndex, partIndex);
        bool inView = args->_viewFrustum->boxInFrustum(partBounds) != ViewFrustum::OUTSIDE;

        glm::vec4 cubeColor;
        if (isSkinned) {
            cubeColor = glm::vec4(0.0f, 1.0f, 1.0f, 1.0f);
        } else if (inView) {
            cubeColor = glm::vec4(1.0f, 0.0f, 1.0f, 1.0f);
        } else {
            cubeColor = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
        }

        Transform transform;
        transform.setTranslation(partBounds.calcCenter());
        transform.setScale(partBounds.getDimensions());
        batch.setModelTransform(transform);
        DependencyManager::get<DeferredLightingEffect>()->renderWireCube(batch, 1.0f, cubeColor);
    }
    #endif //def DEBUG_BOUNDING_PARTS
    
    if (wireframe) {
        translucentMesh = hasTangents = hasSpecular = hasLightmap = isSkinned = false;
    }

    Locations* locations = nullptr;
    pickPrograms(batch, mode, translucentMesh, alphaThreshold, hasLightmap, hasTangents, hasSpecular, isSkinned, wireframe,
                 args, locations);

    {
        updateVisibleJointStates();
    }

    // if our index is ever out of range for either meshes or networkMeshes, then skip it, and set our _meshGroupsKnown
    // to false to rebuild out mesh groups.
    
    if (meshIndex < 0 || meshIndex >= networkMeshes.size() || meshIndex > geometry.meshes.size()) {
        _meshGroupsKnown = false; // regenerate these lists next time around.
        _readyWhenAdded = false; // in case any of our users are using scenes
        invalidCalculatedMeshBoxes(); // if we have to reload, we need to assume our mesh boxes are all invalid
        return; // FIXME!
    }
    
    batch.setIndexBuffer(gpu::UINT32, (networkMesh._indexBuffer), 0);
    int vertexCount = mesh.vertices.size();
    if (vertexCount == 0) {
        // sanity check
        return; // FIXME!
    }

    // Transform stage
    if (_transforms.empty()) {
        _transforms.push_back(Transform());
    }
    
    if (isSkinned) {
        batch._glUniformMatrix4fv(locations->clusterMatrices, state.clusterMatrices.size(), false,
            (const float*)state.clusterMatrices.constData());
       _transforms[0] = Transform();
       _transforms[0].preTranslate(_translation);
    } else {
       _transforms[0] = Transform(state.clusterMatrices[0]);
       _transforms[0].preTranslate(_translation);
    }
    batch.setModelTransform(_transforms[0]);

    if (mesh.blendshapes.isEmpty()) {
        batch.setInputFormat(networkMesh._vertexFormat);
        batch.setInputStream(0, *networkMesh._vertexStream);
    } else {
        batch.setInputFormat(networkMesh._vertexFormat);
        batch.setInputBuffer(0, _blendedVertexBuffers[meshIndex], 0, sizeof(glm::vec3));
        batch.setInputBuffer(1, _blendedVertexBuffers[meshIndex], vertexCount * sizeof(glm::vec3), sizeof(glm::vec3));
        batch.setInputStream(2, *networkMesh._vertexStream);
    }

    if (mesh.colors.isEmpty()) {
        batch._glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    }

    // guard against partially loaded meshes
    if (partIndex >= networkMesh.parts.size() || partIndex >= mesh.parts.size()) {
        return;
    }

    const NetworkMeshPart& networkPart = networkMesh.parts.at(partIndex);
    const FBXMeshPart& part = mesh.parts.at(partIndex);
    model::MaterialPointer material = part._material;

    #ifdef WANT_DEBUG
    if (material == nullptr) {
        qCDebug(renderutils) << "WARNING: material == nullptr!!!";
    }
    #endif
    
    if (material != nullptr) {

        // apply material properties
        if (mode != RenderArgs::SHADOW_RENDER_MODE) {
            #ifdef WANT_DEBUG
            qCDebug(renderutils) << "Material Changed ---------------------------------------------";
            qCDebug(renderutils) << "part INDEX:" << partIndex;
            qCDebug(renderutils) << "NEW part.materialID:" << part.materialID;
            #endif //def WANT_DEBUG

            if (locations->materialBufferUnit >= 0) {
                batch.setUniformBuffer(locations->materialBufferUnit, material->getSchemaBuffer());
            }

            Texture* diffuseMap = networkPart.diffuseTexture.data();
            if (mesh.isEye && diffuseMap) {
                // FIXME - guard against out of bounds here
                if (meshIndex < _dilatedTextures.size()) {
                    if (partIndex < _dilatedTextures[meshIndex].size()) {
                        diffuseMap = (_dilatedTextures[meshIndex][partIndex] =
                            static_cast<DilatableNetworkTexture*>(diffuseMap)->getDilatedTexture(_pupilDilation)).data();
                    }
                }
            }
            if (diffuseMap && static_cast<NetworkTexture*>(diffuseMap)->isLoaded()) {
                batch.setResourceTexture(0, diffuseMap->getGPUTexture());
            } else {
                batch.setResourceTexture(0, textureCache->getGrayTexture());
            }

            if (locations->texcoordMatrices >= 0) {
                glm::mat4 texcoordTransform[2];
                if (!part.diffuseTexture.transform.isIdentity()) {
                    part.diffuseTexture.transform.getMatrix(texcoordTransform[0]);
                }
                if (!part.emissiveTexture.transform.isIdentity()) {
                    part.emissiveTexture.transform.getMatrix(texcoordTransform[1]);
                }
                batch._glUniformMatrix4fv(locations->texcoordMatrices, 2, false, (const float*) &texcoordTransform);
            }

            if (!mesh.tangents.isEmpty()) {                 
                NetworkTexture* normalMap = networkPart.normalTexture.data();
                batch.setResourceTexture(1, (!normalMap || !normalMap->isLoaded()) ?
                                        textureCache->getBlueTexture() : normalMap->getGPUTexture());
            }
    
            if (locations->specularTextureUnit >= 0) {
                NetworkTexture* specularMap = networkPart.specularTexture.data();
                batch.setResourceTexture(locations->specularTextureUnit, (!specularMap || !specularMap->isLoaded()) ?
                                        textureCache->getBlackTexture() : specularMap->getGPUTexture());
            }

            if (args) {
                args->_details._materialSwitches++;
            }

            // HACK: For unknown reason (yet!) this code that should be assigned only if the material changes need to be called for every
            // drawcall with an emissive, so let's do it for now.
            if (locations->emissiveTextureUnit >= 0) {
                //  assert(locations->emissiveParams >= 0); // we should have the emissiveParams defined in the shader
                float emissiveOffset = part.emissiveParams.x;
                float emissiveScale = part.emissiveParams.y;
                batch._glUniform2f(locations->emissiveParams, emissiveOffset, emissiveScale);
                
                NetworkTexture* emissiveMap = networkPart.emissiveTexture.data();
                batch.setResourceTexture(locations->emissiveTextureUnit, (!emissiveMap || !emissiveMap->isLoaded()) ?
                                        textureCache->getGrayTexture() : emissiveMap->getGPUTexture());
            }
            
            if (translucent && locations->lightBufferUnit >= 0) {
                DependencyManager::get<DeferredLightingEffect>()->setupTransparent(args, locations->lightBufferUnit);
            }
        }
    }
    
    qint64 offset;
    {
        // FIXME_STUTTER: We should n't have any lock here
        _mutex.lock();
        offset = _calculatedMeshPartOffset[QPair<int,int>(meshIndex, partIndex)];
        _mutex.unlock();
    }

    if (part.quadIndices.size() > 0) {
        batch.drawIndexed(gpu::QUADS, part.quadIndices.size(), offset);
        offset += part.quadIndices.size() * sizeof(int);
    }

    if (part.triangleIndices.size() > 0) {
        batch.drawIndexed(gpu::TRIANGLES, part.triangleIndices.size(), offset);
        offset += part.triangleIndices.size() * sizeof(int);
    }

    if (args) {
        const int INDICES_PER_TRIANGLE = 3;
        const int INDICES_PER_QUAD = 4;
        args->_details._trianglesRendered += part.triangleIndices.size() / INDICES_PER_TRIANGLE;
        args->_details._quadsRendered += part.quadIndices.size() / INDICES_PER_QUAD;
    }
}

void Model::segregateMeshGroups() {
    const FBXGeometry& geometry = _geometry->getFBXGeometry();
    const QVector<NetworkMesh>& networkMeshes = _geometry->getMeshes();

    // all of our mesh vectors must match in size
    if (networkMeshes.size() != geometry.meshes.size() ||
        geometry.meshes.size() != _meshStates.size()) {
        qDebug() << "WARNING!!!! Mesh Sizes don't match! We will not segregate mesh groups yet.";
        return;
    }
    
    _transparentRenderItems.clear();
    _opaqueRenderItems.clear();

    // Run through all of the meshes, and place them into their segregated, but unsorted buckets
    for (int i = 0; i < networkMeshes.size(); i++) {
        const NetworkMesh& networkMesh = networkMeshes.at(i);
        const FBXMesh& mesh = geometry.meshes.at(i);
        const MeshState& state = _meshStates.at(i);
        

        bool translucentMesh = networkMesh.getTranslucentPartCount(mesh) == networkMesh.parts.size();
        bool hasTangents = !mesh.tangents.isEmpty();
        bool hasSpecular = mesh.hasSpecularTexture();
        bool hasLightmap = mesh.hasEmissiveTexture();
        bool isSkinned = state.clusterMatrices.size() > 1;
        bool wireframe = isWireframe();
        
        if (wireframe) {
            translucentMesh = hasTangents = hasSpecular = hasLightmap = isSkinned = false;
        }

        // Debug...
        int totalParts = mesh.parts.size();
        for (int partIndex = 0; partIndex < totalParts; partIndex++) {
            // this is a good place to create our renderPayloads
            if (translucentMesh) {
                _transparentRenderItems << std::make_shared<MeshPartPayload>(true, this, i, partIndex);
            } else {
                _opaqueRenderItems << std::make_shared<MeshPartPayload>(false, this, i, partIndex);
            }
        }
    }
    _meshGroupsKnown = true;
} 

void Model::pickPrograms(gpu::Batch& batch, RenderMode mode, bool translucent, float alphaThreshold,
                            bool hasLightmap, bool hasTangents, bool hasSpecular, bool isSkinned, bool isWireframe, RenderArgs* args,
                            Locations*& locations) {

    RenderKey key(mode, translucent, alphaThreshold, hasLightmap, hasTangents, hasSpecular, isSkinned, isWireframe);
    if (mode == RenderArgs::MIRROR_RENDER_MODE) {
        key = RenderKey(key.getRaw() | RenderKey::IS_MIRROR);
    }
    auto pipeline = _renderPipelineLib.find(key.getRaw());
    if (pipeline == _renderPipelineLib.end()) {
        qDebug() << "No good, couldn't find a pipeline from the key ?" << key.getRaw();
        locations = 0;
        return;
    }

    gpu::ShaderPointer program = (*pipeline).second._pipeline->getProgram();
    locations = (*pipeline).second._locations.get();

    
    // Setup the One pipeline
    batch.setPipeline((*pipeline).second._pipeline);

    if ((locations->alphaThreshold > -1) && (mode != RenderArgs::SHADOW_RENDER_MODE)) {
        batch._glUniform1f(locations->alphaThreshold, alphaThreshold);
    }

    if ((locations->glowIntensity > -1) && (mode != RenderArgs::SHADOW_RENDER_MODE)) {
        const float DEFAULT_GLOW_INTENSITY = 1.0f; // FIXME - glow is removed
        batch._glUniform1f(locations->glowIntensity, DEFAULT_GLOW_INTENSITY);
    }
}

bool Model::initWhenReady(render::ScenePointer scene) {
    if (isActive() && isRenderable() && !_meshGroupsKnown && isLoaded()) {
        segregateMeshGroups();

        render::PendingChanges pendingChanges;

        foreach (auto renderItem, _transparentRenderItems) {
            auto item = scene->allocateID();
            auto renderData = MeshPartPayload::Pointer(renderItem);
            auto renderPayload = std::make_shared<MeshPartPayload::Payload>(renderData);
            _renderItems.insert(item, renderPayload);
            pendingChanges.resetItem(item, renderPayload);
        }

        foreach (auto renderItem, _opaqueRenderItems) {
            auto item = scene->allocateID();
            auto renderData = MeshPartPayload::Pointer(renderItem);
            auto renderPayload = std::make_shared<MeshPartPayload::Payload>(renderData);
            _renderItems.insert(item, renderPayload);
            pendingChanges.resetItem(item, renderPayload);
        }
        scene->enqueuePendingChanges(pendingChanges);

        _readyWhenAdded = true;
        return true;
    }
    return false;
}

ModelBlender::ModelBlender() :
    _pendingBlenders(0) {
}

ModelBlender::~ModelBlender() {
}

void ModelBlender::noteRequiresBlend(Model* model) {
    if (_pendingBlenders < QThread::idealThreadCount()) {
        if (model->maybeStartBlender()) {
            _pendingBlenders++;
        }
        return;
    }
    if (!_modelsRequiringBlends.contains(model)) {
        _modelsRequiringBlends.append(model);
    }
}

void ModelBlender::setBlendedVertices(const QPointer<Model>& model, int blendNumber,
        const QWeakPointer<NetworkGeometry>& geometry, const QVector<glm::vec3>& vertices, const QVector<glm::vec3>& normals) {
    if (!model.isNull()) {
        model->setBlendedVertices(blendNumber, geometry, vertices, normals);
    }
    _pendingBlenders--;
    while (!_modelsRequiringBlends.isEmpty()) {
        Model* nextModel = _modelsRequiringBlends.takeFirst();
        if (nextModel && nextModel->maybeStartBlender()) {
            _pendingBlenders++;
            return;
        }
    }
}

