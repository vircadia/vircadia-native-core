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

#include <qelapsedtimer.h>
#include <qstring.h>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <vector>
#include <map>
#include <quuid.h>

class Rig;
class AnimSkeleton;

const bool SHOW_AVATAR = true;
const bool SHOW_DEBUG_SHAPES = false;
const bool SHOW_SOLID_SHAPES = false;
const bool SHOW_DUMMY_JOINTS = false;
const bool USE_COLLISIONS = true;

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

const float DEFAULT_STIFFNESS = 0.8f;
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
    float _stiffness{ 0.0f };
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
    void setScale(float scale);

    std::vector<FlowCollisionResult> checkFlowThreadCollisions(FlowThread* flowThread);

    int findSelfCollisionWithJoint(int jointIndex);
    void modifySelfCollisionRadius(int jointIndex, float radius);
    void modifySelfCollisionYOffset(int jointIndex, float offset);
    void modifySelfCollisionOffset(int jointIndex, const glm::vec3& offset);

    std::vector<FlowCollisionSphere>& getSelfCollisions() { return _selfCollisions; };
    void setOthersCollisions(const std::vector<FlowCollisionSphere>& othersCollisions) { _othersCollisions = othersCollisions; }
    void prepareCollisions();
    void resetCollisions();
    void resetOthersCollisions() { _othersCollisions.clear(); }
protected:
    std::vector<FlowCollisionSphere> _selfCollisions;
    std::vector<FlowCollisionSphere> _othersCollisions;
    std::vector<FlowCollisionSphere> _allCollisions;
    float _scale{ 1.0f };
};

class FlowNode {
public:
    FlowNode() {};
    FlowNode(const glm::vec3& initialPosition, FlowPhysicsSettings settings) :
        _initialPosition(initialPosition), _previousPosition(initialPosition), _currentPosition(initialPosition){};

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
    float _scale{ 1.0f };

    bool _anchored { false };
    bool _colliding { false };
    bool _active { true };

    void update(float deltaTime, const glm::vec3& accelerationOffset);
    void solve(const glm::vec3& constrainPoint, float maxDistance, const FlowCollisionResult& collision);
    void solveConstraints(const glm::vec3& constrainPoint, float maxDistance);
    void solveCollisions(const FlowCollisionResult& collision);
};

class FlowJoint {
public:
    FlowJoint() {};
    FlowJoint(int jointIndex, int parentIndex, int childIndex, const QString& name, const QString& group, float scale, const FlowPhysicsSettings& settings);
    void setInitialData(const glm::vec3& initialPosition, const glm::vec3& initialTranslation, const glm::quat& initialRotation, const glm::vec3& parentPosition);
    void setUpdatedData(const glm::vec3& updatedPosition, const glm::vec3& updatedTranslation, const glm::quat& updatedRotation, const glm::vec3& parentPosition, const glm::quat& parentWorldRotation);
    void setRecoveryPosition(const glm::vec3& recoveryPosition);
    void update(float deltaTime);
    void solve(const FlowCollisionResult& collision);

    int _index{ -1 };
    int _parentIndex{ -1 };
    int _childIndex{ -1 };
    QString _name;
    QString _group;
    bool _isDummy{ false };
    glm::vec3 _initialPosition;
    glm::vec3 _initialTranslation;
    glm::quat _initialRotation;

    glm::vec3 _updatedPosition;
    glm::vec3 _updatedTranslation;
    glm::quat _updatedRotation;

    glm::quat _currentRotation;
    glm::vec3 _recoveryPosition;

    glm::vec3 _parentPosition;
    glm::quat _parentWorldRotation;

    FlowNode _node;
    glm::vec3 _translationDirection;
    float _scale { 1.0f };
    float _length { 0.0f };
    float _originalLength { 0.0f };
    bool _applyRecovery { false };
};

class FlowDummyJoint : public FlowJoint {
public:
    FlowDummyJoint(const glm::vec3& initialPosition, int index, int parentIndex, int childIndex, float scale, FlowPhysicsSettings settings);
};


class FlowThread {
public:
    FlowThread() {};
    FlowThread(int rootIndex, std::map<int, FlowJoint>* joints);

    void resetLength();
    void computeFlowThread(int rootIndex);
    void computeRecovery();
    void update(float deltaTime);
    void solve(bool useCollisions, FlowCollisionSystem& collisionSystem);
    void computeJointRotations();
    void apply();
    bool getActive();
    void setRootFramePositions(const std::vector<glm::vec3>& rootFramePositions) { _rootFramePositions = rootFramePositions; }

    std::vector<int> _joints;
    std::vector<glm::vec3> _positions;
    float _radius{ 0.0f };
    float _length{ 0.0f };
    std::map<int, FlowJoint>* _jointsPointer;
    std::vector<glm::vec3> _rootFramePositions;
};

class Flow {
public:
    Flow(Rig* rig) { _rig = rig; };
    void calculateConstraints();
    void update(float deltaTime);
    void setTransform(float scale, const glm::vec3& position, const glm::quat& rotation);
    const std::map<int, FlowJoint>& getJoints() const { return _flowJointData; }
    const std::vector<FlowThread>& getThreads() const { return _jointThreads; }
    void setOthersCollision(const QUuid& otherId, int jointIndex, const glm::vec3& position);
    FlowCollisionSystem& getCollisionSystem() { return _collisionSystem; }
private:
    void setJoints();
    void cleanUp();
    void updateJoints();
    bool updateRootFramePositions(size_t threadIndex);
    bool worldToJointPoint(const glm::vec3& position, const int jointIndex, glm::vec3& jointSpacePosition) const;
    Rig* _rig;
    float _scale { 1.0f };
    glm::vec3 _entityPosition;
    glm::quat _entityRotation;
    std::map<int, FlowJoint> _flowJointData;
    std::vector<FlowThread> _jointThreads;
    std::vector<QString> _flowJointKeywords;
    FlowCollisionSystem _collisionSystem;
    bool _initialized { false };
    bool _active { false };
    int _deltaTime { 0 };
    int _deltaTimeLimit { 4000000 };
    int _updates { 0 };
    bool _invertThreadLoop { false };
};

#endif // hifi_Flow_h
