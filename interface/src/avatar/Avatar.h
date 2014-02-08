//
//  Avatar.h
//  interface
//
//  Copyright (c) 2012 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__avatar__
#define __interface__avatar__

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <QtCore/QUuid>

#include <AvatarData.h>

#include "Hand.h"
#include "Head.h"
#include "InterfaceConfig.h"
#include "SkeletonModel.h"
#include "world.h"

static const float SCALING_RATIO = .05f;
static const float SMOOTHING_RATIO = .05f; // 0 < ratio < 1
static const float RESCALING_TOLERANCE = .02f;

extern const float CHAT_MESSAGE_SCALE;
extern const float CHAT_MESSAGE_HEIGHT;

enum DriveKeys {
    FWD = 0,
    BACK,
    LEFT,
    RIGHT,
    UP,
    DOWN,
    ROT_LEFT,
    ROT_RIGHT,
    ROT_UP,
    ROT_DOWN,
    MAX_DRIVE_KEYS
};

enum AvatarMode {
    AVATAR_MODE_STANDING = 0,
    AVATAR_MODE_WALKING,
    AVATAR_MODE_INTERACTING,
    NUM_AVATAR_MODES
};

enum ScreenTintLayer {
    SCREEN_TINT_BEFORE_LANDSCAPE = 0,
    SCREEN_TINT_BEFORE_AVATARS,
    SCREEN_TINT_BEFORE_MY_AVATAR,
    SCREEN_TINT_AFTER_AVATARS,
    NUM_SCREEN_TINT_LAYERS
};

class MyAvatar;

// Where one's own Avatar begins in the world (will be overwritten if avatar data file is found)
// this is basically in the center of the ground plane. Slightly adjusted. This was asked for by
// Grayson as he's building a street around here for demo dinner 2
const glm::vec3 START_LOCATION(0.485f * TREE_SCALE, 0.f, 0.5f * TREE_SCALE);

class Avatar : public AvatarData {
    Q_OBJECT

public:
    Avatar();
    ~Avatar();

    void init();
    void simulate(float deltaTime);
    void render(bool forceRenderHead);

    //setters
    void setDisplayingLookatVectors(bool displayingLookatVectors) { _head.setRenderLookatVectors(displayingLookatVectors); }
    void setMouseRay(const glm::vec3 &origin, const glm::vec3 &direction);

    //getters
    bool isInitialized() const { return _initialized; }
    SkeletonModel& getSkeletonModel() { return _skeletonModel; }
    glm::vec3 getChestPosition() const;
    float getScale() const { return _scale; }
    const glm::vec3& getVelocity() const { return _velocity; }
    Head& getHead() { return _head; }
    Hand& getHand() { return _hand; }
    glm::quat getWorldAlignedOrientation() const;
    
    Node* getOwningAvatarMixer() { return _owningAvatarMixer.data(); }
    void setOwningAvatarMixer(const QWeakPointer<Node>& owningAvatarMixer) { _owningAvatarMixer = owningAvatarMixer; }

    bool findRayIntersection(const glm::vec3& origin, const glm::vec3& direction, float& distance) const;

    /// Checks for penetration between the described sphere and the avatar.
    /// \param penetratorCenter the center of the penetration test sphere
    /// \param penetratorRadius the radius of the penetration test sphere
    /// \param penetration[out] the vector in which to store the penetration
    /// \param skeletonSkipIndex if not -1, the index of a joint to skip (along with its descendents) in the skeleton model
    /// \return whether or not the sphere penetrated
    bool findSpherePenetration(const glm::vec3& penetratorCenter, float penetratorRadius,
        glm::vec3& penetration, int skeletonSkipIndex = -1) const;

    /// Checks for collision between the a sphere and the avatar.
    /// \param collisionCenter the center of the penetration test sphere
    /// \param collisionRadius the radius of the penetration test sphere
    /// \param collision[out] the details of the collision point
    /// \return whether or not the sphere collided
    virtual bool findSphereCollision(const glm::vec3& sphereCenter, float sphereRadius, CollisionInfo& collision);
    
    virtual bool isMyAvatar() { return false; }
    
    virtual void setFaceModelURL(const QUrl& faceModelURL);
    virtual void setSkeletonModelURL(const QUrl& skeletonModelURL);
    
    int parseData(const QByteArray& packet);

    static void renderJointConnectingCone(glm::vec3 position1, glm::vec3 position2, float radius1, float radius2);

protected:
    Head _head;
    Hand _hand;
    SkeletonModel _skeletonModel;
    float _bodyYawDelta;
    AvatarMode _mode;
    glm::vec3 _velocity;
    glm::vec3 _thrust;
    float _speed;
    float _leanScale;
    float _scale;
    glm::vec3 _worldUpDirection;
    glm::vec3 _mouseRayOrigin;
    glm::vec3 _mouseRayDirection;
    float _stringLength;
    bool _moving; ///< set when position is changing
    QWeakPointer<Node> _owningAvatarMixer;

    // protected methods...
    glm::vec3 getBodyRightDirection() const { return getOrientation() * IDENTITY_RIGHT; }
    glm::vec3 getBodyUpDirection() const { return getOrientation() * IDENTITY_UP; }
    glm::vec3 getBodyFrontDirection() const { return getOrientation() * IDENTITY_FRONT; }
    glm::quat computeRotationFromBodyToWorldUp(float proportion = 1.0f) const;
    void setScale(float scale);

    float getHeight() const;
    float getPelvisFloatingHeight() const;
    float getPelvisToHeadLength() const;

private:

    bool _initialized;

    void renderBody(bool forceRenderHead);
};

#endif
