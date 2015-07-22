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

#include <CapsuleShape.h>
#include <SphereShape.h>

#include "Application.h"
#include "Avatar.h"
#include "Hand.h"
#include "Menu.h"
#include "SkeletonModel.h"
#include "Util.h"
#include "InterfaceLogging.h"

enum StandingFootState {
    LEFT_FOOT,
    RIGHT_FOOT,
    NO_FOOT
};

SkeletonModel::SkeletonModel(Avatar* owningAvatar, QObject* parent) : 
    Model(parent),
    _triangleFanID(DependencyManager::get<GeometryCache>()->allocateID()),
    _owningAvatar(owningAvatar),
    _boundingShape(),
    _boundingShapeLocalOffset(0.0f),
    _defaultEyeModelPosition(glm::vec3(0.0f, 0.0f, 0.0f)),
    _standingFoot(NO_FOOT),
    _standingOffset(0.0f),
    _clampedFootPosition(0.0f),
    _headClipDistance(DEFAULT_NEAR_CLIP),
    _isFirstPerson(false)
{
    assert(_owningAvatar);
    _enableShapes = true;
}

SkeletonModel::~SkeletonModel() {
}

void SkeletonModel::initJointStates(QVector<JointState> states) {
    Model::initJointStates(states);

    // Determine the default eye position for avatar scale = 1.0
    int headJointIndex = _geometry->getFBXGeometry().headJointIndex;
    if (0 <= headJointIndex && headJointIndex < _jointStates.size()) {

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
    for (int i = 0; i < _jointStates.size(); i++) {
        updateJointState(i);
    }

    clearShapes();
    if (_enableShapes) {
        buildShapes();
    }

    Extents meshExtents = getMeshExtents();
    _headClipDistance = -(meshExtents.minimum.z / _scale.z - _defaultEyeModelPosition.z);
    _headClipDistance = std::max(_headClipDistance, DEFAULT_NEAR_CLIP);

    _owningAvatar->rebuildSkeletonBody();
    emit skeletonLoaded();
}

const float PALM_PRIORITY = DEFAULT_PRIORITY;
const float LEAN_PRIORITY = DEFAULT_PRIORITY;

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

    if (_isFirstPerson) {
        cauterizeHead();
        updateClusterMatrices();
    }

    _boundingShape.setTranslation(_translation + _rotation * _boundingShapeLocalOffset);
    _boundingShape.setRotation(_rotation);
}

void SkeletonModel::getHandShapes(int jointIndex, QVector<const Shape*>& shapes) const {
    if (jointIndex < 0 || jointIndex >= int(_shapes.size())) {
        return;
    }
    if (jointIndex == getLeftHandJointIndex()
        || jointIndex == getRightHandJointIndex()) {
        // get all shapes that have this hand as an ancestor in the skeleton heirarchy
        const FBXGeometry& geometry = _geometry->getFBXGeometry();
        for (int i = 0; i < _jointStates.size(); i++) {
            const FBXJoint& joint = geometry.joints[i];
            int parentIndex = joint.parentIndex;
            Shape* shape = _shapes[i];
            if (i == jointIndex) {
                // this shape is the hand
                if (shape) {
                    shapes.push_back(shape);
                }
                if (parentIndex != -1 && _shapes[parentIndex]) {
                    // also add the forearm
                    shapes.push_back(_shapes[parentIndex]);
                }
            } else if (shape) {
                while (parentIndex != -1) {
                    if (parentIndex == jointIndex) {
                        // this shape is a child of the hand
                        shapes.push_back(shape);
                        break;
                    }
                    parentIndex = geometry.joints[parentIndex].parentIndex;
                }
            }
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
    if (jointIndex == -1 || jointIndex >= _jointStates.size()) {
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
    JointState& state = _jointStates[jointIndex];
    glm::quat handRotation = state.getRotation();

    // align hand with forearm
    float sign = (jointIndex == geometry.rightHandJointIndex) ? 1.0f : -1.0f;
    state.applyRotationDelta(rotationBetween(handRotation * glm::vec3(-sign, 0.0f, 0.0f), forearmVector), true, PALM_PRIORITY);
}

void SkeletonModel::applyPalmData(int jointIndex, PalmData& palm) {
    if (jointIndex == -1 || jointIndex >= _jointStates.size()) {
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
        setHandPosition(jointIndex, palmPosition, palmRotation);  
    } else if (Menu::getInstance()->isOptionChecked(MenuOption::AlignForearmsWithWrists)) {
        float forearmLength = geometry.joints.at(jointIndex).distanceToParent * extractUniformScale(_scale);
        glm::vec3 forearm = palmRotation * glm::vec3(sign * forearmLength, 0.0f, 0.0f);
        setJointPosition(parentJointIndex, palmPosition + forearm,
            glm::quat(), false, -1, false, glm::vec3(0.0f, -1.0f, 0.0f), PALM_PRIORITY);
        JointState& parentState = _jointStates[parentJointIndex];
        parentState.setRotationInBindFrame(palmRotation, PALM_PRIORITY);
        // lock hand to forearm by slamming its rotation (in parent-frame) to identity
        _jointStates[jointIndex].setRotationInConstrainedFrame(glm::quat(), PALM_PRIORITY);
    } else {
        inverseKinematics(jointIndex, palmPosition, palmRotation, PALM_PRIORITY);
    }
}

void SkeletonModel::updateJointState(int index) {
    if (index < 0 && index >= _jointStates.size()) {
        return; // bail
    }
    JointState& state = _jointStates[index];
    const FBXJoint& joint = state.getFBXJoint();
    if (joint.parentIndex >= 0 && joint.parentIndex < _jointStates.size()) {
        const JointState& parentState = _jointStates.at(joint.parentIndex);
        const FBXGeometry& geometry = _geometry->getFBXGeometry();
        if (index == geometry.leanJointIndex) {
            maybeUpdateLeanRotation(parentState, state);
        
        } else if (index == geometry.neckJointIndex) {
            maybeUpdateNeckRotation(parentState, joint, state);    
                
        } else if (index == geometry.leftEyeJointIndex || index == geometry.rightEyeJointIndex) {
            maybeUpdateEyeRotation(parentState, joint, state);
        }
    }

    Model::updateJointState(index);

    if (index == _geometry->getFBXGeometry().rootJointIndex) {
        state.clearTransformTranslation();
    }
}

void SkeletonModel::maybeUpdateLeanRotation(const JointState& parentState, JointState& state) {
    if (!_owningAvatar->isMyAvatar()) {
        return;
    }
    // get the rotation axes in joint space and use them to adjust the rotation
    glm::vec3 xAxis(1.0f, 0.0f, 0.0f);
    glm::vec3 yAxis(0.0f, 1.0f, 0.0f);
    glm::vec3 zAxis(0.0f, 0.0f, 1.0f);
    glm::quat inverse = glm::inverse(parentState.getRotation() * state.getDefaultRotationInParentFrame());
    state.setRotationInConstrainedFrame(
              glm::angleAxis(- RADIANS_PER_DEGREE * _owningAvatar->getHead()->getFinalLeanSideways(), inverse * zAxis) 
            * glm::angleAxis(- RADIANS_PER_DEGREE * _owningAvatar->getHead()->getFinalLeanForward(), inverse * xAxis)
            * glm::angleAxis(RADIANS_PER_DEGREE * _owningAvatar->getHead()->getTorsoTwist(), inverse * yAxis)
            * state.getFBXJoint().rotation, LEAN_PRIORITY);
}

void SkeletonModel::maybeUpdateNeckRotation(const JointState& parentState, const FBXJoint& joint, JointState& state) {
    _owningAvatar->getHead()->getFaceModel().maybeUpdateNeckRotation(parentState, joint, state);
}

void SkeletonModel::maybeUpdateEyeRotation(const JointState& parentState, const FBXJoint& joint, JointState& state) {
    _owningAvatar->getHead()->getFaceModel().maybeUpdateEyeRotation(this, parentState, joint, state);
}

void SkeletonModel::renderJointConstraints(gpu::Batch& batch, int jointIndex) {
    if (jointIndex == -1 || jointIndex >= _jointStates.size()) {
        return;
    }
    const FBXGeometry& geometry = _geometry->getFBXGeometry();
    const float BASE_DIRECTION_SIZE = 0.3f;
    float directionSize = BASE_DIRECTION_SIZE * extractUniformScale(_scale);
    batch._glLineWidth(3.0f);
    do {
        const FBXJoint& joint = geometry.joints.at(jointIndex);
        const JointState& jointState = _jointStates.at(jointIndex);
        glm::vec3 position = _rotation * jointState.getPosition() + _translation;
        glm::quat parentRotation = (joint.parentIndex == -1) ? _rotation : _rotation * _jointStates.at(joint.parentIndex).getRotation();
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
        
        renderOrientationDirections(jointIndex, position, _rotation * jointState.getRotation(), directionSize);
        jointIndex = joint.parentIndex;
        
    } while (jointIndex != -1 && geometry.joints.at(jointIndex).isFree);
}

void SkeletonModel::renderOrientationDirections(int jointIndex, glm::vec3 position, const glm::quat& orientation, float size) {
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
    geometryCache->renderLine(position, pRight, red, jointLineIDs._right);

    glm::vec3 green(0.0f, 1.0f, 0.0f);
    geometryCache->renderLine(position, pUp, green, jointLineIDs._up);

    glm::vec3 blue(0.0f, 0.0f, 1.0f);
    geometryCache->renderLine(position, pFront, blue, jointLineIDs._front);
}



void SkeletonModel::setHandPosition(int jointIndex, const glm::vec3& position, const glm::quat& rotation) {
    // this algorithm is from sample code from sixense
    const FBXGeometry& geometry = _geometry->getFBXGeometry();
    int elbowJointIndex = geometry.joints.at(jointIndex).parentIndex;
    if (elbowJointIndex == -1) {
        return;
    }
    int shoulderJointIndex = geometry.joints.at(elbowJointIndex).parentIndex;
    glm::vec3 shoulderPosition;
    if (!getJointPosition(shoulderJointIndex, shoulderPosition)) {
        return;
    }
    // precomputed lengths
    float scale = extractUniformScale(_scale);
    float upperArmLength = geometry.joints.at(elbowJointIndex).distanceToParent * scale;
    float lowerArmLength = geometry.joints.at(jointIndex).distanceToParent * scale;
    
    // first set wrist position
    glm::vec3 wristPosition = position;
    
    glm::vec3 shoulderToWrist = wristPosition - shoulderPosition;
    float distanceToWrist = glm::length(shoulderToWrist);
    
    // prevent gimbal lock
    if (distanceToWrist > upperArmLength + lowerArmLength - EPSILON) {
        distanceToWrist = upperArmLength + lowerArmLength - EPSILON;
        shoulderToWrist = glm::normalize(shoulderToWrist) * distanceToWrist;
        wristPosition = shoulderPosition + shoulderToWrist;
    }
    
    // cosine of angle from upper arm to hand vector 
    float cosA = (upperArmLength * upperArmLength + distanceToWrist * distanceToWrist - lowerArmLength * lowerArmLength) / 
        (2 * upperArmLength * distanceToWrist);
    float mid = upperArmLength * cosA;
    float height = sqrt(upperArmLength * upperArmLength + mid * mid - 2 * upperArmLength * mid * cosA);
    
    // direction of the elbow
    glm::vec3 handNormal = glm::cross(rotation * glm::vec3(0.0f, 1.0f, 0.0f), shoulderToWrist); // elbow rotating with wrist
    glm::vec3 relaxedNormal = glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), shoulderToWrist); // elbow pointing straight down
    const float NORMAL_WEIGHT = 0.5f;
    glm::vec3 finalNormal = glm::mix(relaxedNormal, handNormal, NORMAL_WEIGHT);
    
    bool rightHand = (jointIndex == geometry.rightHandJointIndex);
    if (rightHand ? (finalNormal.y > 0.0f) : (finalNormal.y < 0.0f)) {
        finalNormal.y = 0.0f; // dont allow elbows to point inward (y is vertical axis)
    }
    
    glm::vec3 tangent = glm::normalize(glm::cross(shoulderToWrist, finalNormal));
    
    // ik solution
    glm::vec3 elbowPosition = shoulderPosition + glm::normalize(shoulderToWrist) * mid - tangent * height;
    glm::vec3 forwardVector(rightHand ? -1.0f : 1.0f, 0.0f, 0.0f);
    glm::quat shoulderRotation = rotationBetween(forwardVector, elbowPosition - shoulderPosition);

    JointState& shoulderState = _jointStates[shoulderJointIndex];
    shoulderState.setRotationInBindFrame(shoulderRotation, PALM_PRIORITY);
    
    JointState& elbowState = _jointStates[elbowJointIndex];
    elbowState.setRotationInBindFrame(rotationBetween(shoulderRotation * forwardVector, wristPosition - elbowPosition) * shoulderRotation, PALM_PRIORITY);
    
    JointState& handState = _jointStates[jointIndex];
    handState.setRotationInBindFrame(rotation, PALM_PRIORITY);
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
        neckParentRotation = worldFrameRotation * _jointStates[parentIndex].getFBXJoint().inverseDefaultRotation;
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

/// \return offset of hips after foot animation
void SkeletonModel::updateStandingFoot() {
    if (_geometry == NULL) {
        return;
    }
    glm::vec3 offset(0.0f);
    int leftFootIndex = _geometry->getFBXGeometry().leftToeJointIndex;
    int rightFootIndex = _geometry->getFBXGeometry().rightToeJointIndex;

    if (leftFootIndex != -1 && rightFootIndex != -1) {
        glm::vec3 leftPosition, rightPosition;
        getJointPosition(leftFootIndex, leftPosition);
        getJointPosition(rightFootIndex, rightPosition);

        int lowestFoot = (leftPosition.y < rightPosition.y) ? LEFT_FOOT : RIGHT_FOOT;
        const float MIN_STEP_HEIGHT_THRESHOLD = 0.05f;
        bool oneFoot = fabsf(leftPosition.y - rightPosition.y) > MIN_STEP_HEIGHT_THRESHOLD;
        int currentFoot = oneFoot ? lowestFoot : _standingFoot;

        if (_standingFoot == NO_FOOT) {
            currentFoot = lowestFoot;
        }
        if (currentFoot != _standingFoot) {
            if (_standingFoot == NO_FOOT) {
                // pick the lowest foot
                glm::vec3 lowestPosition = (currentFoot == LEFT_FOOT) ? leftPosition : rightPosition;
                // we ignore zero length positions which can happen for a few frames until skeleton is fully loaded
                if (glm::length(lowestPosition) > 0.0f) {
                    _standingFoot = currentFoot;
                    _clampedFootPosition = lowestPosition;
                }
            } else {
                // swap feet
                _standingFoot = currentFoot;
                glm::vec3 nextPosition = leftPosition;
                glm::vec3 prevPosition = rightPosition;
                if (_standingFoot == RIGHT_FOOT) {
                    nextPosition = rightPosition;
                    prevPosition = leftPosition;
                }
                glm::vec3 oldOffset = _clampedFootPosition - prevPosition;
                _clampedFootPosition = oldOffset + nextPosition;
                offset = _clampedFootPosition - nextPosition;
            }
        } else {
            glm::vec3 nextPosition = (_standingFoot == LEFT_FOOT) ? leftPosition : rightPosition;
            offset = _clampedFootPosition - nextPosition;
        }

        // clamp the offset to not exceed some max distance
        const float MAX_STEP_OFFSET = 1.0f;
        float stepDistance = glm::length(offset);
        if (stepDistance > MAX_STEP_OFFSET) {
            offset *= (MAX_STEP_OFFSET / stepDistance);
        }
    }
    _standingOffset = offset;
}

float DENSITY_OF_WATER = 1000.0f; // kg/m^3
float MIN_JOINT_MASS = 1.0f;
float VERY_BIG_MASS = 1.0e6f;

// virtual
void SkeletonModel::buildShapes() {
    if (_geometry == NULL || _jointStates.isEmpty()) {
        return;
    }
    
    const FBXGeometry& geometry = _geometry->getFBXGeometry();
    if (geometry.joints.isEmpty() || geometry.rootJointIndex == -1) {
        // rootJointIndex == -1 if the avatar model has no skeleton
        return;
    }

    float uniformScale = extractUniformScale(_scale);
    const int numStates = _jointStates.size();
    for (int i = 0; i < numStates; i++) {
        JointState& state = _jointStates[i];
        const FBXJoint& joint = state.getFBXJoint();
        float radius = uniformScale * joint.boneRadius;
        float halfHeight = 0.5f * uniformScale * joint.distanceToParent;
        Shape::Type type = joint.shapeType;
        int parentIndex = joint.parentIndex;
        if (parentIndex == -1 || radius < EPSILON) {
            type = INVALID_SHAPE;
        } else if (type == CAPSULE_SHAPE && halfHeight < EPSILON) {
            // this shape is forced to be a sphere
            type = SPHERE_SHAPE;
        }
        Shape* shape = NULL;
        if (type == SPHERE_SHAPE) {
            shape = new SphereShape(radius);
            shape->setEntity(this);
        } else if (type == CAPSULE_SHAPE) {
            assert(parentIndex != -1);
            shape = new CapsuleShape(radius, halfHeight);
            shape->setEntity(this);
        } 
        if (shape && parentIndex != -1) {
            // always disable collisions between joint and its parent
            disableCollisions(i, parentIndex);
        } 
        _shapes.push_back(shape);
    }

    // This method moves the shapes to their default positions in Model frame.
    computeBoundingShape(geometry);
}

void SkeletonModel::computeBoundingShape(const FBXGeometry& geometry) {
    // compute default joint transforms
    int numStates = _jointStates.size();
    assert(numStates == _shapes.size());
    QVector<glm::mat4> transforms;
    transforms.fill(glm::mat4(), numStates);

    // compute bounding box that encloses all shapes
    Extents totalExtents;
    totalExtents.reset();
    totalExtents.addPoint(glm::vec3(0.0f));
    for (int i = 0; i < numStates; i++) {
        // compute the default transform of this joint
        JointState& state = _jointStates[i];
        const FBXJoint& joint = state.getFBXJoint();
        int parentIndex = joint.parentIndex;
        if (parentIndex == -1) {
            transforms[i] = _jointStates[i].getTransform();
        } else {
            glm::quat modifiedRotation = joint.preRotation * joint.rotation * joint.postRotation;    
            transforms[i] = transforms[parentIndex] * glm::translate(joint.translation) 
                * joint.preTransform * glm::mat4_cast(modifiedRotation) * joint.postTransform;
        }

        // Each joint contributes its point to the bounding box
        glm::vec3 jointPosition = extractTranslation(transforms[i]);
        totalExtents.addPoint(jointPosition);

        Shape* shape = _shapes[i];
        if (!shape) {
            continue;
        }

        // Each joint with a shape contributes to the totalExtents: a box 
        // that contains the sphere centered at the end of the joint with radius of the bone.

        // TODO: skip hand and arm shapes for bounding box calculation
        int type = shape->getType();
        if (type == CAPSULE_SHAPE) {
            // add the two furthest surface points of the capsule
            CapsuleShape* capsule = static_cast<CapsuleShape*>(shape);
            float radius = capsule->getRadius();
            glm::vec3 axis(radius);
            Extents shapeExtents;
            shapeExtents.reset();
            shapeExtents.addPoint(jointPosition + axis);
            shapeExtents.addPoint(jointPosition - axis);
            totalExtents.addExtents(shapeExtents);
        } else if (type == SPHERE_SHAPE) {
            float radius = shape->getBoundingRadius();
            glm::vec3 axis(radius);
            Extents shapeExtents;
            shapeExtents.reset();
            shapeExtents.addPoint(jointPosition + axis);
            shapeExtents.addPoint(jointPosition - axis);
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

    glm::vec3 rootPosition = _jointStates[geometry.rootJointIndex].getPosition();
    _boundingShapeLocalOffset = 0.5f * (totalExtents.maximum + totalExtents.minimum) - rootPosition;
    _boundingRadius = 0.5f * glm::length(diagonal);
}

void SkeletonModel::resetShapePositionsToDefaultPose() {
    // DEBUG method.
    // Moves shapes to the joint default locations for debug visibility into
    // how the bounding shape is computed.

    if (!_geometry || _shapes.isEmpty()) {
        // geometry or joints have not yet been created
        return;
    }
    
    const FBXGeometry& geometry = _geometry->getFBXGeometry();
    if (geometry.joints.isEmpty()) {
        return;
    }

    // The shapes are moved to their default positions in computeBoundingShape().
    computeBoundingShape(geometry);

    // Then we move them into world frame for rendering at the Model's location.
    for (int i = 0; i < _shapes.size(); i++) {
        Shape* shape = _shapes[i];
        if (shape) {
            shape->setTranslation(_translation + _rotation * shape->getTranslation());
            shape->setRotation(_rotation * shape->getRotation());
        }
    }
    _boundingShape.setTranslation(_translation + _rotation * _boundingShapeLocalOffset);
    _boundingShape.setRotation(_rotation);
}

void SkeletonModel::renderBoundingCollisionShapes(gpu::Batch& batch, float alpha) {
    const int BALL_SUBDIVISIONS = 10;
    if (_shapes.isEmpty()) {
        // the bounding shape has not been propery computed
        // so no need to render it
        return;
    }

    // draw a blue sphere at the capsule endpoint
    glm::vec3 endPoint;
    _boundingShape.getEndPoint(endPoint);
    endPoint = endPoint + _translation;
    Transform transform = Transform();
    transform.setTranslation(endPoint);
    batch.setModelTransform(transform);
    auto geometryCache = DependencyManager::get<GeometryCache>();
    geometryCache->renderSphere(batch, _boundingShape.getRadius(), BALL_SUBDIVISIONS, BALL_SUBDIVISIONS, 
                                glm::vec4(0.6f, 0.6f, 0.8f, alpha));

    // draw a yellow sphere at the capsule startpoint
    glm::vec3 startPoint;
    _boundingShape.getStartPoint(startPoint);
    startPoint = startPoint + _translation;
    glm::vec3 axis = endPoint - startPoint;
    Transform axisTransform = Transform();
    axisTransform.setTranslation(-axis);
    batch.setModelTransform(axisTransform);
    geometryCache->renderSphere(batch, _boundingShape.getRadius(), BALL_SUBDIVISIONS, BALL_SUBDIVISIONS, 
                                glm::vec4(0.8f, 0.8f, 0.6f, alpha));

    // draw a green cylinder between the two points
    glm::vec3 origin(0.0f);
    Avatar::renderJointConnectingCone(batch, origin, axis, _boundingShape.getRadius(), _boundingShape.getRadius(), 
                                      glm::vec4(0.6f, 0.8f, 0.6f, alpha));
}

bool SkeletonModel::hasSkeleton() {
    return isActive() ? _geometry->getFBXGeometry().rootJointIndex != -1 : false;
}

void SkeletonModel::initHeadBones() {
    _headBones.clear();
    const FBXGeometry& fbxGeometry = _geometry->getFBXGeometry();
    const int neckJointIndex = fbxGeometry.neckJointIndex;
    std::queue<int> q;
    q.push(neckJointIndex);
    _headBones.push_back(neckJointIndex);

    // fbxJoints only hold links to parents not children, so we have to do a bit of extra work here.
    while (q.size() > 0) {
        int jointIndex = q.front();
        for (int i = 0; i < fbxGeometry.joints.size(); i++) {
            const FBXJoint& fbxJoint = fbxGeometry.joints[i];
            if (jointIndex == fbxJoint.parentIndex) {
                _headBones.push_back(i);
                q.push(i);
            }
        }
        q.pop();
    }
}

void SkeletonModel::invalidateHeadBones() {
    _headBones.clear();
}

void SkeletonModel::cauterizeHead() {
    if (isActive()) {
        const FBXGeometry& geometry = _geometry->getFBXGeometry();
        const int neckJointIndex = geometry.neckJointIndex;
        if (neckJointIndex > 0 && neckJointIndex < _jointStates.size()) {

            // lazy init of headBones
            if (_headBones.size() == 0) {
                initHeadBones();
            }

            // preserve the translation for the neck
            glm::vec4 trans = _jointStates[neckJointIndex].getTransform()[3];
            glm::vec4 zero(0, 0, 0, 0);
            for (const int &i : _headBones) {
                JointState& joint = _jointStates[i];
                glm::mat4 newXform(zero, zero, zero, trans);
                joint.setTransform(newXform);
                joint.setVisibleTransform(newXform);
            }
        }
    }
}

void SkeletonModel::onInvalidate() {
    invalidateHeadBones();
}
