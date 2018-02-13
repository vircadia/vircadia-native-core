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

#include <bitset>

#include <glm/glm.hpp>

#include <QUuid>

#include <SettingHandle.h>
#include <Rig.h>
#include <Sound.h>
#include <ScriptEngine.h>

#include <controllers/Pose.h>
#include <controllers/Actions.h>
#include <AvatarConstants.h>
#include <avatars-renderer/Avatar.h>
#include <avatars-renderer/ScriptAvatar.h>

#include "AtRestDetector.h"
#include "MyCharacterController.h"
#include <ThreadSafeValueCache.h>

class AvatarActionHold;
class ModelItemID;
class MyHead;

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

class MyAvatar : public Avatar {
    Q_OBJECT

    /**jsdoc
     * Your avatar is your in-world representation of you. The MyAvatar API is used to manipulate the avatar.
     * For example, using the MyAvatar API you can customize the avatar's appearance, run custom avatar animations,
     * change the avatar's position within the domain, or manage the avatar's collisions with other objects.
     * NOTE: MyAvatar extends Avatar and AvatarData, see those namespace for more properties/methods.
     *
     * @namespace MyAvatar
     * @augments Avatar
     * @property qmlPosition {Vec3} Used as a stopgap for position access by QML, as glm::vec3 is unavailable outside of scripts
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
     * @property customListenOrientation {Quat} If MyAvatar.audioListenerMode == MyAvatar.audioListenerModeHead, then this determines the orientation
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
     * @property collisionsEnabled {bool} This can be used to disable collisions between the avatar and the world.
     * @property useAdvancedMovementControls {bool} Stores the user preference only, does not change user mappings, this is done in the defaultScript
     *   "scripts/system/controllers/toggleAdvancedMovementForHandControllers.js".
     * @property userHeight {number} The height of the user in sensor space. (meters).
     * @property userEyeHeight {number} Estimated height of the users eyes in sensor space. (meters)
     * @property SELF_ID {string} READ-ONLY. UUID representing "my avatar". Only use for local-only entities and overlays in situations where MyAvatar.sessionUUID is not available (e.g., if not connected to a domain).
     *   Note: Likely to be deprecated.
     * @property hmdRollControlEnabled {bool} When enabled the roll angle of your HMD will turn your avatar while flying.
     * @property hmdRollControlDeadZone {number} If hmdRollControlEnabled is true, this value can be used to tune what roll angle is required to begin turning.
     *   This angle is specified in degrees.
     * @property hmdRollControlRate {number} If hmdRollControlEnabled is true, this value determines the maximum turn rate of your avatar when rolling your HMD in degrees per second.
     */

    // FIXME: `glm::vec3 position` is not accessible from QML, so this exposes position in a QML-native type
    Q_PROPERTY(QVector3D qmlPosition READ getQmlPosition)
    QVector3D getQmlPosition() { auto p = getWorldPosition(); return QVector3D(p.x, p.y, p.z); }

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
    Q_PROPERTY(bool isAway READ getIsAway WRITE setAway)

    Q_PROPERTY(bool hmdLeanRecenterEnabled READ getHMDLeanRecenterEnabled WRITE setHMDLeanRecenterEnabled)
    Q_PROPERTY(bool collisionsEnabled READ getCollisionsEnabled WRITE setCollisionsEnabled)
    Q_PROPERTY(bool characterControllerEnabled READ getCharacterControllerEnabled WRITE setCharacterControllerEnabled)
    Q_PROPERTY(bool useAdvancedMovementControls READ useAdvancedMovementControls WRITE setUseAdvancedMovementControls)

    Q_PROPERTY(float yawSpeed MEMBER _yawSpeed)
    Q_PROPERTY(float pitchSpeed MEMBER _pitchSpeed)

    Q_PROPERTY(bool hmdRollControlEnabled READ getHMDRollControlEnabled WRITE setHMDRollControlEnabled)
    Q_PROPERTY(float hmdRollControlDeadZone READ getHMDRollControlDeadZone WRITE setHMDRollControlDeadZone)
    Q_PROPERTY(float hmdRollControlRate READ getHMDRollControlRate WRITE setHMDRollControlRate)

    Q_PROPERTY(float userHeight READ getUserHeight WRITE setUserHeight)
    Q_PROPERTY(float userEyeHeight READ getUserEyeHeight)

    Q_PROPERTY(QUuid SELF_ID READ getSelfID CONSTANT)

    Q_PROPERTY(float walkSpeed READ getWalkSpeed WRITE setWalkSpeed);

    const QString DOMINANT_LEFT_HAND = "left";
    const QString DOMINANT_RIGHT_HAND = "right";

public:
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
    Q_ENUM(DriveKeys)

    explicit MyAvatar(QThread* thread);
    ~MyAvatar();

    void instantiableAvatar() override {};
    void registerMetaTypes(ScriptEnginePointer engine);

    virtual void simulateAttachments(float deltaTime) override;

    AudioListenerMode getAudioListenerModeHead() const { return FROM_HEAD; }
    AudioListenerMode getAudioListenerModeCamera() const { return FROM_CAMERA; }
    AudioListenerMode getAudioListenerModeCustom() const { return CUSTOM; }

    void reset(bool andRecenter = false, bool andReload = true, bool andHead = true);

    Q_INVOKABLE void resetSensorsAndBody();

    /**jsdoc
     * Moves and orients the avatar, such that it is directly underneath the HMD, with toes pointed forward.
     * @function MyAvatar.centerBody
     */
    Q_INVOKABLE void centerBody(); // thread-safe


    /**jsdoc
     * The internal inverse-kinematics system maintains a record of which joints are "locked". Sometimes it is useful to forget this history, to prevent
     * contorted joints.
     * @function MyAvatar.clearIKJointLimitHistory
     */
    Q_INVOKABLE void clearIKJointLimitHistory(); // thread-safe

    void update(float deltaTime);
    virtual void postUpdate(float deltaTime, const render::ScenePointer& scene) override;
    void preDisplaySide(RenderArgs* renderArgs);

    const glm::mat4& getHMDSensorMatrix() const { return _hmdSensorMatrix; }
    const glm::vec3& getHMDSensorPosition() const { return _hmdSensorPosition; }
    const glm::quat& getHMDSensorOrientation() const { return _hmdSensorOrientation; }

    Q_INVOKABLE void setOrientationVar(const QVariant& newOrientationVar);
    Q_INVOKABLE QVariant getOrientationVar() const;

    // A method intended to be overriden by MyAvatar for polling orientation for network transmission.
    glm::quat getOrientationOutbound() const override;

    // Pass a recent sample of the HMD to the avatar.
    // This can also update the avatar's position to follow the HMD
    // as it moves through the world.
    void updateFromHMDSensorMatrix(const glm::mat4& hmdSensorMatrix);

    // read the location of a hand controller and save the transform
    void updateJointFromController(controller::Action poseKey, ThreadSafeValueCache<glm::mat4>& matrixCache);

    // best called at end of main loop, just before rendering.
    // update sensor to world matrix from current body position and hmd sensor.
    // This is so the correct camera can be used for rendering.
    void updateSensorToWorldMatrix();

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
    Q_INVOKABLE QScriptValue addAnimationStateHandler(QScriptValue handler, QScriptValue propertiesList) { return _skeletonModel->getRig().addAnimationStateHandler(handler, propertiesList); }
    // Removes a handler previously added by addAnimationStateHandler.
    Q_INVOKABLE void removeAnimationStateHandler(QScriptValue handler) { _skeletonModel->getRig().removeAnimationStateHandler(handler); }

    Q_INVOKABLE bool getSnapTurn() const { return _useSnapTurn; }
    Q_INVOKABLE void setSnapTurn(bool on) { _useSnapTurn = on; }
    Q_INVOKABLE bool getClearOverlayWhenMoving() const { return _clearOverlayWhenMoving; }
    Q_INVOKABLE void setClearOverlayWhenMoving(bool on) { _clearOverlayWhenMoving = on; }

    Q_INVOKABLE void setDominantHand(const QString& hand);
    Q_INVOKABLE QString getDominantHand() const { return _dominantHand; }

    Q_INVOKABLE void setHMDLeanRecenterEnabled(bool value) { _hmdLeanRecenterEnabled = value; }
    Q_INVOKABLE bool getHMDLeanRecenterEnabled() const { return _hmdLeanRecenterEnabled; }

    bool useAdvancedMovementControls() const { return _useAdvancedMovementControls.get(); }
    void setUseAdvancedMovementControls(bool useAdvancedMovementControls)
        { _useAdvancedMovementControls.set(useAdvancedMovementControls); }

    void setHMDRollControlEnabled(bool value) { _hmdRollControlEnabled = value; }
    bool getHMDRollControlEnabled() const { return _hmdRollControlEnabled; }
    void setHMDRollControlDeadZone(float value) { _hmdRollControlDeadZone = value; }
    float getHMDRollControlDeadZone() const { return _hmdRollControlDeadZone; }
    void setHMDRollControlRate(float value) { _hmdRollControlRate = value; }
    float getHMDRollControlRate() const { return _hmdRollControlRate; }

    // get/set avatar data
    void saveData();
    void loadData();

    void saveAttachmentData(const AttachmentData& attachment) const;
    AttachmentData loadAttachmentData(const QUrl& modelURL, const QString& jointName = QString()) const;

    //  Set what driving keys are being pressed to control thrust levels
    void clearDriveKeys();
    void setDriveKey(DriveKeys key, float val);
    float getDriveKey(DriveKeys key) const;
    Q_INVOKABLE float getRawDriveKey(DriveKeys key) const;
    void relayDriveKeysToCharacterController();
    
    Q_INVOKABLE void disableDriveKey(DriveKeys key);
    Q_INVOKABLE void enableDriveKey(DriveKeys key);
    Q_INVOKABLE bool isDriveKeyDisabled(DriveKeys key) const;

    eyeContactTarget getEyeContactTarget();

    const MyHead* getMyHead() const;
    Q_INVOKABLE glm::vec3 getHeadPosition() const { return getHead()->getPosition(); }
    Q_INVOKABLE float getHeadFinalYaw() const { return getHead()->getFinalYaw(); }
    Q_INVOKABLE float getHeadFinalRoll() const { return getHead()->getFinalRoll(); }
    Q_INVOKABLE float getHeadFinalPitch() const { return getHead()->getFinalPitch(); }
    Q_INVOKABLE float getHeadDeltaPitch() const { return getHead()->getDeltaPitch(); }

    Q_INVOKABLE glm::vec3 getEyePosition() const { return getHead()->getEyePosition(); }

    Q_INVOKABLE glm::vec3 getTargetAvatarPosition() const { return _targetAvatarPosition; }
    Q_INVOKABLE ScriptAvatarData* getTargetAvatar() const;

    Q_INVOKABLE glm::vec3 getLeftHandPosition() const;
    Q_INVOKABLE glm::vec3 getRightHandPosition() const;
    Q_INVOKABLE glm::vec3 getLeftHandTipPosition() const;
    Q_INVOKABLE glm::vec3 getRightHandTipPosition() const;

    Q_INVOKABLE controller::Pose getLeftHandPose() const;
    Q_INVOKABLE controller::Pose getRightHandPose() const;
    Q_INVOKABLE controller::Pose getLeftHandTipPose() const;
    Q_INVOKABLE controller::Pose getRightHandTipPose() const;

    // world-space to avatar-space rigconversion functions
    Q_INVOKABLE glm::vec3 worldToJointPoint(const glm::vec3& position, const int jointIndex = -1) const;
    Q_INVOKABLE glm::vec3 worldToJointDirection(const glm::vec3& direction, const int jointIndex = -1) const;
    Q_INVOKABLE glm::quat worldToJointRotation(const glm::quat& rotation, const int jointIndex = -1) const;

    Q_INVOKABLE glm::vec3 jointToWorldPoint(const glm::vec3& position, const int jointIndex = -1) const;
    Q_INVOKABLE glm::vec3 jointToWorldDirection(const glm::vec3& direction, const int jointIndex = -1) const;
    Q_INVOKABLE glm::quat jointToWorldRotation(const glm::quat& rotation, const int jointIndex = -1) const;

    AvatarWeakPointer getLookAtTargetAvatar() const { return _lookAtTargetAvatar; }
    void updateLookAtTargetAvatar();
    void clearLookAtTargetAvatar();

    virtual void setJointRotations(const QVector<glm::quat>& jointRotations) override;
    virtual void setJointData(int index, const glm::quat& rotation, const glm::vec3& translation) override;
    virtual void setJointRotation(int index, const glm::quat& rotation) override;
    virtual void setJointTranslation(int index, const glm::vec3& translation) override;
    virtual void clearJointData(int index) override;

    virtual void setJointData(const QString& name, const glm::quat& rotation, const glm::vec3& translation)  override;
    virtual void setJointRotation(const QString& name, const glm::quat& rotation)  override;
    virtual void setJointTranslation(const QString& name, const glm::vec3& translation)  override;
    virtual void clearJointData(const QString& name) override;
    virtual void clearJointsData() override;

    Q_INVOKABLE bool pinJoint(int index, const glm::vec3& position, const glm::quat& orientation);
    bool isJointPinned(int index);
    Q_INVOKABLE bool clearPinOnJoint(int index);

    Q_INVOKABLE float getIKErrorOnLastSolve() const;

    Q_INVOKABLE void useFullAvatarURL(const QUrl& fullAvatarURL, const QString& modelName = QString());
    Q_INVOKABLE QUrl getFullAvatarURLFromPreferences() const { return _fullAvatarURLFromPreferences; }
    Q_INVOKABLE QString getFullAvatarModelName() const { return _fullAvatarModelName; }
    void resetFullAvatarURL();

    virtual void setAttachmentData(const QVector<AttachmentData>& attachmentData) override;

    MyCharacterController* getCharacterController() { return &_characterController; }
    const MyCharacterController* getCharacterController() const { return &_characterController; }

    void updateMotors();
    void prepareForPhysicsSimulation();
    void nextAttitude(glm::vec3 position, glm::quat orientation); // Can be safely called at any time.
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

    const glm::vec2& getHeadControllerFacingMovingAverage() const { return _headControllerFacingMovingAverage; }

    void setControllerPoseInSensorFrame(controller::Action action, const controller::Pose& pose);
    controller::Pose getControllerPoseInSensorFrame(controller::Action action) const;
    controller::Pose getControllerPoseInWorldFrame(controller::Action action) const;
    controller::Pose getControllerPoseInAvatarFrame(controller::Action action) const;

    bool hasDriveInput() const;

    QVariantList getAvatarEntitiesVariant();
    void removeAvatarEntities();

    Q_INVOKABLE bool isFlying();
    Q_INVOKABLE bool isInAir();
    Q_INVOKABLE void setFlyingEnabled(bool enabled);
    Q_INVOKABLE bool getFlyingEnabled();

    Q_INVOKABLE float getAvatarScale();
    Q_INVOKABLE void setAvatarScale(float scale);

    Q_INVOKABLE void setCollisionsEnabled(bool enabled);
    Q_INVOKABLE bool getCollisionsEnabled();
    Q_INVOKABLE void setCharacterControllerEnabled(bool enabled); // deprecated
    Q_INVOKABLE bool getCharacterControllerEnabled(); // deprecated

    virtual glm::quat getAbsoluteJointRotationInObjectFrame(int index) const override;
    virtual glm::vec3 getAbsoluteJointTranslationInObjectFrame(int index) const override;

    // all calibration matrices are in absolute avatar space.
    glm::mat4 getCenterEyeCalibrationMat() const;
    glm::mat4 getHeadCalibrationMat() const;
    glm::mat4 getSpine2CalibrationMat() const;
    glm::mat4 getHipsCalibrationMat() const;
    glm::mat4 getLeftFootCalibrationMat() const;
    glm::mat4 getRightFootCalibrationMat() const;
    glm::mat4 getRightArmCalibrationMat() const;
    glm::mat4 getLeftArmCalibrationMat() const;
    glm::mat4 getLeftHandCalibrationMat() const;
    glm::mat4 getRightHandCalibrationMat() const;

    void addHoldAction(AvatarActionHold* holdAction);  // thread-safe
    void removeHoldAction(AvatarActionHold* holdAction);  // thread-safe
    void updateHoldActions(const AnimPose& prePhysicsPose, const AnimPose& postUpdatePose);

    // derive avatar body position and orientation from the current HMD Sensor location.
    // results are in HMD frame
    glm::mat4 deriveBodyFromHMDSensor() const;

    Q_INVOKABLE bool isUp(const glm::vec3& direction) { return glm::dot(direction, _worldUpDirection) > 0.0f; }; // true iff direction points up wrt avatar's definition of up.
    Q_INVOKABLE bool isDown(const glm::vec3& direction) { return glm::dot(direction, _worldUpDirection) < 0.0f; };

    void setUserHeight(float value);
    float getUserHeight() const;
    float getUserEyeHeight() const;

    virtual SpatialParentTree* getParentTree() const override;

    const QUuid& getSelfID() const { return AVATAR_SELF_ID; }

    void setWalkSpeed(float value);
    float getWalkSpeed() const;

public slots:
    void increaseSize();
    void decreaseSize();
    void resetSize();

    void setGravity(float gravity);
    float getGravity();

    void goToLocation(const glm::vec3& newPosition,
                      bool hasOrientation = false, const glm::quat& newOrientation = glm::quat(),
                      bool shouldFaceLocation = false);
    void goToLocation(const QVariant& properties);
    void goToLocationAndEnableCollisions(const glm::vec3& newPosition);
    bool safeLanding(const glm::vec3& position);

    void restrictScaleFromDomainSettings(const QJsonObject& domainSettingsObject);
    void clearScaleRestriction();

    //  Set/Get update the thrust that will move the avatar around
    void addThrust(glm::vec3 newThrust) { _thrust += newThrust; };
    glm::vec3 getThrust() { return _thrust; };
    void setThrust(glm::vec3 newThrust) { _thrust = newThrust; }

    Q_INVOKABLE void updateMotionBehaviorFromMenu();

    void setEnableDebugDrawDefaultPose(bool isEnabled);
    void setEnableDebugDrawAnimPose(bool isEnabled);
    void setEnableDebugDrawPosition(bool isEnabled);
    void setEnableDebugDrawHandControllers(bool isEnabled);
    void setEnableDebugDrawSensorToWorldMatrix(bool isEnabled);
    void setEnableDebugDrawIKTargets(bool isEnabled);
    void setEnableDebugDrawIKConstraints(bool isEnabled);
    void setEnableDebugDrawIKChains(bool isEnabled);
    void setEnableDebugDrawDetailedCollision(bool isEnabled);

    bool getEnableMeshVisible() const { return _skeletonModel->isVisible(); }
    void setEnableMeshVisible(bool isEnabled);
    void setEnableInverseKinematics(bool isEnabled);

    QUrl getAnimGraphOverrideUrl() const;  // thread-safe
    void setAnimGraphOverrideUrl(QUrl value);  // thread-safe
    QUrl getAnimGraphUrl() const;  // thread-safe
    void setAnimGraphUrl(const QUrl& url);  // thread-safe

    glm::vec3 getPositionForAudio();
    glm::quat getOrientationForAudio();

    virtual void setModelScale(float scale) override;

signals:
    void audioListenerModeChanged();
    void transformChanged();
    void newCollisionSoundURL(const QUrl& url);
    void collisionWithEntity(const Collision& collision);
    void energyChanged(float newEnergy);
    void positionGoneTo();
    void onLoadComplete();
    void wentAway();
    void wentActive();
    void skeletonChanged();
    void dominantHandChanged(const QString& hand);
    void sensorToWorldScaleChanged(float sensorToWorldScale);
    void attachmentsChanged();
    void scaleChanged();

private slots:
    void leaveDomain();


protected:
    virtual void beParentOfChild(SpatiallyNestablePointer newChild) const override;
    virtual void forgetChild(SpatiallyNestablePointer newChild) const override;

private:

    bool requiresSafeLanding(const glm::vec3& positionIn, glm::vec3& positionOut);

    virtual QByteArray toByteArrayStateful(AvatarDataDetail dataDetail, bool dropFaceTracking) override;

    void simulate(float deltaTime);
    void updateFromTrackers(float deltaTime);
    void saveAvatarUrl();
    virtual void render(RenderArgs* renderArgs) override;
    virtual bool shouldRenderHead(const RenderArgs* renderArgs) const override;
    void setShouldRenderLocally(bool shouldRender) { _shouldRender = shouldRender; setEnableMeshVisible(shouldRender); }
    bool getShouldRenderLocally() const { return _shouldRender; }
    bool isMyAvatar() const override { return true; }
    virtual int parseDataFromBuffer(const QByteArray& buffer) override;
    virtual glm::vec3 getSkeletonPosition() const override;

    void saveAvatarScale();

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

    bool cameraInsideHead(const glm::vec3& cameraPosition) const;

    void updateEyeContactTarget(float deltaTime);

    // These are made private for MyAvatar so that you will use the "use" methods instead
    virtual void setSkeletonModelURL(const QUrl& skeletonModelURL) override;

    void setVisibleInSceneIfReady(Model* model, const render::ScenePointer& scene, bool visiblity);

    virtual void updatePalms() override {}
    void lateUpdatePalms();

    void clampTargetScaleToDomainLimits();
    void clampScaleChangeToDomainLimits(float desiredScale);
    glm::mat4 computeCameraRelativeHandControllerMatrix(const glm::mat4& controllerSensorMatrix) const;

    std::array<float, MAX_DRIVE_KEYS> _driveKeys;
    std::bitset<MAX_DRIVE_KEYS> _disabledDriveKeys;

    bool _enableFlying { true };
    bool _wasPushing { false };
    bool _isPushing { false };
    bool _isBeingPushed { false };
    bool _isBraking { false };
    bool _isAway { false };

    float _boomLength { ZOOM_DEFAULT };
    float _yawSpeed; // degrees/sec
    float _pitchSpeed; // degrees/sec

    glm::vec3 _thrust { 0.0f };  // impulse accumulator for outside sources

    glm::vec3 _actionMotorVelocity; // target local-frame velocity of avatar (default controller actions)
    glm::vec3 _scriptedMotorVelocity; // target local-frame velocity of avatar (analog script)
    float _scriptedMotorTimescale; // timescale for avatar to achieve its target velocity
    int _scriptedMotorFrame;
    quint32 _motionBehaviors;
    QString _collisionSoundURL;

    SharedSoundPointer _collisionSound;

    MyCharacterController _characterController;
    int16_t _previousCollisionGroup { BULLET_COLLISION_GROUP_MY_AVATAR };

    AvatarWeakPointer _lookAtTargetAvatar;
    glm::vec3 _targetAvatarPosition;
    bool _shouldRender { true };
    float _oculusYawOffset;

    eyeContactTarget _eyeContactTarget;
    float _eyeContactTargetTimer { 0.0f };

    glm::vec3 _trackedHeadPosition;

    Setting::Handle<float> _realWorldFieldOfView;
    Setting::Handle<bool> _useAdvancedMovementControls;

    // Smoothing.
    const float SMOOTH_TIME_ORIENTATION = 0.5f;

    // Smoothing data for blending from one position/orientation to another on remote agents.
    float _smoothOrientationTimer;
    glm::quat _smoothOrientationInitial;
    glm::quat _smoothOrientationTarget;

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
    QString _dominantHand { DOMINANT_RIGHT_HAND };

    const float ROLL_CONTROL_DEAD_ZONE_DEFAULT = 8.0f; // degrees
    const float ROLL_CONTROL_RATE_DEFAULT = 114.0f; // degrees / sec

    bool _hmdRollControlEnabled { true };
    float _hmdRollControlDeadZone { ROLL_CONTROL_DEAD_ZONE_DEFAULT };
    float _hmdRollControlRate { ROLL_CONTROL_RATE_DEFAULT };

    // working copy -- see AvatarData for thread-safe _sensorToWorldMatrixCache, used for outward facing access
    glm::mat4 _sensorToWorldMatrix { glm::mat4() };

    // cache of the current HMD sensor position and orientation in sensor space.
    glm::mat4 _hmdSensorMatrix;
    glm::quat _hmdSensorOrientation;
    glm::vec3 _hmdSensorPosition;
    // cache head controller pose in sensor space
    glm::vec2 _headControllerFacing;  // facing vector in xz plane
    glm::vec2 _headControllerFacingMovingAverage { 0, 0 };   // facing vector in xz plane

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
        float _timeRemaining[NumFollowTypes];

        void deactivate();
        void deactivate(FollowType type);
        void activate();
        void activate(FollowType type);
        bool isActive() const;
        bool isActive(FollowType followType) const;
        float getMaxTimeRemaining() const;
        void decrementTimeRemaining(float dt);
        bool shouldActivateRotation(const MyAvatar& myAvatar, const glm::mat4& desiredBodyMatrix, const glm::mat4& currentBodyMatrix) const;
        bool shouldActivateVertical(const MyAvatar& myAvatar, const glm::mat4& desiredBodyMatrix, const glm::mat4& currentBodyMatrix) const;
        bool shouldActivateHorizontal(const MyAvatar& myAvatar, const glm::mat4& desiredBodyMatrix, const glm::mat4& currentBodyMatrix) const;
        void prePhysicsUpdate(MyAvatar& myAvatar, const glm::mat4& bodySensorMatrix, const glm::mat4& currentBodyMatrix, bool hasDriveInput);
        glm::mat4 postPhysicsUpdate(const MyAvatar& myAvatar, const glm::mat4& currentBodyMatrix);
    };
    FollowHelper _follow;

    bool _goToPending { false };
    bool _physicsSafetyPending { false };
    glm::vec3 _goToPosition;
    glm::quat _goToOrientation;

    std::unordered_set<int> _headBoneSet;
    bool _prevShouldDrawHead;
    bool _rigEnabled { true };

    bool _enableDebugDrawDefaultPose { false };
    bool _enableDebugDrawAnimPose { false };
    bool _enableDebugDrawHandControllers { false };
    bool _enableDebugDrawSensorToWorldMatrix { false };
    bool _enableDebugDrawIKTargets { false };
    bool _enableDebugDrawIKConstraints { false };
    bool _enableDebugDrawIKChains { false };
    bool _enableDebugDrawDetailedCollision { false };

    mutable bool _cauterizationNeedsUpdate; // do we need to scan children and update their "cauterized" state?

    AudioListenerMode _audioListenerMode;
    glm::vec3 _customListenPosition;
    glm::quat _customListenOrientation;

    AtRestDetector _hmdAtRestDetector;
    bool _lastIsMoving { false };

    // all poses are in sensor-frame
    std::map<controller::Action, controller::Pose> _controllerPoseMap;
    mutable std::mutex _controllerPoseMapMutex;

    bool _hmdLeanRecenterEnabled = true;
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
    bool getIsAway() const { return _isAway; }
    void setAway(bool value);

    std::mutex _pinnedJointsMutex;
    std::vector<int> _pinnedJoints;

    // height of user in sensor space, when standing erect.
    ThreadSafeValueCache<float> _userHeight { DEFAULT_AVATAR_HEIGHT };

    void updateChildCauterization(SpatiallyNestablePointer object);

    // max unscaled forward movement speed
    ThreadSafeValueCache<float> _walkSpeed { DEFAULT_AVATAR_MAX_WALKING_SPEED };
};

QScriptValue audioListenModeToScriptValue(QScriptEngine* engine, const AudioListenerMode& audioListenerMode);
void audioListenModeFromScriptValue(const QScriptValue& object, AudioListenerMode& audioListenerMode);

QScriptValue driveKeysToScriptValue(QScriptEngine* engine, const MyAvatar::DriveKeys& driveKeys);
void driveKeysFromScriptValue(const QScriptValue& object, MyAvatar::DriveKeys& driveKeys);

#endif // hifi_MyAvatar_h
