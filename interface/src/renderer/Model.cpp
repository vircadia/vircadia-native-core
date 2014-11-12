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
#include <PerfStat.h>
#include <PhysicsEntity.h>
#include <ShapeCollider.h>
#include <SphereShape.h>

#include "AnimationHandle.h"
#include "Application.h"
#include "Model.h"

#include "gpu/Batch.h"
#include "gpu/GLBackend.h"
#define GLBATCH( call ) batch._##call
//#define GLBATCH( call ) call

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
    _blendNumber(0),
    _appliedBlendNumber(0),
    _calculatedMeshBoxesValid(false),
    _meshGroupsKnown(false) {
    
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
ProgramObject Model::_translucentProgram;

ProgramObject Model::_shadowProgram;

ProgramObject Model::_skinProgram;
ProgramObject Model::_skinNormalMapProgram;
ProgramObject Model::_skinSpecularMapProgram;
ProgramObject Model::_skinNormalSpecularMapProgram;
ProgramObject Model::_skinTranslucentProgram;

ProgramObject Model::_skinShadowProgram;

Model::Locations Model::_locations;
Model::Locations Model::_normalMapLocations;
Model::Locations Model::_specularMapLocations;
Model::Locations Model::_normalSpecularMapLocations;
Model::Locations Model::_translucentLocations;

Model::SkinLocations Model::_skinLocations;
Model::SkinLocations Model::_skinNormalMapLocations;
Model::SkinLocations Model::_skinSpecularMapLocations;
Model::SkinLocations Model::_skinNormalSpecularMapLocations;
Model::SkinLocations Model::_skinShadowLocations;
Model::SkinLocations Model::_skinTranslucentLocations;

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

void Model::initProgram(ProgramObject& program, Model::Locations& locations, int specularTextureUnit) {
    program.bind();

#ifdef Q_OS_MAC

    // HACK: Assign explicitely the attribute channel to avoid a bug on Yosemite

    glBindAttribLocation(program.programId(), 4, "tangent");

    glLinkProgram(program.programId());

#endif



    glBindAttribLocation(program.programId(), gpu::Stream::TANGENT, "tangent");

    glLinkProgram(program.programId());

    locations.tangent = program.attributeLocation("tangent");

    locations.alphaThreshold = program.uniformLocation("alphaThreshold");

    program.setUniformValue("diffuseMap", 0);

    program.setUniformValue("normalMap", 1);

    program.setUniformValue("specularMap", specularTextureUnit);

    program.release();

}



void Model::initSkinProgram(ProgramObject& program, Model::SkinLocations& locations, int specularTextureUnit) {

    initProgram(program, locations, specularTextureUnit);



#ifdef Q_OS_MAC

    // HACK: Assign explicitely the attribute channel to avoid a bug on Yosemite

    glBindAttribLocation(program.programId(), 5, "clusterIndices");

    glBindAttribLocation(program.programId(), 6, "clusterWeights");

    glLinkProgram(program.programId());

#endif

    // HACK: Assign explicitely the attribute channel to avoid a bug on Yosemite

    glBindAttribLocation(program.programId(), gpu::Stream::SKIN_CLUSTER_INDEX, "clusterIndices");

    glBindAttribLocation(program.programId(), gpu::Stream::SKIN_CLUSTER_WEIGHT, "clusterWeights");

    glLinkProgram(program.programId());

    program.bind();

    locations.clusterMatrices = program.uniformLocation("clusterMatrices");



    locations.clusterIndices = program.attributeLocation("clusterIndices");

    locations.clusterWeights = program.attributeLocation("clusterWeights");

    program.release();
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
    if (!_program.isLinked()) {
        _program.addShaderFromSourceFile(QGLShader::Vertex, Application::resourcesPath() + "shaders/model.vert");
        _program.addShaderFromSourceFile(QGLShader::Fragment, Application::resourcesPath() + "shaders/model.frag");
        _program.link();
        
        initProgram(_program, _locations);
        
        _normalMapProgram.addShaderFromSourceFile(QGLShader::Vertex,
            Application::resourcesPath() + "shaders/model_normal_map.vert");
        _normalMapProgram.addShaderFromSourceFile(QGLShader::Fragment,
            Application::resourcesPath() + "shaders/model_normal_map.frag");
        _normalMapProgram.link();
        
        initProgram(_normalMapProgram, _normalMapLocations);
        
        _specularMapProgram.addShaderFromSourceFile(QGLShader::Vertex,
            Application::resourcesPath() + "shaders/model.vert");
        _specularMapProgram.addShaderFromSourceFile(QGLShader::Fragment,
            Application::resourcesPath() + "shaders/model_specular_map.frag");
        _specularMapProgram.link();
        
        initProgram(_specularMapProgram, _specularMapLocations);
        
        _normalSpecularMapProgram.addShaderFromSourceFile(QGLShader::Vertex,
            Application::resourcesPath() + "shaders/model_normal_map.vert");
        _normalSpecularMapProgram.addShaderFromSourceFile(QGLShader::Fragment,
            Application::resourcesPath() + "shaders/model_normal_specular_map.frag");
        _normalSpecularMapProgram.link();
        
        initProgram(_normalSpecularMapProgram, _normalSpecularMapLocations, 2);
        
        _translucentProgram.addShaderFromSourceFile(QGLShader::Vertex,
            Application::resourcesPath() + "shaders/model.vert");
        _translucentProgram.addShaderFromSourceFile(QGLShader::Fragment,
            Application::resourcesPath() + "shaders/model_translucent.frag");
        _translucentProgram.link();
        
        initProgram(_translucentProgram, _translucentLocations);
        
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
        
        _skinShadowProgram.addShaderFromSourceFile(QGLShader::Vertex,
            Application::resourcesPath() + "shaders/skin_model_shadow.vert");
        _skinShadowProgram.addShaderFromSourceFile(QGLShader::Fragment,
            Application::resourcesPath() + "shaders/model_shadow.frag");
        _skinShadowProgram.link();
        
        initSkinProgram(_skinShadowProgram, _skinShadowLocations);
        
        _skinTranslucentProgram.addShaderFromSourceFile(QGLShader::Vertex,
            Application::resourcesPath() + "shaders/skin_model.vert");
        _skinTranslucentProgram.addShaderFromSourceFile(QGLShader::Fragment,
            Application::resourcesPath() + "shaders/model_translucent.frag");
        _skinTranslucentProgram.link();
        
        initSkinProgram(_skinTranslucentProgram, _skinTranslucentLocations);
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
        _jointStates[i].setRotationInConstrainedFrame(geometry.joints.at(i).rotation, 0.0f);
    }
    
    _meshGroupsKnown = false;
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
        _meshGroupsKnown = false;
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

            gpu::BufferPointer buffer(new gpu::Buffer());
            if (!mesh.blendshapes.isEmpty()) {
                buffer->resize((mesh.vertices.size() + mesh.normals.size()) * sizeof(glm::vec3));
                buffer->setSubData(0, mesh.vertices.size() * sizeof(glm::vec3), (gpu::Resource::Byte*) mesh.vertices.constData());
                buffer->setSubData(mesh.vertices.size() * sizeof(glm::vec3),
                    mesh.normals.size() * sizeof(glm::vec3), (gpu::Resource::Byte*) mesh.normals.constData());
            }
            _blendedVertexBuffers.push_back(buffer);
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

bool Model::findRayIntersectionAgainstSubMeshes(const glm::vec3& origin, const glm::vec3& direction,
                                                        float& distance, BoxFace& face, QString& extraInfo) const {

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
    glm::vec3 corner = dimensions * -0.5f; // since we're going to do the ray picking in the model frame of reference
    AABox overlayFrameBox(corner, dimensions);

    glm::vec3 modelFrameOrigin = glm::vec3(worldToModelMatrix * glm::vec4(origin, 1.0f));
    glm::vec3 modelFrameDirection = glm::vec3(worldToModelMatrix * glm::vec4(direction, 0.0f));

    // we can use the AABox's ray intersection by mapping our origin and direction into the model frame
    // and testing intersection there.
    if (overlayFrameBox.findRayIntersection(modelFrameOrigin, modelFrameDirection, distance, face)) {

        float bestDistance = std::numeric_limits<float>::max();
        float distanceToSubMesh;
        BoxFace subMeshFace;
        int subMeshIndex = 0;
    
        // If we hit the models box, then consider the submeshes...
        foreach(const AABox& subMeshBox, _calculatedMeshBoxes) {
            const FBXGeometry& geometry = _geometry->getFBXGeometry();

            if (subMeshBox.findRayIntersection(origin, direction, distanceToSubMesh, subMeshFace)) {
                if (distanceToSubMesh < bestDistance) {
                    bestDistance = distanceToSubMesh;
                    intersectedSomething = true;
                    face = subMeshFace;
                    extraInfo = geometry.getModelNameOfMesh(subMeshIndex);
                }
            }
            subMeshIndex++;
        }
        
        return intersectedSomething;
    }

    return intersectedSomething;
}

void Model::recalcuateMeshBoxes() {
    if (!_calculatedMeshBoxesValid) {
        PerformanceTimer perfTimer("calculatedMeshBoxes");
        const FBXGeometry& geometry = _geometry->getFBXGeometry();
        int numberOfMeshes = geometry.meshes.size();
        _calculatedMeshBoxes.resize(numberOfMeshes);
        for (int i = 0; i < numberOfMeshes; i++) {
            const FBXMesh& mesh = geometry.meshes.at(i);
            Extents scaledMeshExtents = calculateScaledOffsetExtents(mesh.meshExtents);
            _calculatedMeshBoxes[i] = AABox(scaledMeshExtents);
        }
        _calculatedMeshBoxesValid = true;
    }
}

void Model::renderSetup(RenderArgs* args) {
    // if we don't have valid mesh boxes, calculate them now, this only matters in cases
    // where our caller has passed RenderArgs which will include a view frustum we can cull
    // against. We cache the results of these calculations so long as the model hasn't been
    // simulated and the mesh hasn't changed.
    if (args && !_calculatedMeshBoxesValid) {
        recalcuateMeshBoxes();
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
    
    if (!_meshGroupsKnown) {
        segregateMeshGroups();
    }
}

bool Model::render(float alpha, RenderMode mode, RenderArgs* args) {
    PROFILE_RANGE(__FUNCTION__);

    // render the attachments
    foreach (Model* attachment, _attachments) {
        attachment->render(alpha, mode);
    }
    if (_meshStates.isEmpty()) {
        return false;
    }

    renderSetup(args);
    return renderCore(alpha, mode, args);
}
    
bool Model::renderCore(float alpha, RenderMode mode, RenderArgs* args) {
    PROFILE_RANGE(__FUNCTION__);

    // Let's introduce a gpu::Batch to capture all the calls to the graphics api
    gpu::Batch batch;


    GLBATCH(glDisable)(GL_COLOR_MATERIAL);
    
    if (mode == DIFFUSE_RENDER_MODE || mode == NORMAL_RENDER_MODE) {
        GLBATCH(glDisable)(GL_CULL_FACE);
    } else {
        GLBATCH(glEnable)(GL_CULL_FACE);
        if (mode == SHADOW_RENDER_MODE) {
            GLBATCH(glCullFace)(GL_FRONT);
        }
    }
    
    // render opaque meshes with alpha testing

    GLBATCH(glDisable)(GL_BLEND);
    GLBATCH(glEnable)(GL_ALPHA_TEST);
    
    if (mode == SHADOW_RENDER_MODE) {
        GLBATCH(glAlphaFunc)(GL_EQUAL, 0.0f);
    }


    /*Application::getInstance()->getTextureCache()->setPrimaryDrawBuffers(
        mode == DEFAULT_RENDER_MODE || mode == DIFFUSE_RENDER_MODE,
        mode == DEFAULT_RENDER_MODE || mode == NORMAL_RENDER_MODE,
        mode == DEFAULT_RENDER_MODE);
        */
    {
        GLenum buffers[3];
        int bufferCount = 0;
        if (mode == DEFAULT_RENDER_MODE || mode == DIFFUSE_RENDER_MODE) {
            buffers[bufferCount++] = GL_COLOR_ATTACHMENT0;
        }
        if (mode == DEFAULT_RENDER_MODE || mode == NORMAL_RENDER_MODE) {
            buffers[bufferCount++] = GL_COLOR_ATTACHMENT1;
        }
        if (mode == DEFAULT_RENDER_MODE) {
            buffers[bufferCount++] = GL_COLOR_ATTACHMENT2;
        }
        GLBATCH(glDrawBuffers)(bufferCount, buffers);
    }

    const float DEFAULT_ALPHA_THRESHOLD = 0.5f;
    

    //renderMeshes(RenderMode mode, bool translucent, float alphaThreshold, bool hasTangents, bool hasSpecular, book isSkinned, args);
    int opaqueMeshPartsRendered = 0;
    opaqueMeshPartsRendered += renderMeshes(batch, mode, false, DEFAULT_ALPHA_THRESHOLD, false, false, false, args);
    opaqueMeshPartsRendered += renderMeshes(batch, mode, false, DEFAULT_ALPHA_THRESHOLD, false, false, true, args);
    opaqueMeshPartsRendered += renderMeshes(batch, mode, false, DEFAULT_ALPHA_THRESHOLD, false, true, false, args);
    opaqueMeshPartsRendered += renderMeshes(batch, mode, false, DEFAULT_ALPHA_THRESHOLD, false, true, true, args);
    opaqueMeshPartsRendered += renderMeshes(batch, mode, false, DEFAULT_ALPHA_THRESHOLD, true, false, false, args);
    opaqueMeshPartsRendered += renderMeshes(batch, mode, false, DEFAULT_ALPHA_THRESHOLD, true, false, true, args);
    opaqueMeshPartsRendered += renderMeshes(batch, mode, false, DEFAULT_ALPHA_THRESHOLD, true, true, false, args);
    opaqueMeshPartsRendered += renderMeshes(batch, mode, false, DEFAULT_ALPHA_THRESHOLD, true, false, true, args);

    // render translucent meshes afterwards
    //Application::getInstance()->getTextureCache()->setPrimaryDrawBuffers(false, true, true);
    {
        GLenum buffers[2];
        int bufferCount = 0;
        buffers[bufferCount++] = GL_COLOR_ATTACHMENT1;
        buffers[bufferCount++] = GL_COLOR_ATTACHMENT2;
        GLBATCH(glDrawBuffers)(bufferCount, buffers);
    }

    int translucentMeshPartsRendered = 0;
    const float MOSTLY_OPAQUE_THRESHOLD = 0.75f;
    translucentMeshPartsRendered += renderMeshes(batch, mode, true, MOSTLY_OPAQUE_THRESHOLD, false, false, false, args);
    translucentMeshPartsRendered += renderMeshes(batch, mode, true, MOSTLY_OPAQUE_THRESHOLD, false, false, true, args);
    translucentMeshPartsRendered += renderMeshes(batch, mode, true, MOSTLY_OPAQUE_THRESHOLD, false, true, false, args);
    translucentMeshPartsRendered += renderMeshes(batch, mode, true, MOSTLY_OPAQUE_THRESHOLD, false, true, true, args);
    translucentMeshPartsRendered += renderMeshes(batch, mode, true, MOSTLY_OPAQUE_THRESHOLD, true, false, false, args);
    translucentMeshPartsRendered += renderMeshes(batch, mode, true, MOSTLY_OPAQUE_THRESHOLD, true, false, true, args);
    translucentMeshPartsRendered += renderMeshes(batch, mode, true, MOSTLY_OPAQUE_THRESHOLD, true, true, false, args);
    translucentMeshPartsRendered += renderMeshes(batch, mode, true, MOSTLY_OPAQUE_THRESHOLD, true, false, true, args);

    GLBATCH(glDisable)(GL_ALPHA_TEST);
    GLBATCH(glEnable)(GL_BLEND);
    GLBATCH(glDepthMask)(false);
    GLBATCH(glDepthFunc)(GL_LEQUAL);
    
    //Application::getInstance()->getTextureCache()->setPrimaryDrawBuffers(true);
    {
        GLenum buffers[1];
        int bufferCount = 0;
        buffers[bufferCount++] = GL_COLOR_ATTACHMENT0;
        GLBATCH(glDrawBuffers)(bufferCount, buffers);
    }

    if (mode == DEFAULT_RENDER_MODE || mode == DIFFUSE_RENDER_MODE) {
        const float MOSTLY_TRANSPARENT_THRESHOLD = 0.0f;
        translucentMeshPartsRendered += renderMeshes(batch, mode, true, MOSTLY_TRANSPARENT_THRESHOLD, false, false, false, args);
        translucentMeshPartsRendered += renderMeshes(batch, mode, true, MOSTLY_TRANSPARENT_THRESHOLD, false, false, true, args);
        translucentMeshPartsRendered += renderMeshes(batch, mode, true, MOSTLY_TRANSPARENT_THRESHOLD, false, true, false, args);
        translucentMeshPartsRendered += renderMeshes(batch, mode, true, MOSTLY_TRANSPARENT_THRESHOLD, false, true, true, args);
        translucentMeshPartsRendered += renderMeshes(batch, mode, true, MOSTLY_TRANSPARENT_THRESHOLD, true, false, false, args);
        translucentMeshPartsRendered += renderMeshes(batch, mode, true, MOSTLY_TRANSPARENT_THRESHOLD, true, false, true, args);
        translucentMeshPartsRendered += renderMeshes(batch, mode, true, MOSTLY_TRANSPARENT_THRESHOLD, true, true, false, args);
        translucentMeshPartsRendered += renderMeshes(batch, mode, true, MOSTLY_TRANSPARENT_THRESHOLD, true, false, true, args);
    }

    GLBATCH(glDepthMask)(true);
    GLBATCH(glDepthFunc)(GL_LESS);
    GLBATCH(glDisable)(GL_CULL_FACE);
    
    if (mode == SHADOW_RENDER_MODE) {
        GLBATCH(glCullFace)(GL_BACK);
    }

    // deactivate vertex arrays after drawing
    GLBATCH(glDisableClientState)(GL_NORMAL_ARRAY);
    GLBATCH(glDisableClientState)(GL_VERTEX_ARRAY);
    GLBATCH(glDisableClientState)(GL_TEXTURE_COORD_ARRAY);
    GLBATCH(glDisableClientState)(GL_COLOR_ARRAY);
    GLBATCH(glDisableVertexAttribArray)(gpu::Stream::TANGENT);
    GLBATCH(glDisableVertexAttribArray)(gpu::Stream::SKIN_CLUSTER_INDEX);
    GLBATCH(glDisableVertexAttribArray)(gpu::Stream::SKIN_CLUSTER_WEIGHT);
    
    // bind with 0 to switch back to normal operation
    GLBATCH(glBindBuffer)(GL_ARRAY_BUFFER, 0);
    GLBATCH(glBindBuffer)(GL_ELEMENT_ARRAY_BUFFER, 0);
    GLBATCH(glBindTexture)(GL_TEXTURE_2D, 0);

    // Render!
    {
        PROFILE_RANGE("render Batch");
        ::gpu::GLBackend::renderBatch(batch);
        batch.clear();
    }

    // restore all the default material settings
    Application::getInstance()->setupWorldLight();
    
    if (args) {
        args->_translucentMeshPartsRendered = translucentMeshPartsRendered;
        args->_opaqueMeshPartsRendered = opaqueMeshPartsRendered;
    }

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
    QMetaObject::invokeMethod(Application::getInstance()->getGeometryCache(), "setBlendedVertices",
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

void Model::setScaleToFit(bool scaleToFit, float largestDimension) {
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
    
    if (_scaleToFit != scaleToFit || glm::length(_scaleToFitDimensions) != largestDimension) {
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
    fullUpdate = updateGeometry() || fullUpdate || (_scaleToFit && !_scaledToFit)
                    || (_snapModelToRegistrationPoint && !_snappedToRegistrationPoint);
                    
    if (isActive() && fullUpdate) {
        _calculatedMeshBoxesValid = false; // if we have to simulate, we need to assume our mesh boxes are all invalid

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
    for (int i = 0; i < _jointStates.size(); i++) {
        _jointStates[i].resetTransformChanged();
    }

    _shapesAreDirty = !_shapes.isEmpty();
    
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
    
    // post the blender if we're not currently waiting for one to finish
    if (geometry.hasBlendedMeshes() && _blendshapeCoefficients != _blendedBlendshapeCoefficients) {
        _blendedBlendshapeCoefficients = _blendshapeCoefficients;
        Application::getInstance()->getGeometryCache()->noteRequiresBlend(this);
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
        state.computeTransform(parentState.getTransform(), parentState.getTransformChanged());
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
        buffer->setSubData(0, mesh.vertices.size() * sizeof(glm::vec3), (gpu::Resource::Byte*) vertices.constData() + index*sizeof(glm::vec3));
        buffer->setSubData(mesh.vertices.size() * sizeof(glm::vec3),
            mesh.normals.size() * sizeof(glm::vec3), (gpu::Resource::Byte*) normals.constData() + index*sizeof(glm::vec3));

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
    _meshGroupsKnown = false;
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
    
    _blendedBlendshapeCoefficients.clear();
}

// Scene rendering support
QVector<Model*> Model::_modelsInScene;
void Model::startScene() {
    _modelsInScene.clear();
}

void Model::endScene(RenderMode mode, RenderArgs* args) {

    // first, do all the batch/GPU setup work....
    // Let's introduce a gpu::Batch to capture all the calls to the graphics api
    gpu::Batch batch;

    GLBATCH(glDisable)(GL_COLOR_MATERIAL);
    
    if (mode == DIFFUSE_RENDER_MODE || mode == NORMAL_RENDER_MODE) {
        GLBATCH(glDisable)(GL_CULL_FACE);
    } else {
        GLBATCH(glEnable)(GL_CULL_FACE);
        if (mode == SHADOW_RENDER_MODE) {
            GLBATCH(glCullFace)(GL_FRONT);
        }
    }
    
    // render opaque meshes with alpha testing

    GLBATCH(glDisable)(GL_BLEND);
    GLBATCH(glEnable)(GL_ALPHA_TEST);
    
    if (mode == SHADOW_RENDER_MODE) {
        GLBATCH(glAlphaFunc)(GL_EQUAL, 0.0f);
    }


    /*Application::getInstance()->getTextureCache()->setPrimaryDrawBuffers(
        mode == DEFAULT_RENDER_MODE || mode == DIFFUSE_RENDER_MODE,
        mode == DEFAULT_RENDER_MODE || mode == NORMAL_RENDER_MODE,
        mode == DEFAULT_RENDER_MODE);
        */
    {
        GLenum buffers[3];
        int bufferCount = 0;
        if (mode == DEFAULT_RENDER_MODE || mode == DIFFUSE_RENDER_MODE) {
            buffers[bufferCount++] = GL_COLOR_ATTACHMENT0;
        }
        if (mode == DEFAULT_RENDER_MODE || mode == NORMAL_RENDER_MODE) {
            buffers[bufferCount++] = GL_COLOR_ATTACHMENT1;
        }
        if (mode == DEFAULT_RENDER_MODE) {
            buffers[bufferCount++] = GL_COLOR_ATTACHMENT2;
        }
        GLBATCH(glDrawBuffers)(bufferCount, buffers);
    }

    const float DEFAULT_ALPHA_THRESHOLD = 0.5f;

    int opaqueMeshPartsRendered = 0;

    // now, for each model in the scene, render the mesh portions
    foreach(Model* model, _modelsInScene) {
        opaqueMeshPartsRendered += model->renderMeshes(batch, mode, false, DEFAULT_ALPHA_THRESHOLD, false, false, false, args);
    }
    foreach(Model* model, _modelsInScene) {
        opaqueMeshPartsRendered += model->renderMeshes(batch, mode, false, DEFAULT_ALPHA_THRESHOLD, false, false, true, args);
    }
    foreach(Model* model, _modelsInScene) {
        opaqueMeshPartsRendered += model->renderMeshes(batch, mode, false, DEFAULT_ALPHA_THRESHOLD, false, true, false, args);
    }
    foreach(Model* model, _modelsInScene) {
        opaqueMeshPartsRendered += model->renderMeshes(batch, mode, false, DEFAULT_ALPHA_THRESHOLD, false, true, true, args);
    }
    foreach(Model* model, _modelsInScene) {
        opaqueMeshPartsRendered += model->renderMeshes(batch, mode, false, DEFAULT_ALPHA_THRESHOLD, true, false, false, args);
    }
    foreach(Model* model, _modelsInScene) {
        opaqueMeshPartsRendered += model->renderMeshes(batch, mode, false, DEFAULT_ALPHA_THRESHOLD, true, false, true, args);
    }
    foreach(Model* model, _modelsInScene) {
        opaqueMeshPartsRendered += model->renderMeshes(batch, mode, false, DEFAULT_ALPHA_THRESHOLD, true, true, false, args);
    }
    foreach(Model* model, _modelsInScene) {
        opaqueMeshPartsRendered += model->renderMeshes(batch, mode, false, DEFAULT_ALPHA_THRESHOLD, true, false, true, args);
    }
    
    // render translucent meshes afterwards
    //Application::getInstance()->getTextureCache()->setPrimaryDrawBuffers(false, true, true);
    {
        GLenum buffers[2];
        int bufferCount = 0;
        buffers[bufferCount++] = GL_COLOR_ATTACHMENT1;
        buffers[bufferCount++] = GL_COLOR_ATTACHMENT2;
        GLBATCH(glDrawBuffers)(bufferCount, buffers);
    }

    int translucentParts = 0;
    const float MOSTLY_OPAQUE_THRESHOLD = 0.75f;
    foreach(Model* model, _modelsInScene) {
        translucentParts += model->renderMeshes(batch, mode, true, MOSTLY_OPAQUE_THRESHOLD, false, false, false, args);
    }
    foreach(Model* model, _modelsInScene) {
        translucentParts += model->renderMeshes(batch, mode, true, MOSTLY_OPAQUE_THRESHOLD, false, false, true, args);
    }
    foreach(Model* model, _modelsInScene) {
        translucentParts += model->renderMeshes(batch, mode, true, MOSTLY_OPAQUE_THRESHOLD, false, true, false, args);
    }
    foreach(Model* model, _modelsInScene) {
        translucentParts += model->renderMeshes(batch, mode, true, MOSTLY_OPAQUE_THRESHOLD, false, true, true, args);
    }
    foreach(Model* model, _modelsInScene) {
        translucentParts += model->renderMeshes(batch, mode, true, MOSTLY_OPAQUE_THRESHOLD, true, false, false, args);
    }
    foreach(Model* model, _modelsInScene) {
        translucentParts += model->renderMeshes(batch, mode, true, MOSTLY_OPAQUE_THRESHOLD, true, false, true, args);
    }
    foreach(Model* model, _modelsInScene) {
        translucentParts += model->renderMeshes(batch, mode, true, MOSTLY_OPAQUE_THRESHOLD, true, true, false, args);
    }
    foreach(Model* model, _modelsInScene) {
        translucentParts += model->renderMeshes(batch, mode, true, MOSTLY_OPAQUE_THRESHOLD, true, false, true, args);
    }
    
    GLBATCH(glDisable)(GL_ALPHA_TEST);
    GLBATCH(glEnable)(GL_BLEND);
    GLBATCH(glDepthMask)(false);
    GLBATCH(glDepthFunc)(GL_LEQUAL);
    
    //Application::getInstance()->getTextureCache()->setPrimaryDrawBuffers(true);
    {
        GLenum buffers[1];
        int bufferCount = 0;
        buffers[bufferCount++] = GL_COLOR_ATTACHMENT0;
        GLBATCH(glDrawBuffers)(bufferCount, buffers);
    }
    
    if (mode == DEFAULT_RENDER_MODE || mode == DIFFUSE_RENDER_MODE) {
        const float MOSTLY_TRANSPARENT_THRESHOLD = 0.0f;
        foreach(Model* model, _modelsInScene) {
            translucentParts += model->renderMeshes(batch, mode, true, MOSTLY_TRANSPARENT_THRESHOLD, false, false, false, args);
        }
        foreach(Model* model, _modelsInScene) {
            translucentParts += model->renderMeshes(batch, mode, true, MOSTLY_TRANSPARENT_THRESHOLD, false, false, true, args);
        }
        foreach(Model* model, _modelsInScene) {
            translucentParts += model->renderMeshes(batch, mode, true, MOSTLY_TRANSPARENT_THRESHOLD, false, true, false, args);
        }
        foreach(Model* model, _modelsInScene) {
            translucentParts += model->renderMeshes(batch, mode, true, MOSTLY_TRANSPARENT_THRESHOLD, false, true, true, args);
        }
        foreach(Model* model, _modelsInScene) {
            translucentParts += model->renderMeshes(batch, mode, true, MOSTLY_TRANSPARENT_THRESHOLD, true, false, false, args);
        }
        foreach(Model* model, _modelsInScene) {
            translucentParts += model->renderMeshes(batch, mode, true, MOSTLY_TRANSPARENT_THRESHOLD, true, false, true, args);
        }
        foreach(Model* model, _modelsInScene) {
            translucentParts += model->renderMeshes(batch, mode, true, MOSTLY_TRANSPARENT_THRESHOLD, true, true, false, args);
        }
        foreach(Model* model, _modelsInScene) {
            translucentParts += model->renderMeshes(batch, mode, true, MOSTLY_TRANSPARENT_THRESHOLD, true, false, true, args);
        }
    }

    GLBATCH(glDepthMask)(true);
    GLBATCH(glDepthFunc)(GL_LESS);
    GLBATCH(glDisable)(GL_CULL_FACE);
    
    if (mode == SHADOW_RENDER_MODE) {
        GLBATCH(glCullFace)(GL_BACK);
    }

    // deactivate vertex arrays after drawing
    GLBATCH(glDisableClientState)(GL_NORMAL_ARRAY);
    GLBATCH(glDisableClientState)(GL_VERTEX_ARRAY);
    GLBATCH(glDisableClientState)(GL_TEXTURE_COORD_ARRAY);
    GLBATCH(glDisableClientState)(GL_COLOR_ARRAY);
    GLBATCH(glDisableVertexAttribArray)(gpu::Stream::TANGENT);
    GLBATCH(glDisableVertexAttribArray)(gpu::Stream::SKIN_CLUSTER_INDEX);
    GLBATCH(glDisableVertexAttribArray)(gpu::Stream::SKIN_CLUSTER_WEIGHT);
    
    // bind with 0 to switch back to normal operation
    GLBATCH(glBindBuffer)(GL_ARRAY_BUFFER, 0);
    GLBATCH(glBindBuffer)(GL_ELEMENT_ARRAY_BUFFER, 0);
    GLBATCH(glBindTexture)(GL_TEXTURE_2D, 0);

    // Render!
    {
        PROFILE_RANGE("render Batch");
        ::gpu::GLBackend::renderBatch(batch);
        batch.clear();
    }

    // restore all the default material settings
    Application::getInstance()->setupWorldLight();
    
    if (args) {
        args->_translucentMeshPartsRendered = translucentParts;
        args->_opaqueMeshPartsRendered = opaqueMeshPartsRendered;
    }
}

bool Model::renderInScene(float alpha, RenderArgs* args) {
    // render the attachments
    foreach (Model* attachment, _attachments) {
        attachment->renderInScene(alpha);
    }
    if (_meshStates.isEmpty()) {
        return false;
    }
    renderSetup(args);
    _modelsInScene.push_back(this);
    return true;
}

void Model::segregateMeshGroups() {
    _meshesTranslucentTangents.clear();
    _meshesTranslucent.clear();
    _meshesTranslucentTangentsSpecular.clear();
    _meshesTranslucentSpecular.clear();

    _meshesTranslucentTangentsSkinned.clear();
    _meshesTranslucentSkinned.clear();
    _meshesTranslucentTangentsSpecularSkinned.clear();
    _meshesTranslucentSpecularSkinned.clear();

    _meshesOpaqueTangents.clear();
    _meshesOpaque.clear();
    _meshesOpaqueTangentsSpecular.clear();
    _meshesOpaqueSpecular.clear();

    _meshesOpaqueTangentsSkinned.clear();
    _meshesOpaqueSkinned.clear();
    _meshesOpaqueTangentsSpecularSkinned.clear();
    _meshesOpaqueSpecularSkinned.clear();
    
    _unsortedMeshesTranslucentTangents.clear();
    _unsortedMeshesTranslucent.clear();
    _unsortedMeshesTranslucentTangentsSpecular.clear();
    _unsortedMeshesTranslucentSpecular.clear();

    _unsortedMeshesTranslucentTangentsSkinned.clear();
    _unsortedMeshesTranslucentSkinned.clear();
    _unsortedMeshesTranslucentTangentsSpecularSkinned.clear();
    _unsortedMeshesTranslucentSpecularSkinned.clear();

    _unsortedMeshesOpaqueTangents.clear();
    _unsortedMeshesOpaque.clear();
    _unsortedMeshesOpaqueTangentsSpecular.clear();
    _unsortedMeshesOpaqueSpecular.clear();

    _unsortedMeshesOpaqueTangentsSkinned.clear();
    _unsortedMeshesOpaqueSkinned.clear();
    _unsortedMeshesOpaqueTangentsSpecularSkinned.clear();
    _unsortedMeshesOpaqueSpecularSkinned.clear();

    const FBXGeometry& geometry = _geometry->getFBXGeometry();
    const QVector<NetworkMesh>& networkMeshes = _geometry->getMeshes();


    // Run through all of the meshes, and place them into their segregated, but unsorted buckets
    for (int i = 0; i < networkMeshes.size(); i++) {
        const NetworkMesh& networkMesh = networkMeshes.at(i);
        const FBXMesh& mesh = geometry.meshes.at(i);
        const MeshState& state = _meshStates.at(i);

        bool translucentMesh = networkMesh.getTranslucentPartCount(mesh) == networkMesh.parts.size();
        bool hasTangents = !mesh.tangents.isEmpty();
        bool hasSpecular = mesh.hasSpecularTexture();
        bool isSkinned = state.clusterMatrices.size() > 1;
        QString materialID;

        // create a material name from all the parts. If there's one part, this will be a single material and its
        // true name. If however the mesh has multiple parts the name will be all the part's materials mashed together
        // which will result in those parts being sorted away from single material parts.
        QString lastPartMaterialID;
        foreach(FBXMeshPart part, mesh.parts) {
            if (part.materialID != lastPartMaterialID) {
                materialID += part.materialID;
            }
            lastPartMaterialID = part.materialID;
        }
        const bool wantDebug = false;
        if (wantDebug) {
            qDebug() << "materialID:" << materialID << "parts:" << mesh.parts.size();
        }

        if (translucentMesh && !hasTangents && !hasSpecular && !isSkinned) {

            _unsortedMeshesTranslucent.insertMulti(materialID, i);

        } else if (translucentMesh && hasTangents && !hasSpecular && !isSkinned) {

            _unsortedMeshesTranslucentTangents.insertMulti(materialID, i);

        } else if (translucentMesh && hasTangents && hasSpecular && !isSkinned) {

            _unsortedMeshesTranslucentTangentsSpecular.insertMulti(materialID, i);

        } else if (translucentMesh && !hasTangents && hasSpecular && !isSkinned) {

            _unsortedMeshesTranslucentSpecular.insertMulti(materialID, i);

        } else if (translucentMesh && hasTangents && !hasSpecular && isSkinned) {

            _unsortedMeshesTranslucentTangentsSkinned.insertMulti(materialID, i);

        } else if (translucentMesh && !hasTangents && !hasSpecular && isSkinned) {

            _unsortedMeshesTranslucentSkinned.insertMulti(materialID, i);

        } else if (translucentMesh && hasTangents && hasSpecular && isSkinned) {

            _unsortedMeshesTranslucentTangentsSpecularSkinned.insertMulti(materialID, i);

        } else if (translucentMesh && !hasTangents && hasSpecular && isSkinned) {

            _unsortedMeshesTranslucentSpecularSkinned.insertMulti(materialID, i);

        } else if (!translucentMesh && !hasTangents && !hasSpecular && !isSkinned) {

            _unsortedMeshesOpaque.insertMulti(materialID, i);

        } else if (!translucentMesh && hasTangents && !hasSpecular && !isSkinned) {

            _unsortedMeshesOpaqueTangents.insertMulti(materialID, i);

        } else if (!translucentMesh && hasTangents && hasSpecular && !isSkinned) {

            _unsortedMeshesOpaqueTangentsSpecular.insertMulti(materialID, i);

        } else if (!translucentMesh && !hasTangents && hasSpecular && !isSkinned) {

            _unsortedMeshesOpaqueSpecular.insertMulti(materialID, i);

        } else if (!translucentMesh && hasTangents && !hasSpecular && isSkinned) {

            _unsortedMeshesOpaqueTangentsSkinned.insertMulti(materialID, i);

        } else if (!translucentMesh && !hasTangents && !hasSpecular && isSkinned) {

            _unsortedMeshesOpaqueSkinned.insertMulti(materialID, i);

        } else if (!translucentMesh && hasTangents && hasSpecular && isSkinned) {

            _unsortedMeshesOpaqueTangentsSpecularSkinned.insertMulti(materialID, i);

        } else if (!translucentMesh && !hasTangents && hasSpecular && isSkinned) {

            _unsortedMeshesOpaqueSpecularSkinned.insertMulti(materialID, i);
        } else {
            qDebug() << "unexpected!!! this mesh didn't fall into any or our groups???";
        }
    }
    
    foreach(int i, _unsortedMeshesTranslucent) {
        _meshesTranslucent.append(i);
    }

    foreach(int i, _unsortedMeshesTranslucentTangents) {
        _meshesTranslucentTangents.append(i);
    }

    foreach(int i, _unsortedMeshesTranslucentTangentsSpecular) {
        _meshesTranslucentTangentsSpecular.append(i);
    }

    foreach(int i, _unsortedMeshesTranslucentSpecular) {
        _meshesTranslucentSpecular.append(i);
    }

    foreach(int i, _unsortedMeshesTranslucentSkinned) {
        _meshesTranslucentSkinned.append(i);
    }

    foreach(int i, _unsortedMeshesTranslucentTangentsSkinned) {
        _meshesTranslucentTangentsSkinned.append(i);
    }

    foreach(int i, _unsortedMeshesTranslucentTangentsSpecularSkinned) {
        _meshesTranslucentTangentsSpecularSkinned.append(i);
    }

    foreach(int i, _unsortedMeshesTranslucentSpecularSkinned) {
        _meshesTranslucentSpecularSkinned.append(i);
    }

    foreach(int i, _unsortedMeshesOpaque) {
        _meshesOpaque.append(i);
    }

    foreach(int i, _unsortedMeshesOpaqueTangents) {
        _meshesOpaqueTangents.append(i);
    }

    foreach(int i, _unsortedMeshesOpaqueTangentsSpecular) {
        _meshesOpaqueTangentsSpecular.append(i);
    }

    foreach(int i, _unsortedMeshesOpaqueSpecular) {
        _meshesOpaqueSpecular.append(i);
    }

    foreach(int i, _unsortedMeshesOpaqueSkinned) {
        _meshesOpaqueSkinned.append(i);
    }

    foreach(int i, _unsortedMeshesOpaqueTangentsSkinned) {
        _meshesOpaqueTangentsSkinned.append(i);
    }

    foreach(int i, _unsortedMeshesOpaqueTangentsSpecularSkinned) {
        _meshesOpaqueTangentsSpecularSkinned.append(i);
    }

    foreach(int i, _unsortedMeshesOpaqueSpecularSkinned) {
        _meshesOpaqueSpecularSkinned.append(i);
    }

    _unsortedMeshesTranslucentTangents.clear();
    _unsortedMeshesTranslucent.clear();
    _unsortedMeshesTranslucentTangentsSpecular.clear();
    _unsortedMeshesTranslucentSpecular.clear();

    _unsortedMeshesTranslucentTangentsSkinned.clear();
    _unsortedMeshesTranslucentSkinned.clear();
    _unsortedMeshesTranslucentTangentsSpecularSkinned.clear();
    _unsortedMeshesTranslucentSpecularSkinned.clear();

    _unsortedMeshesOpaqueTangents.clear();
    _unsortedMeshesOpaque.clear();
    _unsortedMeshesOpaqueTangentsSpecular.clear();
    _unsortedMeshesOpaqueSpecular.clear();

    _unsortedMeshesOpaqueTangentsSkinned.clear();
    _unsortedMeshesOpaqueSkinned.clear();
    _unsortedMeshesOpaqueTangentsSpecularSkinned.clear();
    _unsortedMeshesOpaqueSpecularSkinned.clear();

    _meshGroupsKnown = true;
}

int Model::renderMeshes(gpu::Batch& batch, RenderMode mode, bool translucent, float alphaThreshold,
                            bool hasTangents, bool hasSpecular, bool isSkinned, RenderArgs* args) {

    PROFILE_RANGE(__FUNCTION__);
    bool dontCullOutOfViewMeshParts = Menu::getInstance()->isOptionChecked(MenuOption::DontCullOutOfViewMeshParts);
    bool cullTooSmallMeshParts = !Menu::getInstance()->isOptionChecked(MenuOption::DontCullTooSmallMeshParts);
    bool dontReduceMaterialSwitches = Menu::getInstance()->isOptionChecked(MenuOption::DontReduceMaterialSwitches);
                            
    QString lastMaterialID;
    int meshPartsRendered = 0;
    updateVisibleJointStates();
    const FBXGeometry& geometry = _geometry->getFBXGeometry();
    const QVector<NetworkMesh>& networkMeshes = _geometry->getMeshes();

    // depending on which parameters we were called with, pick the correct mesh group to render
    QVector<int>* whichList = NULL;
    if (translucent && !hasTangents && !hasSpecular && !isSkinned) {
        whichList = &_meshesTranslucent;
    } else if (translucent && hasTangents && !hasSpecular && !isSkinned) {
        whichList = &_meshesTranslucentTangents;
    } else if (translucent && hasTangents && hasSpecular && !isSkinned) {
        whichList = &_meshesTranslucentTangentsSpecular;
    } else if (translucent && !hasTangents && hasSpecular && !isSkinned) {
        whichList = &_meshesTranslucentSpecular;
    } else if (translucent && hasTangents && !hasSpecular && isSkinned) {
        whichList = &_meshesTranslucentTangentsSkinned;
    } else if (translucent && !hasTangents && !hasSpecular && isSkinned) {
        whichList = &_meshesTranslucentSkinned;
    } else if (translucent && hasTangents && hasSpecular && isSkinned) {
        whichList = &_meshesTranslucentTangentsSpecularSkinned;
    } else if (translucent && !hasTangents && hasSpecular && isSkinned) {
        whichList = &_meshesTranslucentSpecularSkinned;
    } else if (!translucent && !hasTangents && !hasSpecular && !isSkinned) {
        whichList = &_meshesOpaque;
    } else if (!translucent && hasTangents && !hasSpecular && !isSkinned) {
        whichList = &_meshesOpaqueTangents;
    } else if (!translucent && hasTangents && hasSpecular && !isSkinned) {
        whichList = &_meshesOpaqueTangentsSpecular;
    } else if (!translucent && !hasTangents && hasSpecular && !isSkinned) {
        whichList = &_meshesOpaqueSpecular;
    } else if (!translucent && hasTangents && !hasSpecular && isSkinned) {
        whichList = &_meshesOpaqueTangentsSkinned;
    } else if (!translucent && !hasTangents && !hasSpecular && isSkinned) {
        whichList = &_meshesOpaqueSkinned;
    } else if (!translucent && hasTangents && hasSpecular && isSkinned) {
        whichList = &_meshesOpaqueTangentsSpecularSkinned;
    } else if (!translucent && !hasTangents && hasSpecular && isSkinned) {
        whichList = &_meshesOpaqueSpecularSkinned;
    } else {
        qDebug() << "unexpected!!! this mesh didn't fall into any or our groups???";
    }
    
    if (!whichList) {
        qDebug() << "unexpected!!! we don't know which list of meshes to render...";
        return 0;
    }
    QVector<int>& list = *whichList;

    // If this list has nothing to render, then don't bother proceeding. This saves us on binding to programs    
    if (list.size() == 0) {
        return 0;
    }

    ProgramObject* program = &_program;
    Locations* locations = &_locations;
    ProgramObject* skinProgram = &_skinProgram;
    SkinLocations* skinLocations = &_skinLocations;
    GLenum specularTextureUnit = 0;
    if (mode == SHADOW_RENDER_MODE) {
        program = &_shadowProgram;
        skinProgram = &_skinShadowProgram;
        skinLocations = &_skinShadowLocations;
    } else if (translucent && alphaThreshold == 0.0f) {
        program = &_translucentProgram;
        locations = &_translucentLocations;
        skinProgram = &_skinTranslucentProgram;
        skinLocations = &_skinTranslucentLocations;
        
    } else if (hasTangents) {
        if (hasSpecular) {
            program = &_normalSpecularMapProgram;
            locations = &_normalSpecularMapLocations;
            skinProgram = &_skinNormalSpecularMapProgram;
            skinLocations = &_skinNormalSpecularMapLocations;
            specularTextureUnit = GL_TEXTURE2;
        } else {
            program = &_normalMapProgram;
            locations = &_normalMapLocations;
            skinProgram = &_skinNormalMapProgram;
            skinLocations = &_skinNormalMapLocations;
        }
    } else if (hasSpecular) {
        program = &_specularMapProgram;
        locations = &_specularMapLocations;
        skinProgram = &_skinSpecularMapProgram;
        skinLocations = &_skinSpecularMapLocations;
        specularTextureUnit = GL_TEXTURE1;   
    }
    
    ProgramObject* activeProgram = program;
    Locations* activeLocations = locations;

    if (isSkinned) {
        activeProgram = skinProgram;
        activeLocations = skinLocations;
    }
    // This code replace the "bind()" on the QGLProgram
    if (!activeProgram->isLinked()) {
        activeProgram->link();
    }
    GLBATCH(glUseProgram)(activeProgram->programId());
    GLBATCH(glUniform1f)(activeLocations->alphaThreshold, alphaThreshold);

    // i is the "index" from the original networkMeshes QVector...
    foreach (int i, list) {
    
        // if our index is ever out of range for either meshes or networkMeshes, then skip it, and set our _meshGroupsKnown
        // to false to rebuild out mesh groups.
        
        if (i < 0 || i >= networkMeshes.size() || i > geometry.meshes.size()) {
            _meshGroupsKnown = false; // regenerate these lists next time around.
            continue;
        }
        
        // exit early if the translucency doesn't match what we're drawing
        const NetworkMesh& networkMesh = networkMeshes.at(i);
        const FBXMesh& mesh = geometry.meshes.at(i);    

        batch.setIndexBuffer(gpu::UINT32, (networkMesh._indexBuffer), 0);
        int vertexCount = mesh.vertices.size();
        if (vertexCount == 0) {
            // sanity check
            continue;
        }
        
        // if we got here, then check to see if this mesh is in view
        if (args) {
            bool shouldRender = true;
            args->_meshesConsidered++;

            if (args->_viewFrustum) {
                shouldRender = dontCullOutOfViewMeshParts || 
                                args->_viewFrustum->boxInFrustum(_calculatedMeshBoxes.at(i)) != ViewFrustum::OUTSIDE;
                if (shouldRender && cullTooSmallMeshParts) {
                    float distance = args->_viewFrustum->distanceToCamera(_calculatedMeshBoxes.at(i).calcCenter());
                    shouldRender = Menu::getInstance()->shouldRenderMesh(_calculatedMeshBoxes.at(i).getLargestDimension(), 
                                                                            distance);
                    if (!shouldRender) {
                        args->_meshesTooSmall++;
                    }
                } else {
                    args->_meshesOutOfView++;
                }
            }

            if (shouldRender) {
                args->_meshesRendered++;
            } else {
                continue; // skip this mesh
            }
        }

        GLBATCH(glPushMatrix)();
        //Application::getInstance()->loadTranslatedViewMatrix(_translation);
        GLBATCH(glLoadMatrixf)((const GLfloat*)&Application::getInstance()->getUntranslatedViewMatrix());
        glm::vec3 viewMatTranslation = Application::getInstance()->getViewMatrixTranslation();
        GLBATCH(glTranslatef)(_translation.x + viewMatTranslation.x, _translation.y + viewMatTranslation.y,
            _translation.z + viewMatTranslation.z);

        const MeshState& state = _meshStates.at(i);
        if (state.clusterMatrices.size() > 1) {
            GLBATCH(glUniformMatrix4fv)(skinLocations->clusterMatrices, state.clusterMatrices.size(), false,
                (const float*)state.clusterMatrices.constData());
        } else {
            GLBATCH(glMultMatrixf)((const GLfloat*)&state.clusterMatrices[0]);
        }
        
        if (mesh.blendshapes.isEmpty()) {
            batch.setInputFormat(networkMesh._vertexFormat);
            batch.setInputStream(0, *networkMesh._vertexStream);
        } else {
            batch.setInputFormat(networkMesh._vertexFormat);
            batch.setInputBuffer(0, _blendedVertexBuffers[i], 0, sizeof(glm::vec3));
            batch.setInputBuffer(1, _blendedVertexBuffers[i], vertexCount * sizeof(glm::vec3), sizeof(glm::vec3));
            batch.setInputStream(2, *networkMesh._vertexStream);
        }

        if (mesh.colors.isEmpty()) {
            GLBATCH(glColor4f)(1.0f, 1.0f, 1.0f, 1.0f);
        }

        qint64 offset = 0;
        for (int j = 0; j < networkMesh.parts.size(); j++) {
            const NetworkMeshPart& networkPart = networkMesh.parts.at(j);
            const FBXMeshPart& part = mesh.parts.at(j);
            if ((networkPart.isTranslucent() || part.opacity != 1.0f) != translucent) {
                offset += (part.quadIndices.size() + part.triangleIndices.size()) * sizeof(int);
                continue;
            }

            // apply material properties
            if (mode == SHADOW_RENDER_MODE) {
                GLBATCH(glBindTexture)(GL_TEXTURE_2D, 0);
                
            } else {
                if (dontReduceMaterialSwitches || lastMaterialID != part.materialID) {
                    const bool wantDebug = false;
                    if (wantDebug) {
                        qDebug() << "Material Changed ---------------------------------------------";
                        qDebug() << "part INDEX:" << j;
                        qDebug() << "NEW part.materialID:" << part.materialID;
                    }

                    glm::vec4 diffuse = glm::vec4(part.diffuseColor, part.opacity);
                    if (!(translucent && alphaThreshold == 0.0f)) {
                        GLBATCH(glAlphaFunc)(GL_EQUAL, diffuse.a = Application::getInstance()->getGlowEffect()->getIntensity());
                    }
                    glm::vec4 specular = glm::vec4(part.specularColor, 1.0f);
                    GLBATCH(glMaterialfv)(GL_FRONT, GL_AMBIENT, (const float*)&diffuse);
                    GLBATCH(glMaterialfv)(GL_FRONT, GL_DIFFUSE, (const float*)&diffuse);
                    GLBATCH(glMaterialfv)(GL_FRONT, GL_SPECULAR, (const float*)&specular);
                    GLBATCH(glMaterialf)(GL_FRONT, GL_SHININESS, (part.shininess > 128.f ? 128.f: part.shininess));
            
                    Texture* diffuseMap = networkPart.diffuseTexture.data();
                    if (mesh.isEye && diffuseMap) {
                        diffuseMap = (_dilatedTextures[i][j] =
                            static_cast<DilatableNetworkTexture*>(diffuseMap)->getDilatedTexture(_pupilDilation)).data();
                    }
                    GLBATCH(glBindTexture)(GL_TEXTURE_2D, !diffuseMap ?
                        Application::getInstance()->getTextureCache()->getWhiteTextureID() : diffuseMap->getID());
                
                    if (!mesh.tangents.isEmpty()) {                 
                        GLBATCH(glActiveTexture)(GL_TEXTURE1);
                        Texture* normalMap = networkPart.normalTexture.data();
                        GLBATCH(glBindTexture)(GL_TEXTURE_2D, !normalMap ?
                            Application::getInstance()->getTextureCache()->getBlueTextureID() : normalMap->getID());
                        GLBATCH(glActiveTexture)(GL_TEXTURE0);
                    }
                
                    if (specularTextureUnit) {
                        GLBATCH(glActiveTexture)(specularTextureUnit);
                        Texture* specularMap = networkPart.specularTexture.data();
                        GLBATCH(glBindTexture)(GL_TEXTURE_2D, !specularMap ?
                            Application::getInstance()->getTextureCache()->getWhiteTextureID() : specularMap->getID());
                        GLBATCH(glActiveTexture)(GL_TEXTURE0);
                    }
                    if (args) {
                        args->_materialSwitches++;
                    }

                }
                lastMaterialID = part.materialID;
            }
            
            meshPartsRendered++;
            
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
                args->_trianglesRendered += part.triangleIndices.size() / INDICES_PER_TRIANGLE;
                args->_quadsRendered += part.quadIndices.size() / INDICES_PER_QUAD;
            }
        }

        if (!(mesh.tangents.isEmpty() || mode == SHADOW_RENDER_MODE)) {
            GLBATCH(glActiveTexture)(GL_TEXTURE1);
            GLBATCH(glBindTexture)(GL_TEXTURE_2D, 0);
            GLBATCH(glActiveTexture)(GL_TEXTURE0);
        }

        if (specularTextureUnit) {
            GLBATCH(glActiveTexture)(specularTextureUnit);
            GLBATCH(glBindTexture)(GL_TEXTURE_2D, 0);
            GLBATCH(glActiveTexture)(GL_TEXTURE0);
        }

        GLBATCH(glPopMatrix)();

    }

    GLBATCH(glUseProgram)(0);

    return meshPartsRendered;
}
