//
//  Flow.cpp
//
//  Created by Luis Cuenca on 1/21/2019.
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Flow.h"
#include "Rig.h"
#include "AnimSkeleton.h"

const std::map<QString, FlowPhysicsSettings> PRESET_FLOW_DATA = { { "hair", FlowPhysicsSettings() },
{ "skirt", FlowPhysicsSettings(true, 1.0f, DEFAULT_GRAVITY, 0.65f, 0.8f, 0.45f, 0.01f) },
{ "breast", FlowPhysicsSettings(true, 1.0f, DEFAULT_GRAVITY, 0.65f, 0.8f, 0.45f, 0.01f) } };

const std::map<QString, FlowCollisionSettings> PRESET_COLLISION_DATA = {
    { "Spine2", FlowCollisionSettings(QUuid(), FlowCollisionType::CollisionSphere, glm::vec3(0.0f, 0.2f, 0.0f), 0.14f) },
    { "LeftArm", FlowCollisionSettings(QUuid(), FlowCollisionType::CollisionSphere, glm::vec3(0.0f, 0.02f, 0.0f), 0.05f) },
    { "RightArm", FlowCollisionSettings(QUuid(), FlowCollisionType::CollisionSphere, glm::vec3(0.0f, 0.02f, 0.0f), 0.05f) },
    { "HeadTop_End", FlowCollisionSettings(QUuid(), FlowCollisionType::CollisionSphere, glm::vec3(0.0f, -0.15f, 0.0f), 0.09f) }
};

FlowCollisionSphere::FlowCollisionSphere(const int& jointIndex, const FlowCollisionSettings& settings, bool isTouch) {
    _jointIndex = jointIndex;
    _radius = _initialRadius = settings._radius;
    _offset = _initialOffset = settings._offset;
    _entityID = settings._entityID;
    _isTouch = isTouch;
}

FlowCollisionResult FlowCollisionSphere::computeSphereCollision(const glm::vec3& point, float radius) const {
    FlowCollisionResult result;
    auto centerToJoint = point - _position;
    result._distance = glm::length(centerToJoint) - radius;
    result._offset = _radius - result._distance;
    result._normal = glm::normalize(centerToJoint);
    result._radius = _radius;
    result._position = _position;
    return result;
}

FlowCollisionResult FlowCollisionSphere::checkSegmentCollision(const glm::vec3& point1, const glm::vec3& point2, const FlowCollisionResult& collisionResult1, const FlowCollisionResult& collisionResult2) {
    FlowCollisionResult result;
    auto segment = point2 - point1;
    auto segmentLength = glm::length(segment);
    auto maxDistance = glm::sqrt(glm::pow(collisionResult1._radius, 2) + glm::pow(segmentLength, 2));
    if (collisionResult1._distance < maxDistance && collisionResult2._distance < maxDistance) {
        float segmentPercentage = collisionResult1._distance / (collisionResult1._distance + collisionResult2._distance);
        glm::vec3 collisionPoint = point1 + segment * segmentPercentage;
        glm::vec3 centerToSegment = collisionPoint - _position;
        float distance = glm::length(centerToSegment);
        if (distance < _radius) {
            result._offset = _radius - distance;
            result._position = _position;
            result._radius = _radius;
            result._normal = glm::normalize(centerToSegment);
            result._distance = distance;
        }
    }
    return result;
}

void FlowCollisionSystem::addCollisionSphere(int jointIndex, const FlowCollisionSettings& settings, const glm::vec3& position, bool isSelfCollision, bool isTouch) {
    auto collision = FlowCollisionSphere(jointIndex, settings, isTouch);
    collision.setPosition(position);
    if (isSelfCollision) {
        _selfCollisions.push_back(collision);
    } else {
        _othersCollisions.push_back(collision);
    }

};
void FlowCollisionSystem::resetCollisions() {
    _allCollisions.clear();
    _othersCollisions.clear();
    _selfCollisions.clear();
}
FlowCollisionResult FlowCollisionSystem::computeCollision(const std::vector<FlowCollisionResult> collisions) {
    FlowCollisionResult result;
    if (collisions.size() > 1) {
        for (size_t i = 0; i < collisions.size(); i++) {
            result._offset += collisions[i]._offset;
            result._normal = result._normal + collisions[i]._normal * collisions[i]._distance;
            result._position = result._position + collisions[i]._position;
            result._radius += collisions[i]._radius;
            result._distance += collisions[i]._distance;
        }
        result._offset = result._offset / collisions.size();
        result._radius = 0.5f * glm::length(result._normal);
        result._normal = glm::normalize(result._normal);
        result._position = result._position / (float)collisions.size();
        result._distance = result._distance / collisions.size();
    }
    else if (collisions.size() == 1) {
        result = collisions[0];
    }
    result._count = (int)collisions.size();
    return result;
};

void FlowCollisionSystem::setScale(float scale) {
    _scale = scale;
    for (size_t j = 0; j < _selfCollisions.size(); j++) {
        _selfCollisions[j]._radius = _selfCollisions[j]._initialRadius * scale;
        _selfCollisions[j]._offset = _selfCollisions[j]._initialOffset * scale;
    }
};

std::vector<FlowCollisionResult> FlowCollisionSystem::checkFlowThreadCollisions(FlowThread* flowThread) {
    std::vector<std::vector<FlowCollisionResult>> FlowThreadResults;
    FlowThreadResults.resize(flowThread->_joints.size());
    for (size_t j = 0; j < _allCollisions.size(); j++) {
        FlowCollisionSphere &sphere = _allCollisions[j];
        FlowCollisionResult rootCollision = sphere.computeSphereCollision(flowThread->_positions[0], flowThread->_radius);
        std::vector<FlowCollisionResult> collisionData = { rootCollision };
        bool tooFar = rootCollision._distance >(flowThread->_length + rootCollision._radius);
        FlowCollisionResult nextCollision;
        if (!tooFar) {
            if (sphere._isTouch) {
                for (size_t i = 1; i < flowThread->_joints.size(); i++) {
                    auto prevCollision = collisionData[i - 1];
                    nextCollision = _allCollisions[j].computeSphereCollision(flowThread->_positions[i], flowThread->_radius);
                    collisionData.push_back(nextCollision);
                    bool isTouching = false;
                    if (prevCollision._offset > 0.0f) {
                        if (i == 1) {
                            FlowThreadResults[i - 1].push_back(prevCollision);
                            isTouching = true;
                        }
                    } else if (nextCollision._offset > 0.0f) {
                        FlowThreadResults[i].push_back(nextCollision);
                        isTouching = true;
                    } else {
                        FlowCollisionResult segmentCollision = _allCollisions[j].checkSegmentCollision(flowThread->_positions[i - 1], flowThread->_positions[i], prevCollision, nextCollision);
                        if (segmentCollision._offset > 0) {
                            FlowThreadResults[i - 1].push_back(segmentCollision);
                            FlowThreadResults[i].push_back(segmentCollision);
                            isTouching = true;
                        }
                    }
                }
            } else {
                if (rootCollision._offset > 0.0f) {
                    FlowThreadResults[0].push_back(rootCollision);
                }
                for (size_t i = 1; i < flowThread->_joints.size(); i++) {
                    nextCollision = _allCollisions[j].computeSphereCollision(flowThread->_positions[i], flowThread->_radius);
                    if (nextCollision._offset > 0.0f) {
                        FlowThreadResults[i].push_back(nextCollision);
                    }
                }
            }
        }
    }

    std::vector<FlowCollisionResult> results;
    for (size_t i = 0; i < flowThread->_joints.size(); i++) {
        results.push_back(computeCollision(FlowThreadResults[i]));
    }
    return results;
};

int FlowCollisionSystem::findSelfCollisionWithJoint(int jointIndex) {
    for (size_t i = 0; i < _selfCollisions.size(); i++) {
        if (_selfCollisions[i]._jointIndex == jointIndex) {
            return (int)i;
        }
    }
    return -1;
};

void FlowCollisionSystem::modifySelfCollisionRadius(int jointIndex, float radius) {
    int collisionIndex = findSelfCollisionWithJoint(jointIndex);
    if (collisionIndex > -1) {
        _selfCollisions[collisionIndex]._initialRadius = radius;
        _selfCollisions[collisionIndex]._radius = _scale * radius;
    }
};

void FlowCollisionSystem::modifySelfCollisionYOffset(int jointIndex, float offset) {
    int collisionIndex = findSelfCollisionWithJoint(jointIndex);
    if (collisionIndex > -1) {
        auto currentOffset = _selfCollisions[collisionIndex]._offset;
        _selfCollisions[collisionIndex]._initialOffset = glm::vec3(currentOffset.x, offset, currentOffset.z);
        _selfCollisions[collisionIndex]._offset = _selfCollisions[collisionIndex]._initialOffset * _scale;
    }
};

void FlowCollisionSystem::modifySelfCollisionOffset(int jointIndex, const glm::vec3& offset) {
    int collisionIndex = findSelfCollisionWithJoint(jointIndex);
    if (collisionIndex > -1) {
        _selfCollisions[collisionIndex]._initialOffset = offset;
        _selfCollisions[collisionIndex]._offset = _selfCollisions[collisionIndex]._initialOffset * _scale;
    }
};


void FlowCollisionSystem::prepareCollisions() {
    _allCollisions.clear();
    _allCollisions.resize(_selfCollisions.size() + _othersCollisions.size());
    std::copy(_selfCollisions.begin(), _selfCollisions.begin() + _selfCollisions.size(), _allCollisions.begin());
    std::copy(_othersCollisions.begin(), _othersCollisions.begin() + _othersCollisions.size(), _allCollisions.begin() + _selfCollisions.size());
    _othersCollisions.clear();
}

void FlowNode::update(const glm::vec3& accelerationOffset) {
    _acceleration = glm::vec3(0.0f, _settings._gravity, 0.0f);
    _previousVelocity = _currentVelocity;
    _currentVelocity = _currentPosition - _previousPosition;
    _previousPosition = _currentPosition;
    if (!_anchored) {
        // Add inertia
        auto deltaVelocity = _previousVelocity - _currentVelocity;
        auto centrifugeVector = glm::length(deltaVelocity) != 0.0f ? glm::normalize(deltaVelocity) : glm::vec3();
        _acceleration = _acceleration + centrifugeVector * _settings._inertia * glm::length(_currentVelocity);

        // Add offset
        _acceleration += accelerationOffset;
        // Calculate new position
        _currentPosition = (_currentPosition + _currentVelocity * _settings._damping) +
            (_acceleration * glm::pow(_settings._delta * _scale, 2));
    }
    else {
        _acceleration = glm::vec3(0.0f);
        _currentVelocity = glm::vec3(0.0f);
    }
};


void FlowNode::solve(const glm::vec3& constrainPoint, float maxDistance, const FlowCollisionResult& collision) {
    solveConstraints(constrainPoint, maxDistance);
    solveCollisions(collision);
};

void FlowNode::solveConstraints(const glm::vec3& constrainPoint, float maxDistance) {
    glm::vec3 constrainVector = _currentPosition - constrainPoint;
    float difference = maxDistance / glm::length(constrainVector);
    _currentPosition = difference < 1.0 ? constrainPoint + constrainVector * difference : _currentPosition;
};

void FlowNode::solveCollisions(const FlowCollisionResult& collision) {
    _colliding = collision._offset > 0.0f;
    _collision = collision;
    if (_colliding) {
        _currentPosition = _currentPosition + collision._normal * collision._offset;
        _previousCollision = collision;
    }
    else {
        _previousCollision = FlowCollisionResult();
    }
};

FlowJoint::FlowJoint(int jointIndex, int parentIndex, int childIndex, const QString& name, const QString& group, float scale, const FlowPhysicsSettings& settings) {
    _index = jointIndex;
    _name = name;
    _group = group;
    _scale = scale;
    _childIndex = childIndex;
    _parentIndex = parentIndex;
    _node = FlowNode(glm::vec3(), settings);
};

void FlowJoint::setInitialData(const glm::vec3& initialPosition, const glm::vec3& initialTranslation, const glm::quat& initialRotation, const glm::vec3& parentPosition) {
    _initialPosition = initialPosition;
    _node._initialPosition = initialPosition;
    _node._previousPosition = initialPosition;
    _node._currentPosition = initialPosition;
    _initialTranslation = initialTranslation;
    _initialRotation = initialRotation;
    _translationDirection = glm::normalize(_initialTranslation);
    _parentPosition = parentPosition;
    _length = glm::length(_initialPosition - parentPosition);
    _originalLength = _length / _scale;
}

void FlowJoint::setUpdatedData(const glm::vec3& updatedPosition, const glm::vec3& updatedTranslation, const glm::quat& updatedRotation, const glm::vec3& parentPosition, const glm::quat& parentWorldRotation) {
    _updatedPosition = updatedPosition;
    _updatedRotation = updatedRotation;
    _updatedTranslation = updatedTranslation;
    _parentPosition = parentPosition;
    _parentWorldRotation = parentWorldRotation;
}

void FlowJoint::setRecoveryPosition(const glm::vec3& recoveryPosition) {
    _recoveryPosition = recoveryPosition;
    _applyRecovery = true;
}

void FlowJoint::update() {
    glm::vec3 accelerationOffset = glm::vec3(0.0f);
    if (_node._settings._stiffness > 0.0f) {
        glm::vec3 recoveryVector = _recoveryPosition - _node._currentPosition;
        accelerationOffset = recoveryVector * glm::pow(_node._settings._stiffness, 3);
    }
    _node.update(accelerationOffset);
    if (_node._anchored) {
        if (!_isDummy) {
            _node._currentPosition = _updatedPosition;
        } else {
            _node._currentPosition = _parentPosition;
        }
    }
};

void FlowJoint::solve(const FlowCollisionResult& collision) {
    _node.solve(_parentPosition, _length, collision);
};

FlowDummyJoint::FlowDummyJoint(const glm::vec3& initialPosition, int index, int parentIndex, int childIndex, float scale, FlowPhysicsSettings settings) :
    FlowJoint(index, parentIndex, childIndex, DUMMY_KEYWORD + "_" + index, DUMMY_KEYWORD, scale, settings) {
    _isDummy = true;
    _initialPosition = initialPosition;
    _node = FlowNode(_initialPosition, settings);
    _length = DUMMY_JOINT_DISTANCE;
}


FlowThread::FlowThread(int rootIndex, std::map<int, FlowJoint>* joints) {
    _jointsPointer = joints;
    computeFlowThread(rootIndex);
}

void FlowThread::resetLength() {
    _length = 0.0f;
    for (size_t i = 1; i < _joints.size(); i++) {
        int index = _joints[i];
        _length += _jointsPointer->at(index)._length;
    }
}

void FlowThread::computeFlowThread(int rootIndex) {
    int parentIndex = rootIndex;
    if (_jointsPointer->size() == 0) {
        return;
    }
    int childIndex = _jointsPointer->at(parentIndex)._childIndex;
    std::vector<int> indexes = { parentIndex };
    for (size_t i = 0; i < _jointsPointer->size(); i++) {
        if (childIndex > -1) {
            indexes.push_back(childIndex);
            childIndex = _jointsPointer->at(childIndex)._childIndex;
        } else {
            break;
        }
    }
    for (size_t i = 0; i < indexes.size(); i++) {
        int index = indexes[i];
        _joints.push_back(index);
        if (i > 0) {
            _length += _jointsPointer->at(index)._length;
        }
    }
};

void FlowThread::computeRecovery() {
    int parentIndex = _joints[0];
    auto parentJoint = _jointsPointer->at(parentIndex);
    _jointsPointer->at(parentIndex)._recoveryPosition = parentJoint._recoveryPosition = parentJoint._node._currentPosition;
    glm::quat parentRotation = parentJoint._parentWorldRotation * parentJoint._initialRotation;
    for (size_t i = 1; i < _joints.size(); i++) {
        auto joint = _jointsPointer->at(_joints[i]);
        glm::quat rotation = i == 1 ? parentRotation : rotation * parentJoint._initialRotation;
        _jointsPointer->at(_joints[i])._recoveryPosition = joint._recoveryPosition = parentJoint._recoveryPosition + (rotation * (joint._initialTranslation * 0.01f));
        parentJoint = joint;
    }
};

void FlowThread::update() {
    if (getActive()) {
        _positions.clear();
        _radius = _jointsPointer->at(_joints[0])._node._settings._radius;
        computeRecovery();
        for (size_t i = 0; i < _joints.size(); i++) {
            auto &joint = _jointsPointer->at(_joints[i]);
            joint.update();
            _positions.push_back(joint._node._currentPosition);
        }
    }
};

void FlowThread::solve(bool useCollisions, FlowCollisionSystem& collisionSystem) {
    if (getActive()) {
        if (useCollisions) {
            auto bodyCollisions = collisionSystem.checkFlowThreadCollisions(this);
            int handTouchedJoint = -1;
            for (size_t i = 0; i < _joints.size(); i++) {
                int index = _joints[i];
                _jointsPointer->at(index).solve(bodyCollisions[i]);
            }
        } else {
            for (size_t i = 0; i < _joints.size(); i++) {
                int index = _joints[i];
                _jointsPointer->at(index).solve(FlowCollisionResult());
            }
        }
    }
};

void FlowThread::computeJointRotations() {

    auto pos0 = _rootFramePositions[0];
    auto pos1 = _rootFramePositions[1];

    auto joint0 = _jointsPointer->at(_joints[0]);
    auto joint1 = _jointsPointer->at(_joints[1]);

    auto initial_pos1 = pos0 + (joint0._initialRotation * (joint1._initialTranslation * 0.01f));

    auto vec0 = initial_pos1 - pos0;
    auto vec1 = pos1 - pos0;

    auto delta = rotationBetween(vec0, vec1);

    joint0._currentRotation = _jointsPointer->at(_joints[0])._currentRotation = delta * joint0._initialRotation;
    
    for (size_t i = 1; i < _joints.size() - 1; i++) {
        auto nextJoint = _jointsPointer->at(_joints[i + 1]);
        for (size_t j = i; j < _joints.size(); j++) {
            _rootFramePositions[j] = glm::inverse(joint0._currentRotation) * _rootFramePositions[j] - (joint0._initialTranslation * 0.01f);
        }
        pos0 = _rootFramePositions[i];
        pos1 = _rootFramePositions[i + 1];
        initial_pos1 = pos0 + joint1._initialRotation * (nextJoint._initialTranslation * 0.01f);

        vec0 = initial_pos1 - pos0;
        vec1 = pos1 - pos0;

        delta = rotationBetween(vec0, vec1);

        joint1._currentRotation = _jointsPointer->at(joint1._index)._currentRotation = delta * joint1._initialRotation;
        joint0 = joint1;
        joint1 = nextJoint;
    }
};

void FlowThread::apply() {
    if (!getActive()) {
        return;
    }
    computeJointRotations();
};

bool FlowThread::getActive() {
    return _jointsPointer->at(_joints[0])._node._active;
};

void Flow::setRig(Rig* rig) {
    _rig = rig;
};

void Flow::calculateConstraints() {
    cleanUp();
    if (!_rig) {
        return;
    }
    int rightHandIndex = -1;
    auto flowPrefix = FLOW_JOINT_PREFIX.toUpper();
    auto simPrefix = SIM_JOINT_PREFIX.toUpper();
    auto skeleton = _rig->getAnimSkeleton();
    std::vector<int> handsIndices;
    _collisionSystem.resetCollisions();
    if (skeleton) {
        for (int i = 0; i < skeleton->getNumJoints(); i++) {
            auto name = skeleton->getJointName(i);
            if (name == "RightHand") {
                rightHandIndex = i;
            }
            if (std::find(HAND_COLLISION_JOINTS.begin(), HAND_COLLISION_JOINTS.end(), name) != HAND_COLLISION_JOINTS.end()) {
                handsIndices.push_back(i);
            }
            auto parentIndex = skeleton->getParentIndex(i);
            if (parentIndex == -1) {
                continue;
            }
            auto jointChildren = skeleton->getChildrenOfJoint(i);
            // auto childIndex = jointChildren.size() > 0 ? jointChildren[0] : -1;
            auto group = QStringRef(&name, 0, 3).toString().toUpper();
            auto split = name.split("_");
            bool isSimJoint = (group == simPrefix);
            bool isFlowJoint = split.size() > 2 && split[0].toUpper() == flowPrefix;
            if (isFlowJoint || isSimJoint) {
                group = "";
                if (isSimJoint) {
                    qDebug() << "FLOW is sim: " << name;
                    for (size_t j = 1; j < name.size() - 1; j++) {
                        bool toFloatSuccess;
                        auto subname = (QStringRef(&name, (int)(name.size() - j), 1)).toString().toFloat(&toFloatSuccess);
                        if (!toFloatSuccess && (name.size() - j) > simPrefix.size()) {
                            group = QStringRef(&name, simPrefix.size(), (int)(name.size() - j + 1)).toString();
                            break;
                        }
                    }
                    if (group.isEmpty()) {
                        group = QStringRef(&name, simPrefix.size(), name.size() - 1).toString();
                    }
                } else {
                    group = split[1];
                }
                if (!group.isEmpty()) {
                    _flowJointKeywords.push_back(group);
                    FlowPhysicsSettings jointSettings;
                    if (PRESET_FLOW_DATA.find(group) != PRESET_FLOW_DATA.end()) {
                        jointSettings = PRESET_FLOW_DATA.at(group);
                    } else {
                        jointSettings = DEFAULT_JOINT_SETTINGS;
                    }
                    if (_flowJointData.find(i) ==  _flowJointData.end()) {
                        auto flowJoint = FlowJoint(i, parentIndex, -1, name, group, _scale, jointSettings);
                        _flowJointData.insert(std::pair<int, FlowJoint>(i, flowJoint));
                    }
                }
            } else {
                if (PRESET_COLLISION_DATA.find(name) != PRESET_COLLISION_DATA.end()) {
                    _collisionSystem.addCollisionSphere(i, PRESET_COLLISION_DATA.at(name));
                }
            }
            if (isFlowJoint || isSimJoint) {
                auto jointInfo = FlowJointInfo(i, parentIndex, -1, name);
                _flowJointInfos.push_back(jointInfo);
            }
        }
    }

    for (auto &jointData : _flowJointData) {
        int jointIndex = jointData.first;
        glm::vec3 jointPosition, parentPosition, jointTranslation;
        glm::quat jointRotation;
        _rig->getJointPositionInWorldFrame(jointIndex, jointPosition, _entityPosition, _entityRotation);
        _rig->getJointPositionInWorldFrame(jointData.second._parentIndex, parentPosition, _entityPosition, _entityRotation);
        _rig->getJointTranslation(jointIndex, jointTranslation);
        _rig->getJointRotation(jointIndex, jointRotation);

        jointData.second.setInitialData(jointPosition, jointTranslation, jointRotation, parentPosition);
    }

    std::vector<int> roots;

    for (auto& itr = _flowJointData.begin(); itr != _flowJointData.end(); itr++) {
        if (_flowJointData.find(itr->second._parentIndex) == _flowJointData.end()) {
            itr->second._node._anchored = true;
            roots.push_back(itr->first);
        } else {
            _flowJointData[itr->second._parentIndex]._childIndex = itr->first;
        }
    }

    for (size_t i = 0; i < roots.size(); i++) {
        FlowThread thread = FlowThread(roots[i], &_flowJointData);
        // add threads with at least 2 joints
        if (thread._joints.size() > 0) {
            if (thread._joints.size() == 1) {
                int jointIndex = roots[i];
                auto joint = _flowJointData[jointIndex];
                auto jointPosition = joint._updatedPosition;
                auto newSettings = FlowPhysicsSettings(joint._node._settings);
                newSettings._stiffness = ISOLATED_JOINT_STIFFNESS;
                int extraIndex = (int)_flowJointData.size();
                auto newJoint = FlowDummyJoint(jointPosition, extraIndex, jointIndex, -1, _scale, newSettings);
                newJoint._isDummy = false;
                newJoint._length = ISOLATED_JOINT_LENGTH;
                newJoint._childIndex = extraIndex;
                newJoint._group = _flowJointData[jointIndex]._group;
                thread = FlowThread(jointIndex, &_flowJointData);
            }
            _jointThreads.push_back(thread);
        }
    }
    
    if (_jointThreads.size() == 0) {
        _rig->clearJointStates();
    }
    if (SHOW_DUMMY_JOINTS && rightHandIndex > -1) {
        int jointCount = (int)_flowJointData.size();
        int extraIndex = (int)_flowJointData.size();
        glm::vec3 rightHandPosition;
        _rig->getJointPositionInWorldFrame(rightHandIndex, rightHandPosition, _entityPosition, _entityRotation);
        int parentIndex = rightHandIndex;
        for (int i = 0; i < DUMMY_JOINT_COUNT; i++) {
            int childIndex = (i == (DUMMY_JOINT_COUNT - 1)) ? -1 : extraIndex + 1;
            auto newJoint = FlowDummyJoint(rightHandPosition, extraIndex, parentIndex, childIndex, _scale, DEFAULT_JOINT_SETTINGS);
            _flowJointData.insert(std::pair<int, FlowJoint>(extraIndex, newJoint));
            parentIndex = extraIndex;
            extraIndex++;
        }
        _flowJointData[jointCount]._node._anchored = true;

        auto extraThread = FlowThread(jointCount, &_flowJointData);
        _jointThreads.push_back(extraThread);
    }
    if (handsIndices.size() > 0) {
        FlowCollisionSettings handSettings;
        handSettings._radius = HAND_COLLISION_RADIUS;
        for (size_t i = 0; i < handsIndices.size(); i++) {
            _collisionSystem.addCollisionSphere(handsIndices[i], handSettings, glm::vec3(), true, true);
        }
    }
    _initialized = _jointThreads.size() > 0;
}

void Flow::cleanUp() {
    _flowJointData.clear();
    _jointThreads.clear();
    _flowJointKeywords.clear();
    _flowJointInfos.clear();
 }

void Flow::setTransform(float scale, const glm::vec3& position, const glm::quat& rotation) {
    _scale = scale;
    for (auto &joint : _flowJointData) {
        joint.second._scale = scale;
        joint.second._node._scale = scale;
    }
    _entityPosition = position;
    _entityRotation = rotation;
    _active = true;
}

void Flow::update() {
    QElapsedTimer _timer;
    _timer.start();
    if (_initialized && _active) {
        updateJoints();
        int count = 0;
        for (auto &thread : _jointThreads) {
            thread.update();
            thread.solve(USE_COLLISIONS, _collisionSystem);
            if (!updateRootFramePositions(count++)) {
                return;
            }
            thread.apply();
        }
        setJoints();
    }
    _deltaTime += _timer.nsecsElapsed();
    _updates++;
    if (_deltaTime > _deltaTimeLimit) {
        qDebug() << "Flow C++ update " << _deltaTime / _updates << " nanoSeconds";
        _deltaTime = 0;
        _updates = 0;
    }
    
}

bool Flow::worldToJointPoint(const glm::vec3& position, const int jointIndex, glm::vec3& jointSpacePosition) const {
    glm::vec3 jointPos;
    glm::quat jointRot;
    if (_rig->getJointPositionInWorldFrame(jointIndex, jointPos, _entityPosition, _entityRotation) && _rig->getJointRotationInWorldFrame(jointIndex, jointRot, _entityRotation)) {
        glm::vec3 modelOffset = position - jointPos;
        jointSpacePosition = glm::inverse(jointRot) * modelOffset;
        return true;
    }
    return false;
}

bool Flow::updateRootFramePositions(int threadIndex) {
    auto &joints = _jointThreads[threadIndex]._joints;
    int rootIndex = _flowJointData[joints[0]]._parentIndex;
    _jointThreads[threadIndex]._rootFramePositions.clear();
    for (size_t j = 0; j < joints.size(); j++) {
        glm::vec3 jointPos;
        if (worldToJointPoint(_flowJointData[joints[j]]._node._currentPosition, rootIndex, jointPos)) {
            _jointThreads[threadIndex]._rootFramePositions.push_back(jointPos);
        } else {
            return false;
        }
    }
    return true;
}

void Flow::updateJoints() {
    for (auto &jointData : _flowJointData) {
        int jointIndex = jointData.first;
        glm::vec3 jointPosition, parentPosition, jointTranslation;
        glm::quat jointRotation, parentWorldRotation;
        _rig->getJointPositionInWorldFrame(jointIndex, jointPosition, _entityPosition, _entityRotation);
        _rig->getJointPositionInWorldFrame(jointData.second._parentIndex, parentPosition, _entityPosition, _entityRotation);
        _rig->getJointTranslation(jointIndex, jointTranslation);
        _rig->getJointRotation(jointIndex, jointRotation);
        _rig->getJointRotationInWorldFrame(jointData.second._parentIndex, parentWorldRotation, _entityRotation);
        jointData.second.setUpdatedData(jointPosition, jointTranslation, jointRotation, parentPosition, parentWorldRotation);
    }
    auto &selfCollisions = _collisionSystem.getSelfCollisions();
    for (auto &collision : selfCollisions) {
        glm::quat jointRotation;
        _rig->getJointPositionInWorldFrame(collision._jointIndex, collision._position, _entityPosition, _entityRotation);
        _rig->getJointRotationInWorldFrame(collision._jointIndex, jointRotation, _entityRotation);
        glm::vec3 worldOffset = jointRotation * collision._offset;
        collision._position = collision._position + worldOffset;
    }
    _collisionSystem.prepareCollisions();
}

void Flow::setJoints() {
    for (auto &thread : _jointThreads) {
        auto &joints = thread._joints;
        for (int jointIndex : joints) {
            auto &joint = _flowJointData[jointIndex];
            _rig->setJointRotation(jointIndex, true, joint._currentRotation, 2.0f);
        }
    }
}

void Flow::setOthersCollision(const QUuid& otherId, int jointIndex, const glm::vec3& position) {
    FlowCollisionSettings settings;
    settings._entityID = otherId;
    settings._radius = HAND_COLLISION_RADIUS;
    _collisionSystem.addCollisionSphere(jointIndex, settings, position, false, true);
}