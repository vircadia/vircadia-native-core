//
//  Avatar.h
//  interface/src/avatar
//
//  Copyright 2012 High Fidelity, Inc.
//  Copyright 2021 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Avatar_h
#define hifi_Avatar_h

#include <functional>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <map>
#include <set>
#include <vector>

#include <QtCore/QUuid>

#include <AvatarData.h>
#include <ShapeInfo.h>
#include <render/Scene.h>
#include <graphics-scripting/Forward.h>
#include <GLMHelpers.h>
#include <EntityItem.h>

#include <Grab.h>
#include <ThreadSafeValueCache.h>

#include "Head.h"
#include "SkeletonModel.h"
#include "Rig.h"

#include "MetaModelPayload.h"
#include "MultiSphereShape.h"

namespace render {
    template <> const ItemKey payloadGetKey(const AvatarSharedPointer& avatar);
    template <> const Item::Bound payloadGetBound(const AvatarSharedPointer& avatar, RenderArgs* args);
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

class AvatarTransit {
public:
    enum Status {
        IDLE = 0,
        STARTED,
        PRE_TRANSIT,
        START_TRANSIT,
        TRANSITING,
        END_TRANSIT,
        POST_TRANSIT,
        ENDED,
        ABORT_TRANSIT
    };

    enum EaseType {
        NONE = 0,
        EASE_IN,
        EASE_OUT,
        EASE_IN_OUT
    };

    struct TransitConfig {
        TransitConfig() {};
        int _totalFrames { 0 };
        float _framesPerMeter { 0.0f };
        bool _isDistanceBased { false };
        float _minTriggerDistance { 0.0f };
        float _maxTriggerDistance { 0.0f };
        float _abortDistance{ 0.0f };
        EaseType _easeType { EaseType::EASE_OUT };
    };

    AvatarTransit() {};
    Status update(float deltaTime, const glm::vec3& avatarPosition, const TransitConfig& config);
    void slamPosition(const glm::vec3& avatarPosition);
    Status getStatus() { return _status; }
    bool isActive() { return _isActive; }
    glm::vec3 getCurrentPosition() { return _currentPosition; }
    glm::vec3 getEndPosition() { return _endPosition; }
    void setScale(float scale) { _scale = scale; }
    void reset();

private:
    Status updatePosition(float deltaTime);
    void start(float deltaTime, const glm::vec3& startPosition, const glm::vec3& endPosition, const TransitConfig& config);
    float getEaseValue(AvatarTransit::EaseType type, float value);
    bool _isActive { false };

    glm::vec3 _startPosition;
    glm::vec3 _endPosition;
    glm::vec3 _currentPosition;

    glm::vec3 _lastPosition;

    glm::vec3 _transitLine;
    float _totalDistance { 0.0f };
    float _preTransitTime { 0.0f };
    float _totalTime { 0.0f };
    float _transitTime { 0.0f };
    float _postTransitTime { 0.0f };
    float _currentTime { 0.0f };
    EaseType _easeType { EaseType::EASE_OUT };
    Status _status { Status::IDLE };
    float _scale { 1.0f };
};

class Avatar : public AvatarData, public scriptable::ModelProvider, public MetaModelPayload {
    Q_OBJECT

    /*jsdoc
     * @comment IMPORTANT: The JSDoc for the following properties should be copied to MyAvatar.h.
     *
     * @property {Vec3} skeletonOffset - Can be used to apply a translation offset between the avatar's position and the
     *     registration point of the 3D model.
     */
    Q_PROPERTY(glm::vec3 skeletonOffset READ getSkeletonOffset WRITE setSkeletonOffset)

public:
    static void setShowAvatars(bool render);
    static void setShowReceiveStats(bool receiveStats);
    static void setShowMyLookAtVectors(bool showMine);
    static void setShowMyLookAtTarget(bool showMine);
    static void setShowOtherLookAtVectors(bool showOthers);
    static void setShowOtherLookAtTarget(bool showOthers);
    static void setShowCollisionShapes(bool render);
    static void setShowNamesAboveHeads(bool show);

    explicit Avatar(QThread* thread);
    virtual ~Avatar();

    virtual void instantiableAvatar() = 0;

    typedef render::Payload<AvatarData> Payload;

    void init();
    void removeAvatarEntitiesFromTree();
    virtual void simulate(float deltaTime, bool inView) = 0;
    virtual void simulateAttachments(float deltaTime);

    virtual void render(RenderArgs* renderArgs);

    void addToScene(AvatarSharedPointer self, const render::ScenePointer& scene,
                            render::Transaction& transaction);

    void removeFromScene(AvatarSharedPointer self, const render::ScenePointer& scene,
                                render::Transaction& transaction);

    void updateRenderItem(render::Transaction& transaction);

    virtual void postUpdate(float deltaTime, const render::ScenePointer& scene);

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

    virtual void createOrb() { }

    enum class LoadingStatus {
        NoModel,
        LoadModel,
        LoadSuccess,
        LoadFailure
    };
    virtual void indicateLoadingStatus(LoadingStatus loadingStatus) { _loadingStatus = loadingStatus; }

    virtual QVector<glm::quat> getJointRotations() const override;
    using AvatarData::getJointRotation;
    virtual glm::quat getJointRotation(int index) const override;
    virtual QVector<glm::vec3> getJointTranslations() const override;
    using AvatarData::getJointTranslation;
    virtual glm::vec3 getJointTranslation(int index) const override;
    virtual int getJointIndex(const QString& name) const override;
    virtual QStringList getJointNames() const override;

    std::vector<AvatarSkeletonTrait::UnpackedJointData> getSkeletonDefaultData();

    /*@jsdoc
     * Gets the default rotation of a joint (in the current avatar) relative to its parent.
     * <p>For information on the joint hierarchy used, see
     * <a href="https://docs.vircadia.com/create/avatars/avatar-standards.html">Avatar Standards</a>.</p>
     * @function MyAvatar.getDefaultJointRotation
     * @param {number} index - The joint index.
     * @returns {Quat} The default rotation of the joint if the joint index is valid, otherwise {@link Quat(0)|Quat.IDENTITY}.
     */
    Q_INVOKABLE virtual glm::quat getDefaultJointRotation(int index) const;

    /*@jsdoc
     * Gets the default translation of a joint (in the current avatar) relative to its parent, in model coordinates.
     * <p><strong>Warning:</strong> These coordinates are not necessarily in meters.</p>
     * <p>For information on the joint hierarchy used, see
     * <a href="https://docs.vircadia.com/create/avatars/avatar-standards.html">Avatar Standards</a>.</p>
     * @function MyAvatar.getDefaultJointTranslation
     * @param {number} index - The joint index.
     * @returns {Vec3} The default translation of the joint (in model coordinates) if the joint index is valid, otherwise
     *     {@link Vec3(0)|Vec3.ZERO}.
     */
    Q_INVOKABLE virtual glm::vec3 getDefaultJointTranslation(int index) const;

    /*@jsdoc
     * Gets the default joint rotations in avatar coordinates.
     * The default pose of the avatar is defined by the position and orientation of all bones
     * in the avatar's model file. Typically this is a T-pose.
     * @function MyAvatar.getAbsoluteDefaultJointRotationInObjectFrame
     * @param index {number} - The joint index.
     * @returns {Quat} The default rotation of the joint in avatar coordinates.
     * @example <caption>Report the default rotation of your avatar's head joint relative to your avatar.</caption>
     * var headIndex = MyAvatar.getJointIndex("Head");
     * var defaultHeadRotation = MyAvatar.getAbsoluteDefaultJointRotationInObjectFrame(headIndex);
     * print("Default head rotation: " + JSON.stringify(Quat.safeEulerAngles(defaultHeadRotation))); // Degrees
     */
    Q_INVOKABLE virtual glm::quat getAbsoluteDefaultJointRotationInObjectFrame(int index) const;

    /*@jsdoc
     * Gets the default joint translations in avatar coordinates.
     * The default pose of the avatar is defined by the position and orientation of all bones
     * in the avatar's model file. Typically this is a T-pose.
     * @function MyAvatar.getAbsoluteDefaultJointTranslationInObjectFrame
     * @param index {number} - The joint index.
     * @returns {Vec3} The default position of the joint in avatar coordinates.
     * @example <caption>Report the default translation of your avatar's head joint relative to your avatar.</caption>
     * var headIndex = MyAvatar.getJointIndex("Head");
     * var defaultHeadTranslation = MyAvatar.getAbsoluteDefaultJointTranslationInObjectFrame(headIndex);
     * print("Default head translation: " + JSON.stringify(defaultHeadTranslation));
     */
    Q_INVOKABLE virtual glm::vec3 getAbsoluteDefaultJointTranslationInObjectFrame(int index) const;


    virtual glm::vec3 getAbsoluteJointScaleInObjectFrame(int index) const override;
    virtual glm::quat getAbsoluteJointRotationInObjectFrame(int index) const override;
    virtual glm::vec3 getAbsoluteJointTranslationInObjectFrame(int index) const override;

    /*@jsdoc
     * Sets the rotation of a joint relative to the avatar.
     * <p><strong>Warning:</strong> Not able to be used in the <code>MyAvatar</code> API.</p>
     * @function MyAvatar.setAbsoluteJointRotationInObjectFrame
     * @param {number} index - The index of the joint. <em>Not used.</em>
     * @param {Quat} rotation - The rotation of the joint relative to the avatar. <em>Not used.</em>
     * @returns {boolean} <code>false</code>.
     */
    virtual bool setAbsoluteJointRotationInObjectFrame(int index, const glm::quat& rotation) override { return false; }

    /*@jsdoc
     * Sets the translation of a joint relative to the avatar.
     * <p><strong>Warning:</strong> Not able to be used in the <code>MyAvatar</code> API.</p>
     * @function MyAvatar.setAbsoluteJointTranslationInObjectFrame
     * @param {number} index - The index of the joint. <em>Not used.</em>
     * @param {Vec3} translation - The translation of the joint relative to the avatar. <em>Not used.</em>
     * @returns {boolean} <code>false</code>.
     */
    virtual bool setAbsoluteJointTranslationInObjectFrame(int index, const glm::vec3& translation) override { return false; }
    virtual glm::vec3 getSpine2SplineOffset() const { return _spine2SplineOffset; }
    virtual float getSpine2SplineRatio() const { return _spine2SplineRatio; }

    // world-space to avatar-space rigconversion functions
    /*@jsdoc
     * Transforms a position in world coordinates to a position in a joint's coordinates, or avatar coordinates if no joint is
     * specified.
     * @function MyAvatar.worldToJointPoint
     * @param {Vec3} position - The position in world coordinates.
     * @param {number} [jointIndex=-1] - The index of the joint.
     * @returns {Vec3} The position in the joint's coordinate system, or avatar coordinate system if no joint is specified.
     */
    Q_INVOKABLE glm::vec3 worldToJointPoint(const glm::vec3& position, const int jointIndex = -1) const;

    /*@jsdoc
     * Transforms a direction in world coordinates to a direction in a joint's coordinates, or avatar coordinates if no joint
     * is specified.
     * @function MyAvatar.worldToJointDirection
     * @param {Vec3} direction - The direction in world coordinates.
     * @param {number} [jointIndex=-1] - The index of the joint.
     * @returns {Vec3} The direction in the joint's coordinate system, or avatar coordinate system if no joint is specified.
     */
    Q_INVOKABLE glm::vec3 worldToJointDirection(const glm::vec3& direction, const int jointIndex = -1) const;

    /*@jsdoc
     * Transforms a rotation in world coordinates to a rotation in a joint's coordinates, or avatar coordinates if no joint is
     * specified.
     * @function MyAvatar.worldToJointRotation
     * @param {Quat} rotation - The rotation in world coordinates.
     * @param {number} [jointIndex=-1] - The index of the joint.
     * @returns {Quat} The rotation in the joint's coordinate system, or avatar coordinate system if no joint is specified.
    */
    Q_INVOKABLE glm::quat worldToJointRotation(const glm::quat& rotation, const int jointIndex = -1) const;

    /*@jsdoc
     * Transforms a position in a joint's coordinates, or avatar coordinates if no joint is specified, to a position in world
     * coordinates.
     * @function MyAvatar.jointToWorldPoint
     * @param {Vec3} position - The position in joint coordinates, or avatar coordinates if no joint is specified.
     * @param {number} [jointIndex=-1] - The index of the joint.
     * @returns {Vec3} The position in world coordinates.
     */
    Q_INVOKABLE glm::vec3 jointToWorldPoint(const glm::vec3& position, const int jointIndex = -1) const;

    /*@jsdoc
     * Transforms a direction in a joint's coordinates, or avatar coordinates if no joint is specified, to a direction in world
     * coordinates.
     * @function MyAvatar.jointToWorldDirection
     * @param {Vec3} direction - The direction in joint coordinates, or avatar coordinates if no joint is specified.
     * @param {number} [jointIndex=-1] - The index of the joint.
     * @returns {Vec3} The direction in world coordinates.
     */
    Q_INVOKABLE glm::vec3 jointToWorldDirection(const glm::vec3& direction, const int jointIndex = -1) const;

    /*@jsdoc
     * Transforms a rotation in a joint's coordinates, or avatar coordinates if no joint is specified, to a rotation in world
     * coordinates.
     * @function MyAvatar.jointToWorldRotation
     * @param {Quat} rotation - The rotation in joint coordinates, or avatar coordinates if no joint is specified.
     * @param {number} [jointIndex=-1] - The index of the joint.
     * @returns {Quat} The rotation in world coordinates.
     */
    Q_INVOKABLE glm::quat jointToWorldRotation(const glm::quat& rotation, const int jointIndex = -1) const;

    Q_INVOKABLE virtual void setSkeletonModelURL(const QUrl& skeletonModelURL) override;
    virtual void setAttachmentData(const QVector<AttachmentData>& attachmentData) override;

    void updateDisplayNameAlpha(bool showDisplayName);
    virtual void setSessionDisplayName(const QString& sessionDisplayName) override { }; // no-op

    virtual int parseDataFromBuffer(const QByteArray& buffer) override;

    /*@jsdoc
     * Sets the offset applied to the current avatar. The offset adjusts the position that the avatar is rendered. For example,
     * with an offset of <code>{ x: 0, y: 0.1, z: 0 }</code>, your avatar will appear to be raised off the ground slightly.
     * @function MyAvatar.setSkeletonOffset
     * @param {Vec3} offset - The skeleton offset to set.
     * @example <caption>Raise your avatar off the ground a little.</caption>
     * // Raise your avatar off the ground a little.
     * MyAvatar.setSkeletonOffset({ x: 0, y: 0.1: z: 0 });
     *
     * // Restore its offset after 5s.
     * Script.setTimeout(function () {
     *     MyAvatar.setSkeletonOffset(Vec3.ZERO);
     * }, 5000);
     */
    Q_INVOKABLE void setSkeletonOffset(const glm::vec3& offset);

    /*@jsdoc
     * Gets the offset applied to the current avatar. The offset adjusts the position that the avatar is rendered. For example,
     * with an offset of <code>{ x: 0, y: 0.1, z: 0 }</code>, your avatar will appear to be raised off the ground slightly.
     * @function MyAvatar.getSkeletonOffset
     * @returns {Vec3} The current skeleton offset.
     * @example <caption>Report your avatar's current skeleton offset.</caption>
     * print(JSON.stringify(MyAvatar.getSkeletonOffset());
     */
    Q_INVOKABLE glm::vec3 getSkeletonOffset() { return _skeletonOffset; }

    virtual glm::vec3 getSkeletonPosition() const;

    /*@jsdoc
     * Gets the position of a joint in the current avatar.
     * @function MyAvatar.getJointPosition
     * @param {number} index - The index of the joint.
     * @returns {Vec3} The position of the joint in world coordinates.
     */
    Q_INVOKABLE glm::vec3 getJointPosition(int index) const;

    /*@jsdoc
     * Gets the position of a joint in the current avatar.
     * @function MyAvatar.getJointPosition
     * @param {string} name - The name of the joint.
     * @returns {Vec3} The position of the joint in world coordinates.
     * @example <caption>Report the position of your avatar's hips.</caption>
     * print(JSON.stringify(MyAvatar.getJointPosition("Hips")));
     */
    Q_INVOKABLE glm::vec3 getJointPosition(const QString& name) const;

    /*@jsdoc
     * Gets the position of the current avatar's neck in world coordinates.
     * @function MyAvatar.getNeckPosition
     * @returns {Vec3} The position of the neck in world coordinates.
     * @example <caption>Report the position of your avatar's neck.</caption>
     * print(JSON.stringify(MyAvatar.getNeckPosition()));
     */
    Q_INVOKABLE glm::vec3 getNeckPosition() const;

    /*@jsdoc
     * Gets the current acceleration of the avatar.
     * @function MyAvatar.getAcceleration
     * @returns {Vec3} The current acceleration of the avatar.
     */
    Q_INVOKABLE glm::vec3 getAcceleration() const { return _acceleration; }

    /// Scales a world space position vector relative to the avatar position and scale
    /// \param vector position to be scaled. Will store the result
    void scaleVectorRelativeToPosition(glm::vec3& positionToScale) const;

    void slamPosition(const glm::vec3& position);
    virtual void updateAttitude(const glm::quat& orientation) override;

    // Call this when updating Avatar position with a delta.  This will allow us to
    // _accurately_ measure position changes and compute the resulting velocity
    // (otherwise floating point error will cause problems at large positions).
    void applyPositionDelta(const glm::vec3& delta);

    virtual void rebuildCollisionShape() = 0;

    virtual void computeShapeInfo(ShapeInfo& shapeInfo);
    virtual void computeDetailedShapeInfo(ShapeInfo& shapeInfo, int jointIndex);

    void getCapsule(glm::vec3& start, glm::vec3& end, float& radius);
    float computeMass();
    /*@jsdoc
     * Gets the position of the current avatar's feet (or rather, bottom of its collision capsule) in world coordinates.
     * @function MyAvatar.getWorldFeetPosition
     * @returns {Vec3} The position of the avatar's feet in world coordinates.
     */
    Q_INVOKABLE glm::vec3 getWorldFeetPosition();

    void setPositionViaScript(const glm::vec3& position) override;
    void setOrientationViaScript(const glm::quat& orientation) override;

    /*@jsdoc
     * Gets the ID of the entity or avatar that the avatar is parented to.
     * @function MyAvatar.getParentID
     * @returns {Uuid} The ID of the entity or avatar that the avatar is parented to. {@link Uuid(0)|Uuid.NULL} if not parented.
     */
    // This calls through to the SpatiallyNestable versions, but is here to expose these to JavaScript.
    Q_INVOKABLE virtual const QUuid getParentID() const override { return SpatiallyNestable::getParentID(); }

    /*@jsdoc
     * Sets the ID of the entity or avatar that the avatar is parented to.
     * @function MyAvatar.setParentID
     * @param {Uuid} parentID - The ID of the entity or avatar that the avatar should be parented to. Set to
     *    {@link Uuid(0)|Uuid.NULL} to unparent.
     */
    // This calls through to the SpatiallyNestable versions, but is here to expose these to JavaScript.
    Q_INVOKABLE virtual void setParentID(const QUuid& parentID) override;

    /*@jsdoc
     * Gets the joint of the entity or avatar that the avatar is parented to.
     * @function MyAvatar.getParentJointIndex
     * @returns {number} The joint of the entity or avatar that the avatar is parented to. <code>65535</code> or
     *     <code>-1</code> if parented to the entity or avatar's position and orientation rather than a joint.
     */
    // This calls through to the SpatiallyNestable versions, but is here to expose these to JavaScript.
    Q_INVOKABLE virtual quint16 getParentJointIndex() const override { return SpatiallyNestable::getParentJointIndex(); }

    /*@jsdoc
     * Sets the joint of the entity or avatar that the avatar is parented to.
     * @function MyAvatar.setParentJointIndex
     * @param {number} parentJointIndex - The joint of the entity or avatar that the avatar should be parented to. Use
     *     <code>65535</code> or <code>-1</code> to parent to the entity or avatar's position and orientation rather than a
     *     joint.
     */
    // This calls through to the SpatiallyNestable versions, but is here to expose these to JavaScript.
    Q_INVOKABLE virtual void setParentJointIndex(quint16 parentJointIndex) override;

    /*@jsdoc
     * Gets information on all the joints in the avatar's skeleton.
     * @function MyAvatar.getSkeleton
     * @returns {SkeletonJoint[]} Information about each joint in the avatar's skeleton.
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

    /*@jsdoc
     * @function MyAvatar.getSimulationRate
     * @param {AvatarSimulationRate} [rateName=""] - Rate name.
     * @returns {number} Simulation rate in Hz.
     * @deprecated This function is deprecated and will be removed.
     */
    Q_INVOKABLE float getSimulationRate(const QString& rateName = QString("")) const;

    bool hasNewJointData() const { return _hasNewJointData; }

    float getBoundingRadius() const;
    AABox getRenderBounds() const; // THis call is accessible from rendering thread only to report the bounding box of the avatar during the frame.
    AABox getFitBounds() const { return _fitBoundingBox; }

    void addToScene(AvatarSharedPointer self, const render::ScenePointer& scene);
    void ensureInScene(AvatarSharedPointer self, const render::ScenePointer& scene);
    bool isInScene() const { return render::Item::isValidID(_renderItemID); }
    render::ItemID getRenderItemID() { return _renderItemID; }
    bool isMoving() const { return _moving; }

    void fadeIn(render::ScenePointer scene);
    void fadeOut(render::Transaction& transaction, KillAvatarReason reason);
    render::Transition::Type getLastFadeRequested() const;

    // JSDoc is in AvatarData.h.
    Q_INVOKABLE virtual float getEyeHeight() const override;

    // returns eye height of avatar in meters, ignoring avatar scale.
    // if _targetScale is 1 then this will be identical to getEyeHeight.
    virtual float getUnscaledEyeHeight() const override;

    // returns true, if an acurate eye height estimage can be obtained by inspecting the avatar model skeleton and geometry,
    // not all subclasses of AvatarData have access to this data.
    virtual bool canMeasureEyeHeight() const override { return true; }

    virtual float getModelScale() const { return _modelScale; }
    virtual void setModelScale(float scale) { _modelScale = scale; }
    virtual glm::vec3 scaleForChildren() const override { return glm::vec3(getModelScale()); }

    // Show hide the model representation of the avatar
    virtual void setEnableMeshVisible(bool isEnabled);
    virtual bool getEnableMeshVisible() const;

    void addMaterial(graphics::MaterialLayer material, const std::string& parentMaterialName);
    void removeMaterial(graphics::MaterialPointer material, const std::string& parentMaterialName);

    virtual scriptable::ScriptableModelBase getScriptableModel() override;

    std::shared_ptr<AvatarTransit> getTransit() { return std::make_shared<AvatarTransit>(_transit); };
    AvatarTransit::Status updateTransit(float deltaTime, const glm::vec3& avatarPosition, float avatarScale, const AvatarTransit::TransitConfig& config);

    void accumulateGrabPositions(std::map<QUuid, GrabLocationAccumulator>& grabAccumulators);

    const std::vector<MultiSphereShape>& getMultiSphereShapes() const { return _multiSphereShapes; }
    void tearDownGrabs();

    uint32_t appendSubMetaItems(render::ItemIDs& subItems);

signals:
    /*@jsdoc
     * Triggered when the avatar's target scale is changed. The target scale is the desired scale of the avatar without any
     * restrictions on permissible scale values imposed by the domain.
     * @function MyAvatar.targetScaleChanged
     * @param {number} targetScale - The avatar's target scale.
     * @returns Signal
     */
    void targetScaleChanged(float targetScale);

public slots:

    // FIXME - these should be migrated to use Pose data instead
    // thread safe, will return last valid palm from cache

    /*@jsdoc
     * Gets the position of the left palm in world coordinates.
     * @function MyAvatar.getLeftPalmPosition
     * @returns {Vec3} The position of the left palm in world coordinates.
     * @example <caption>Report the position of your avatar's left palm.</caption>
     * print(JSON.stringify(MyAvatar.getLeftPalmPosition()));
     */
    glm::vec3 getLeftPalmPosition() const;

    /*@jsdoc
     * Gets the rotation of the left palm in world coordinates.
     * @function MyAvatar.getLeftPalmRotation
     * @returns {Quat} The rotation of the left palm in world coordinates.
     * @example <caption>Report the rotation of your avatar's left palm.</caption>
     * print(JSON.stringify(MyAvatar.getLeftPalmRotation()));
     */
    glm::quat getLeftPalmRotation() const;

    /*@jsdoc
     * Gets the position of the right palm in world coordinates.
     * @function MyAvatar.getRightPalmPosition
     * @returns {Vec3} The position of the right palm in world coordinates.
     * @example <caption>Report the position of your avatar's right palm.</caption>
     * print(JSON.stringify(MyAvatar.getRightPalmPosition()));
     */
    glm::vec3 getRightPalmPosition() const;

    /*@jsdoc
     * Get the rotation of the right palm in world coordinates.
     * @function MyAvatar.getRightPalmRotation
     * @returns {Quat} The rotation of the right palm in world coordinates.
     * @example <caption>Report the rotation of your avatar's right palm.</caption>
     * print(JSON.stringify(MyAvatar.getRightPalmRotation()));
     */
    glm::quat getRightPalmRotation() const;

    /*@jsdoc
     * @function MyAvatar.setModelURLFinished
     * @param {boolean} success
     * @deprecated This function is deprecated and will be removed.
     */
    // hooked up to Model::setURLFinished signal
    void setModelURLFinished(bool success);

    /*@jsdoc
     * @function MyAvatar.rigReady
     * @deprecated This function is deprecated and will be removed.
     */
    // Hooked up to Model::rigReady signal
    void rigReady();

    /*@jsdoc
     * @function MyAvatar.rigReset
     * @deprecated This function is deprecated and will be removed.
     */
    // Hooked up to Model::rigReset signal
    void rigReset();

protected:
    float getUnscaledEyeHeightFromSkeleton() const;
    void buildUnscaledEyeHeightCache();
    void buildSpine2SplineRatioCache();
    void clearUnscaledEyeHeightCache();
    void clearSpine2SplineRatioCache();
    virtual const QString& getSessionDisplayNameForTransport() const override { return _empty; } // Save a tiny bit of bandwidth. Mixer won't look at what we send.
    QString _empty{};
    virtual void maybeUpdateSessionDisplayNameFromTransport(const QString& sessionDisplayName) override { _sessionDisplayName = sessionDisplayName; } // don't use no-op setter!
    void computeMultiSphereShapes();
    void updateFitBoundingBox();

    SkeletonModelPointer _skeletonModel;

    void invalidateJointIndicesCache() const;
    void withValidJointIndicesCache(std::function<void()> const& worker) const;
    mutable QHash<QString, int> _modelJointIndicesCache;
    mutable QReadWriteLock _modelJointIndicesCacheLock;
    mutable bool _modelJointsCached { false };

    glm::vec3 _skeletonOffset;
    std::vector<std::shared_ptr<Model>> _attachmentModels;
    std::vector<bool> _attachmentModelsTexturesLoaded;
    std::vector<std::shared_ptr<Model>> _attachmentsToRemove;
    std::vector<std::shared_ptr<Model>> _attachmentsToDelete;

    float _bodyYawDelta { 0.0f };  // degrees/sec
    float _seatedBodyYawDelta{ 0.0f };  // degrees/renderframe

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
    virtual void sendPacket(const QUuid& entityID) const { }
    bool applyGrabChanges();
    void relayJointDataToChildren();

    void fade(render::Transaction& transaction, render::Transition::Type type);

    glm::vec3 getBodyRightDirection() const { return getWorldOrientation() * IDENTITY_RIGHT; }
    glm::vec3 getBodyUpDirection() const { return getWorldOrientation() * IDENTITY_UP; }
    void measureMotionDerivatives(float deltaTime);
    bool getCollideWithOtherAvatars() const { return _collideWithOtherAvatars; }

    float getSkeletonHeight() const;
    float getHeadHeight() const;
    float getPelvisFloatingHeight() const;
    glm::vec3 getDisplayNamePosition() const;

    Transform calculateDisplayNameTransform(const ViewFrustum& view, const glm::vec3& textPosition) const;
    void renderDisplayName(gpu::Batch& batch, const ViewFrustum& view, const glm::vec3& textPosition, bool forward) const;
    virtual bool shouldRenderHead(const RenderArgs* renderArgs) const;
    virtual void fixupModelsInScene(const render::ScenePointer& scene);

    virtual void updatePalms();

    render::ItemID _renderItemID{ render::Item::INVALID_ITEM_ID };
    render::Transition::Type _lastFadeRequested { render::Transition::Type::NONE }; // Used for sanity checking

    ThreadSafeValueCache<glm::vec3> _leftPalmPositionCache { glm::vec3() };
    ThreadSafeValueCache<glm::quat> _leftPalmRotationCache { glm::quat() };
    ThreadSafeValueCache<glm::vec3> _rightPalmPositionCache { glm::vec3() };
    ThreadSafeValueCache<glm::quat> _rightPalmRotationCache { glm::quat() };

    // Some rate tracking support
    RateCounter<> _simulationRate;
    RateCounter<> _simulationInViewRate;
    RateCounter<> _skeletonModelSimulationRate;
    RateCounter<> _jointDataSimulationRate;

    class AvatarEntityDataHash {
    public:
        AvatarEntityDataHash(uint32_t h) : hash(h) {};
        uint32_t hash { 0 };
        bool success { false };
    };

    uint64_t _lastRenderUpdateTime { 0 };
    int _leftPointerGeometryID { 0 };
    int _rightPointerGeometryID { 0 };
    int _nameRectGeometryID { 0 };
    bool _initialized { false };
    bool _isAnimatingScale { false };
    bool _mustFadeIn { false };
    bool _reconstructSoftEntitiesJointMap { false };
    float _modelScale { 1.0f };

    AvatarTransit _transit;
    std::mutex _transitLock;

    int _voiceSphereID;

    float _displayNameTargetAlpha { 1.0f };
    float _displayNameAlpha { 1.0f };

    ThreadSafeValueCache<float> _unscaledEyeHeightCache { DEFAULT_AVATAR_EYE_HEIGHT };
    float _spine2SplineRatio { DEFAULT_SPINE2_SPLINE_PROPORTION };
    glm::vec3 _spine2SplineOffset;

    std::unordered_map<std::string, graphics::MultiMaterial> _materials;
    std::mutex _materialsLock;

    void processMaterials();

    AABox _renderBound;
    bool _isMeshVisible{ true };
    bool _needMeshVisibleSwitch{ true };

    static const float MYAVATAR_LOADING_PRIORITY;
    static const float OTHERAVATAR_LOADING_PRIORITY;
    static const float ATTACHMENT_LOADING_PRIORITY;

    LoadingStatus _loadingStatus { LoadingStatus::NoModel };

    static void metaBlendshapeOperator(render::ItemID renderItemID, int blendshapeNumber, const QVector<BlendshapeOffset>& blendshapeOffsets,
                                       const QVector<int>& blendedMeshSizes, const render::ItemIDs& subItemIDs);

    std::vector<MultiSphereShape> _multiSphereShapes;
    AABox _fitBoundingBox;
    void clearAvatarGrabData(const QUuid& grabID) override;

    using SetOfIDs = std::set<QUuid>;
    using VectorOfIDs = std::vector<QUuid>;
    using MapOfGrabs = std::map<QUuid, GrabPointer>;

    MapOfGrabs _avatarGrabs;
    SetOfIDs _grabsToChange; // updated grab IDs -- changes needed to entities or physics
    VectorOfIDs _grabsToDelete; // deleted grab IDs -- changes needed to entities or physics

    ReadWriteLockable _subItemLock;
    void updateAttachmentRenderIDs();
    render::ItemIDs _attachmentRenderIDs;
    void updateDescendantRenderIDs();
    render::ItemIDs _descendantRenderIDs;
    std::unordered_set<EntityItemID> _renderingDescendantEntityIDs;
    uint32_t _lastAncestorChainRenderableVersion { 0 };
};

#endif // hifi_Avatar_h
