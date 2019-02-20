//
//  Flow.h
//
//  Created by Luis Cuenca on 1/21/2019.
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Flow_h
#define hifi_Flow_h

#include <memory>
#include <qstring.h>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <vector>
#include <map>
#include <quuid.h>
#include "AnimPose.h"

class Rig;
class AnimSkeleton;

const bool SHOW_DUMMY_JOINTS = false;

const int LEFT_HAND = 0;
const int RIGHT_HAND = 1;

const float HAPTIC_TOUCH_STRENGTH = 0.25f;
const float HAPTIC_TOUCH_DURATION = 10.0f;
const float HAPTIC_SLOPE = 0.18f;
const float HAPTIC_THRESHOLD = 40.0f;

const QString FLOW_JOINT_PREFIX = "flow";
const QString SIM_JOINT_PREFIX = "sim";

const std::vector<QString> HAND_COLLISION_JOINTS = { "RightHandMiddle1", "RightHandThumb3", "LeftHandMiddle1", "LeftHandThumb3", "RightHandMiddle3", "LeftHandMiddle3" };

const QString JOINT_COLLISION_PREFIX = "joint_";
const QString HAND_COLLISION_PREFIX = "hand_";
const float HAND_COLLISION_RADIUS = 0.03f;
const float HAND_TOUCHING_DISTANCE = 2.0f;

const int COLLISION_SHAPES_LIMIT = 4;

const QString DUMMY_KEYWORD = "Extra";
const int DUMMY_JOINT_COUNT = 8;
const float DUMMY_JOINT_DISTANCE = 0.05f;

const float ISOLATED_JOINT_STIFFNESS = 0.0f;
const float ISOLATED_JOINT_LENGTH = 0.05f;

const float DEFAULT_STIFFNESS = 0.0f;
const float DEFAULT_GRAVITY = -0.0096f;
const float DEFAULT_DAMPING = 0.85f;
const float DEFAULT_INERTIA = 0.8f;
const float DEFAULT_DELTA = 0.55f;
const float DEFAULT_RADIUS = 0.01f;

const uint64_t MAX_UPDATE_FLOW_TIME_BUDGET = 2000;

struct FlowPhysicsSettings {
    FlowPhysicsSettings() {};
    FlowPhysicsSettings(bool active, float stiffness, float gravity, float damping, float inertia, float delta, float radius) {
        _active = active;
        _stiffness = stiffness;
        _gravity = gravity;
        _damping = damping;
        _inertia = inertia;
        _delta = delta;
        _radius = radius;
    }
    bool _active{ true };
    float _stiffness{ DEFAULT_STIFFNESS };
    float _gravity{ DEFAULT_GRAVITY };
    float _damping{ DEFAULT_DAMPING };
    float _inertia{ DEFAULT_INERTIA };
    float _delta{ DEFAULT_DELTA };
    float _radius{ DEFAULT_RADIUS };
};

enum FlowCollisionType {
    CollisionSphere = 0
};

struct FlowCollisionSettings {
    FlowCollisionSettings() {};
    FlowCollisionSettings(const QUuid& id, const FlowCollisionType& type, const glm::vec3& offset, float radius) {
        _entityID = id;
        _type = type;
        _offset = offset;
        _radius = radius;
    };
    QUuid _entityID;
    FlowCollisionType _type { FlowCollisionType::CollisionSphere };
    float _radius { 0.05f };
    glm::vec3 _offset;
};

const FlowPhysicsSettings DEFAULT_JOINT_SETTINGS;

struct FlowJointInfo {
    FlowJointInfo() {};
    FlowJointInfo(int index, int parentIndex, int childIndex, const QString& name) {
        _index = index;
        _parentIndex = parentIndex;
        _childIndex = childIndex;
        _name = name;
    }
    int _index { -1 };
    QString _name; 
    int _parentIndex { -1 };
    int _childIndex { -1 };
};

struct FlowCollisionResult {
    int _count { 0 };
    float _offset { 0.0f };
    glm::vec3 _position;
    float _radius { 0.0f };
    glm::vec3 _normal;
    float _distance { 0.0f };
};

class FlowCollisionSphere {
public:
    FlowCollisionSphere() {};
    FlowCollisionSphere(const int& jointIndex, const FlowCollisionSettings& settings, bool isTouch = false);
    void setPosition(const glm::vec3& position) { _position = position; }
    FlowCollisionResult computeSphereCollision(const glm::vec3& point, float radius) const;
    FlowCollisionResult checkSegmentCollision(const glm::vec3& point1, const glm::vec3& point2, const FlowCollisionResult& collisionResult1, const FlowCollisionResult& collisionResult2);

    QUuid _entityID;

    glm::vec3 _offset;
    glm::vec3 _initialOffset;
    glm::vec3 _position;

    bool _isTouch { false };
    int _jointIndex { -1 };
    int collisionIndex { -1 };
    float _radius { 0.0f };
    float _initialRadius{ 0.0f };
};

class FlowThread;

class FlowCollisionSystem {
public:
    FlowCollisionSystem() {};
    void addCollisionSphere(int jointIndex, const FlowCollisionSettings& settings, const glm::vec3& position = { 0.0f, 0.0f, 0.0f }, bool isSelfCollision = true, bool isTouch = false);
    FlowCollisionResult computeCollision(const std::vector<FlowCollisionResult> collisions);

    std::vector<FlowCollisionResult> checkFlowThreadCollisions(FlowThread* flowThread);

    std::vector<FlowCollisionSphere>& getSelfCollisions() { return _selfCollisions; };
    void setOthersCollisions(const std::vector<FlowCollisionSphere>& othersCollisions) { _othersCollisions = othersCollisions; }
    void prepareCollisions();
    void resetCollisions();
    void resetOthersCollisions() { _othersCollisions.clear(); }
    void setScale(float scale);
    FlowCollisionSettings getCollisionSettingsByJoint(int jointIndex);
    void setCollisionSettingsByJoint(int jointIndex, const FlowCollisionSettings& settings);
    void setActive(bool active) { _active = active; }
    bool getActive() const { return _active; }
protected:
    std::vector<FlowCollisionSphere> _selfCollisions;
    std::vector<FlowCollisionSphere> _othersCollisions;
    std::vector<FlowCollisionSphere> _allCollisions;
    float _scale { 1.0f };
    bool _active { false };
};

class FlowNode {
public:
    FlowNode() {};
    FlowNode(const glm::vec3& initialPosition, FlowPhysicsSettings settings);
    
    void update(float deltaTime, const glm::vec3& accelerationOffset);
    void solve(const glm::vec3& constrainPoint, float maxDistance, const FlowCollisionResult& collision);
    void solveConstraints(const glm::vec3& constrainPoint, float maxDistance);
    void solveCollisions(const FlowCollisionResult& collision);    

protected:

    FlowPhysicsSettings _settings;
    glm::vec3 _initialPosition;
    glm::vec3 _previousPosition;
    glm::vec3 _currentPosition;

    glm::vec3 _currentVelocity;
    glm::vec3 _previousVelocity;
    glm::vec3 _acceleration;

    FlowCollisionResult _collision;
    FlowCollisionResult _previousCollision;

    float _initialRadius { 0.0f };

    bool _anchored { false };
    bool _colliding { false };
    bool _active { true };

    float _scale{ 1.0f };
};

class FlowJoint : public FlowNode {
public:
    friend class FlowThread;

    FlowJoint(): FlowNode() {};
    FlowJoint(int jointIndex, int parentIndex, int childIndex, const QString& name, const QString& group, const FlowPhysicsSettings& settings);
    void setInitialData(const glm::vec3& initialPosition, const glm::vec3& initialTranslation, const glm::quat& initialRotation, const glm::vec3& parentPosition);
    void setUpdatedData(const glm::vec3& updatedPosition, const glm::vec3& updatedTranslation, const glm::quat& updatedRotation, const glm::vec3& parentPosition, const glm::quat& parentWorldRotation);
    void setRecoveryPosition(const glm::vec3& recoveryPosition);
    void update(float deltaTime);
    void solve(const FlowCollisionResult& collision);

    void setScale(float scale, bool initScale);
    bool isAnchored() { return _anchored; }
    void setAnchored(bool anchored) { _anchored = anchored; }

    const FlowPhysicsSettings& getSettings() { return _settings; }
    void setSettings(const FlowPhysicsSettings& settings) { _settings = settings; }

    const glm::vec3& getCurrentPosition() { return _currentPosition; }
    int getIndex() { return _index; }
    int getParentIndex() { return _parentIndex; }
    void setChildIndex(int index) { _childIndex = index; }
    const glm::vec3& getUpdatedPosition() { return _updatedPosition; }
    const QString& getGroup() { return _group; }
    const glm::quat& getCurrentRotation() { return _currentRotation; }

protected:

    int _index{ -1 };
    int _parentIndex{ -1 };
    int _childIndex{ -1 };
    QString _name;
    QString _group;
    bool _isDummy{ false };

    glm::vec3 _initialTranslation;
    glm::quat _initialRotation;

    glm::vec3 _updatedPosition;
    glm::vec3 _updatedTranslation;
    glm::quat _updatedRotation;

    glm::quat _currentRotation;
    glm::vec3 _recoveryPosition;

    glm::vec3 _parentPosition;
    glm::quat _parentWorldRotation;
    glm::vec3 _translationDirection;

    float _length { 0.0f };
    float _initialLength { 0.0f };

    bool _applyRecovery { false };
};

class FlowDummyJoint : public FlowJoint {
public:
    FlowDummyJoint(const glm::vec3& initialPosition, int index, int parentIndex, int childIndex, FlowPhysicsSettings settings);
    void toIsolatedJoint(float length, int childIndex, const QString& group);
};


class FlowThread {
public:
    FlowThread() {};
    FlowThread& operator=(const FlowThread& otherFlowThread);

    FlowThread(int rootIndex, std::map<int, FlowJoint>* joints);

    void resetLength();
    void computeFlowThread(int rootIndex);
    void computeRecovery();
    void update(float deltaTime);
    void solve(FlowCollisionSystem& collisionSystem);
    void computeJointRotations();
    void setRootFramePositions(const std::vector<glm::vec3>& rootFramePositions) { _rootFramePositions = rootFramePositions; }
    void setScale(float scale, bool initScale = false);

    std::vector<int> _joints;
    std::vector<glm::vec3> _positions;
    float _radius{ 0.0f };
    float _length{ 0.0f };
    std::map<int, FlowJoint>* _jointsPointer;
    std::vector<glm::vec3> _rootFramePositions;
};

class Flow : public QObject{
    Q_OBJECT
public:
    Flow() { }
    Flow& operator=(const Flow& otherFlow);
    bool getActive() const { return _active; }
    void setActive(bool active) { _active = active; }
    bool isInitialized() const { return _initialized; }
    float getScale() const { return _scale; }
    void calculateConstraints(const std::shared_ptr<AnimSkeleton>& skeleton, AnimPoseVec& relativePoses, AnimPoseVec& absolutePoses);
    void update(float deltaTime, AnimPoseVec& relativePoses, AnimPoseVec& absolutePoses, const std::vector<bool>& overrideFlags);
    void setTransform(float scale, const glm::vec3& position, const glm::quat& rotation);
    const std::map<int, FlowJoint>& getJoints() const { return _flowJointData; }
    const std::vector<FlowThread>& getThreads() const { return _jointThreads; }
    void setOthersCollision(const QUuid& otherId, int jointIndex, const glm::vec3& position);
    FlowCollisionSystem& getCollisionSystem() { return _collisionSystem; }
    void setPhysicsSettingsForGroup(const QString& group, const FlowPhysicsSettings& settings);
    void cleanUp();

signals:
    void onCleanup();

private:    
    void updateAbsolutePoses(const AnimPoseVec& relativePoses, AnimPoseVec& absolutePoses);
    bool getJointPositionInWorldFrame(const AnimPoseVec& absolutePoses, int jointIndex, glm::vec3& position, glm::vec3 translation, glm::quat rotation) const;
    bool getJointRotationInWorldFrame(const AnimPoseVec& absolutePoses, int jointIndex, glm::quat& result, const glm::quat& rotation) const;
    bool getJointRotation(const AnimPoseVec& relativePoses, int jointIndex, glm::quat& rotation) const;
    bool getJointTranslation(const AnimPoseVec& relativePoses, int jointIndex, glm::vec3& translation) const;
    bool worldToJointPoint(const AnimPoseVec& absolutePoses, const glm::vec3& position, const int jointIndex, glm::vec3& jointSpacePosition) const;

    void setJoints(AnimPoseVec& relativePoses, const std::vector<bool>& overrideFlags);
    void updateJoints(AnimPoseVec& relativePoses, AnimPoseVec& absolutePoses);
    bool updateRootFramePositions(const AnimPoseVec& absolutePoses, size_t threadIndex);
    void setScale(float scale);
    
    float _scale { 1.0f };
    float _lastScale{ 1.0f };
    glm::vec3 _entityPosition;
    glm::quat _entityRotation;
    std::map<int, FlowJoint> _flowJointData;
    std::vector<FlowThread> _jointThreads;
    std::vector<QString> _flowJointKeywords;
    FlowCollisionSystem _collisionSystem;
    bool _initialized { false };
    bool _active { false };
    bool _isScaleSet { false };
    bool _invertThreadLoop { false };
};

#endif // hifi_Flow_h
