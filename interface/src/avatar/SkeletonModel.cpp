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

#include "Application.h"
#include "Avatar.h"
#include "Hand.h"
#include "Menu.h"
#include "SkeletonModel.h"

SkeletonModel::SkeletonModel(Avatar* owningAvatar, QObject* parent) : 
    Model(parent),
    Ragdoll(),
    _owningAvatar(owningAvatar) {
}

void SkeletonModel::setJointStates(QVector<JointState> states) {
    Model::setJointStates(states);

    if (isActive() && _owningAvatar->isMyAvatar()) {
        // extract lists of parentIndex and position from _jointStates...
        QVector<int> parentIndices;
        QVector<glm::vec3> points;
        int numJoints = _jointStates.size();
        parentIndices.reserve(numJoints);
        points.reserve(numJoints);
        for (int i = 0; i < numJoints; ++i) {
            JointState& state = _jointStates[i];
            parentIndices.push_back(state.getFBXJoint().parentIndex);
            points.push_back(state.getPosition());
        }
        // ... and feed the results to Ragdoll
        initShapesAndPoints(&_jointShapes, parentIndices, points);
    }
}

const float PALM_PRIORITY = 3.0f;

void SkeletonModel::simulate(float deltaTime, bool fullUpdate) {
    setTranslation(_owningAvatar->getPosition());
    setRotation(_owningAvatar->getOrientation() * glm::angleAxis(PI, glm::vec3(0.0f, 1.0f, 0.0f)));
    const float MODEL_SCALE = 0.0006f;
    setScale(glm::vec3(1.0f, 1.0f, 1.0f) * _owningAvatar->getScale() * MODEL_SCALE);
    
    Model::simulate(deltaTime, fullUpdate);
    
    if (!(isActive() && _owningAvatar->isMyAvatar())) {
        return; // only simulate for own avatar
    }

    const FBXGeometry& geometry = _geometry->getFBXGeometry();
    PrioVR* prioVR = Application::getInstance()->getPrioVR();
    if (prioVR->isActive()) {
        for (int i = 0; i < prioVR->getJointRotations().size(); i++) {
            int humanIKJointIndex = prioVR->getHumanIKJointIndices().at(i);
            if (humanIKJointIndex == -1) {
                continue;
            }
            int jointIndex = geometry.humanIKJointIndices.at(humanIKJointIndex);
            if (jointIndex != -1) {
                JointState& state = _jointStates[jointIndex];
                state.setRotationFromBindFrame(prioVR->getJointRotations().at(i), PALM_PRIORITY);
            }
        }
        return;
    }

    // find the left and rightmost active palms
    int leftPalmIndex, rightPalmIndex;
    Hand* hand = _owningAvatar->getHand();
    hand->getLeftRightPalmIndices(leftPalmIndex, rightPalmIndex);

    const float HAND_RESTORATION_RATE = 0.25f;    
    if (leftPalmIndex == -1) {
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
        applyPalmData(geometry.leftHandJointIndex, hand->getPalms()[leftPalmIndex]);
        applyPalmData(geometry.rightHandJointIndex, hand->getPalms()[rightPalmIndex]);
    }
    
    simulateRagdoll(deltaTime);
}

void SkeletonModel::simulateRagdoll(float deltaTime) {
    // move ragdoll _points toward joints
    const int numStates = _jointStates.size();
    assert(numStates == _points.size());
    float fraction = 0.1f; // fraction = 0.1f left intentionally low for demo purposes
    float oneMinusFraction = 1.0f - fraction;
    for (int i = 0; i < numStates; ++i) {
        _points[i] = oneMinusFraction * _points[i] + fraction * _jointStates[i].getPosition();
    }

    // enforce the constraints
    float MIN_CONSTRAINT_ERROR = 0.005f; // 5mm
    int MAX_ITERATIONS = 4;
    int iterations = 0;
    float delta = 0.0f;
    do {
        delta = enforceConstraints();
        ++iterations;
    } while (delta > MIN_CONSTRAINT_ERROR && iterations < MAX_ITERATIONS);
}

void SkeletonModel::getHandShapes(int jointIndex, QVector<const Shape*>& shapes) const {
    if (jointIndex < 0 || jointIndex >= int(_jointShapes.size())) {
        return;
    }
    if (jointIndex == getLeftHandJointIndex()
        || jointIndex == getRightHandJointIndex()) {
        // get all shapes that have this hand as an ancestor in the skeleton heirarchy
        const FBXGeometry& geometry = _geometry->getFBXGeometry();
        for (int i = 0; i < _jointStates.size(); i++) {
            const FBXJoint& joint = geometry.joints[i];
            int parentIndex = joint.parentIndex;
            Shape* shape = _jointShapes[i];
            if (i == jointIndex) {
                // this shape is the hand
                if (shape) {
                    shapes.push_back(shape);
                }
                if (parentIndex != -1 && _jointShapes[parentIndex]) {
                    // also add the forearm
                    shapes.push_back(_jointShapes[parentIndex]);
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

void SkeletonModel::getBodyShapes(QVector<const Shape*>& shapes) const {
    // for now we push a single bounding shape, 
    // but later we could push a subset of joint shapes
    shapes.push_back(&_boundingShape);
}

void SkeletonModel::renderIKConstraints() {
    renderJointConstraints(getRightHandJointIndex());
    renderJointConstraints(getLeftHandJointIndex());
    //if (isActive() && _owningAvatar->isMyAvatar()) {
    //    renderRagdoll();
    //}
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
        parentState.setRotationFromBindFrame(palmRotation, PALM_PRIORITY);
        // lock hand to forearm by slamming its rotation (in parent-frame) to identity
        _jointStates[jointIndex]._rotationInParentFrame = glm::quat();
    } else {
        setJointPosition(jointIndex, palmPosition, palmRotation, 
                true, -1, false, glm::vec3(0.0f, -1.0f, 0.0f), PALM_PRIORITY);
    }
}

void SkeletonModel::updateJointState(int index) {
    JointState& state = _jointStates[index];
    const FBXJoint& joint = state.getFBXJoint();
    if (joint.parentIndex != -1) {
        const JointState& parentState = _jointStates.at(joint.parentIndex);
        const FBXGeometry& geometry = _geometry->getFBXGeometry();
        if (index == geometry.leanJointIndex) {
            maybeUpdateLeanRotation(parentState, joint, state);
        
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

void SkeletonModel::updateShapePositions() {
    if (isActive() && _owningAvatar->isMyAvatar() && 
            Menu::getInstance()->isOptionChecked(MenuOption::CollideAsRagdoll)) {
        // TODO: Andrew to move shape updates to SimulationEngine
        //updateShapes(_rotation, _translation);
    } else {
        Model::updateShapePositions();
    }
}

void SkeletonModel::maybeUpdateLeanRotation(const JointState& parentState, const FBXJoint& joint, JointState& state) {
    if (!_owningAvatar->isMyAvatar() || Application::getInstance()->getPrioVR()->isActive()) {
        return;
    }
    // get the rotation axes in joint space and use them to adjust the rotation
    glm::mat3 axes = glm::mat3_cast(glm::quat());
    glm::mat3 inverse = glm::mat3(glm::inverse(parentState.getTransform() * glm::translate(state.getDefaultTranslationInParentFrame()) *
        joint.preTransform * glm::mat4_cast(joint.preRotation * joint.rotation)));
    state._rotationInParentFrame = glm::angleAxis(- RADIANS_PER_DEGREE * _owningAvatar->getHead()->getFinalLeanSideways(), 
        glm::normalize(inverse * axes[2])) * glm::angleAxis(- RADIANS_PER_DEGREE * _owningAvatar->getHead()->getFinalLeanForward(), 
        glm::normalize(inverse * axes[0])) * joint.rotation;
}

void SkeletonModel::maybeUpdateNeckRotation(const JointState& parentState, const FBXJoint& joint, JointState& state) {
    _owningAvatar->getHead()->getFaceModel().maybeUpdateNeckRotation(parentState, joint, state);
}

void SkeletonModel::maybeUpdateEyeRotation(const JointState& parentState, const FBXJoint& joint, JointState& state) {
    _owningAvatar->getHead()->getFaceModel().maybeUpdateEyeRotation(parentState, joint, state);
}

void SkeletonModel::renderJointConstraints(int jointIndex) {
    if (jointIndex == -1 || jointIndex >= _jointStates.size()) {
        return;
    }
    const FBXGeometry& geometry = _geometry->getFBXGeometry();
    const float BASE_DIRECTION_SIZE = 300.0f;
    float directionSize = BASE_DIRECTION_SIZE * extractUniformScale(_scale);
    glLineWidth(3.0f);
    do {
        const FBXJoint& joint = geometry.joints.at(jointIndex);
        const JointState& jointState = _jointStates.at(jointIndex);
        glm::vec3 position = _rotation * jointState.getPosition() + _translation;
        
        glPushMatrix();
        glTranslatef(position.x, position.y, position.z);
        glm::quat parentRotation = (joint.parentIndex == -1) ? _rotation : _rotation * _jointStates.at(joint.parentIndex).getRotation();
        glm::vec3 rotationAxis = glm::axis(parentRotation);
        glRotatef(glm::degrees(glm::angle(parentRotation)), rotationAxis.x, rotationAxis.y, rotationAxis.z);
        float fanScale = directionSize * 0.75f;
        glScalef(fanScale, fanScale, fanScale);
        const int AXIS_COUNT = 3;
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
            glColor4f(otherAxis.r, otherAxis.g, otherAxis.b, 0.75f);
        
            glBegin(GL_TRIANGLE_FAN);
            glVertex3f(0.0f, 0.0f, 0.0f);
            const int FAN_SEGMENTS = 16;
            for (int j = 0; j < FAN_SEGMENTS; j++) {
                glm::vec3 rotated = glm::angleAxis(glm::mix(joint.rotationMin[i], joint.rotationMax[i],
                    (float)j / (FAN_SEGMENTS - 1)), axis) * otherAxis;
                glVertex3f(rotated.x, rotated.y, rotated.z);
            }
            glEnd();
        }
        glPopMatrix();
        
        renderOrientationDirections(position, _rotation * jointState.getRotation(), directionSize);
        jointIndex = joint.parentIndex;
        
    } while (jointIndex != -1 && geometry.joints.at(jointIndex).isFree);
    
    glLineWidth(1.0f);
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
    shoulderState.setRotationFromBindFrame(shoulderRotation, PALM_PRIORITY);
    
    JointState& elbowState = _jointStates[elbowJointIndex];
    elbowState.setRotationFromBindFrame(rotationBetween(shoulderRotation * forwardVector, wristPosition - elbowPosition) * shoulderRotation, PALM_PRIORITY);
    
    JointState& handState = _jointStates[jointIndex];
    handState.setRotationFromBindFrame(rotation, PALM_PRIORITY);
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

bool SkeletonModel::getNeckParentRotation(glm::quat& neckParentRotation) const {
    if (!isActive()) {
        return false;
    }
    const FBXGeometry& geometry = _geometry->getFBXGeometry();
    if (geometry.neckJointIndex == -1) {
        return false;
    }
    return getJointRotationInWorldFrame(geometry.joints.at(geometry.neckJointIndex).parentIndex, neckParentRotation);
}

bool SkeletonModel::getEyePositions(glm::vec3& firstEyePosition, glm::vec3& secondEyePosition) const {
    if (!isActive()) {
        return false;
    }
    const FBXGeometry& geometry = _geometry->getFBXGeometry();
    if (getJointPositionInWorldFrame(geometry.leftEyeJointIndex, firstEyePosition) &&
            getJointPositionInWorldFrame(geometry.rightEyeJointIndex, secondEyePosition)) {
        return true;
    }
    // no eye joints; try to estimate based on head/neck joints
    glm::vec3 neckPosition, headPosition;
    if (getJointPositionInWorldFrame(geometry.neckJointIndex, neckPosition) &&
            getJointPositionInWorldFrame(geometry.headJointIndex, headPosition)) {
        const float EYE_PROPORTION = 0.6f;
        glm::vec3 baseEyePosition = glm::mix(neckPosition, headPosition, EYE_PROPORTION);
        glm::quat headRotation;
        getJointRotationInWorldFrame(geometry.headJointIndex, headRotation);
        const float EYES_FORWARD = 0.25f;
        const float EYE_SEPARATION = 0.1f;
        float headHeight = glm::distance(neckPosition, headPosition);
        firstEyePosition = baseEyePosition + headRotation * glm::vec3(EYE_SEPARATION, 0.0f, EYES_FORWARD) * headHeight;
        secondEyePosition = baseEyePosition + headRotation * glm::vec3(-EYE_SEPARATION, 0.0f, EYES_FORWARD) * headHeight;
        return true;
    }
    return false;
}

void SkeletonModel::renderRagdoll() {
    const int BALL_SUBDIVISIONS = 6;
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glPushMatrix();

    Application::getInstance()->loadTranslatedViewMatrix(_translation);
    int numPoints = _points.size();
    float alpha = 0.3f;
    float radius1 = 0.008f;
    float radius2 = 0.01f;
    for (int i = 0; i < numPoints; ++i) {
        glPushMatrix();
        // draw each point as a yellow hexagon with black border
        glm::vec3 position = _rotation * _points[i];
        glTranslatef(position.x, position.y, position.z);
        glColor4f(0.0f, 0.0f, 0.0f, alpha);
        glutSolidSphere(radius2, BALL_SUBDIVISIONS, BALL_SUBDIVISIONS);
        glColor4f(1.0f, 1.0f, 0.0f, alpha);
        glutSolidSphere(radius1, BALL_SUBDIVISIONS, BALL_SUBDIVISIONS);
        glPopMatrix();
    }
    glPopMatrix();
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
}
