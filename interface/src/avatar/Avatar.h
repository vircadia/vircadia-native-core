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
#include <ShapeInfo.h>

#include <render/Scene.h>

#include "Head.h"
#include "SkeletonModel.h"
#include "world.h"
#include "Rig.h"
#include <ThreadSafeValueCache.h>

namespace render {
    template <> const ItemKey payloadGetKey(const AvatarSharedPointer& avatar);
    template <> const Item::Bound payloadGetBound(const AvatarSharedPointer& avatar);
    template <> void payloadRender(const AvatarSharedPointer& avatar, RenderArgs* args);
}

static const float SCALING_RATIO = .05f;
static const float SMOOTHING_RATIO = .05f; // 0 < ratio < 1

extern const float CHAT_MESSAGE_SCALE;
extern const float CHAT_MESSAGE_HEIGHT;


enum ScreenTintLayer {
    SCREEN_TINT_BEFORE_LANDSCAPE = 0,
    SCREEN_TINT_BEFORE_AVATARS,
    SCREEN_TINT_BEFORE_MY_AVATAR,
    SCREEN_TINT_AFTER_AVATARS,
    NUM_SCREEN_TINT_LAYERS
};

class AvatarMotionState;
class Texture;

class Avatar : public AvatarData {
    Q_OBJECT
    Q_PROPERTY(glm::vec3 skeletonOffset READ getSkeletonOffset WRITE setSkeletonOffset)

public:
    explicit Avatar(RigPointer rig = nullptr);
    ~Avatar();

    typedef render::Payload<AvatarData> Payload;
    typedef std::shared_ptr<render::Item::PayloadInterface> PayloadPointer;

    void init();
    void updateAvatarEntities();
    void simulate(float deltaTime);
    virtual void simulateAttachments(float deltaTime);

    virtual void render(RenderArgs* renderArgs, const glm::vec3& cameraPosition);

    bool addToScene(AvatarSharedPointer self, std::shared_ptr<render::Scene> scene,
                            render::PendingChanges& pendingChanges);

    void removeFromScene(AvatarSharedPointer self, std::shared_ptr<render::Scene> scene,
                                render::PendingChanges& pendingChanges);

    void updateRenderItem(render::PendingChanges& pendingChanges);

    virtual void postUpdate(float deltaTime);

    //setters
    void setIsLookAtTarget(const bool isLookAtTarget) { _isLookAtTarget = isLookAtTarget; }
    bool getIsLookAtTarget() const { return _isLookAtTarget; }
    //getters
    bool isInitialized() const { return _initialized; }
    SkeletonModelPointer getSkeletonModel() { return _skeletonModel; }
    const SkeletonModelPointer getSkeletonModel() const { return _skeletonModel; }
    glm::vec3 getChestPosition() const;
    float getUniformScale() const { return getScale().y; }
    const Head* getHead() const { return static_cast<const Head*>(_headData); }
    Head* getHead() { return static_cast<Head*>(_headData); }

    glm::quat getWorldAlignedOrientation() const;

    AABox getBounds() const;

    /// Returns the distance to use as a LOD parameter.
    float getLODDistance() const;

    virtual bool isMyAvatar() const override { return false; }

    virtual QVector<glm::quat> getJointRotations() const override;
    virtual glm::quat getJointRotation(int index) const override;
    virtual glm::vec3 getJointTranslation(int index) const override;
    virtual int getJointIndex(const QString& name) const override;
    virtual QStringList getJointNames() const override;

    Q_INVOKABLE virtual glm::quat getDefaultJointRotation(int index) const;
    Q_INVOKABLE virtual glm::vec3 getDefaultJointTranslation(int index) const;

    virtual glm::quat getAbsoluteJointRotationInObjectFrame(int index) const override;
    virtual glm::vec3 getAbsoluteJointTranslationInObjectFrame(int index) const override;
    virtual bool setAbsoluteJointRotationInObjectFrame(int index, const glm::quat& rotation) override { return false; }
    virtual bool setAbsoluteJointTranslationInObjectFrame(int index, const glm::vec3& translation) override { return false; }

    Q_INVOKABLE virtual void setSkeletonModelURL(const QUrl& skeletonModelURL) override;
    virtual void setAttachmentData(const QVector<AttachmentData>& attachmentData) override;

    void setShowDisplayName(bool showDisplayName);

    virtual int parseDataFromBuffer(const QByteArray& buffer) override;

    static void renderJointConnectingCone( gpu::Batch& batch, glm::vec3 position1, glm::vec3 position2,
                                                float radius1, float radius2, const glm::vec4& color);

    virtual void applyCollision(const glm::vec3& contactPoint, const glm::vec3& penetration) { }

    Q_INVOKABLE void setSkeletonOffset(const glm::vec3& offset);
    Q_INVOKABLE glm::vec3 getSkeletonOffset() { return _skeletonOffset; }
    virtual glm::vec3 getSkeletonPosition() const;

    Q_INVOKABLE glm::vec3 getJointPosition(int index) const;
    Q_INVOKABLE glm::vec3 getJointPosition(const QString& name) const;
    Q_INVOKABLE glm::vec3 getNeckPosition() const;

    Q_INVOKABLE glm::vec3 getAcceleration() const { return _acceleration; }

    Q_INVOKABLE bool getShouldRender() const { return !_shouldSkipRender; }

    /// Scales a world space position vector relative to the avatar position and scale
    /// \param vector position to be scaled. Will store the result
    void scaleVectorRelativeToPosition(glm::vec3 &positionToScale) const;

    void slamPosition(const glm::vec3& position);
    virtual void updateAttitude() override { _skeletonModel->updateAttitude(); }

    // Call this when updating Avatar position with a delta.  This will allow us to
    // _accurately_ measure position changes and compute the resulting velocity
    // (otherwise floating point error will cause problems at large positions).
    void applyPositionDelta(const glm::vec3& delta);

    virtual void rebuildCollisionShape();

    virtual void computeShapeInfo(ShapeInfo& shapeInfo);
    void getCapsule(glm::vec3& start, glm::vec3& end, float& radius);

    AvatarMotionState* getMotionState() { return _motionState; }

    using SpatiallyNestable::setPosition;
    virtual void setPosition(const glm::vec3& position) override;
    using SpatiallyNestable::setOrientation;
    virtual void setOrientation(const glm::quat& orientation) override;

    // these call through to the SpatiallyNestable versions, but they are here to expose these to javascript.
    Q_INVOKABLE virtual const QUuid getParentID() const override { return SpatiallyNestable::getParentID(); }
    Q_INVOKABLE virtual void setParentID(const QUuid& parentID) override;
    Q_INVOKABLE virtual quint16 getParentJointIndex() const override { return SpatiallyNestable::getParentJointIndex(); }
    Q_INVOKABLE virtual void setParentJointIndex(quint16 parentJointIndex) override;

    // NOT thread safe, must be called on main thread.
    glm::vec3 getUncachedLeftPalmPosition() const;
    glm::quat getUncachedLeftPalmRotation() const;
    glm::vec3 getUncachedRightPalmPosition() const;
    glm::quat getUncachedRightPalmRotation() const;

public slots:

    // FIXME - these should be migrated to use Pose data instead
    // thread safe, will return last valid palm from cache
    glm::vec3 getLeftPalmPosition() const;
    glm::quat getLeftPalmRotation() const;
    glm::vec3 getRightPalmPosition() const;
    glm::quat getRightPalmRotation() const;

    void setModelURLFinished(bool success);

protected:
    friend class AvatarManager;

    void setMotionState(AvatarMotionState* motionState);

    SkeletonModelPointer _skeletonModel;
    glm::vec3 _skeletonOffset;
    std::vector<std::shared_ptr<Model>> _attachmentModels;
    std::vector<std::shared_ptr<Model>> _attachmentsToRemove;
    std::vector<std::shared_ptr<Model>> _attachmentsToDelete;

    float _bodyYawDelta;  // degrees/sec

    // These position histories and derivatives are in the world-frame.
    // The derivatives are the MEASURED results of all external and internal forces
    // and are therefore READ-ONLY --> motion control of the Avatar is NOT obtained
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

    glm::vec3 _worldUpDirection;
    float _stringLength;
    bool _moving; ///< set when position is changing

    // protected methods...
    bool isLookingAtMe(AvatarSharedPointer avatar) const;

    virtual void animateScaleChanges(float deltaTime);

    glm::vec3 getBodyRightDirection() const { return getOrientation() * IDENTITY_RIGHT; }
    glm::vec3 getBodyUpDirection() const { return getOrientation() * IDENTITY_UP; }
    glm::vec3 getBodyFrontDirection() const { return getOrientation() * IDENTITY_FRONT; }
    glm::quat computeRotationFromBodyToWorldUp(float proportion = 1.0f) const;
    void measureMotionDerivatives(float deltaTime);

    float getSkeletonHeight() const;
    float getHeadHeight() const;
    float getPelvisFloatingHeight() const;
    glm::vec3 getDisplayNamePosition() const;

    Transform calculateDisplayNameTransform(const ViewFrustum& view, const glm::vec3& textPosition) const;
    void renderDisplayName(gpu::Batch& batch, const ViewFrustum& view, const glm::vec3& textPosition) const;
    virtual bool shouldRenderHead(const RenderArgs* renderArgs) const;
    virtual void fixupModelsInScene();

    virtual void updatePalms();

    render::ItemID _renderItemID{ render::Item::INVALID_ITEM_ID };

    ThreadSafeValueCache<glm::vec3> _leftPalmPositionCache { glm::vec3() };
    ThreadSafeValueCache<glm::quat> _leftPalmRotationCache { glm::quat() };
    ThreadSafeValueCache<glm::vec3> _rightPalmPositionCache { glm::vec3() };
    ThreadSafeValueCache<glm::quat> _rightPalmRotationCache { glm::quat() };

private:
    bool _initialized;
    bool _shouldAnimate { true };
    bool _shouldSkipRender { false };
    bool _isLookAtTarget { false };

    float getBoundingRadius() const;

    static int _jointConesID;

    int _voiceSphereID;

    AvatarMotionState* _motionState = nullptr;
};

#endif // hifi_Avatar_h
