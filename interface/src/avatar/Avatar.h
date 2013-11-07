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

#include "AvatarTouch.h"
#include "AvatarVoxelSystem.h"
#include "Balls.h"
#include "Hand.h"
#include "Head.h"
#include "InterfaceConfig.h"
#include "Skeleton.h"
#include "SkeletonModel.h"
#include "world.h"
#include "devices/SerialInterface.h"
#include "devices/Transmitter.h"

static const float MAX_SCALE = 1000.f;
static const float MIN_SCALE = .005f;
static const float SCALING_RATIO = .05f;
static const float SMOOTHING_RATIO = .05f; // 0 < ratio < 1
static const float RESCALING_TOLERANCE = .02f;

const float BODY_BALL_RADIUS_PELVIS = 0.07;
const float BODY_BALL_RADIUS_TORSO = 0.065;
const float BODY_BALL_RADIUS_CHEST = 0.08;
const float BODY_BALL_RADIUS_NECK_BASE = 0.03;
const float BODY_BALL_RADIUS_HEAD_BASE = 0.07;
const float BODY_BALL_RADIUS_LEFT_COLLAR = 0.04;
const float BODY_BALL_RADIUS_LEFT_SHOULDER = 0.03;
const float BODY_BALL_RADIUS_LEFT_ELBOW = 0.02;
const float BODY_BALL_RADIUS_LEFT_WRIST = 0.02;
const float BODY_BALL_RADIUS_LEFT_FINGERTIPS = 0.01;
const float BODY_BALL_RADIUS_RIGHT_COLLAR = 0.04;
const float BODY_BALL_RADIUS_RIGHT_SHOULDER = 0.03;
const float BODY_BALL_RADIUS_RIGHT_ELBOW = 0.02;
const float BODY_BALL_RADIUS_RIGHT_WRIST = 0.02;
const float BODY_BALL_RADIUS_RIGHT_FINGERTIPS = 0.01;
const float BODY_BALL_RADIUS_LEFT_HIP = 0.04;
const float BODY_BALL_RADIUS_LEFT_MID_THIGH = 0.03;
const float BODY_BALL_RADIUS_LEFT_KNEE = 0.025;
const float BODY_BALL_RADIUS_LEFT_HEEL = 0.025;
const float BODY_BALL_RADIUS_LEFT_TOES = 0.025;
const float BODY_BALL_RADIUS_RIGHT_HIP = 0.04;
const float BODY_BALL_RADIUS_RIGHT_KNEE = 0.025;
const float BODY_BALL_RADIUS_RIGHT_HEEL = 0.025;
const float BODY_BALL_RADIUS_RIGHT_TOES = 0.025;

extern const bool usingBigSphereCollisionTest;

extern const float chatMessageScale;
extern const float chatMessageHeight;

enum AvatarBodyBallID {
	BODY_BALL_NULL = -1,
	BODY_BALL_PELVIS,	
	BODY_BALL_TORSO,	
	BODY_BALL_CHEST,	
	BODY_BALL_NECK_BASE,	
	BODY_BALL_HEAD_BASE,	
	BODY_BALL_HEAD_TOP,	
	BODY_BALL_LEFT_COLLAR,
	BODY_BALL_LEFT_SHOULDER,
	BODY_BALL_LEFT_ELBOW,
	BODY_BALL_LEFT_WRIST,
	BODY_BALL_LEFT_FINGERTIPS,
	BODY_BALL_RIGHT_COLLAR,
	BODY_BALL_RIGHT_SHOULDER,
	BODY_BALL_RIGHT_ELBOW,
	BODY_BALL_RIGHT_WRIST,
	BODY_BALL_RIGHT_FINGERTIPS,
    BODY_BALL_LEFT_HIP,
    BODY_BALL_LEFT_KNEE,
    BODY_BALL_LEFT_HEEL,
    BODY_BALL_LEFT_TOES,
    BODY_BALL_RIGHT_HIP,
    BODY_BALL_RIGHT_KNEE,
    BODY_BALL_RIGHT_HEEL,
    BODY_BALL_RIGHT_TOES,
	NUM_AVATAR_BODY_BALLS
};

enum DriveKeys {
    FWD = 0,
    BACK,
    LEFT, 
    RIGHT, 
    UP,
    DOWN,
    ROT_LEFT, 
    ROT_RIGHT, 
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
    static void sendAvatarURLsMessage(const QUrl& voxelURL);
    
    Avatar(Node* owningNode = NULL);
    ~Avatar();
    
    void init();
    void simulate(float deltaTime, Transmitter* transmitter);
    void follow(Avatar* leadingAvatar);
    void render(bool lookingInMirror, bool renderAvatarBalls);

    //setters
    void setDisplayingLookatVectors(bool displayingLookatVectors) { _head.setRenderLookatVectors(displayingLookatVectors); }
    void setMouseRay(const glm::vec3 &origin, const glm::vec3 &direction);

    //getters
    bool isInitialized() const { return _initialized; }
    const Skeleton& getSkeleton() const { return _skeleton; }
    SkeletonModel& getSkeletonModel() { return _skeletonModel; }
    float getHeadYawRate() const { return _head.yawRate; }
    const glm::vec3& getHeadJointPosition() const { return _skeleton.joint[ AVATAR_JOINT_HEAD_BASE ].position; }
    const glm::vec3& getChestJointPosition() const { return _skeleton.joint[ AVATAR_JOINT_CHEST ].position; }
    float getScale() const { return _scale; }
    const glm::vec3& getVelocity() const { return _velocity; }
    Head& getHead() { return _head; }
    Hand& getHand() { return _hand; }
    glm::quat getOrientation() const;
    glm::quat getWorldAlignedOrientation() const;
    AvatarVoxelSystem* getVoxels() { return &_voxels; }

    void getSkinColors(glm::vec3& lighter, glm::vec3& darker);

    // Get the position/rotation of a single body ball
    void getBodyBallTransform(AvatarJointID jointID, glm::vec3& position, glm::quat& rotation) const;

    /// Checks for an intersection between the described ray and any of the avatar's body balls.
    /// \param origin the origin of the ray
    /// \param direction the unit direction vector
    /// \param[out] distance the variable in which to store the distance to intersection
    /// \return whether or not the ray intersected
    bool findRayIntersection(const glm::vec3& origin, const glm::vec3& direction, float& distance) const;

    virtual int parseData(unsigned char* sourceBuffer, int numBytes);

    static void renderJointConnectingCone(glm::vec3 position1, glm::vec3 position2, float radius1, float radius2);
    
public slots:
    void setWantCollisionsOn(bool wantCollisionsOn) { _isCollisionsOn = wantCollisionsOn; }
    void goHome();
    void increaseSize();
    void decreaseSize();
    void resetSize();

    friend class MyAvatar;


protected:
    struct AvatarBall {
        AvatarJointID parentJoint; /// the skeletal joint that serves as a reference for determining the position
        glm::vec3 parentOffset; /// a 3D vector in the frame of reference of the parent skeletal joint
        AvatarBodyBallID parentBall; /// the ball to which this ball is constrained for spring forces 
        glm::vec3 position; /// the actual dynamic position of the ball at any given time
        glm::quat rotation; /// the rotation of the ball           
        glm::vec3 velocity; /// the velocity of the ball
        float springLength; /// the ideal length of the spring between this ball and its parentBall 
        float jointTightness; /// how tightly the ball position attempts to stay at its ideal position (determined by parentOffset)
        float radius; /// the radius of the ball
        bool isCollidable; /// whether or not the ball responds to collisions 
        float touchForce; /// a scalar determining the amount that the cursor (or hand) is penetrating the ball
    };

    Head _head;
    Hand _hand;
    Skeleton _skeleton;
    SkeletonModel _skeletonModel;
    bool _ballSpringsInitialized;
    float _bodyYawDelta;
    AvatarBall _bodyBall[ NUM_AVATAR_BODY_BALLS ];
    AvatarMode _mode;
    glm::vec3 _velocity;
    glm::vec3 _thrust;
    float _speed;
    float _leanScale;
    float _pelvisFloatingHeight;
    float _pelvisToHeadLength;
    float _scale;
    float _height;
    Balls* _balls;
    AvatarTouch _avatarTouch;
    glm::vec3 _worldUpDirection;
    glm::vec3 _mouseRayOrigin;
    glm::vec3 _mouseRayDirection;
    bool _isCollisionsOn;
    Avatar* _leadingAvatar;
    float _stringLength;
    AvatarVoxelSystem _voxels;

    bool _moving; ///< set when position is changing

    // protected methods...
    glm::vec3 getBodyRightDirection() const { return getOrientation() * IDENTITY_RIGHT; }
    glm::vec3 getBodyUpDirection() const { return getOrientation() * IDENTITY_UP; }
    glm::vec3 getBodyFrontDirection() const { return getOrientation() * IDENTITY_FRONT; }
    glm::quat computeRotationFromBodyToWorldUp(float proportion = 1.0f) const;
    void updateBodyBalls(float deltaTime);
    bool updateLeapHandPositions();
    void updateArmIKAndConstraints(float deltaTime, AvatarJointID fingerTipJointID);
    void setScale(const float scale);


private:
    // privatize copy constructor and assignment operator to avoid copying
    Avatar(const Avatar&);
    Avatar& operator= (const Avatar&);
    
    bool _initialized;
    glm::vec3 _handHoldingPosition;
    float _maxArmLength;
    float _pelvisStandingHeight;
    
    
    // private methods...
    glm::vec3 calculateAverageEyePosition() { return _head.calculateAverageEyePosition(); } // get the position smack-dab between the eyes (for lookat)
    float getBallRenderAlpha(int ball, bool lookingInMirror) const;
    void renderBody(bool lookingInMirror, bool renderAvatarBalls);
    void initializeBodyBalls();
    void resetBodyBalls();
    void updateHandMovementAndTouching(float deltaTime, bool enableHandMovement);
};

#endif
