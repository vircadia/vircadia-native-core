//
//  Avatar.h
//  interface/src/avatar
//
//  Copyright 2012 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Avatar_h
#define hifi_Avatar_h

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <QtCore/QScopedPointer>
#include <QtCore/QUuid>

#include <AvatarData.h>

#include "Hair.h"
#include "Hand.h"
#include "Head.h"
#include "InterfaceConfig.h"
#include "Recorder.h"
#include "SkeletonModel.h"
#include "world.h"

static const float SCALING_RATIO = .05f;
static const float SMOOTHING_RATIO = .05f; // 0 < ratio < 1
static const float RESCALING_TOLERANCE = .02f;

extern const float CHAT_MESSAGE_SCALE;
extern const float CHAT_MESSAGE_HEIGHT;

const int HAIR_STRANDS = 150;           //  Number of strands of hair
const int HAIR_LINKS = 10;              //  Number of links in a hair strand
const int HAIR_MAX_CONSTRAINTS = 2;     //  Hair verlet is connected to at most how many others

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

enum ScreenTintLayer {
    SCREEN_TINT_BEFORE_LANDSCAPE = 0,
    SCREEN_TINT_BEFORE_AVATARS,
    SCREEN_TINT_BEFORE_MY_AVATAR,
    SCREEN_TINT_AFTER_AVATARS,
    NUM_SCREEN_TINT_LAYERS
};

// Where one's own Avatar begins in the world (will be overwritten if avatar data file is found).
// This is the start location in the Sandbox (xyz: 6270, 211, 6000).
const glm::vec3 START_LOCATION(0.38269043f * TREE_SCALE, 0.01287842f * TREE_SCALE, 0.36621094f * TREE_SCALE);

class Texture;

class Avatar : public AvatarData {
    Q_OBJECT
    Q_PROPERTY(quint32 collisionGroups READ getCollisionGroups WRITE setCollisionGroups)

public:
    Avatar();
    ~Avatar();

    void init();
    void simulate(float deltaTime);
    
    enum RenderMode { NORMAL_RENDER_MODE, SHADOW_RENDER_MODE, MIRROR_RENDER_MODE };
    
    virtual void render(const glm::vec3& cameraPosition, RenderMode renderMode = NORMAL_RENDER_MODE,
        bool postLighting = false);

    //setters
    void setDisplayingLookatVectors(bool displayingLookatVectors) { getHead()->setRenderLookatVectors(displayingLookatVectors); }
    void setMouseRay(const glm::vec3 &origin, const glm::vec3 &direction);
    void setIsLookAtTarget(const bool isLookAtTarget) { _isLookAtTarget = isLookAtTarget; }
    //getters
    bool isInitialized() const { return _initialized; }
    SkeletonModel& getSkeletonModel() { return _skeletonModel; }
    const QVector<Model*>& getAttachmentModels() const { return _attachmentModels; }
    glm::vec3 getChestPosition() const;
    float getScale() const { return _scale; }
    const Head* getHead() const { return static_cast<const Head*>(_headData); }
    Head* getHead() { return static_cast<Head*>(_headData); }
    Hand* getHand() { return static_cast<Hand*>(_handData); }
    glm::quat getWorldAlignedOrientation() const;

    /// Returns the distance to use as a LOD parameter.
    float getLODDistance() const;

    bool findRayIntersection(RayIntersectionInfo& intersection) const;

    /// \param shapes list of shapes to collide against avatar
    /// \param collisions list to store collision results
    /// \return true if at least one shape collided with avatar
    bool findCollisions(const QVector<const Shape*>& shapes, CollisionList& collisions);

    /// Checks for penetration between the a sphere and the avatar's models.
    /// \param penetratorCenter the center of the penetration test sphere
    /// \param penetratorRadius the radius of the penetration test sphere
    /// \param collisions[out] a list to which collisions get appended
    /// \return whether or not the sphere penetrated
    bool findSphereCollisions(const glm::vec3& penetratorCenter, float penetratorRadius, CollisionList& collisions);

    /// Checks for penetration between the described plane and the avatar.
    /// \param plane the penetration plane
    /// \param collisions[out] a list to which collisions get appended
    /// \return whether or not the plane penetrated
    bool findPlaneCollisions(const glm::vec4& plane, CollisionList& collisions);

    virtual bool isMyAvatar() { return false; }
    
    virtual QVector<glm::quat> getJointRotations() const;
    virtual glm::quat getJointRotation(int index) const;
    virtual int getJointIndex(const QString& name) const;
    virtual QStringList getJointNames() const;
    
    virtual void setFaceModelURL(const QUrl& faceModelURL);
    virtual void setSkeletonModelURL(const QUrl& skeletonModelURL);
    virtual void setAttachmentData(const QVector<AttachmentData>& attachmentData);
    virtual void setDisplayName(const QString& displayName);
    virtual void setBillboard(const QByteArray& billboard);

    void setShowDisplayName(bool showDisplayName);
    
    virtual int parseDataAtOffset(const QByteArray& packet, int offset);

    static void renderJointConnectingCone(glm::vec3 position1, glm::vec3 position2, float radius1, float radius2);

    virtual void applyCollision(const glm::vec3& contactPoint, const glm::vec3& penetration) { }

    /// \return bounding radius of avatar
    virtual float getBoundingRadius() const;

    quint32 getCollisionGroups() const { return _collisionGroups; }
    virtual void setCollisionGroups(quint32 collisionGroups) { _collisionGroups = (collisionGroups & VALID_COLLISION_GROUPS); }
    
    Q_INVOKABLE glm::vec3 getJointPosition(int index) const;
    Q_INVOKABLE glm::vec3 getJointPosition(const QString& name) const;
    Q_INVOKABLE glm::quat getJointCombinedRotation(int index) const;
    Q_INVOKABLE glm::quat getJointCombinedRotation(const QString& name) const;
    
    Q_INVOKABLE void setJointModelPositionAndOrientation(int index, const glm::vec3 position, const glm::quat& rotation);
    Q_INVOKABLE void setJointModelPositionAndOrientation(const QString& name, const glm::vec3 position, 
        const glm::quat& rotation);
    
    Q_INVOKABLE glm::vec3 getNeckPosition() const;

    Q_INVOKABLE glm::vec3 getVelocity() const { return _velocity; }
    Q_INVOKABLE glm::vec3 getAcceleration() const { return _acceleration; }
    Q_INVOKABLE glm::vec3 getAngularVelocity() const { return _angularVelocity; }
    Q_INVOKABLE glm::vec3 getAngularAcceleration() const { return _angularAcceleration; }
    

    /// Scales a world space position vector relative to the avatar position and scale
    /// \param vector position to be scaled. Will store the result
    void scaleVectorRelativeToPosition(glm::vec3 &positionToScale) const;

    void slamPosition(const glm::vec3& position);

    // Call this when updating Avatar position with a delta.  This will allow us to 
    // _accurately_ measure position changes and compute the resulting velocity 
    // (otherwise floating point error will cause problems at large positions).
    void applyPositionDelta(const glm::vec3& delta);

public slots:
    void updateCollisionGroups();
    
signals:
    void collisionWithAvatar(const QUuid& myUUID, const QUuid& theirUUID, const CollisionInfo& collision);

protected:
    Hair _hair;
    SkeletonModel _skeletonModel;
    QVector<Model*> _attachmentModels;
    float _bodyYawDelta;

    glm::vec3 _velocity;

    // These position histories and derivatives are in the world-frame.
    // The derivatives are the MEASURED results of all external and internal forces
    // and are therefor READ-ONLY --> motion control of the Avatar is NOT obtained 
    // by setting these values.
    // Floating point error prevents us from accurately measuring velocity using a naive approach 
    // (e.g. vel = (pos - lastPos)/dt) so instead we use _positionDeltaAccumulator.
    glm::vec3 _positionDeltaAccumulator;
    glm::vec3 _lastVelocity;
    glm::vec3 _acceleration;
    glm::vec3 _angularVelocity;
    glm::vec3 _lastAngularVelocity;
    glm::vec3 _angularAcceleration;
    glm::quat _lastOrientation;

    float _leanScale;
    float _scale;
    glm::vec3 _worldUpDirection;
    glm::vec3 _mouseRayOrigin;
    glm::vec3 _mouseRayDirection;
    float _stringLength;
    bool _moving; ///< set when position is changing
    
    quint32 _collisionGroups;
 
    // protected methods...
    glm::vec3 getBodyRightDirection() const { return getOrientation() * IDENTITY_RIGHT; }
    glm::vec3 getBodyUpDirection() const { return getOrientation() * IDENTITY_UP; }
    glm::vec3 getBodyFrontDirection() const { return getOrientation() * IDENTITY_FRONT; }
    glm::quat computeRotationFromBodyToWorldUp(float proportion = 1.0f) const;
    void setScale(float scale);
    void measureMotionDerivatives(float deltaTime);

    float getSkeletonHeight() const;
    float getHeadHeight() const;
    float getPelvisFloatingHeight() const;
    float getPelvisToHeadLength() const;
    glm::vec3 getDisplayNamePosition();

    void renderDisplayName();
    virtual void renderBody(RenderMode renderMode, float glowLevel = 0.0f);
    virtual bool shouldRenderHead(const glm::vec3& cameraPosition, RenderMode renderMode) const;

    void simulateAttachments(float deltaTime);
    virtual void renderAttachments(RenderMode renderMode);

    virtual void updateJointMappings();
    
private:

    bool _initialized;
    QScopedPointer<Texture> _billboardTexture;
    bool _shouldRenderBillboard;
    bool _isLookAtTarget;

    void renderBillboard();
    
    float getBillboardSize() const;
};

#endif // hifi_Avatar_h
