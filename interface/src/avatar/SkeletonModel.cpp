//
//  SkeletonModel.cpp
//  interface/src/avatar
//
//  Created by Andrzej Kapolka on 10/17/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <glm/gtx/transform.hpp>
#include <QMultiMap>

#include <DeferredLightingEffect.h>

#include "Application.h"
#include "Avatar.h"
#include "Hand.h"
#include "Menu.h"
#include "SkeletonModel.h"
#include "Util.h"
#include "InterfaceLogging.h"

SkeletonModel::SkeletonModel(Avatar* owningAvatar, QObject* parent, RigPointer rig) :
    Model(rig, parent),
    _triangleFanID(DependencyManager::get<GeometryCache>()->allocateID()),
    _owningAvatar(owningAvatar),
    _boundingCapsuleLocalOffset(0.0f),
    _boundingCapsuleRadius(0.0f),
    _boundingCapsuleHeight(0.0f),
    _defaultEyeModelPosition(glm::vec3(0.0f, 0.0f, 0.0f)),
    _headClipDistance(DEFAULT_NEAR_CLIP)
{
    assert(_rig);
    assert(_owningAvatar);
}

SkeletonModel::~SkeletonModel() {
}

void SkeletonModel::initJointStates(QVector<JointState> states) {
    const FBXGeometry& geometry = _geometry->getFBXGeometry();
    glm::mat4 parentTransform = glm::scale(_scale) * glm::translate(_offset) * geometry.offset;

    int rootJointIndex = geometry.rootJointIndex;
    int leftHandJointIndex = geometry.leftHandJointIndex;
    int leftElbowJointIndex = leftHandJointIndex >= 0 ? geometry.joints.at(leftHandJointIndex).parentIndex : -1;
    int leftShoulderJointIndex = leftElbowJointIndex >= 0 ? geometry.joints.at(leftElbowJointIndex).parentIndex : -1;
    int rightHandJointIndex = geometry.rightHandJointIndex;
    int rightElbowJointIndex = rightHandJointIndex >= 0 ? geometry.joints.at(rightHandJointIndex).parentIndex : -1;
    int rightShoulderJointIndex = rightElbowJointIndex >= 0 ? geometry.joints.at(rightElbowJointIndex).parentIndex : -1;

    _boundingRadius = _rig->initJointStates(states, parentTransform,
                                            rootJointIndex,
                                            leftHandJointIndex,
                                            leftElbowJointIndex,
                                            leftShoulderJointIndex,
                                            rightHandJointIndex,
                                            rightElbowJointIndex,
                                            rightShoulderJointIndex);

    // Determine the default eye position for avatar scale = 1.0
    int headJointIndex = _geometry->getFBXGeometry().headJointIndex;
    if (0 <= headJointIndex && headJointIndex < _rig->getJointStateCount()) {

        glm::vec3 leftEyePosition, rightEyePosition;
        getEyeModelPositions(leftEyePosition, rightEyePosition);
        glm::vec3 midEyePosition = (leftEyePosition + rightEyePosition) / 2.0f;

        int rootJointIndex = _geometry->getFBXGeometry().rootJointIndex;
        glm::vec3 rootModelPosition;
        getJointPosition(rootJointIndex, rootModelPosition);

        _defaultEyeModelPosition = midEyePosition - rootModelPosition;
        _defaultEyeModelPosition.z = -_defaultEyeModelPosition.z;

        // Skeleton may have already been scaled so unscale it
        _defaultEyeModelPosition = _defaultEyeModelPosition / _scale;
    }

    // the SkeletonModel override of updateJointState() will clear the translation part
    // of its root joint and we need that done before we try to build shapes hence we
    // recompute all joint transforms at this time.
    for (int i = 0; i < _rig->getJointStateCount(); i++) {
        _rig->updateJointState(i, parentTransform);
    }

    buildShapes();

    Extents meshExtents = getMeshExtents();
    _headClipDistance = -(meshExtents.minimum.z / _scale.z - _defaultEyeModelPosition.z);
    _headClipDistance = std::max(_headClipDistance, DEFAULT_NEAR_CLIP);

    _owningAvatar->rebuildSkeletonBody();
    emit skeletonLoaded();
}

const float PALM_PRIORITY = DEFAULT_PRIORITY;

void SkeletonModel::updateRig(float deltaTime, glm::mat4 parentTransform) {
    if (_owningAvatar->isMyAvatar()) {
        _rig->computeMotionAnimationState(deltaTime, _owningAvatar->getPosition(), _owningAvatar->getVelocity(), _owningAvatar->getOrientation());
    }
    Model::updateRig(deltaTime, parentTransform);
    if (_owningAvatar->isMyAvatar()) {
        const FBXGeometry& geometry = _geometry->getFBXGeometry();

        Rig::HeadParameters params;
        params.leanSideways = _owningAvatar->getHead()->getFinalLeanSideways();
        params.leanForward = _owningAvatar->getHead()->getFinalLeanSideways();
        params.torsoTwist = _owningAvatar->getHead()->getTorsoTwist();
        params.localHeadOrientation = _owningAvatar->getHead()->getFinalOrientationInLocalFrame();
        params.worldHeadOrientation = _owningAvatar->getHead()->getFinalOrientationInWorldFrame();
        params.eyeLookAt = _owningAvatar->getHead()->getCorrectedLookAtPosition();
        params.eyeSaccade = _owningAvatar->getHead()->getSaccade();
        params.leanJointIndex = geometry.leanJointIndex;
        params.neckJointIndex = geometry.neckJointIndex;
        params.leftEyeJointIndex = geometry.leftEyeJointIndex;
        params.rightEyeJointIndex = geometry.rightEyeJointIndex;

        _rig->updateFromHeadParameters(params);
    }
}

void SkeletonModel::simulate(float deltaTime, bool fullUpdate) {
    setTranslation(_owningAvatar->getSkeletonPosition());
    static const glm::quat refOrientation = glm::angleAxis(PI, glm::vec3(0.0f, 1.0f, 0.0f));
    setRotation(_owningAvatar->getOrientation() * refOrientation);
    setScale(glm::vec3(1.0f, 1.0f, 1.0f) * _owningAvatar->getScale());
    setBlendshapeCoefficients(_owningAvatar->getHead()->getBlendshapeCoefficients());

    Model::simulate(deltaTime, fullUpdate);
    
    if (!isActive() || !_owningAvatar->isMyAvatar()) {
        return; // only simulate for own avatar
    }
    
    MyAvatar* myAvatar = static_cast<MyAvatar*>(_owningAvatar);
    if (myAvatar->isPlaying()) {
        // Don't take inputs if playing back a recording.
        return;
    }

    const FBXGeometry& geometry = _geometry->getFBXGeometry();

    // find the left and rightmost active palms
    int leftPalmIndex, rightPalmIndex;
    Hand* hand = _owningAvatar->getHand();
    hand->getLeftRightPalmIndices(leftPalmIndex, rightPalmIndex);

    const float HAND_RESTORATION_RATE = 0.25f;
    if (leftPalmIndex == -1 && rightPalmIndex == -1) {
        // palms are not yet set, use mouse
        if (_owningAvatar->getHandState() == HAND_STATE_NULL) {
            restoreRightHandPosition(HAND_RESTORATION_RATE, PALM_PRIORITY);
        } else {
            // transform into model-frame
            glm::vec3 handPosition = glm::inverse(_rotation) * (_owningAvatar->getHandPosition() - _translation);
            applyHandPosition(geometry.rightHandJointIndex, handPosition);
        }
        restoreLeftHandPosition(HAND_RESTORATION_RATE, PALM_PRIORITY);

    } else if (leftPalmIndex == rightPalmIndex) {
        // right hand only
        applyPalmData(geometry.rightHandJointIndex, hand->getPalms()[leftPalmIndex]);
        restoreLeftHandPosition(HAND_RESTORATION_RATE, PALM_PRIORITY);

    } else {
        if (leftPalmIndex != -1) {
            applyPalmData(geometry.leftHandJointIndex, hand->getPalms()[leftPalmIndex]);
        } else {
            restoreLeftHandPosition(HAND_RESTORATION_RATE, PALM_PRIORITY);
        }
        if (rightPalmIndex != -1) {
            applyPalmData(geometry.rightHandJointIndex, hand->getPalms()[rightPalmIndex]);
        } else {
            restoreRightHandPosition(HAND_RESTORATION_RATE, PALM_PRIORITY);
        }
    }
}

void SkeletonModel::renderIKConstraints(gpu::Batch& batch) {
    renderJointConstraints(batch, getRightHandJointIndex());
    renderJointConstraints(batch, getLeftHandJointIndex());
}

class IndexValue {
public:
    int index;
    float value;
};

bool operator<(const IndexValue& firstIndex, const IndexValue& secondIndex) {
    return firstIndex.value < secondIndex.value;
}

void SkeletonModel::applyHandPosition(int jointIndex, const glm::vec3& position) {
    if (jointIndex == -1 || jointIndex >= _rig->getJointStateCount()) {
        return;
    }
    // NOTE: 'position' is in model-frame
    setJointPosition(jointIndex, position, glm::quat(), false, -1, false, glm::vec3(0.0f, -1.0f, 0.0f), PALM_PRIORITY);

    const FBXGeometry& geometry = _geometry->getFBXGeometry();
    glm::vec3 handPosition, elbowPosition;
    getJointPosition(jointIndex, handPosition);
    getJointPosition(geometry.joints.at(jointIndex).parentIndex, elbowPosition);
    glm::vec3 forearmVector = handPosition - elbowPosition;
    float forearmLength = glm::length(forearmVector);
    if (forearmLength < EPSILON) {
        return;
    }
    glm::quat handRotation;
    if (!_rig->getJointStateRotation(jointIndex, handRotation)) {
        return;
    }

    // align hand with forearm
    float sign = (jointIndex == geometry.rightHandJointIndex) ? 1.0f : -1.0f;
    _rig->applyJointRotationDelta(jointIndex,
                                  rotationBetween(handRotation * glm::vec3(-sign, 0.0f, 0.0f), forearmVector),
                                  true, PALM_PRIORITY);
}

void SkeletonModel::applyPalmData(int jointIndex, PalmData& palm) {
    if (jointIndex == -1 || jointIndex >= _rig->getJointStateCount()) {
        return;
    }
    const FBXGeometry& geometry = _geometry->getFBXGeometry();
    float sign = (jointIndex == geometry.rightHandJointIndex) ? 1.0f : -1.0f;
    int parentJointIndex = geometry.joints.at(jointIndex).parentIndex;
    if (parentJointIndex == -1) {
        return;
    }
  
    // rotate palm to align with its normal (normal points out of hand's palm)
    glm::quat inverseRotation = glm::inverse(_rotation);
    glm::vec3 palmPosition = inverseRotation * (palm.getPosition() - _translation);
    glm::vec3 palmNormal = inverseRotation * palm.getNormal();
    glm::vec3 fingerDirection = inverseRotation * palm.getFingerDirection();

    glm::quat palmRotation = rotationBetween(geometry.palmDirection, palmNormal);
    palmRotation = rotationBetween(palmRotation * glm::vec3(-sign, 0.0f, 0.0f), fingerDirection) * palmRotation;

    if (Menu::getInstance()->isOptionChecked(MenuOption::AlternateIK)) {
        _rig->setHandPosition(jointIndex, palmPosition, palmRotation, extractUniformScale(_scale), PALM_PRIORITY);
    } else if (Menu::getInstance()->isOptionChecked(MenuOption::AlignForearmsWithWrists)) {
        float forearmLength = geometry.joints.at(jointIndex).distanceToParent * extractUniformScale(_scale);
        glm::vec3 forearm = palmRotation * glm::vec3(sign * forearmLength, 0.0f, 0.0f);
        setJointPosition(parentJointIndex, palmPosition + forearm,
            glm::quat(), false, -1, false, glm::vec3(0.0f, -1.0f, 0.0f), PALM_PRIORITY);
        _rig->setJointRotationInBindFrame(parentJointIndex, palmRotation, PALM_PRIORITY);
        // lock hand to forearm by slamming its rotation (in parent-frame) to identity
        _rig->setJointRotationInConstrainedFrame(jointIndex, glm::quat(), PALM_PRIORITY);
    } else {
        inverseKinematics(jointIndex, palmPosition, palmRotation, PALM_PRIORITY);
    }
}

void SkeletonModel::renderJointConstraints(gpu::Batch& batch, int jointIndex) {
    if (jointIndex == -1 || jointIndex >= _rig->getJointStateCount()) {
        return;
    }
    const FBXGeometry& geometry = _geometry->getFBXGeometry();
    const float BASE_DIRECTION_SIZE = 0.3f;
    float directionSize = BASE_DIRECTION_SIZE * extractUniformScale(_scale);
    // FIXME: THe line width of 3.0f is not supported anymore, we ll need a workaround

    do {
        const FBXJoint& joint = geometry.joints.at(jointIndex);
        const JointState& jointState = _rig->getJointState(jointIndex);
        glm::vec3 position = _rotation * jointState.getPosition() + _translation;
        glm::quat parentRotation = (joint.parentIndex == -1) ?
            _rotation :
            _rotation * _rig->getJointState(joint.parentIndex).getRotation();
        float fanScale = directionSize * 0.75f;
        
        Transform transform = Transform();
        transform.setTranslation(position);
        transform.setRotation(parentRotation);
        transform.setScale(fanScale);
        batch.setModelTransform(transform);
        
        const int AXIS_COUNT = 3;

        auto geometryCache = DependencyManager::get<GeometryCache>();

        for (int i = 0; i < AXIS_COUNT; i++) {
            if (joint.rotationMin[i] <= -PI + EPSILON && joint.rotationMax[i] >= PI - EPSILON) {
                continue; // unconstrained
            }
            glm::vec3 axis;
            axis[i] = 1.0f;
            
            glm::vec3 otherAxis;
            if (i == 0) {
                otherAxis.y = 1.0f;
            } else {
                otherAxis.x = 1.0f;
            }
            glm::vec4 color(otherAxis.r, otherAxis.g, otherAxis.b, 0.75f);

            QVector<glm::vec3> points;
            points << glm::vec3(0.0f, 0.0f, 0.0f);
            const int FAN_SEGMENTS = 16;
            for (int j = 0; j < FAN_SEGMENTS; j++) {
                glm::vec3 rotated = glm::angleAxis(glm::mix(joint.rotationMin[i], joint.rotationMax[i],
                    (float)j / (FAN_SEGMENTS - 1)), axis) * otherAxis;
                points << rotated;
            }
            // TODO: this is really inefficient constantly recreating these vertices buffers. It would be
            // better if the skeleton model cached these buffers for each of the joints they are rendering
            geometryCache->updateVertices(_triangleFanID, points, color);
            geometryCache->renderVertices(batch, gpu::TRIANGLE_FAN, _triangleFanID);
            
        }
        
        renderOrientationDirections(batch, jointIndex, position, _rotation * jointState.getRotation(), directionSize);
        jointIndex = joint.parentIndex;
        
    } while (jointIndex != -1 && geometry.joints.at(jointIndex).isFree);
}

void SkeletonModel::renderOrientationDirections(gpu::Batch& batch, int jointIndex, 
                                                glm::vec3 position, const glm::quat& orientation, float size) {
                                                
    auto geometryCache = DependencyManager::get<GeometryCache>();

    if (!_jointOrientationLines.contains(jointIndex)) {
        OrientationLineIDs jointLineIDs;
        jointLineIDs._up = geometryCache->allocateID();
        jointLineIDs._front = geometryCache->allocateID();
        jointLineIDs._right = geometryCache->allocateID();
        _jointOrientationLines[jointIndex] = jointLineIDs;
    }
    OrientationLineIDs& jointLineIDs = _jointOrientationLines[jointIndex];

    glm::vec3 pRight	= position + orientation * IDENTITY_RIGHT * size;
    glm::vec3 pUp		= position + orientation * IDENTITY_UP    * size;
    glm::vec3 pFront	= position + orientation * IDENTITY_FRONT * size;

    glm::vec3 red(1.0f, 0.0f, 0.0f);
    geometryCache->renderLine(batch, position, pRight, red, jointLineIDs._right);

    glm::vec3 green(0.0f, 1.0f, 0.0f);
    geometryCache->renderLine(batch, position, pUp, green, jointLineIDs._up);

    glm::vec3 blue(0.0f, 0.0f, 1.0f);
    geometryCache->renderLine(batch, position, pFront, blue, jointLineIDs._front);
}

bool SkeletonModel::getLeftHandPosition(glm::vec3& position) const {
    return getJointPositionInWorldFrame(getLeftHandJointIndex(), position);
}

bool SkeletonModel::getRightHandPosition(glm::vec3& position) const {
    return getJointPositionInWorldFrame(getRightHandJointIndex(), position);
}

bool SkeletonModel::restoreLeftHandPosition(float fraction, float priority) {
    return restoreJointPosition(getLeftHandJointIndex(), fraction, priority);
}

bool SkeletonModel::getLeftShoulderPosition(glm::vec3& position) const {
    return getJointPositionInWorldFrame(getLastFreeJointIndex(getLeftHandJointIndex()), position);
}

float SkeletonModel::getLeftArmLength() const {
    return getLimbLength(getLeftHandJointIndex());
}

bool SkeletonModel::restoreRightHandPosition(float fraction, float priority) {
    return restoreJointPosition(getRightHandJointIndex(), fraction, priority);
}

bool SkeletonModel::getRightShoulderPosition(glm::vec3& position) const {
    return getJointPositionInWorldFrame(getLastFreeJointIndex(getRightHandJointIndex()), position);
}

float SkeletonModel::getRightArmLength() const {
    return getLimbLength(getRightHandJointIndex());
}

bool SkeletonModel::getHeadPosition(glm::vec3& headPosition) const {
    return isActive() && getJointPositionInWorldFrame(_geometry->getFBXGeometry().headJointIndex, headPosition);
}

bool SkeletonModel::getNeckPosition(glm::vec3& neckPosition) const {
    return isActive() && getJointPositionInWorldFrame(_geometry->getFBXGeometry().neckJointIndex, neckPosition);
}

bool SkeletonModel::getNeckParentRotationFromDefaultOrientation(glm::quat& neckParentRotation) const {
    if (!isActive()) {
        return false;
    }
    const FBXGeometry& geometry = _geometry->getFBXGeometry();
    if (geometry.neckJointIndex == -1) {
        return false;
    }
    int parentIndex = geometry.joints.at(geometry.neckJointIndex).parentIndex;
    glm::quat worldFrameRotation;
    bool success = getJointRotationInWorldFrame(parentIndex, worldFrameRotation);
    if (success) {
        neckParentRotation = worldFrameRotation * _rig->getJointState(parentIndex).getInverseDefaultRotation();
    }
    return success;
}

bool SkeletonModel::getEyeModelPositions(glm::vec3& firstEyePosition, glm::vec3& secondEyePosition) const {
    if (!isActive()) {
        return false;
        }
    const FBXGeometry& geometry = _geometry->getFBXGeometry();
    if (getJointPosition(geometry.leftEyeJointIndex, firstEyePosition) &&
        getJointPosition(geometry.rightEyeJointIndex, secondEyePosition)) {
        return true;
    }
    // no eye joints; try to estimate based on head/neck joints
    glm::vec3 neckPosition, headPosition;
    if (getJointPosition(geometry.neckJointIndex, neckPosition) &&
        getJointPosition(geometry.headJointIndex, headPosition)) {
        const float EYE_PROPORTION = 0.6f;
        glm::vec3 baseEyePosition = glm::mix(neckPosition, headPosition, EYE_PROPORTION);
        glm::quat headRotation;
        getJointRotation(geometry.headJointIndex, headRotation);
        const float EYES_FORWARD = 0.25f;
        const float EYE_SEPARATION = 0.1f;
        float headHeight = glm::distance(neckPosition, headPosition);
        firstEyePosition = baseEyePosition + headRotation * glm::vec3(EYE_SEPARATION, 0.0f, EYES_FORWARD) * headHeight;
        secondEyePosition = baseEyePosition + headRotation * glm::vec3(-EYE_SEPARATION, 0.0f, EYES_FORWARD) * headHeight;
        return true;
    }
    return false;
}

bool SkeletonModel::getEyePositions(glm::vec3& firstEyePosition, glm::vec3& secondEyePosition) const {
    if (getEyeModelPositions(firstEyePosition, secondEyePosition)) {
        firstEyePosition = _translation + _rotation * firstEyePosition;
        secondEyePosition = _translation + _rotation * secondEyePosition;
        return true;
    }
    return false;
}

glm::vec3 SkeletonModel::getDefaultEyeModelPosition() const {
    return _owningAvatar->getScale() * _defaultEyeModelPosition;
}

float DENSITY_OF_WATER = 1000.0f; // kg/m^3
float MIN_JOINT_MASS = 1.0f;
float VERY_BIG_MASS = 1.0e6f;

// virtual
void SkeletonModel::buildShapes() {
    if (_geometry == NULL || _rig->jointStatesEmpty()) {
        return;
    }
    
    const FBXGeometry& geometry = _geometry->getFBXGeometry();
    if (geometry.joints.isEmpty() || geometry.rootJointIndex == -1) {
        // rootJointIndex == -1 if the avatar model has no skeleton
        return;
    }
    computeBoundingShape(geometry);
}

void SkeletonModel::computeBoundingShape(const FBXGeometry& geometry) {
    // compute default joint transforms
    int numStates = _rig->getJointStateCount();
    QVector<glm::mat4> transforms;
    transforms.fill(glm::mat4(), numStates);

    // compute bounding box that encloses all shapes
    Extents totalExtents;
    totalExtents.reset();
    totalExtents.addPoint(glm::vec3(0.0f));
    for (int i = 0; i < numStates; i++) {
        // compute the default transform of this joint
        const JointState& state = _rig->getJointState(i);
        int parentIndex = state.getParentIndex();
        if (parentIndex == -1) {
            transforms[i] = _rig->getJointTransform(i);
        } else {
            glm::quat modifiedRotation = state.getPreRotation() * state.getOriginalRotation() * state.getPostRotation();
            transforms[i] = transforms[parentIndex] * glm::translate(state.getTranslation())
                * state.getPreTransform() * glm::mat4_cast(modifiedRotation) * state.getPostTransform();
        }

        // Each joint contributes a sphere at its position
        glm::vec3 axis(state.getBoneRadius());
        glm::vec3 jointPosition = extractTranslation(transforms[i]);
        totalExtents.addPoint(jointPosition + axis);
        totalExtents.addPoint(jointPosition - axis);
    }

    // compute bounding shape parameters
    // NOTE: we assume that the longest side of totalExtents is the yAxis...
    glm::vec3 diagonal = totalExtents.maximum - totalExtents.minimum;
    // ... and assume the radius is half the RMS of the X and Z sides:
    _boundingCapsuleRadius = 0.5f * sqrtf(0.5f * (diagonal.x * diagonal.x + diagonal.z * diagonal.z));
    _boundingCapsuleHeight = diagonal.y - 2.0f * _boundingCapsuleRadius;

    glm::vec3 rootPosition = _rig->getJointState(geometry.rootJointIndex).getPosition();
    _boundingCapsuleLocalOffset = 0.5f * (totalExtents.maximum + totalExtents.minimum) - rootPosition;
    _boundingRadius = 0.5f * glm::length(diagonal);
}

void SkeletonModel::renderBoundingCollisionShapes(gpu::Batch& batch, float alpha) {
    const int BALL_SUBDIVISIONS = 10;

    auto geometryCache = DependencyManager::get<GeometryCache>();
    auto deferredLighting = DependencyManager::get<DeferredLightingEffect>();
    Transform transform; // = Transform();

    // draw a blue sphere at the capsule top point
    glm::vec3 topPoint = _translation + _boundingCapsuleLocalOffset + (0.5f * _boundingCapsuleHeight) * glm::vec3(0.0f, 1.0f, 0.0f);
    transform.setTranslation(topPoint);
    batch.setModelTransform(transform);
    deferredLighting->bindSimpleProgram(batch);
    geometryCache->renderSphere(batch, _boundingCapsuleRadius, BALL_SUBDIVISIONS, BALL_SUBDIVISIONS,
                                glm::vec4(0.6f, 0.6f, 0.8f, alpha));

    // draw a yellow sphere at the capsule bottom point 
    glm::vec3 bottomPoint = topPoint - glm::vec3(0.0f, -_boundingCapsuleHeight, 0.0f);
    glm::vec3 axis = topPoint - bottomPoint;
    transform.setTranslation(bottomPoint);
    batch.setModelTransform(transform);
    deferredLighting->bindSimpleProgram(batch);
    geometryCache->renderSphere(batch, _boundingCapsuleRadius, BALL_SUBDIVISIONS, BALL_SUBDIVISIONS,
                                glm::vec4(0.8f, 0.8f, 0.6f, alpha));

    // draw a green cylinder between the two points
    glm::vec3 origin(0.0f);
    Avatar::renderJointConnectingCone(batch, origin, axis, _boundingCapsuleRadius, _boundingCapsuleRadius,
                                      glm::vec4(0.6f, 0.8f, 0.6f, alpha));
}

bool SkeletonModel::hasSkeleton() {
    return isActive() ? _geometry->getFBXGeometry().rootJointIndex != -1 : false;
}

void SkeletonModel::onInvalidate() {
}
