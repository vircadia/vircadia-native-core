//
//  Model.cpp
//  interface
//
//  Created by Andrzej Kapolka on 10/18/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
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
    _shapesAreDirty(true),
    _boundingRadius(0.f),
    _boundingShape(), 
    _boundingShapeLocalOffset(0.f),
    _lodDistance(0.0f),
    _pupilDilation(0.0f) {
    // we may have been created in the network thread, but we live in the main thread
    moveToThread(Application::getInstance()->thread());
}

Model::~Model() {
    deleteGeometry();
}

ProgramObject Model::_program;
ProgramObject Model::_normalMapProgram;
ProgramObject Model::_shadowProgram;
ProgramObject Model::_skinProgram;
ProgramObject Model::_skinNormalMapProgram;
ProgramObject Model::_skinShadowProgram;
int Model::_normalMapTangentLocation;
Model::SkinLocations Model::_skinLocations;
Model::SkinLocations Model::_skinNormalMapLocations;
Model::SkinLocations Model::_skinShadowLocations;

void Model::setScale(const glm::vec3& scale) {
    glm::vec3 deltaScale = _scale - scale;
    if (glm::length2(deltaScale) > EPSILON) {
        _scale = scale;
        rebuildShapes();
    }
}

void Model::initSkinProgram(ProgramObject& program, Model::SkinLocations& locations) {
    program.bind();
    locations.clusterMatrices = program.uniformLocation("clusterMatrices");
    locations.clusterIndices = program.attributeLocation("clusterIndices");
    locations.clusterWeights = program.attributeLocation("clusterWeights");
    locations.tangent = program.attributeLocation("tangent");
    program.setUniformValue("diffuseMap", 0);
    program.setUniformValue("normalMap", 1);
    program.release();
}

QVector<Model::JointState> Model::createJointStates(const FBXGeometry& geometry) {
    QVector<JointState> jointStates;
    foreach (const FBXJoint& joint, geometry.joints) {
        JointState state;
        state.translation = joint.translation;
        state.rotation = joint.rotation;
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
            const FBXJoint& joint = geometry.joints[i];
            int parentIndex = joint.parentIndex;
            if (parentIndex == -1) {
                glm::mat4 baseTransform = glm::mat4_cast(_rotation) * glm::scale(_scale) * glm::translate(_offset);
                glm::quat combinedRotation = joint.preRotation * state.rotation * joint.postRotation;    
                state.transform = baseTransform * geometry.offset * glm::translate(state.translation) * joint.preTransform *
                    glm::mat4_cast(combinedRotation) * joint.postTransform;
                state.combinedRotation = _rotation * combinedRotation;
                ++numJointsSet;
                jointIsSet[i] = true;
            } else if (jointIsSet[parentIndex]) {
                const JointState& parentState = jointStates.at(parentIndex);
                glm::quat combinedRotation = joint.preRotation * state.rotation * joint.postRotation;    
                state.transform = parentState.transform * glm::translate(state.translation) * joint.preTransform *
                    glm::mat4_cast(combinedRotation) * joint.postTransform;
                state.combinedRotation = parentState.combinedRotation * combinedRotation;
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
        _program.setUniformValue("texture", 0);
        _program.release();
        
        _normalMapProgram.addShaderFromSourceFile(QGLShader::Vertex, Application::resourcesPath()
                                                  + "shaders/model_normal_map.vert");
        _normalMapProgram.addShaderFromSourceFile(QGLShader::Fragment, Application::resourcesPath()
                                                  + "shaders/model_normal_map.frag");
        _normalMapProgram.link();
        
        _normalMapProgram.bind();
        _normalMapProgram.setUniformValue("diffuseMap", 0);
        _normalMapProgram.setUniformValue("normalMap", 1);
        _normalMapTangentLocation = _normalMapProgram.attributeLocation("tangent");
        _normalMapProgram.release();
        
        _shadowProgram.addShaderFromSourceFile(QGLShader::Vertex, Application::resourcesPath() + "shaders/model_shadow.vert");
        _shadowProgram.addShaderFromSourceFile(QGLShader::Fragment, Application::resourcesPath() +
            "shaders/model_shadow.frag");
        _shadowProgram.link();
        
        _skinProgram.addShaderFromSourceFile(QGLShader::Vertex, Application::resourcesPath()
                                             + "shaders/skin_model.vert");
        _skinProgram.addShaderFromSourceFile(QGLShader::Fragment, Application::resourcesPath()
                                             + "shaders/model.frag");
        _skinProgram.link();
        
        initSkinProgram(_skinProgram, _skinLocations);
        
        _skinNormalMapProgram.addShaderFromSourceFile(QGLShader::Vertex, Application::resourcesPath()
                                                      + "shaders/skin_model_normal_map.vert");
        _skinNormalMapProgram.addShaderFromSourceFile(QGLShader::Fragment, Application::resourcesPath()
                                                      + "shaders/model_normal_map.frag");
        _skinNormalMapProgram.link();
        
        initSkinProgram(_skinNormalMapProgram, _skinNormalMapLocations);
        
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
        _jointStates[i].rotation = geometry.joints.at(i).rotation;
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

bool Model::render(float alpha, RenderMode mode) {
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
    }
    
    // render opaque meshes with alpha testing
    
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.5f * alpha);
    
    renderMeshes(alpha, mode, false);
    
    glDisable(GL_ALPHA_TEST);
    
    // render translucent meshes afterwards
    
    renderMeshes(alpha, mode, true);
    
    glDisable(GL_CULL_FACE);
    
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

bool Model::getJointState(int index, glm::quat& rotation) const {
    if (index == -1 || index >= _jointStates.size()) {
        return false;
    }
    rotation = _jointStates.at(index).rotation;
    const glm::quat& defaultRotation = _geometry->getFBXGeometry().joints.at(index).rotation;
    return glm::abs(rotation.x - defaultRotation.x) >= EPSILON ||
        glm::abs(rotation.y - defaultRotation.y) >= EPSILON ||
        glm::abs(rotation.z - defaultRotation.z) >= EPSILON ||
        glm::abs(rotation.w - defaultRotation.w) >= EPSILON;
}

void Model::setJointState(int index, bool valid, const glm::quat& rotation) {
    if (index != -1 && index < _jointStates.size()) {
        _jointStates[index].rotation = valid ? rotation : _geometry->getFBXGeometry().joints.at(index).rotation;
    }
}

int Model::getParentJointIndex(int jointIndex) const {
    return (isActive() && jointIndex != -1) ? _geometry->getFBXGeometry().joints.at(jointIndex).parentIndex : -1;
}

int Model::getLastFreeJointIndex(int jointIndex) const {
    return (isActive() && jointIndex != -1) ? _geometry->getFBXGeometry().joints.at(jointIndex).freeLineage.last() : -1;
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
    
bool Model::getLeftHandPosition(glm::vec3& position) const {
    return getJointPosition(getLeftHandJointIndex(), position);
}

bool Model::getLeftHandRotation(glm::quat& rotation) const {
    return getJointRotation(getLeftHandJointIndex(), rotation);
}

bool Model::getRightHandPosition(glm::vec3& position) const {
    return getJointPosition(getRightHandJointIndex(), position);
}

bool Model::getRightHandRotation(glm::quat& rotation) const {
    return getJointRotation(getRightHandJointIndex(), rotation);
}

bool Model::restoreLeftHandPosition(float percent) {
    return restoreJointPosition(getLeftHandJointIndex(), percent);
}

bool Model::getLeftShoulderPosition(glm::vec3& position) const {
    return getJointPosition(getLastFreeJointIndex(getLeftHandJointIndex()), position);
}

float Model::getLeftArmLength() const {
    return getLimbLength(getLeftHandJointIndex());
}

bool Model::restoreRightHandPosition(float percent) {
    return restoreJointPosition(getRightHandJointIndex(), percent);
}

bool Model::getRightShoulderPosition(glm::vec3& position) const {
    return getJointPosition(getLastFreeJointIndex(getRightHandJointIndex()), position);
}

float Model::getRightArmLength() const {
    return getLimbLength(getRightHandJointIndex());
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

void Model::clearShapes() {
    for (int i = 0; i < _jointShapes.size(); ++i) {
        delete _jointShapes[i];
    }
    _jointShapes.clear();
}

void Model::rebuildShapes() {
    clearShapes();

    if (_jointStates.isEmpty()) {
        return;
    }

    // make sure all the joints are updated correctly before we try to create their shapes
    for (int i = 0; i < _jointStates.size(); i++) {
        updateJointState(i);
    }
    
    const FBXGeometry& geometry = _geometry->getFBXGeometry();
    float uniformScale = extractUniformScale(_scale);
    glm::quat inverseRotation = glm::inverse(_rotation);
    glm::vec3 rootPosition(0.f);

    // joint shapes
    Extents totalExtents;
    totalExtents.reset();
    for (int i = 0; i < _jointStates.size(); i++) {
        const FBXJoint& joint = geometry.joints[i];

        glm::vec3 jointToShapeOffset = uniformScale * (_jointStates[i].combinedRotation * joint.shapePosition);
        glm::vec3 worldPosition = extractTranslation(_jointStates[i].transform) + jointToShapeOffset + _translation;
        Extents shapeExtents;
        shapeExtents.reset();

        if (joint.parentIndex == -1) {
            rootPosition = worldPosition;
        }

        float radius = uniformScale * joint.boneRadius;
        float halfHeight = 0.5f * uniformScale * joint.distanceToParent;
        if (joint.shapeType == Shape::CAPSULE_SHAPE && halfHeight > EPSILON) {
            CapsuleShape* capsule = new CapsuleShape(radius, halfHeight);
            capsule->setPosition(worldPosition);
            capsule->setRotation(_jointStates[i].combinedRotation * joint.shapeRotation);
            _jointShapes.push_back(capsule);

            glm::vec3 endPoint; 
            capsule->getEndPoint(endPoint);
            glm::vec3 startPoint;
            capsule->getStartPoint(startPoint);
            glm::vec3 axis = (halfHeight + radius) * glm::normalize(endPoint - startPoint);
            shapeExtents.addPoint(worldPosition + axis);
            shapeExtents.addPoint(worldPosition - axis);
        } else {
            SphereShape* sphere = new SphereShape(radius, worldPosition);
            _jointShapes.push_back(sphere);

            glm::vec3 axis = glm::vec3(radius);
            shapeExtents.addPoint(worldPosition + axis);
            shapeExtents.addPoint(worldPosition - axis);
        }
        totalExtents.addExtents(shapeExtents);
    }

    // bounding shape
    // NOTE: we assume that the longest side of totalExtents is the yAxis
    glm::vec3 diagonal = totalExtents.maximum - totalExtents.minimum;
    float capsuleRadius = 0.25f * (diagonal.x + diagonal.z);    // half the average of x and z
    _boundingShape.setRadius(capsuleRadius);
    _boundingShape.setHalfHeight(0.5f * diagonal.y - capsuleRadius);
    _boundingShapeLocalOffset = inverseRotation * (0.5f * (totalExtents.maximum + totalExtents.minimum) - rootPosition);
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
            glm::vec3 jointToShapeOffset = uniformScale * (_jointStates[i].combinedRotation * joint.shapePosition);
            glm::vec3 worldPosition = extractTranslation(_jointStates[i].transform) + jointToShapeOffset + _translation;
            _jointShapes[i]->setPosition(worldPosition);
            _jointShapes[i]->setRotation(_jointStates[i].combinedRotation * joint.shapeRotation);
            float distance2 = glm::distance2(worldPosition, _translation);
            if (distance2 > _boundingRadius) {
                _boundingRadius = distance2;
            }
            if (joint.parentIndex == -1) {
                rootPosition = worldPosition;
            }
        }
        _boundingRadius = sqrtf(_boundingRadius);
        _shapesAreDirty = false;
        _boundingShape.setPosition(rootPosition + _rotation * _boundingShapeLocalOffset);
    }
}

bool Model::findRayIntersection(const glm::vec3& origin, const glm::vec3& direction, float& distance) const {
    const glm::vec3 relativeOrigin = origin - _translation;
    const FBXGeometry& geometry = _geometry->getFBXGeometry();
    float minDistance = FLT_MAX;
    float radiusScale = extractUniformScale(_scale);
    for (int i = 0; i < _jointStates.size(); i++) {
        const FBXJoint& joint = geometry.joints[i];
        glm::vec3 end = extractTranslation(_jointStates[i].transform);
        float endRadius = joint.boneRadius * radiusScale;
        glm::vec3 start = end;
        float startRadius = joint.boneRadius * radiusScale;
        if (joint.parentIndex != -1) {
            start = extractTranslation(_jointStates[joint.parentIndex].transform);
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
            if (ShapeCollider::shapeShape(theirShape, ourShape, collisions)) {
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
        if (ShapeCollider::shapeShape(&sphere, _jointShapes[i], collisions)) {
            CollisionInfo* collision = collisions.getLastCollision();
            collision->_type = MODEL_COLLISION;
            collision->_data = (void*)(this);
            collision->_flags = i;
            collided = true;
        }
        outerContinue: ;
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

void Model::simulate(float deltaTime, bool fullUpdate) {
    fullUpdate = updateGeometry() || fullUpdate;
    if (isActive() && fullUpdate) {
        simulateInternal(deltaTime);
    }
}

void Model::simulateInternal(float deltaTime) {
    // NOTE: this is a recursive call that walks all attachments, and their attachments
    // update the world space transforms for all joints
    for (int i = 0; i < _jointStates.size(); i++) {
        updateJointState(i);
    }
    _shapesAreDirty = true;
    
    const FBXGeometry& geometry = _geometry->getFBXGeometry();

    // update the attachment transforms and simulate them
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
            state.clusterMatrices[j] = _jointStates[cluster.jointIndex].transform * cluster.inverseBindMatrix;
        }
    }
    
    // post the blender
    if (geometry.hasBlendedMeshes()) {
        QThreadPool::globalInstance()->start(new Blender(this, _geometry, geometry.meshes, _blendshapeCoefficients));
    }
}

void Model::updateJointState(int index) {
    JointState& state = _jointStates[index];
    const FBXGeometry& geometry = _geometry->getFBXGeometry();
    const FBXJoint& joint = geometry.joints.at(index);
    
    if (joint.parentIndex == -1) {
        glm::mat4 baseTransform = glm::mat4_cast(_rotation) * glm::scale(_scale) * glm::translate(_offset);
    
        glm::quat combinedRotation = joint.preRotation * state.rotation * joint.postRotation;    
        state.transform = baseTransform * geometry.offset * glm::translate(state.translation) * joint.preTransform *
            glm::mat4_cast(combinedRotation) * joint.postTransform;
        state.combinedRotation = _rotation * combinedRotation;
        
    } else {
        const JointState& parentState = _jointStates.at(joint.parentIndex);
        if (index == geometry.leanJointIndex) {
            maybeUpdateLeanRotation(parentState, joint, state);
        
        } else if (index == geometry.neckJointIndex) {
            maybeUpdateNeckRotation(parentState, joint, state);    
                
        } else if (index == geometry.leftEyeJointIndex || index == geometry.rightEyeJointIndex) {
            maybeUpdateEyeRotation(parentState, joint, state);
        }
        glm::quat combinedRotation = joint.preRotation * state.rotation * joint.postRotation;    
        state.transform = parentState.transform * glm::translate(state.translation) * joint.preTransform *
            glm::mat4_cast(combinedRotation) * joint.postTransform;
        state.combinedRotation = parentState.combinedRotation * combinedRotation;
    }
}

void Model::maybeUpdateLeanRotation(const JointState& parentState, const FBXJoint& joint, JointState& state) {
    // nothing by default
}

void Model::maybeUpdateNeckRotation(const JointState& parentState, const FBXJoint& joint, JointState& state) {
    // nothing by default
}

void Model::maybeUpdateEyeRotation(const JointState& parentState, const FBXJoint& joint, JointState& state) {
    // nothing by default
}

bool Model::getJointPosition(int jointIndex, glm::vec3& position) const {
    if (jointIndex == -1 || _jointStates.isEmpty()) {
        return false;
    }
    position = _translation + extractTranslation(_jointStates[jointIndex].transform);
    return true;
}

bool Model::getJointRotation(int jointIndex, glm::quat& rotation, bool fromBind) const {
    if (jointIndex == -1 || _jointStates.isEmpty()) {
        return false;
    }
    rotation = _jointStates[jointIndex].combinedRotation *
        (fromBind ? _geometry->getFBXGeometry().joints[jointIndex].inverseBindRotation :
            _geometry->getFBXGeometry().joints[jointIndex].inverseDefaultRotation);
    return true;
}

bool Model::setJointPosition(int jointIndex, const glm::vec3& position, int lastFreeIndex,
        bool allIntermediatesFree, const glm::vec3& alignment) {
    if (jointIndex == -1 || _jointStates.isEmpty()) {
        return false;
    }
    glm::vec3 relativePosition = position - _translation;
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
        // first, we go from the joint upwards, rotating the end as close as possible to the target
        glm::vec3 endPosition = extractTranslation(_jointStates[jointIndex].transform);
        for (int j = 1; freeLineage.at(j - 1) != lastFreeIndex; j++) {
            int index = freeLineage.at(j);
            const FBXJoint& joint = geometry.joints.at(index);
            if (!(joint.isFree || allIntermediatesFree)) {
                continue;
            }
            JointState& state = _jointStates[index];
            glm::vec3 jointPosition = extractTranslation(state.transform);
            glm::vec3 jointVector = endPosition - jointPosition;
            glm::quat oldCombinedRotation = state.combinedRotation;
            applyRotationDelta(index, rotationBetween(jointVector, relativePosition - jointPosition));
            endPosition = state.combinedRotation * glm::inverse(oldCombinedRotation) * jointVector + jointPosition;
            if (alignment != glm::vec3() && j > 1) {
                jointVector = endPosition - jointPosition;
                glm::vec3 positionSum;
                for (int k = j - 1; k > 0; k--) {
                    int index = freeLineage.at(k);
                    updateJointState(index);
                    positionSum += extractTranslation(_jointStates.at(index).transform);
                }
                glm::vec3 projectedCenterOfMass = glm::cross(jointVector,
                    glm::cross(positionSum / (j - 1.0f) - jointPosition, jointVector));
                glm::vec3 projectedAlignment = glm::cross(jointVector, glm::cross(worldAlignment, jointVector));
                const float LENGTH_EPSILON = 0.001f;
                if (glm::length(projectedCenterOfMass) > LENGTH_EPSILON && glm::length(projectedAlignment) > LENGTH_EPSILON) {
                    applyRotationDelta(index, rotationBetween(projectedCenterOfMass, projectedAlignment));
                }
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

bool Model::setJointRotation(int jointIndex, const glm::quat& rotation, bool fromBind) {
    if (jointIndex == -1 || _jointStates.isEmpty()) {
        return false;
    }
    JointState& state = _jointStates[jointIndex];
    state.rotation = state.rotation * glm::inverse(state.combinedRotation) * rotation *
        glm::inverse(fromBind ? _geometry->getFBXGeometry().joints.at(jointIndex).inverseBindRotation :
            _geometry->getFBXGeometry().joints.at(jointIndex).inverseDefaultRotation);
    return true;
}

void Model::setJointTranslation(int jointIndex, const glm::vec3& translation) {
    const FBXGeometry& geometry = _geometry->getFBXGeometry();
    const FBXJoint& joint = geometry.joints.at(jointIndex);
    
    glm::mat4 parentTransform;
    if (joint.parentIndex == -1) {
        parentTransform = glm::mat4_cast(_rotation) * glm::scale(_scale) * glm::translate(_offset) * geometry.offset;
        
    } else {
        parentTransform = _jointStates.at(joint.parentIndex).transform;
    }
    JointState& state = _jointStates[jointIndex];
    glm::vec3 preTranslation = extractTranslation(joint.preTransform * glm::mat4_cast(joint.preRotation *
        state.rotation * joint.postRotation) * joint.postTransform); 
    state.translation = glm::vec3(glm::inverse(parentTransform) * glm::vec4(translation, 1.0f)) - preTranslation;
}

bool Model::restoreJointPosition(int jointIndex, float percent) {
    if (jointIndex == -1 || _jointStates.isEmpty()) {
        return false;
    }
    const FBXGeometry& geometry = _geometry->getFBXGeometry();
    const QVector<int>& freeLineage = geometry.joints.at(jointIndex).freeLineage;
    
    foreach (int index, freeLineage) {
        JointState& state = _jointStates[index];
        const FBXJoint& joint = geometry.joints.at(index);
        state.rotation = safeMix(state.rotation, joint.rotation, percent);
        state.translation = glm::mix(state.translation, joint.translation, percent);
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

void Model::applyRotationDelta(int jointIndex, const glm::quat& delta, bool constrain) {
    JointState& state = _jointStates[jointIndex];
    const FBXJoint& joint = _geometry->getFBXGeometry().joints[jointIndex];
    if (!constrain || (joint.rotationMin == glm::vec3(-PI, -PI, -PI) &&
            joint.rotationMax == glm::vec3(PI, PI, PI))) {
        // no constraints
        state.rotation = state.rotation * glm::inverse(state.combinedRotation) * delta * state.combinedRotation;
        state.combinedRotation = delta * state.combinedRotation;
        return;
    }
    glm::quat newRotation = glm::quat(glm::clamp(safeEulerAngles(state.rotation *
        glm::inverse(state.combinedRotation) * delta * state.combinedRotation), joint.rotationMin, joint.rotationMax));
    state.combinedRotation = state.combinedRotation * glm::inverse(state.rotation) * newRotation;
    state.rotation = newRotation;
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
    if (collision._type == MODEL_COLLISION) {
        // the joint is pokable by a collision if it exists and is free to move
        const FBXJoint& joint = _geometry->getFBXGeometry().joints[collision._flags];
        if (joint.parentIndex == -1 || _jointStates.isEmpty()) {
            return false;
        }
        // an empty freeLineage means the joint can't move
        const FBXGeometry& geometry = _geometry->getFBXGeometry();
        int jointIndex = collision._flags;
        const QVector<int>& freeLineage = geometry.joints.at(jointIndex).freeLineage;
        return !freeLineage.isEmpty();
    }
    return false;
}

void Model::applyCollision(CollisionInfo& collision) {
    if (collision._type != MODEL_COLLISION) {
        return;
    }

    glm::vec3 jointPosition(0.f);
    int jointIndex = collision._flags;
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
                setJointPosition(jointIndex, newEnd, -1, true);
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
    
    if (_geometry) {
        _geometry->clearLoadPriority(this);
    }
}

void Model::renderMeshes(float alpha, RenderMode mode, bool translucent) {
    const FBXGeometry& geometry = _geometry->getFBXGeometry();
    const QVector<NetworkMesh>& networkMeshes = _geometry->getMeshes();
    
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
        if (mode == SHADOW_RENDER_MODE) {
            program = &_shadowProgram;
            skinProgram = &_skinShadowProgram;
            skinLocations = &_skinShadowLocations;
            
        } else if (!mesh.tangents.isEmpty()) {
            program = &_normalMapProgram;
            skinProgram = &_skinNormalMapProgram;
            skinLocations = &_skinNormalMapLocations;
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
                
        if (state.clusterMatrices.size() > 1) {
            skinProgram->disableAttributeArray(skinLocations->clusterIndices);
            skinProgram->disableAttributeArray(skinLocations->clusterWeights);  
        } 
        glPopMatrix();

        activeProgram->release();
    }
}
