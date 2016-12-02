//
//  MyAvatar.h
//  interface/src/avatar
//
//  Created by Mark Peng on 8/16/13.
//  Copyright 2012 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_MyAvatar_h
#define hifi_MyAvatar_h

#include <glm/glm.hpp>

#include <SettingHandle.h>
#include <Rig.h>
#include <Sound.h>

#include <controllers/Pose.h>
#include <controllers/Actions.h>

#include "Avatar.h"
#include "AtRestDetector.h"
#include "MyCharacterController.h"
#include <ThreadSafeValueCache.h>

class AvatarActionHold;
class ModelItemID;

enum DriveKeys {
    TRANSLATE_X = 0,
    TRANSLATE_Y,
    TRANSLATE_Z,
    YAW,
    STEP_TRANSLATE_X,
    STEP_TRANSLATE_Y,
    STEP_TRANSLATE_Z,
    STEP_YAW,
    PITCH,
    ZOOM,
    MAX_DRIVE_KEYS
};

enum eyeContactTarget {
    LEFT_EYE,
    RIGHT_EYE,
    MOUTH
};

enum AudioListenerMode {
    FROM_HEAD = 0,
    FROM_CAMERA,
    CUSTOM
};
Q_DECLARE_METATYPE(AudioListenerMode);

const size_t DEBUG_LINE_LOOP_SIZE = 500;

class MyAvatar : public Avatar {
    Q_OBJECT
    Q_PROPERTY(bool shouldRenderLocally READ getShouldRenderLocally WRITE setShouldRenderLocally)
    Q_PROPERTY(glm::vec3 motorVelocity READ getScriptedMotorVelocity WRITE setScriptedMotorVelocity)
    Q_PROPERTY(float motorTimescale READ getScriptedMotorTimescale WRITE setScriptedMotorTimescale)
    Q_PROPERTY(QString motorReferenceFrame READ getScriptedMotorFrame WRITE setScriptedMotorFrame)
    Q_PROPERTY(QString collisionSoundURL READ getCollisionSoundURL WRITE setCollisionSoundURL)
    Q_PROPERTY(AudioListenerMode audioListenerMode READ getAudioListenerMode WRITE setAudioListenerMode)
    Q_PROPERTY(glm::vec3 customListenPosition READ getCustomListenPosition WRITE setCustomListenPosition)
    Q_PROPERTY(glm::quat customListenOrientation READ getCustomListenOrientation WRITE setCustomListenOrientation)
    Q_PROPERTY(AudioListenerMode audioListenerModeHead READ getAudioListenerModeHead)
    Q_PROPERTY(AudioListenerMode audioListenerModeCamera READ getAudioListenerModeCamera)
    Q_PROPERTY(AudioListenerMode audioListenerModeCustom READ getAudioListenerModeCustom)
    //TODO: make gravity feature work Q_PROPERTY(glm::vec3 gravity READ getGravity WRITE setGravity)

    Q_PROPERTY(glm::vec3 leftHandPosition READ getLeftHandPosition)
    Q_PROPERTY(glm::vec3 rightHandPosition READ getRightHandPosition)
    Q_PROPERTY(glm::vec3 leftHandTipPosition READ getLeftHandTipPosition)
    Q_PROPERTY(glm::vec3 rightHandTipPosition READ getRightHandTipPosition)

    Q_PROPERTY(controller::Pose leftHandPose READ getLeftHandPose)
    Q_PROPERTY(controller::Pose rightHandPose READ getRightHandPose)
    Q_PROPERTY(controller::Pose leftHandTipPose READ getLeftHandTipPose)
    Q_PROPERTY(controller::Pose rightHandTipPose READ getRightHandTipPose)

    Q_PROPERTY(float energy READ getEnergy WRITE setEnergy)

    Q_PROPERTY(bool hmdLeanRecenterEnabled READ getHMDLeanRecenterEnabled WRITE setHMDLeanRecenterEnabled)
    Q_PROPERTY(bool avatarCollisionsEnabled READ getAvatarCollisionsEnabled WRITE setAvatarCollisionsEnabled)

    /**jsdoc
     * Your avatar is your in-world representation of you. The MyAvatar API is used to manipulate the avatar.
     * For example, using the MyAvatar API you can customize the avatar's appearance, run custom avatar animations,
     * change the avatar's position within the domain, or manage the avatar's collisions with other objects.
     * NOTE: MyAvatar extends Avatar and AvatarData, see those namespace for more properties/methods.
     *
     * @namespace MyAvatar
     * @augments Avatar
     * @property shouldRenderLocally {bool} Set it to true if you would like to see MyAvatar in your local interface,
     *   and false if you would not like to see MyAvatar in your local interface.
     * @property motorVelocity {Vec3} Can be used to move the avatar with this velocity.
     * @property motorTimescale {float} Specifies how quickly the avatar should accelerate to meet the motorVelocity,
     *   smaller values will result in higher acceleration.
     * @property motorReferenceFrame {string} Reference frame of the motorVelocity, must be one of the following: "avatar", "camera", "world"
     * @property collisionSoundURL {string} Specifies the sound to play when the avatar experiences a collision.
     *   You can provide a mono or stereo 16-bit WAV file running at either 24 Khz or 48 Khz.
     *   The latter is downsampled by the audio mixer, so all audio effectively plays back at a 24 Khz sample rate.
     *   48 Khz RAW files are also supported.
     * @property audioListenerMode {number} When hearing spatialized audio this determines where the listener placed.
     *   Should be one of the following values:
     *   MyAvatar.audioListenerModeHead - the listener located at the avatar's head.
     *   MyAvatar.audioListenerModeCamera - the listener is relative to the camera.
     *   MyAvatar.audioListenerModeCustom - the listener is at a custom location specified by the MyAvatar.customListenPosition
     *   and MyAvatar.customListenOrientation properties.
     * @property customListenPosition {Vec3} If MyAvatar.audioListenerMode == MyAvatar.audioListenerModeHead, then this determines the position
     *   of audio spatialization listener.
     * @property customListenOreintation {Quat} If MyAvatar.audioListenerMode == MyAvatar.audioListenerModeHead, then this determines the orientation
     *   of the audio spatialization listener.
     * @property audioListenerModeHead {number} READ-ONLY. When passed to MyAvatar.audioListenerMode, it will set the audio listener
     *   around the avatar's head.
     * @property audioListenerModeCamera {number} READ-ONLY. When passed to MyAvatar.audioListenerMode, it will set the audio listener
     *   around the camera.
     * @property audioListenerModeCustom {number} READ-ONLY. When passed to MyAvatar.audioListenerMode, it will set the audio listener
     *  around the value specified by MyAvatar.customListenPosition and MyAvatar.customListenOrientation.
     * @property leftHandPosition {Vec3} READ-ONLY. The desired position of the left wrist in avatar space, determined by the hand controllers.
     *   Note: only valid if hand controllers are in use.
     * @property rightHandPosition {Vec3} READ-ONLY. The desired position of the right wrist in avatar space, determined by the hand controllers.
     *   Note: only valid if hand controllers are in use.
     * @property leftHandTipPosition {Vec3} READ-ONLY. A position 30 cm offset from MyAvatar.leftHandPosition
     * @property rightHandTipPosition {Vec3} READ-ONLY. A position 30 cm offset from MyAvatar.rightHandPosition
     * @property leftHandPose {Pose} READ-ONLY. Returns full pose (translation, orientation, velocity & angularVelocity) of the desired
     *   wrist position, determined by the hand controllers.
     * @property rightHandPose {Pose} READ-ONLY. Returns full pose (translation, orientation, velocity & angularVelocity) of the desired
     *   wrist position, determined by the hand controllers.
     * @property leftHandTipPose {Pose} READ-ONLY. Returns a pose offset 30 cm from MyAvatar.leftHandPose
     * @property rightHandTipPose {Pose} READ-ONLY. Returns a pose offset 30 cm from MyAvatar.rightHandPose
     * @property hmdLeanRecenterEnabled {bool} This can be used disable the hmd lean recenter behavior.  This behavior is what causes your avatar
     *   to follow your HMD as you walk around the room, in room scale VR.  Disabling this is useful if you desire to pin the avatar to a fixed location.
     * @property avatarCollisionsEnabled {bool} This can be used to disable collisions between the avatar and the world.
     */

public:
    explicit MyAvatar(RigPointer rig);
    ~MyAvatar();

    virtual void simulateAttachments(float deltaTime) override;

    AudioListenerMode getAudioListenerModeHead() const { return FROM_HEAD; }
    AudioListenerMode getAudioListenerModeCamera() const { return FROM_CAMERA; }
    AudioListenerMode getAudioListenerModeCustom() const { return CUSTOM; }

    void reset(bool andRecenter = false, bool andReload = true, bool andHead = true);

    /**jsdoc
     * Moves and orients the avatar, such that it is directly underneath the HMD, with toes pointed forward.
     * @function MyAvatar.centerBody
     */
    Q_INVOKABLE void centerBody(); // thread-safe

    /**jsdoc
     * The internal inverse-kinematics system maintains a record of which joints are "locked". Sometimes it is useful to forget this history, to prevent
     * contorted joints.
     * @function MyAvatar.centerBody
     */
    Q_INVOKABLE void clearIKJointLimitHistory(); // thread-safe

    void update(float deltaTime);
    virtual void postUpdate(float deltaTime) override;
    void preDisplaySide(RenderArgs* renderArgs);

    const glm::mat4& getHMDSensorMatrix() const { return _hmdSensorMatrix; }
    const glm::vec3& getHMDSensorPosition() const { return _hmdSensorPosition; }
    const glm::quat& getHMDSensorOrientation() const { return _hmdSensorOrientation; }
    const glm::vec2& getHMDSensorFacing() const { return _hmdSensorFacing; }
    const glm::vec2& getHMDSensorFacingMovingAverage() const { return _hmdSensorFacingMovingAverage; }

    Q_INVOKABLE void setOrientationVar(const QVariant& newOrientationVar);
    Q_INVOKABLE QVariant getOrientationVar() const;


    // Pass a recent sample of the HMD to the avatar.
    // This can also update the avatar's position to follow the HMD
    // as it moves through the world.
    void updateFromHMDSensorMatrix(const glm::mat4& hmdSensorMatrix);

    // read the location of a hand controller and save the transform
    void updateJointFromController(controller::Action poseKey, ThreadSafeValueCache<glm::mat4>& matrixCache);

    // best called at end of main loop, just before rendering.
    // update sensor to world matrix from current body position and hmd sensor.
    // This is so the correct camera can be used for rendering.
    enum class SensorToWorldUpdateMode {
        Full = 0,
        Vertical,
        VerticalComfort
    };
    void updateSensorToWorldMatrix(SensorToWorldUpdateMode mode = SensorToWorldUpdateMode::Full);

    void setSensorToWorldMatrix(const glm::mat4& sensorToWorld);

    void setRealWorldFieldOfView(float realWorldFov) { _realWorldFieldOfView.set(realWorldFov); }

    /**jsdoc
     * The default position in world coordinates of the point directly between the avatar's eyes
     * @function MyAvatar.getDefaultEyePosition
     * @example <caption>This example gets the default eye position and prints it to the debug log.</caption>
     * var defaultEyePosition = MyAvatar.getDefaultEyePosition();
     * print (JSON.stringify(defaultEyePosition));
     * @returns {Vec3} Position between the avatar's eyes.
     */
    Q_INVOKABLE glm::vec3 getDefaultEyePosition() const;

    float getRealWorldFieldOfView() { return _realWorldFieldOfView.get(); }

    /**jsdoc
     * The avatar animation system includes a set of default animations along with rules for how those animations are blended
     * together with procedural data (such as look at vectors, hand sensors etc.). overrideAnimation() is used to completely
     * override all motion from the default animation system (including inverse kinematics for hand and head controllers) and
     * play a specified animation.  To end this animation and restore the default animations, use MyAvatar.restoreAnimation.
     * @function MyAvatar.overrideAnimation
     * @example <caption> Play a clapping animation on your avatar for three seconds. </caption>
     * // Clap your hands for 3 seconds then restore animation back to the avatar.
     * var ANIM_URL = "https://s3.amazonaws.com/hifi-public/animations/ClapAnimations/ClapHands_Standing.fbx";
     * MyAvatar.overrideAnimation(ANIM_URL, 30, true, 0, 53);
     * Script.setTimeout(function () {
     *     MyAvatar.restoreAnimation();
     * }, 3000);
     * @param url {string} The URL to the animation file. Animation files need to be .FBX format, but only need to contain the avatar skeleton and animation data.
     * @param fps {number} The frames per second (FPS) rate for the animation playback. 30 FPS is normal speed.
     * @param loop {bool} Set to true if the animation should loop.
     * @param firstFrame {number} The frame the animation should start at.
     * @param lastFrame {number} The frame the animation should end at.
     */
    Q_INVOKABLE void overrideAnimation(const QString& url, float fps, bool loop, float firstFrame, float lastFrame);

    /**jsdoc
     * The avatar animation system includes a set of default animations along with rules for how those animations are blended together with
     * procedural data (such as look at vectors, hand sensors etc.). Playing your own custom animations will override the default animations.
     * restoreAnimation() is used to restore all motion from the default animation system including inverse kinematics for hand and head
     * controllers. If you aren't currently playing an override animation, this function will have no effect.
     * @function MyAvatar.restoreAnimation
     * @example <caption> Play a clapping animation on your avatar for three seconds. </caption>
     * // Clap your hands for 3 seconds then restore animation back to the avatar.
     * var ANIM_URL = "https://s3.amazonaws.com/hifi-public/animations/ClapAnimations/ClapHands_Standing.fbx";
     * MyAvatar.overrideAnimation(ANIM_URL, 30, true, 0, 53);
     * Script.setTimeout(function () {
     *     MyAvatar.restoreAnimation();
     * }, 3000);
     */
    Q_INVOKABLE void restoreAnimation();

    /**jsdoc
     * Each avatar has an avatar-animation.json file that defines which animations are used and how they are blended together with procedural data
     * (such as look at vectors, hand sensors etc.). Each animation specified in the avatar-animation.json file is known as an animation role.
     * Animation roles map to easily understandable actions that the avatar can perform, such as "idleStand", "idleTalk", or "walkFwd."
     * getAnimationRoles() is used get the list of animation roles defined in the avatar-animation.json.
     * @function MyAvatar.getAnimatationRoles
     * @example <caption>This example prints the list of animation roles defined in the avatar's avatar-animation.json file to the debug log.</caption>
     * var roles = MyAvatar.getAnimationRoles();
     * print("Animation Roles:");
     * for (var i = 0; i < roles.length; i++) {
     *     print(roles[i]);
     * }
     * @returns {string[]} Array of role strings
     */
    Q_INVOKABLE QStringList getAnimationRoles();

    /**jsdoc
     * Each avatar has an avatar-animation.json file that defines a set of animation roles. Animation roles map to easily understandable actions
     * that the avatar can perform, such as "idleStand", "idleTalk", or "walkFwd". To get the full list of roles, use getAnimationRoles().
     * For each role, the avatar-animation.json defines when the animation is used, the animation clip (.FBX) used, and how animations are blended
     * together with procedural data (such as look at vectors, hand sensors etc.).
     * overrideRoleAnimation() is used to change the animation clip (.FBX) associated with a specified animation role.
     * Note: Hand roles only affect the hand. Other 'main' roles, like 'idleStand', 'idleTalk', 'takeoffStand' are full body.
     * @function MyAvatar.overrideRoleAnimation
     * @example <caption>The default avatar-animation.json defines an "idleStand" animation role. This role specifies that when the avatar is not moving,
     * an animation clip of the avatar idling with hands hanging at its side will be used. It also specifies that when the avatar moves, the animation
     * will smoothly blend to the walking animation used by the "walkFwd" animation role.
     * In this example, the "idleStand" role animation clip has been replaced with a clapping animation clip. Now instead of standing with its arms
     * hanging at its sides when it is not moving, the avatar will stand and clap its hands. Note that just as it did before, as soon as the avatar
     * starts to move, the animation will smoothly blend into the walk animation used by the "walkFwd" animation role.</caption>
     * // An animation of the avatar clapping its hands while standing
     * var ANIM_URL = "https://s3.amazonaws.com/hifi-public/animations/ClapAnimations/ClapHands_Standing.fbx";
     * MyAvatar.overrideRoleAnimation("idleStand", ANIM_URL, 30, true, 0, 53);
     * // To restore the default animation, use MyAvatar.restoreRoleAnimation().
     * @param role {string} The animation role to override
     * @param url {string} The URL to the animation file. Animation files need to be .FBX format, but only need to contain the avatar skeleton and animation data.
     * @param fps {number} The frames per second (FPS) rate for the animation playback. 30 FPS is normal speed.
     * @param loop {bool} Set to true if the animation should loop
     * @param firstFrame {number} The frame the animation should start at
     * @param lastFrame {number} The frame the animation should end at
     */
    Q_INVOKABLE void overrideRoleAnimation(const QString& role, const QString& url, float fps, bool loop, float firstFrame, float lastFrame);

    /**jsdoc
     * Each avatar has an avatar-animation.json file that defines a set of animation roles. Animation roles map to easily understandable actions that
     * the avatar can perform, such as "idleStand", "idleTalk", or "walkFwd". To get the full list of roles, use getAnimationRoles(). For each role,
     * the avatar-animation.json defines when the animation is used, the animation clip (.FBX) used, and how animations are blended together with
     * procedural data (such as look at vectors, hand sensors etc.). You can change the animation clip (.FBX) associated with a specified animation
     * role using overrideRoleAnimation().
     * restoreRoleAnimation() is used to restore a specified animation role's default animation clip. If you have not specified an override animation
     * for the specified role, this function will have no effect.
     * @function MyAvatar.restoreRoleAnimation
     * @param rule {string} The animation role clip to restore
     */
    Q_INVOKABLE void restoreRoleAnimation(const QString& role);

    // Adds handler(animStateDictionaryIn) => animStateDictionaryOut, which will be invoked just before each animGraph state update.
    // The handler will be called with an animStateDictionaryIn that has all those properties specified by the (possibly empty)
    // propertiesList argument. However for debugging, if the properties argument is null, all internal animGraph state is provided.
    // The animStateDictionaryOut can be a different object than animStateDictionaryIn. Any properties set in animStateDictionaryOut
    // will override those of the internal animation machinery.
    // The animStateDictionaryIn may be shared among multiple handlers, and thus may contain additional properties specified when
    // adding one of the other handlers. While any handler may change a value in animStateDictionaryIn (or supply different values in animStateDictionaryOut)
    // a handler must not remove properties from animStateDictionaryIn, nor change property values that it does not intend to change.
    // It is not specified in what order multiple handlers are called.
    Q_INVOKABLE QScriptValue addAnimationStateHandler(QScriptValue handler, QScriptValue propertiesList) { return _rig->addAnimationStateHandler(handler, propertiesList); }
    // Removes a handler previously added by addAnimationStateHandler.
    Q_INVOKABLE void removeAnimationStateHandler(QScriptValue handler) { _rig->removeAnimationStateHandler(handler); }

    Q_INVOKABLE bool getSnapTurn() const { return _useSnapTurn; }
    Q_INVOKABLE void setSnapTurn(bool on) { _useSnapTurn = on; }
    Q_INVOKABLE bool getClearOverlayWhenMoving() const { return _clearOverlayWhenMoving; }
    Q_INVOKABLE void setClearOverlayWhenMoving(bool on) { _clearOverlayWhenMoving = on; }

    Q_INVOKABLE void setHMDLeanRecenterEnabled(bool value) { _hmdLeanRecenterEnabled = value; }
    Q_INVOKABLE bool getHMDLeanRecenterEnabled() const { return _hmdLeanRecenterEnabled; }

    // get/set avatar data
    void saveData();
    void loadData();

    void saveAttachmentData(const AttachmentData& attachment) const;
    AttachmentData loadAttachmentData(const QUrl& modelURL, const QString& jointName = QString()) const;

    //  Set what driving keys are being pressed to control thrust levels
    void clearDriveKeys();
    void setDriveKeys(int key, float val) { _driveKeys[key] = val; };
    void relayDriveKeysToCharacterController();

    eyeContactTarget getEyeContactTarget();

    Q_INVOKABLE glm::vec3 getTrackedHeadPosition() const { return _trackedHeadPosition; }
    Q_INVOKABLE glm::vec3 getHeadPosition() const { return getHead()->getPosition(); }
    Q_INVOKABLE float getHeadFinalYaw() const { return getHead()->getFinalYaw(); }
    Q_INVOKABLE float getHeadFinalRoll() const { return getHead()->getFinalRoll(); }
    Q_INVOKABLE float getHeadFinalPitch() const { return getHead()->getFinalPitch(); }
    Q_INVOKABLE float getHeadDeltaPitch() const { return getHead()->getDeltaPitch(); }

    Q_INVOKABLE glm::vec3 getEyePosition() const { return getHead()->getEyePosition(); }

    Q_INVOKABLE glm::vec3 getTargetAvatarPosition() const { return _targetAvatarPosition; }

    Q_INVOKABLE glm::vec3 getLeftHandPosition() const;
    Q_INVOKABLE glm::vec3 getRightHandPosition() const;
    Q_INVOKABLE glm::vec3 getLeftHandTipPosition() const;
    Q_INVOKABLE glm::vec3 getRightHandTipPosition() const;

    Q_INVOKABLE controller::Pose getLeftHandPose() const;
    Q_INVOKABLE controller::Pose getRightHandPose() const;
    Q_INVOKABLE controller::Pose getLeftHandTipPose() const;
    Q_INVOKABLE controller::Pose getRightHandTipPose() const;

    AvatarWeakPointer getLookAtTargetAvatar() const { return _lookAtTargetAvatar; }
    void updateLookAtTargetAvatar();
    void clearLookAtTargetAvatar();

    virtual void setJointRotations(QVector<glm::quat> jointRotations) override;
    virtual void setJointData(int index, const glm::quat& rotation, const glm::vec3& translation) override;
    virtual void setJointRotation(int index, const glm::quat& rotation) override;
    virtual void setJointTranslation(int index, const glm::vec3& translation) override;
    virtual void clearJointData(int index) override;
    virtual void clearJointsData() override;

    Q_INVOKABLE void useFullAvatarURL(const QUrl& fullAvatarURL, const QString& modelName = QString());
    Q_INVOKABLE QUrl getFullAvatarURLFromPreferences() const { return _fullAvatarURLFromPreferences; }
    Q_INVOKABLE QString getFullAvatarModelName() const { return _fullAvatarModelName; }
    void resetFullAvatarURL();


    virtual void setAttachmentData(const QVector<AttachmentData>& attachmentData) override;

    MyCharacterController* getCharacterController() { return &_characterController; }
    const MyCharacterController* getCharacterController() const { return &_characterController; }

    void updateMotors();
    void prepareForPhysicsSimulation(float deltaTime);
    void harvestResultsFromPhysicsSimulation(float deltaTime);

    const QString& getCollisionSoundURL() { return _collisionSoundURL; }
    void setCollisionSoundURL(const QString& url);

    SharedSoundPointer getCollisionSound();
    void setCollisionSound(SharedSoundPointer sound) { _collisionSound = sound; }

    void clearScriptableSettings();

    float getBoomLength() const { return _boomLength; }
    void setBoomLength(float boomLength) { _boomLength = boomLength; }

    float getPitchSpeed() const { return _pitchSpeed; }
    void setPitchSpeed(float speed) { _pitchSpeed = speed; }

    float getYawSpeed() const { return _yawSpeed; }
    void setYawSpeed(float speed) { _yawSpeed = speed; }

    static const float ZOOM_MIN;
    static const float ZOOM_MAX;
    static const float ZOOM_DEFAULT;

    void destroyAnimGraph();

    AudioListenerMode getAudioListenerMode() { return _audioListenerMode; }
    void setAudioListenerMode(AudioListenerMode audioListenerMode);
    glm::vec3 getCustomListenPosition() { return _customListenPosition; }
    void setCustomListenPosition(glm::vec3 customListenPosition) { _customListenPosition = customListenPosition; }
    glm::quat getCustomListenOrientation() { return _customListenOrientation; }
    void setCustomListenOrientation(glm::quat customListenOrientation) { _customListenOrientation = customListenOrientation; }

    virtual void rebuildCollisionShape() override;

    void setHandControllerPosesInSensorFrame(const controller::Pose& left, const controller::Pose& right);
    controller::Pose getLeftHandControllerPoseInSensorFrame() const;
    controller::Pose getRightHandControllerPoseInSensorFrame() const;
    controller::Pose getLeftHandControllerPoseInWorldFrame() const;
    controller::Pose getRightHandControllerPoseInWorldFrame() const;
    controller::Pose getLeftHandControllerPoseInAvatarFrame() const;
    controller::Pose getRightHandControllerPoseInAvatarFrame() const;

    bool hasDriveInput() const;

    Q_INVOKABLE void setAvatarCollisionsEnabled(bool enabled);
    Q_INVOKABLE bool getAvatarCollisionsEnabled();

    virtual glm::quat getAbsoluteJointRotationInObjectFrame(int index) const override;
    virtual glm::vec3 getAbsoluteJointTranslationInObjectFrame(int index) const override;

    glm::vec3 getPreActionVelocity() const;

    void addHoldAction(AvatarActionHold* holdAction);  // thread-safe
    void removeHoldAction(AvatarActionHold* holdAction);  // thread-safe
    void updateHoldActions(const AnimPose& prePhysicsPose, const AnimPose& postUpdatePose);

public slots:
    void increaseSize();
    void decreaseSize();
    void resetSize();

    void goToLocation(const glm::vec3& newPosition,
                      bool hasOrientation = false, const glm::quat& newOrientation = glm::quat(),
                      bool shouldFaceLocation = false);
    void goToLocation(const QVariant& properties);

    void restrictScaleFromDomainSettings(const QJsonObject& domainSettingsObject);
    void clearScaleRestriction();

    //  Set/Get update the thrust that will move the avatar around
    void addThrust(glm::vec3 newThrust) { _thrust += newThrust; };
    glm::vec3 getThrust() { return _thrust; };
    void setThrust(glm::vec3 newThrust) { _thrust = newThrust; }

    Q_INVOKABLE void updateMotionBehaviorFromMenu();

    void setEnableDebugDrawLeftFootTrace(bool isEnabled);
    void setEnableDebugDrawRightFootTrace(bool isEnabled);
    void setEnableDebugDrawDefaultPose(bool isEnabled);
    void setEnableDebugDrawAnimPose(bool isEnabled);
    void setEnableDebugDrawPosition(bool isEnabled);
    void setEnableDebugDrawHandControllers(bool isEnabled);
    void setEnableDebugDrawSensorToWorldMatrix(bool isEnabled);
    void setEnableDebugDrawIKTargets(bool isEnabled);
    bool getEnableMeshVisible() const { return _skeletonModel->isVisible(); }
    void setEnableMeshVisible(bool isEnabled);
    void setUseAnimPreAndPostRotations(bool isEnabled);
    void setEnableInverseKinematics(bool isEnabled);
    void setEnableVerticalComfortMode(bool isEnabled);

    QUrl getAnimGraphOverrideUrl() const;  // thread-safe
    void setAnimGraphOverrideUrl(QUrl value);  // thread-safe
    QUrl getAnimGraphUrl() const;  // thread-safe
    void setAnimGraphUrl(const QUrl& url);  // thread-safe

    glm::vec3 getPositionForAudio();
    glm::quat getOrientationForAudio();

    bool isOutOfBody() const;

signals:
    void audioListenerModeChanged();
    void transformChanged();
    void newCollisionSoundURL(const QUrl& url);
    void collisionWithEntity(const Collision& collision);
    void energyChanged(float newEnergy);
    void positionGoneTo();
    void onLoadComplete();

private:

    glm::vec3 getWorldBodyPosition() const;
    glm::quat getWorldBodyOrientation() const;
    QByteArray toByteArray(bool cullSmallChanges, bool sendAll) override;
    void simulate(float deltaTime);
    void updateFromTrackers(float deltaTime);
    virtual void render(RenderArgs* renderArgs, const glm::vec3& cameraPositio) override;
    virtual bool shouldRenderHead(const RenderArgs* renderArgs) const override;
    void setShouldRenderLocally(bool shouldRender) { _shouldRender = shouldRender; setEnableMeshVisible(shouldRender); }
    bool getShouldRenderLocally() const { return _shouldRender; }
    bool getDriveKeys(int key) { return _driveKeys[key] != 0.0f; };
    bool isMyAvatar() const override { return true; }
    virtual int parseDataFromBuffer(const QByteArray& buffer) override;
    virtual glm::vec3 getSkeletonPosition() const override;

    glm::vec3 getScriptedMotorVelocity() const { return _scriptedMotorVelocity; }
    float getScriptedMotorTimescale() const { return _scriptedMotorTimescale; }
    QString getScriptedMotorFrame() const;
    void setScriptedMotorVelocity(const glm::vec3& velocity);
    void setScriptedMotorTimescale(float timescale);
    void setScriptedMotorFrame(QString frame);
    virtual void attach(const QString& modelURL, const QString& jointName = QString(),
                        const glm::vec3& translation = glm::vec3(), const glm::quat& rotation = glm::quat(),
                        float scale = 1.0f, bool isSoft = false,
                        bool allowDuplicates = false, bool useSaved = true) override;

    bool cameraInsideHead() const;

    void updateEyeContactTarget(float deltaTime);

    // These are made private for MyAvatar so that you will use the "use" methods instead
    virtual void setSkeletonModelURL(const QUrl& skeletonModelURL) override;

    void setVisibleInSceneIfReady(Model* model, render::ScenePointer scene, bool visiblity);

    // derive avatar body position and orientation from the current HMD Sensor location.
    // results are in HMD frame
    glm::mat4 deriveBodyFromHMDSensor() const;

    virtual void updatePalms() override {}
    void lateUpdatePalms();

    void applyVelocityToSensorToWorldMatrix(const glm::vec3& velocity, float deltaTime);

    void clampTargetScaleToDomainLimits();
    void clampScaleChangeToDomainLimits(float desiredScale);
    glm::mat4 computeCameraRelativeHandControllerMatrix(const glm::mat4& controllerSensorMatrix) const;

    float _driveKeys[MAX_DRIVE_KEYS];
    bool _wasPushing;
    bool _isPushing;
    bool _isBeingPushed;
    bool _isBraking;

    float _boomLength;
    float _yawSpeed; // degrees/sec
    float _pitchSpeed; // degrees/sec

    glm::vec3 _thrust;  // impulse accumulator for outside sources

    glm::vec3 _actionMotorVelocity; // target local-frame velocity of avatar (default controller actions)
    glm::vec3 _scriptedMotorVelocity; // target local-frame velocity of avatar (analog script)
    float _scriptedMotorTimescale; // timescale for avatar to achieve its target velocity
    int _scriptedMotorFrame;
    quint32 _motionBehaviors;
    QString _collisionSoundURL;

    SharedSoundPointer _collisionSound;

    MyCharacterController _characterController;

    AvatarWeakPointer _lookAtTargetAvatar;
    glm::vec3 _targetAvatarPosition;
    bool _shouldRender;
    float _oculusYawOffset;

    eyeContactTarget _eyeContactTarget;
    float _eyeContactTargetTimer { 0.0f };

    glm::vec3 _trackedHeadPosition;

    Setting::Handle<float> _realWorldFieldOfView;

    // private methods
    void updateOrientation(float deltaTime);
    void updateActionMotor(float deltaTime);
    void updatePosition(float deltaTime);
    void updateCollisionSound(const glm::vec3& penetration, float deltaTime, float frequency);
    void initHeadBones();
    void initAnimGraph();

    // Avatar Preferences
    QUrl _fullAvatarURLFromPreferences;
    QString _fullAvatarModelName;
    ThreadSafeValueCache<QUrl> _currentAnimGraphUrl;
    ThreadSafeValueCache<QUrl> _prefOverrideAnimGraphUrl;
    QUrl _fstAnimGraphOverrideUrl;
    bool _useSnapTurn { true };
    bool _clearOverlayWhenMoving { true };

    // working copies -- see AvatarData for thread-safe _sensorToWorldMatrixCache, used for outward facing access
    glm::mat4 _sensorToWorldMatrix { glm::mat4() };

    // cache of the current HMD sensor position and orientation
    // in sensor space.
    glm::mat4 _hmdSensorMatrix;
    glm::quat _hmdSensorOrientation;
    glm::vec3 _hmdSensorPosition;
    glm::vec2 _hmdSensorFacing;  // facing vector in xz plane
    glm::vec2 _hmdSensorFacingMovingAverage { 0, 0 };   // facing vector in xz plane

    // cache of the current body position and orientation of the avatar's body,
    // in sensor space.
    glm::mat4 _bodySensorMatrix;

    struct FollowHelper {
        FollowHelper();

        enum FollowType {
            Rotation = 0,
            Horizontal,
            Vertical,
            NumFollowTypes
        };
        uint8_t _activeBits { 0 };
        bool _isOutOfBody { false };
        float _outOfBodyDistance { 0.0f };

        void deactivate();
        void deactivate(FollowType type);
        void activate();
        void activate(FollowType type);
        bool isActive() const;
        bool isActive(FollowType followType) const;
        void updateRotationActivation(const MyAvatar& myAvatar, const glm::mat4& desiredBodyMatrix, const glm::mat4& currentBodyMatrix);
        void updateHorizontalActivation(const MyAvatar& myAvatar, const glm::mat4& desiredBodyMatrix, const glm::mat4& currentBodyMatrix);
        void updateVerticalActivation(const MyAvatar& myAvatar, const glm::mat4& desiredBodyMatrix, const glm::mat4& currentBodyMatrix);
        glm::mat4 prePhysicsUpdate(const MyAvatar& myAvatar, const glm::mat4& desiredBodyMatrix, const glm::mat4& currentBodyMatrix, bool hasDriveInput, float deltaTime);
        void postPhysicsUpdate(MyAvatar& myAvatar);
    };
    FollowHelper _follow;

    bool _goToPending;
    glm::vec3 _goToPosition;
    glm::quat _goToOrientation;

    std::unordered_set<int> _headBoneSet;
    RigPointer _rig;
    bool _prevShouldDrawHead;

    bool _enableDebugDrawLeftFootTrace { false };
    bool _enableDebugDrawRightFootTrace { false };
    bool _enableDebugDrawDefaultPose { false };
    bool _enableDebugDrawAnimPose { false };
    bool _enableDebugDrawHandControllers { false };
    bool _enableDebugDrawSensorToWorldMatrix { false };
    bool _enableDebugDrawIKTargets { false };
    bool _enableVerticalComfortMode { false };

    AudioListenerMode _audioListenerMode;
    glm::vec3 _customListenPosition;
    glm::quat _customListenOrientation;

    AtRestDetector _hmdAtRestDetector;
    bool _lastIsMoving { false };
    bool _hoverReferenceCameraFacingIsCaptured { false };
    glm::vec3 _hoverReferenceCameraFacing { 0.0f, 0.0f, -1.0f }; // hmd sensor space

    // These are stored in SENSOR frame
    ThreadSafeValueCache<controller::Pose> _leftHandControllerPoseInSensorFrameCache { controller::Pose() };
    ThreadSafeValueCache<controller::Pose> _rightHandControllerPoseInSensorFrameCache { controller::Pose() };

    bool _hmdLeanRecenterEnabled = true;
    bool _moveKinematically { false }; // KINEMATIC_CONTROLLER_HACK
    float _canonicalScale { 1.0f };

    AnimPose _prePhysicsRoomPose;
    std::mutex _holdActionsMutex;
    std::vector<AvatarActionHold*> _holdActions;

    float AVATAR_MOVEMENT_ENERGY_CONSTANT { 0.001f };
    float AUDIO_ENERGY_CONSTANT { 0.000001f };
    float MAX_AVATAR_MOVEMENT_PER_FRAME { 30.0f };
    float currentEnergy { 0.0f };
    float energyChargeRate { 0.003f };
    glm::vec3 priorVelocity;
    glm::vec3 lastPosition;
    float getAudioEnergy();
    float getAccelerationEnergy();
    float getEnergy();
    void setEnergy(float value);
    bool didTeleport();

    struct DebugDrawVertex {
        DebugDrawVertex() : pos(), color() {}
        DebugDrawVertex(const glm::vec3& posIn, const glm::vec4& colorIn) : pos(posIn), color(colorIn) {}
        glm::vec3 pos;
        glm::vec4 color;
    };
    DebugDrawVertex _debugLineLoop[DEBUG_LINE_LOOP_SIZE];
    size_t _debugLineLoopIndex { 0 };

    bool _handControllerShow { false };
    float _handControllerShowTimer { 0.0f };
};

QScriptValue audioListenModeToScriptValue(QScriptEngine* engine, const AudioListenerMode& audioListenerMode);
void audioListenModeFromScriptValue(const QScriptValue& object, AudioListenerMode& audioListenerMode);

#endif // hifi_MyAvatar_h
