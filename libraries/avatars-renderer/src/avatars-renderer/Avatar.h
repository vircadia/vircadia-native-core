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

#include <functional>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <QtCore/QUuid>

#include <AvatarData.h>
#include <ShapeInfo.h>
#include <render/Scene.h>
#include <GLMHelpers.h>


#include "Head.h"
#include "SkeletonModel.h"
#include "Rig.h"
#include <ThreadSafeValueCache.h>

namespace render {
    template <> const ItemKey payloadGetKey(const AvatarSharedPointer& avatar);
    template <> const Item::Bound payloadGetBound(const AvatarSharedPointer& avatar);
    template <> void payloadRender(const AvatarSharedPointer& avatar, RenderArgs* args);
    template <> uint32_t metaFetchMetaSubItems(const AvatarSharedPointer& avatar, ItemIDs& subItems);
}

static const float SCALING_RATIO = .05f;

extern const float CHAT_MESSAGE_SCALE;
extern const float CHAT_MESSAGE_HEIGHT;


enum ScreenTintLayer {
    SCREEN_TINT_BEFORE_LANDSCAPE = 0,
    SCREEN_TINT_BEFORE_AVATARS,
    SCREEN_TINT_BEFORE_MY_AVATAR,
    SCREEN_TINT_AFTER_AVATARS,
    NUM_SCREEN_TINT_LAYERS
};

class Texture;

using AvatarPhysicsCallback = std::function<void(uint32_t)>;

class Avatar : public AvatarData {
    Q_OBJECT

    /**jsdoc
     * An avatar is representation of yourself or another user. The Avatar API can be used to query or manipulate the avatar of a user.
     * NOTE: Avatar extends AvatarData, see those namespace for more properties/methods.
     *
     * @namespace Avatar
     * @augments AvatarData
     *
     * @property skeletonOffset {Vec3} can be used to apply a translation offset between the avatar's position and the registration point of the 3d model.
     */
    Q_PROPERTY(glm::vec3 skeletonOffset READ getSkeletonOffset WRITE setSkeletonOffset)

public:
    static void setShowAvatars(bool render);
    static void setShowReceiveStats(bool receiveStats);
    static void setShowMyLookAtVectors(bool showMine);
    static void setShowOtherLookAtVectors(bool showOthers);
    static void setShowCollisionShapes(bool render);
    static void setShowNamesAboveHeads(bool show);

    explicit Avatar(QThread* thread);
    ~Avatar();

    virtual void instantiableAvatar() = 0;

    typedef render::Payload<AvatarData> Payload;
    typedef std::shared_ptr<render::Item::PayloadInterface> PayloadPointer;

    void init();
    void updateAvatarEntities();
    void simulate(float deltaTime, bool inView);
    virtual void simulateAttachments(float deltaTime);

    virtual void render(RenderArgs* renderArgs);

    void addToScene(AvatarSharedPointer self, const render::ScenePointer& scene,
                            render::Transaction& transaction);

    void removeFromScene(AvatarSharedPointer self, const render::ScenePointer& scene,
                                render::Transaction& transaction);

    void updateRenderItem(render::Transaction& transaction);

    virtual void postUpdate(float deltaTime, const render::ScenePointer& scene);

    //setters
    void setIsLookAtTarget(const bool isLookAtTarget) { _isLookAtTarget = isLookAtTarget; }
    bool getIsLookAtTarget() const { return _isLookAtTarget; }
    //getters
    bool isInitialized() const { return _initialized; }
    SkeletonModelPointer getSkeletonModel() { return _skeletonModel; }
    const SkeletonModelPointer getSkeletonModel() const { return _skeletonModel; }
    glm::vec3 getChestPosition() const;
    const Head* getHead() const { return static_cast<const Head*>(_headData); }
    Head* getHead() { return static_cast<Head*>(_headData); }

    AABox getBounds() const;

    /// Returns the distance to use as a LOD parameter.
    float getLODDistance() const;

    virtual bool isMyAvatar() const override { return false; }

    virtual QVector<glm::quat> getJointRotations() const override;
    using AvatarData::getJointRotation;
    virtual glm::quat getJointRotation(int index) const override;
    virtual QVector<glm::vec3> getJointTranslations() const override;
    using AvatarData::getJointTranslation;
    virtual glm::vec3 getJointTranslation(int index) const override;
    virtual int getJointIndex(const QString& name) const override;
    virtual QStringList getJointNames() const override;

    Q_INVOKABLE virtual glm::quat getDefaultJointRotation(int index) const;
    Q_INVOKABLE virtual glm::vec3 getDefaultJointTranslation(int index) const;

    /**jsdoc
     * Provides read only access to the default joint rotations in avatar coordinates.
     * The default pose of the avatar is defined by the position and orientation of all bones
     * in the avatar's model file.  Typically this is a t-pose.
     * @function Avatar.getAbsoluteDefaultJointRotationInObjectFrame
     * @param index {number} index number
     * @returns {Quat} The rotation of this joint in avatar coordinates.
     */
    Q_INVOKABLE virtual glm::quat getAbsoluteDefaultJointRotationInObjectFrame(int index) const;

    /**jsdoc
     * Provides read only access to the default joint translations in avatar coordinates.
     * The default pose of the avatar is defined by the position and orientation of all bones
     * in the avatar's model file.  Typically this is a t-pose.
     * @function Avatar.getAbsoluteDefaultJointTranslationInObjectFrame
     * @param index {number} index number
     * @returns {Vec3} The position of this joint in avatar coordinates.
     */
    Q_INVOKABLE virtual glm::vec3 getAbsoluteDefaultJointTranslationInObjectFrame(int index) const;

    virtual glm::vec3 getAbsoluteJointScaleInObjectFrame(int index) const override;
    virtual glm::quat getAbsoluteJointRotationInObjectFrame(int index) const override;
    virtual glm::vec3 getAbsoluteJointTranslationInObjectFrame(int index) const override;
    virtual bool setAbsoluteJointRotationInObjectFrame(int index, const glm::quat& rotation) override { return false; }
    virtual bool setAbsoluteJointTranslationInObjectFrame(int index, const glm::vec3& translation) override { return false; }

    virtual void setSkeletonModelURL(const QUrl& skeletonModelURL) override;
    virtual void setAttachmentData(const QVector<AttachmentData>& attachmentData) override;

    void updateDisplayNameAlpha(bool showDisplayName);
    virtual void setSessionDisplayName(const QString& sessionDisplayName) override { }; // no-op

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

    /// Scales a world space position vector relative to the avatar position and scale
    /// \param vector position to be scaled. Will store the result
    void scaleVectorRelativeToPosition(glm::vec3 &positionToScale) const;

    void slamPosition(const glm::vec3& position);
    virtual void updateAttitude(const glm::quat& orientation) override;

    // Call this when updating Avatar position with a delta.  This will allow us to
    // _accurately_ measure position changes and compute the resulting velocity
    // (otherwise floating point error will cause problems at large positions).
    void applyPositionDelta(const glm::vec3& delta);

    virtual void rebuildCollisionShape();

    virtual void computeShapeInfo(ShapeInfo& shapeInfo);
    void getCapsule(glm::vec3& start, glm::vec3& end, float& radius);
    float computeMass();

    void setPositionViaScript(const glm::vec3& position) override;
    void setOrientationViaScript(const glm::quat& orientation) override;

    // these call through to the SpatiallyNestable versions, but they are here to expose these to javascript.
    Q_INVOKABLE virtual const QUuid getParentID() const override { return SpatiallyNestable::getParentID(); }
    Q_INVOKABLE virtual void setParentID(const QUuid& parentID) override;
    Q_INVOKABLE virtual quint16 getParentJointIndex() const override { return SpatiallyNestable::getParentJointIndex(); }
    Q_INVOKABLE virtual void setParentJointIndex(quint16 parentJointIndex) override;

    /**jsdoc
     * Information about a single joint in an Avatar's skeleton hierarchy.
     * @typedef Avatar.SkeletonJoint
     * @property {string} name - name of joint
     * @property {number} index - joint index
     * @property {number} parentIndex - index of this joint's parent (-1 if no parent)
     */

    /**jsdoc
     * Returns an array of joints, where each joint is an object containing name, index and parentIndex fields.
     * @function Avatar.getSkeleton
     * @returns {Avatar.SkeletonJoint[]} returns a list of information about each joint in this avatar's skeleton.
     */
    Q_INVOKABLE QList<QVariant> getSkeleton();

    // NOT thread safe, must be called on main thread.
    glm::vec3 getUncachedLeftPalmPosition() const;
    glm::quat getUncachedLeftPalmRotation() const;
    glm::vec3 getUncachedRightPalmPosition() const;
    glm::quat getUncachedRightPalmRotation() const;

    uint64_t getLastRenderUpdateTime() const { return _lastRenderUpdateTime; }
    void setLastRenderUpdateTime(uint64_t time) { _lastRenderUpdateTime = time; }

    void animateScaleChanges(float deltaTime);
    void setTargetScale(float targetScale) override;
    float getTargetScale() const { return _targetScale; }

    Q_INVOKABLE float getSimulationRate(const QString& rateName = QString("")) const;

    bool hasNewJointData() const { return _hasNewJointData; }

    float getBoundingRadius() const;

    void addToScene(AvatarSharedPointer self, const render::ScenePointer& scene);
    void ensureInScene(AvatarSharedPointer self, const render::ScenePointer& scene);
    bool isInScene() const { return render::Item::isValidID(_renderItemID); }
    render::ItemID getRenderItemID() { return _renderItemID; }
    bool isMoving() const { return _moving; }

    void setPhysicsCallback(AvatarPhysicsCallback cb);
    void addPhysicsFlags(uint32_t flags);
    bool isInPhysicsSimulation() const { return _physicsCallback != nullptr; }

    void fadeIn(render::ScenePointer scene);
    void fadeOut(render::ScenePointer scene, KillAvatarReason reason);
    bool isFading() const { return _isFading; }
    void updateFadingStatus(render::ScenePointer scene);

    /**jsdoc
     * Provides read only access to the current eye height of the avatar.
     * @function Avatar.getEyeHeight
     * @returns {number} eye height of avatar in meters
     */
    Q_INVOKABLE float getEyeHeight() const;

    virtual float getModelScale() const { return _modelScale; }
    virtual void setModelScale(float scale) { _modelScale = scale; }

public slots:

    // FIXME - these should be migrated to use Pose data instead
    // thread safe, will return last valid palm from cache
    glm::vec3 getLeftPalmPosition() const;
    glm::quat getLeftPalmRotation() const;
    glm::vec3 getRightPalmPosition() const;
    glm::quat getRightPalmRotation() const;

    void setModelURLFinished(bool success);

protected:
    virtual const QString& getSessionDisplayNameForTransport() const override { return _empty; } // Save a tiny bit of bandwidth. Mixer won't look at what we send.
    QString _empty{};
    virtual void maybeUpdateSessionDisplayNameFromTransport(const QString& sessionDisplayName) override { _sessionDisplayName = sessionDisplayName; } // don't use no-op setter!

    SkeletonModelPointer _skeletonModel;

    void invalidateJointIndicesCache() const;
    void withValidJointIndicesCache(std::function<void()> const& worker) const;
    mutable QHash<QString, int> _modelJointIndicesCache;
    mutable QReadWriteLock _modelJointIndicesCacheLock;
    mutable bool _modelJointsCached { false };

    glm::vec3 _skeletonOffset;
    std::vector<std::shared_ptr<Model>> _attachmentModels;
    std::vector<std::shared_ptr<Model>> _attachmentsToRemove;
    std::vector<std::shared_ptr<Model>> _attachmentsToDelete;

    float _bodyYawDelta { 0.0f };  // degrees/sec

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

    glm::vec3 _worldUpDirection { Vectors::UP };
    bool _moving { false }; ///< set when position is changing

    // protected methods...
    bool isLookingAtMe(AvatarSharedPointer avatar) const;

    void fade(render::Transaction& transaction, render::Transition::Type type);

    glm::vec3 getBodyRightDirection() const { return getOrientation() * IDENTITY_RIGHT; }
    glm::vec3 getBodyUpDirection() const { return getOrientation() * IDENTITY_UP; }
    void measureMotionDerivatives(float deltaTime);

    float getSkeletonHeight() const;
    float getHeadHeight() const;
    float getPelvisFloatingHeight() const;
    glm::vec3 getDisplayNamePosition() const;

    Transform calculateDisplayNameTransform(const ViewFrustum& view, const glm::vec3& textPosition) const;
    void renderDisplayName(gpu::Batch& batch, const ViewFrustum& view, const glm::vec3& textPosition) const;
    virtual bool shouldRenderHead(const RenderArgs* renderArgs) const;
    virtual void fixupModelsInScene(const render::ScenePointer& scene);

    virtual void updatePalms();

    render::ItemID _renderItemID{ render::Item::INVALID_ITEM_ID };

    ThreadSafeValueCache<glm::vec3> _leftPalmPositionCache { glm::vec3() };
    ThreadSafeValueCache<glm::quat> _leftPalmRotationCache { glm::quat() };
    ThreadSafeValueCache<glm::vec3> _rightPalmPositionCache { glm::vec3() };
    ThreadSafeValueCache<glm::quat> _rightPalmRotationCache { glm::quat() };

    // Some rate tracking support
    RateCounter<> _simulationRate;
    RateCounter<> _simulationInViewRate;
    RateCounter<> _skeletonModelSimulationRate;
    RateCounter<> _jointDataSimulationRate;

private:
    class AvatarEntityDataHash {
    public:
        AvatarEntityDataHash(uint32_t h) : hash(h) {};
        uint32_t hash { 0 };
        bool success { false };
    };

    using MapOfAvatarEntityDataHashes = QMap<QUuid, AvatarEntityDataHash>;
    MapOfAvatarEntityDataHashes _avatarEntityDataHashes;

    uint64_t _lastRenderUpdateTime { 0 };
    int _leftPointerGeometryID { 0 };
    int _rightPointerGeometryID { 0 };
    int _nameRectGeometryID { 0 };
    bool _initialized { false };
    bool _isLookAtTarget { false };
    bool _isAnimatingScale { false };
    bool _mustFadeIn { false };
    bool _isFading { false };
    float _modelScale { 1.0f };

    static int _jointConesID;

    int _voiceSphereID;

    AvatarPhysicsCallback _physicsCallback { nullptr };

    float _displayNameTargetAlpha { 1.0f };
    float _displayNameAlpha { 1.0f };
};

#endif // hifi_Avatar_h
