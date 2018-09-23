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

#include <AvatarConstants.h>
#include <avatars-renderer/Avatar.h>
#include <avatars-renderer/ScriptAvatar.h>
#include <ClientTraitsHandler.h>
#include <controllers/Pose.h>
#include <controllers/Actions.h>
#include <EntityItem.h>
#include <ThreadSafeValueCache.h>
#include <Rig.h>
#include <ScriptEngine.h>
#include <SettingHandle.h>
#include <Sound.h>

#include "AtRestDetector.h"
#include "MyCharacterController.h"
#include "RingBufferHistory.h"

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
     * Your avatar is your in-world representation of you. The <code>MyAvatar</code> API is used to manipulate the avatar.
     * For example, you can customize the avatar's appearance, run custom avatar animations,
     * change the avatar's position within the domain, or manage the avatar's collisions with other objects.
     *
     * @namespace MyAvatar
     *
     * @hifi-interface
     * @hifi-client-entity
     *
     * @property {Vec3} qmlPosition - A synonym for <code>position</code> for use by QML.
     * @property {boolean} shouldRenderLocally=true - If <code>true</code> then your avatar is rendered for you in Interface,
     *     otherwise it is not rendered for you (but it is still rendered for other users).
     * @property {Vec3} motorVelocity=Vec3.ZERO - The target velocity of your avatar to be achieved by a scripted motor.
     * @property {number} motorTimescale=1000000 - The timescale for the scripted motor to achieve the target 
     *     <code>motorVelocity</code> avatar velocity. Smaller values result in higher acceleration.
     * @property {string} motorReferenceFrame="camera" -  Reference frame of the <code>motorVelocity</code>. Must be one of the 
     *     following: <code>"camera"</code>, <code>"avatar"</code>, and <code>"world"</code>.
     * @property {string} motorMode="simple" - The Type of scripted motor behavior: <code>"simple"</code> to use the 
     *     <code>motorTimescale</code> time scale; <code>"dynamic"</code> to use character controller timescales.
     * @property {string} collisionSoundURL="Body_Hits_Impact.wav" - The sound that's played when the avatar experiences a 
     *     collision. It can be a mono or stereo 16-bit WAV file running at either 24kHz or 48kHz. The latter is down-sampled 
     *     by the audio mixer, so all audio effectively plays back at a 24khz. 48kHz RAW files are also supported.
     * @property {number} audioListenerMode=0 - Specifies the listening position when hearing spatialized audio. Must be one 
     *     of the following property values:<br />
     *     <code>audioListenerModeHead</code><br />
     *     <code>audioListenerModeCamera</code><br />
     *     <code>audioListenerModeCustom</code>
     * @property {number} audioListenerModeHead=0 - The audio listening position is at the avatar's head. <em>Read-only.</em>
     * @property {number} audioListenerModeCamera=1 - The audio listening position is at the camera. <em>Read-only.</em>
     * @property {number} audioListenerModeCustom=2 - The audio listening position is at a the position specified by set by the 
     *     <code>customListenPosition</code> and <code>customListenOrientation</code> property values. <em>Read-only.</em>
     * @property {boolean} hasScriptedBlendshapes=false - Blendshapes will be transmitted over the network if set to true.
     * @property {boolean} hasProceduralBlinkFaceMovement=true - procedural blinking will be turned on if set to true.
     * @property {boolean} hasProceduralEyeFaceMovement=true - procedural eye movement will be turned on if set to true.
     * @property {boolean} hasAudioEnabledFaceMovement=true - If set to true, voice audio will move the mouth Blendshapes while MyAvatar.hasScriptedBlendshapes is enabled.
     * @property {Vec3} customListenPosition=Vec3.ZERO - The listening position used when the <code>audioListenerMode</code>
     *     property value is <code>audioListenerModeCustom</code>.
     * @property {Quat} customListenOrientation=Quat.IDENTITY - The listening orientation used when the 
     *     <code>audioListenerMode</code> property value is <code>audioListenerModeCustom</code>.
     * @property {Vec3} leftHandPosition - The position of the left hand in avatar coordinates if it's being positioned by 
     *     controllers, otherwise {@link Vec3(0)|Vec3.ZERO}. <em>Read-only.</em>
     * @property {Vec3} rightHandPosition - The position of the right hand in avatar coordinates if it's being positioned by
     *     controllers, otherwise {@link Vec3(0)|Vec3.ZERO}. <em>Read-only.</em>

     * @property {Vec3} leftHandTipPosition - The position 30cm offset from the left hand in avatar coordinates if it's being 
     *     positioned by controllers, otherwise {@link Vec3(0)|Vec3.ZERO}. <em>Read-only.</em>
     * @property {Vec3} rightHandTipPosition - The position 30cm offset from the right hand in avatar coordinates if it's being
     *     positioned by controllers, otherwise {@link Vec3(0)|Vec3.ZERO}. <em>Read-only.</em>
     * @property {Pose} leftHandPose - The pose of the left hand as determined by the hand controllers. <em>Read-only.</em>
     * @property {Pose} rightHandPose - The pose right hand position as determined by the hand controllers. <em>Read-only.</em>
     * @property {Pose} leftHandTipPose - The pose of the left hand as determined by the hand controllers, with the position 
     *     by 30cm. <em>Read-only.</em>
     * @property {Pose} rightHandTipPose - The pose of the right hand as determined by the hand controllers, with the position
     *     by 30cm. <em>Read-only.</em>
     * @property {boolean} centerOfGravityModelEnabled=true - If <code>true</code> then the avatar hips are placed according to the center of
     *     gravity model that balance the center of gravity over the base of support of the feet.  Setting the value <code>false</code> 
     *     will result in the default behaviour where the hips are placed under the head.
     * @property {boolean} hmdLeanRecenterEnabled=true - If <code>true</code> then the avatar is re-centered to be under the 
     *     head's position. In room-scale VR, this behavior is what causes your avatar to follow your HMD as you walk around 
     *     the room. Setting the value <code>false</code> is useful if you want to pin the avatar to a fixed position.
     * @property {boolean} collisionsEnabled - Set to <code>true</code> to enable collisions for the avatar, <code>false</code> 
     *     to disable collisions. May return <code>true</code> even though the value was set <code>false</code> because the 
     *     zone may disallow collisionless avatars.
     * @property {boolean} characterControllerEnabled - Synonym of <code>collisionsEnabled</code>. 
     *     <strong>Deprecated:</strong> Use <code>collisionsEnabled</code> instead.
     * @property {boolean} useAdvancedMovementControls - Returns the value of the Interface setting, Settings > Advanced 
     *     Movement for Hand Controller. Note: Setting the value has no effect unless Interface is restarted.
     * @property {number} yawSpeed=75
     * @property {number} pitchSpeed=50
     * @property {boolean} hmdRollControlEnabled=true - If <code>true</code>, the roll angle of your HMD turns your avatar 
     *     while flying.
     * @property {number} hmdRollControlDeadZone=8 - The amount of HMD roll, in degrees, required before your avatar turns if 
     *    <code>hmdRollControlEnabled</code> is enabled.
     * @property {number} hmdRollControlRate If hmdRollControlEnabled is true, this value determines the maximum turn rate of
     *     your avatar when rolling your HMD in degrees per second.
     * @property {number} userHeight=1.75 - The height of the user in sensor space.
     * @property {number} userEyeHeight=1.65 - The estimated height of the user's eyes in sensor space. <em>Read-only.</em>
     * @property {Uuid} SELF_ID - UUID representing "my avatar". Only use for local-only entities and overlays in situations 
     *     where MyAvatar.sessionUUID is not available (e.g., if not connected to a domain). Note: Likely to be deprecated. 
     *     <em>Read-only.</em>
     * @property {number} walkSpeed
     * @property {number} walkBackwardSpeed
     * @property {number} sprintSpeed
     *
     * @property {Vec3} skeletonOffset - Can be used to apply a translation offset between the avatar's position and the
     *     registration point of the 3D model.
     *
     * @property {Vec3} position 
     * @property {number} scale - Returns the clamped scale of the avatar.
     * @property {number} density <em>Read-only.</em>
     * @property {Vec3} handPosition 
     * @property {number} bodyYaw - The rotation left or right about an axis running from the head to the feet of the avatar. 
     *     Yaw is sometimes called "heading".
     * @property {number} bodyPitch - The rotation about an axis running from shoulder to shoulder of the avatar. Pitch is 
     *     sometimes called "elevation".
     * @property {number} bodyRoll - The rotation about an axis running from the chest to the back of the avatar. Roll is 
     *     sometimes called "bank".
     * @property {Quat} orientation 
     * @property {Quat} headOrientation - The orientation of the avatar's head.
     * @property {number} headPitch - The rotation about an axis running from ear to ear of the avatar's head. Pitch is 
     *     sometimes called "elevation".
     * @property {number} headYaw - The rotation left or right about an axis running from the base to the crown of the avatar's 
     *     head. Yaw is sometimes called "heading".
     * @property {number} headRoll - The rotation about an axis running from the nose to the back of the avatar's head. Roll is 
     *     sometimes called "bank".
     * @property {Vec3} velocity 
     * @property {Vec3} angularVelocity 
     * @property {number} audioLoudness 
     * @property {number} audioAverageLoudness 
     * @property {string} displayName 
     * @property {string} sessionDisplayName - Sanitized, defaulted version displayName that is defined by the AvatarMixer 
     *     rather than by Interface clients. The result is unique among all avatars present at the time.
     * @property {boolean} lookAtSnappingEnabled
     * @property {string} skeletonModelURL 
     * @property {AttachmentData[]} attachmentData
     * @property {string[]} jointNames - The list of joints in the current avatar model. <em>Read-only.</em>
     * @property {Uuid} sessionUUID <em>Read-only.</em>
     * @property {Mat4} sensorToWorldMatrix <em>Read-only.</em>
     * @property {Mat4} controllerLeftHandMatrix <em>Read-only.</em>
     * @property {Mat4} controllerRightHandMatrix <em>Read-only.</em>
     * @property {number} sensorToWorldScale <em>Read-only.</em>
     */
    // FIXME: `glm::vec3 position` is not accessible from QML, so this exposes position in a QML-native type
    Q_PROPERTY(QVector3D qmlPosition READ getQmlPosition)
    QVector3D getQmlPosition() { auto p = getWorldPosition(); return QVector3D(p.x, p.y, p.z); }

    Q_PROPERTY(bool shouldRenderLocally READ getShouldRenderLocally WRITE setShouldRenderLocally)
    Q_PROPERTY(glm::vec3 motorVelocity READ getScriptedMotorVelocity WRITE setScriptedMotorVelocity)
    Q_PROPERTY(float motorTimescale READ getScriptedMotorTimescale WRITE setScriptedMotorTimescale)
    Q_PROPERTY(QString motorReferenceFrame READ getScriptedMotorFrame WRITE setScriptedMotorFrame)
    Q_PROPERTY(QString motorMode READ getScriptedMotorMode WRITE setScriptedMotorMode)
    Q_PROPERTY(QString collisionSoundURL READ getCollisionSoundURL WRITE setCollisionSoundURL)
    Q_PROPERTY(AudioListenerMode audioListenerMode READ getAudioListenerMode WRITE setAudioListenerMode)
    Q_PROPERTY(glm::vec3 customListenPosition READ getCustomListenPosition WRITE setCustomListenPosition)
    Q_PROPERTY(glm::quat customListenOrientation READ getCustomListenOrientation WRITE setCustomListenOrientation)
    Q_PROPERTY(AudioListenerMode audioListenerModeHead READ getAudioListenerModeHead)
    Q_PROPERTY(AudioListenerMode audioListenerModeCamera READ getAudioListenerModeCamera)
    Q_PROPERTY(AudioListenerMode audioListenerModeCustom READ getAudioListenerModeCustom)
    Q_PROPERTY(bool hasScriptedBlendshapes READ getHasScriptedBlendshapes WRITE setHasScriptedBlendshapes)
    Q_PROPERTY(bool hasProceduralBlinkFaceMovement READ getHasProceduralBlinkFaceMovement WRITE setHasProceduralBlinkFaceMovement)
    Q_PROPERTY(bool hasProceduralEyeFaceMovement READ getHasProceduralEyeFaceMovement WRITE setHasProceduralEyeFaceMovement)
    Q_PROPERTY(bool hasAudioEnabledFaceMovement READ getHasAudioEnabledFaceMovement WRITE setHasAudioEnabledFaceMovement)
    Q_PROPERTY(float rotationRecenterFilterLength READ getRotationRecenterFilterLength WRITE setRotationRecenterFilterLength)
    Q_PROPERTY(float rotationThreshold READ getRotationThreshold WRITE setRotationThreshold)
    Q_PROPERTY(bool enableStepResetRotation READ getEnableStepResetRotation WRITE setEnableStepResetRotation)
    Q_PROPERTY(bool enableDrawAverageFacing READ getEnableDrawAverageFacing WRITE setEnableDrawAverageFacing)
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

    Q_PROPERTY(bool centerOfGravityModelEnabled READ getCenterOfGravityModelEnabled WRITE setCenterOfGravityModelEnabled)
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
    Q_PROPERTY(float walkBackwardSpeed READ getWalkBackwardSpeed WRITE setWalkBackwardSpeed);
    Q_PROPERTY(float sprintSpeed READ getSprintSpeed WRITE setSprintSpeed);

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
    virtual ~MyAvatar();

    void instantiableAvatar() override {};
    void registerMetaTypes(ScriptEnginePointer engine);

    virtual void simulateAttachments(float deltaTime) override;

    AudioListenerMode getAudioListenerModeHead() const { return FROM_HEAD; }
    AudioListenerMode getAudioListenerModeCamera() const { return FROM_CAMERA; }
    AudioListenerMode getAudioListenerModeCustom() const { return CUSTOM; }

    void reset(bool andRecenter = false, bool andReload = true, bool andHead = true);

    /**jsdoc
     * @function MyAvatar.resetSensorsAndBody
     */
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
    void preDisplaySide(const RenderArgs* renderArgs);

    const glm::mat4& getHMDSensorMatrix() const { return _hmdSensorMatrix; }
    const glm::vec3& getHMDSensorPosition() const { return _hmdSensorPosition; }
    const glm::quat& getHMDSensorOrientation() const { return _hmdSensorOrientation; }

    /**jsdoc
     * @function MyAvatar.setOrientationVar
     * @param {object} newOrientationVar
     */
    Q_INVOKABLE void setOrientationVar(const QVariant& newOrientationVar);

    /**jsdoc
     * @function MyAvatar.getOrientationVar
     * @returns {object} 
     */
    Q_INVOKABLE QVariant getOrientationVar() const;

    // A method intended to be overriden by MyAvatar for polling orientation for network transmission.
    glm::quat getOrientationOutbound() const override;

    // Pass a recent sample of the HMD to the avatar.
    // This can also update the avatar's position to follow the HMD
    // as it moves through the world.
    void updateFromHMDSensorMatrix(const glm::mat4& hmdSensorMatrix);

    // compute the hip to hand average azimuth.
    glm::vec2 computeHandAzimuth() const;

    // read the location of a hand controller and save the transform
    void updateJointFromController(controller::Action poseKey, ThreadSafeValueCache<glm::mat4>& matrixCache);

    // best called at end of main loop, just before rendering.
    // update sensor to world matrix from current body position and hmd sensor.
    // This is so the correct camera can be used for rendering.
    void updateSensorToWorldMatrix();

    void setRealWorldFieldOfView(float realWorldFov) { _realWorldFieldOfView.set(realWorldFov); }

    /**jsdoc
     * Get the position in world coordinates of the point directly between your avatar's eyes assuming your avatar was in its
     * default pose. This is a reference position; it does not change as your avatar's head moves relative to the avatar 
     * position.
     * @function MyAvatar.getDefaultEyePosition
     * @returns {Vec3} Default position between your avatar's eyes in world coordinates.
     * @example <caption>Report your avatar's default eye position.</caption>
     * var defaultEyePosition = MyAvatar.getDefaultEyePosition();
     * print(JSON.stringify(defaultEyePosition));
     */
    Q_INVOKABLE glm::vec3 getDefaultEyePosition() const;

    float getRealWorldFieldOfView() { return _realWorldFieldOfView.get(); }

    /**jsdoc
     * The avatar animation system includes a set of default animations along with rules for how those animations are blended
     * together with procedural data (such as look at vectors, hand sensors etc.). overrideAnimation() is used to completely
     * override all motion from the default animation system (including inverse kinematics for hand and head controllers) and
     * play a set of specified animations. To end these animations and restore the default animations, use 
     * {@link MyAvatar.restoreAnimation}.<br />
     * <p>Note: When using pre-built animation data, it's critical that the joint orientation of the source animation and target 
     * rig are equivalent, since the animation data applies absolute values onto the joints. If the orientations are different, 
     * the avatar will move in unpredictable ways. For more information about avatar joint orientation standards, see 
     * <a href="https://docs.highfidelity.com/create-and-explore/avatars/avatar-standards">Avatar Standards</a>.</p>
     * @function MyAvatar.overrideAnimation
     * @param url {string} The URL to the animation file. Animation files need to be .FBX format, but only need to contain the 
     * avatar skeleton and animation data.
     * @param fps {number} The frames per second (FPS) rate for the animation playback. 30 FPS is normal speed.
     * @param loop {boolean} Set to true if the animation should loop.
     * @param firstFrame {number} The frame the animation should start at.
     * @param lastFrame {number} The frame the animation should end at.
     * @example <caption> Play a clapping animation on your avatar for three seconds. </caption>
     * // Clap your hands for 3 seconds then restore animation back to the avatar.
     * var ANIM_URL = "https://s3.amazonaws.com/hifi-public/animations/ClapAnimations/ClapHands_Standing.fbx";
     * MyAvatar.overrideAnimation(ANIM_URL, 30, true, 0, 53);
     * Script.setTimeout(function () {
     *     MyAvatar.restoreAnimation();
     * }, 3000);
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
     * @function MyAvatar.getAnimationRoles
     * @returns {string[]} Array of role strings.
     * @example <caption>Print the list of animation roles defined in the avatar's avatar-animation.json file to the debug log.</caption>
     * var roles = MyAvatar.getAnimationRoles();
     * print("Animation Roles:");
     * for (var i = 0; i < roles.length; i++) {
     *     print(roles[i]);
     * }
     */
    Q_INVOKABLE QStringList getAnimationRoles();

    /**jsdoc
     * Each avatar has an avatar-animation.json file that defines a set of animation roles. Animation roles map to easily understandable actions
     * that the avatar can perform, such as "idleStand", "idleTalk", or "walkFwd". To get the full list of roles, use getAnimationRoles().
     * For each role, the avatar-animation.json defines when the animation is used, the animation clip (.FBX) used, and how animations are blended
     * together with procedural data (such as look at vectors, hand sensors etc.).
     * overrideRoleAnimation() is used to change the animation clip (.FBX) associated with a specified animation role. To end 
     * the animations and restore the default animations, use {@link MyAvatar.restoreRoleAnimation}.<br />
     * <p>Note: Hand roles only affect the hand. Other 'main' roles, like 'idleStand', 'idleTalk', 'takeoffStand' are full body.</p>
     * <p>Note: When using pre-built animation data, it's critical that the joint orientation of the source animation and target
     * rig are equivalent, since the animation data applies absolute values onto the joints. If the orientations are different,
     * the avatar will move in unpredictable ways. For more information about avatar joint orientation standards, see 
     * <a href="https://docs.highfidelity.com/create-and-explore/avatars/avatar-standards">Avatar Standards</a>.
     * @function MyAvatar.overrideRoleAnimation
     * @param role {string} The animation role to override
     * @param url {string} The URL to the animation file. Animation files need to be .FBX format, but only need to contain the avatar skeleton and animation data.
     * @param fps {number} The frames per second (FPS) rate for the animation playback. 30 FPS is normal speed.
     * @param loop {boolean} Set to true if the animation should loop
     * @param firstFrame {number} The frame the animation should start at
     * @param lastFrame {number} The frame the animation should end at
     * @example <caption>The default avatar-animation.json defines an "idleStand" animation role. This role specifies that when the avatar is not moving,
     * an animation clip of the avatar idling with hands hanging at its side will be used. It also specifies that when the avatar moves, the animation
     * will smoothly blend to the walking animation used by the "walkFwd" animation role.
     * In this example, the "idleStand" role animation clip has been replaced with a clapping animation clip. Now instead of standing with its arms
     * hanging at its sides when it is not moving, the avatar will stand and clap its hands. Note that just as it did before, as soon as the avatar
     * starts to move, the animation will smoothly blend into the walk animation used by the "walkFwd" animation role.</caption>
     * // An animation of the avatar clapping its hands while standing. Restore default after 30s.
     * var ANIM_URL = "https://s3.amazonaws.com/hifi-public/animations/ClapAnimations/ClapHands_Standing.fbx";
     * MyAvatar.overrideRoleAnimation("idleStand", ANIM_URL, 30, true, 0, 53);
     * Script.setTimeout(function () {
     *     MyAvatar.restoreRoleAnimation();
     * }, 30000);
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
     * @param role {string} The animation role clip to restore.
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

    /**jsdoc
     * @function MyAvatar.removeAnimationStateHandler
     * @param {number} handler
     */
    // Removes a handler previously added by addAnimationStateHandler.
    Q_INVOKABLE void removeAnimationStateHandler(QScriptValue handler) { _skeletonModel->getRig().removeAnimationStateHandler(handler); }


    /**jsdoc
     * @function MyAvatar.getSnapTurn
     * @returns {boolean} 
     */
    Q_INVOKABLE bool getSnapTurn() const { return _useSnapTurn; }
    /**jsdoc
     * @function MyAvatar.setSnapTurn
     * @param {boolean} on
     */
    Q_INVOKABLE void setSnapTurn(bool on) { _useSnapTurn = on; }


    /**jsdoc
     * @function MyAvatar.setDominantHand
     * @param {string} hand
     */
    Q_INVOKABLE void setDominantHand(const QString& hand);
    /**jsdoc
     * @function MyAvatar.getDominantHand
     * @returns {string} 
     */
    Q_INVOKABLE QString getDominantHand() const { return _dominantHand; }

    /**jsdoc
    * @function MyAvatar.setCenterOfGravityModelEnabled
    * @param {boolean} enabled
    */
    Q_INVOKABLE void setCenterOfGravityModelEnabled(bool value) { _centerOfGravityModelEnabled = value; }
    /**jsdoc
    * @function MyAvatar.getCenterOfGravityModelEnabled
    * @returns {boolean}
    */
    Q_INVOKABLE bool getCenterOfGravityModelEnabled() const { return _centerOfGravityModelEnabled; }
    /**jsdoc
     * @function MyAvatar.setHMDLeanRecenterEnabled
     * @param {boolean} enabled
     */
    Q_INVOKABLE void setHMDLeanRecenterEnabled(bool value) { _hmdLeanRecenterEnabled = value; }
    /**jsdoc
     * @function MyAvatar.getHMDLeanRecenterEnabled
     * @returns {boolean} 
     */
    Q_INVOKABLE bool getHMDLeanRecenterEnabled() const { return _hmdLeanRecenterEnabled; }
    /**jsdoc
     * Request to enable hand touch effect globally
     * @function MyAvatar.requestEnableHandTouch
     */
    Q_INVOKABLE void requestEnableHandTouch();
    /**jsdoc
     * Request to disable hand touch effect globally
     * @function MyAvatar.requestDisableHandTouch
     */
    Q_INVOKABLE void requestDisableHandTouch();
    /**jsdoc
     * Disables hand touch effect on a specific entity
     * @function MyAvatar.disableHandTouchForID
     * @param {Uuid} entityID - ID of the entity that will disable hand touch effect
     */
    Q_INVOKABLE void disableHandTouchForID(const QUuid& entityID);
    /**jsdoc
     * Enables hand touch effect on a specific entity
     * @function MyAvatar.enableHandTouchForID
     * @param {Uuid} entityID - ID of the entity that will enable hand touch effect
     */
    Q_INVOKABLE void enableHandTouchForID(const QUuid& entityID);

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
    void setSprintMode(bool sprint);
    float getDriveKey(DriveKeys key) const;

    /**jsdoc
     * @function MyAvatar.getRawDriveKey
     * @param {DriveKeys} key
     * @returns {number}
     */
    Q_INVOKABLE float getRawDriveKey(DriveKeys key) const;

    void relayDriveKeysToCharacterController();
    
    /**jsdoc
     * @function MyAvatar.disableDriveKey
     * @param {DriveKeys} key
     */
    Q_INVOKABLE void disableDriveKey(DriveKeys key);

    /**jsdoc
     * @function MyAvatar.enableDriveKey
     * @param {DriveKeys} key
     */
    Q_INVOKABLE void enableDriveKey(DriveKeys key);
    /**jsdoc
     * @function MyAvatar.isDriveKeyDisabled
     * @param {DriveKeys} key
     * @returns {boolean} 
     */
    Q_INVOKABLE bool isDriveKeyDisabled(DriveKeys key) const;


    /**jsdoc
     * Recenter the avatar in the vertical direction, if <code>{@link MyAvatar|MyAvatar.hmdLeanRecenterEnabled}</code> is 
     * <code>false</code>.
     * @function MyAvatar.triggerVerticalRecenter
     */
    Q_INVOKABLE void triggerVerticalRecenter();

    /**jsdoc
     * Recenter the avatar in the horizontal direction, if <code>{@link MyAvatar|MyAvatar.hmdLeanRecenterEnabled}</code> is 
     * <code>false</code>.
     * @ function MyAvatar.triggerHorizontalRecenter
     */
    Q_INVOKABLE void triggerHorizontalRecenter();

    /**jsdoc
     * Recenter the avatar's rotation, if <code>{@link MyAvatar|MyAvatar.hmdLeanRecenterEnabled}</code> is <code>false</code>.
     * @function MyAvatar.triggerRotationRecenter
     */
    Q_INVOKABLE void triggerRotationRecenter();

    /**jsdoc
    *The isRecenteringHorizontally function returns true if MyAvatar
    *is translating the root of the Avatar to keep the center of gravity under the head.
    *isActive(Horizontal) is returned.
    *@function MyAvatar.isRecenteringHorizontally
    */
    Q_INVOKABLE bool isRecenteringHorizontally() const;

    eyeContactTarget getEyeContactTarget();

    const MyHead* getMyHead() const;

    Q_INVOKABLE void toggleSmoothPoleVectors() { _skeletonModel->getRig().toggleSmoothPoleVectors(); };

    /**jsdoc
     * Get the current position of the avatar's "Head" joint.
     * @function MyAvatar.getHeadPosition
     * @returns {Vec3} The current position of the avatar's "Head" joint.
     * @example <caption>Report the current position of your avatar's head.</caption>
     * print(JSON.stringify(MyAvatar.getHeadPosition()));
     */
    Q_INVOKABLE glm::vec3 getHeadPosition() const { return getHead()->getPosition(); }

    /**jsdoc
     * @function MyAvatar.getHeadFinalYaw
     * @returns {number} 
     */
    Q_INVOKABLE float getHeadFinalYaw() const { return getHead()->getFinalYaw(); }

    /**jsdoc
     * @function MyAvatar.getHeadFinalRoll
     * @returns {number} 
     */
    Q_INVOKABLE float getHeadFinalRoll() const { return getHead()->getFinalRoll(); }

    /**jsdoc
     * @function MyAvatar.getHeadFinalPitch
     * @returns {number} 
     */
    Q_INVOKABLE float getHeadFinalPitch() const { return getHead()->getFinalPitch(); }

    /**jsdoc
     * @function MyAvatar.getHeadDeltaPitch
     * @returns {number} 
     */
    Q_INVOKABLE float getHeadDeltaPitch() const { return getHead()->getDeltaPitch(); }

    /**jsdoc
     * Get the current position of the point directly between the avatar's eyes.
     * @function MyAvatar.getEyePosition
     * @returns {Vec3} The current position of the point directly between the avatar's eyes.
     * @example <caption>Report your avatar's current eye position.</caption>
     * var eyePosition = MyAvatar.getEyePosition();
     * print(JSON.stringify(eyePosition));
     */
    Q_INVOKABLE glm::vec3 getEyePosition() const { return getHead()->getEyePosition(); }

    /**jsdoc
     * @function MyAvatar.getTargetAvatarPosition
     * @returns {Vec3} The position of the avatar you're currently looking at.
     * @example <caption>Report the position of the avatar you're currently looking at.</caption>
     * print(JSON.stringify(MyAvatar.getTargetAvatarPosition()));
     */
    Q_INVOKABLE glm::vec3 getTargetAvatarPosition() const { return _targetAvatarPosition; }

    /**jsdoc
     * @function MyAvatar.getTargetAvatar
     * @returns {AvatarData} 
     */
    Q_INVOKABLE ScriptAvatarData* getTargetAvatar() const;


    /**jsdoc
     * Get the position of the avatar's left hand as positioned by a hand controller (e.g., Oculus Touch or Vive).<br />
     * <p>Note: The Leap Motion isn't part of the hand controller input system. (Instead, it manipulates the avatar's joints 
     * for hand animation.)</p>
     * @function MyAvatar.getLeftHandPosition
     * @returns {Vec3} The position of the left hand in avatar coordinates if positioned by a hand controller, otherwise 
     *     <code>{@link Vec3(0)|Vec3.ZERO}</code>.
     * @example <caption>Report the position of your left hand relative to your avatar.</caption>
     * print(JSON.stringify(MyAvatar.getLeftHandPosition()));
     */
    Q_INVOKABLE glm::vec3 getLeftHandPosition() const;

    /**jsdoc
     * Get the position of the avatar's right hand as positioned by a hand controller (e.g., Oculus Touch or Vive).<br />
     * <p>Note: The Leap Motion isn't part of the hand controller input system. (Instead, it manipulates the avatar's joints 
     * for hand animation.)</p>
     * @function MyAvatar.getRightHandPosition
     * @returns {Vec3} The position of the right hand in avatar coordinates if positioned by a hand controller, otherwise 
     *     <code>{@link Vec3(0)|Vec3.ZERO}</code>.
     * @example <caption>Report the position of your right hand relative to your avatar.</caption>
     * print(JSON.stringify(MyAvatar.getLeftHandPosition()));
     */
    Q_INVOKABLE glm::vec3 getRightHandPosition() const;

    /**jsdoc
     * @function MyAvatar.getLeftHandTipPosition
     * @returns {Vec3} 
     */
    Q_INVOKABLE glm::vec3 getLeftHandTipPosition() const;

    /**jsdoc
     * @function MyAvatar.getRightHandTipPosition
     * @returns {Vec3} 
     */
    Q_INVOKABLE glm::vec3 getRightHandTipPosition() const;


    /**jsdoc
     * Get the pose (position, rotation, velocity, and angular velocity) of the avatar's left hand as positioned by a 
     * hand controller (e.g., Oculus Touch or Vive).<br />
     * <p>Note: The Leap Motion isn't part of the hand controller input system. (Instead, it manipulates the avatar's joints 
     * for hand animation.) If you are using the Leap Motion, the return value's <code>valid</code> property will be 
     * <code>false</code> and any pose values returned will not be meaningful.</p>
     * @function MyAvatar.getLeftHandPose
     * @returns {Pose} 
     * @example <caption>Report the pose of your avatar's left hand.</caption>
     * print(JSON.stringify(MyAvatar.getLeftHandPose()));
     */
    Q_INVOKABLE controller::Pose getLeftHandPose() const;

    /**jsdoc
     * Get the pose (position, rotation, velocity, and angular velocity) of the avatar's left hand as positioned by a 
     * hand controller (e.g., Oculus Touch or Vive).<br />
     * <p>Note: The Leap Motion isn't part of the hand controller input system. (Instead, it manipulates the avatar's joints 
     * for hand animation.) If you are using the Leap Motion, the return value's <code>valid</code> property will be 
     * <code>false</code> and any pose values returned will not be meaningful.</p>
     * @function MyAvatar.getRightHandPose
     * @returns {Pose} 
     * @example <caption>Report the pose of your avatar's right hand.</caption>
     * print(JSON.stringify(MyAvatar.getRightHandPose()));
     */
    Q_INVOKABLE controller::Pose getRightHandPose() const;

    /**jsdoc
     * @function MyAvatar.getLeftHandTipPose
     * @returns {Pose} 
     */
    Q_INVOKABLE controller::Pose getLeftHandTipPose() const;

    /**jsdoc
     * @function MyAvatar.getRightHandTipPose
     * @returns {Pose} 
     */
    Q_INVOKABLE controller::Pose getRightHandTipPose() const;

    // world-space to avatar-space rigconversion functions
    /**jsdoc
     * @function MyAvatar.worldToJointPoint
     * @param {Vec3} position
     * @param {number} [jointIndex=-1]
     * @returns {Vec3}
     */
    Q_INVOKABLE glm::vec3 worldToJointPoint(const glm::vec3& position, const int jointIndex = -1) const;

    /**jsdoc
     * @function MyAvatar.worldToJointDirection
     * @param {Vec3} direction
     * @param {number} [jointIndex=-1]
     * @returns {Vec3}
     */
    Q_INVOKABLE glm::vec3 worldToJointDirection(const glm::vec3& direction, const int jointIndex = -1) const;

    /**jsdoc
     * @function MyAvatar.worldToJointRotation
     * @param {Quat} rotation
     * @param {number} [jointIndex=-1]
     * @returns {Quat}
     */
    Q_INVOKABLE glm::quat worldToJointRotation(const glm::quat& rotation, const int jointIndex = -1) const;


    /**jsdoc
     * @function MyAvatar.jointToWorldPoint
     * @param {vec3} position
     * @param {number} [jointIndex=-1]
     * @returns {Vec3}
     */
    Q_INVOKABLE glm::vec3 jointToWorldPoint(const glm::vec3& position, const int jointIndex = -1) const;

    /**jsdoc
     * @function MyAvatar.jointToWorldDirection
     * @param {Vec3} direction
     * @param {number} [jointIndex=-1]
     * @returns {Vec3}
     */
    Q_INVOKABLE glm::vec3 jointToWorldDirection(const glm::vec3& direction, const int jointIndex = -1) const;

    /**jsdoc
     * @function MyAvatar.jointToWorldRotation
     * @param {Quat} rotation
     * @param {number} [jointIndex=-1]
     * @returns {Quat}
     */
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

    /**jsdoc
     * @function MyAvatar.pinJoint
     * @param {number} index
     * @param {Vec3} position
     * @param {Quat} orientation
     * @returns {boolean}
     */
    Q_INVOKABLE bool pinJoint(int index, const glm::vec3& position, const glm::quat& orientation);

    bool isJointPinned(int index);

    /**jsdoc
     * @function MyAvatar.clearPinOnJoint
     * @param {number} index
     * @returns {boolean} 
     */
    Q_INVOKABLE bool clearPinOnJoint(int index);

    /**jsdoc
     * @function MyAvatar.getIKErrorOnLastSolve
     * @returns {number} 
     */
    Q_INVOKABLE float getIKErrorOnLastSolve() const;

    /**jsdoc
     * @function MyAvatar.useFullAvatarURL
     * @param {string} fullAvatarURL
     * @param {string} [modelName=""]
     */
    Q_INVOKABLE void useFullAvatarURL(const QUrl& fullAvatarURL, const QString& modelName = QString());

    /**jsdoc
     * Get the complete URL for the current avatar.
     * @function MyAvatar.getFullAvatarURLFromPreferences
     * @returns {string} The full avatar model name.
     * @example <caption>Report the URL for the current avatar.</caption>
     * print(MyAvatar.getFullAvatarURLFromPreferences());
     */
    Q_INVOKABLE QUrl getFullAvatarURLFromPreferences() const { return _fullAvatarURLFromPreferences; }

    /**jsdoc
     * Get the full avatar model name for the current avatar.
     * @function MyAvatar.getFullAvatarModelName
     * @returns {string} The full avatar model name.
     * @example <caption>Report the current full avatar model name.</caption>
     * print(MyAvatar.getFullAvatarModelName());
     */
    Q_INVOKABLE QString getFullAvatarModelName() const { return _fullAvatarModelName; }

    void resetFullAvatarURL();

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

    const glm::vec2& getHipToHandController() const { return _hipToHandController; }
    void setHipToHandController(glm::vec2 currentHipToHandController) { _hipToHandController = currentHipToHandController; }
    const glm::vec2& getHeadControllerFacing() const { return _headControllerFacing; }
    void setHeadControllerFacing(glm::vec2 currentHeadControllerFacing) { _headControllerFacing = currentHeadControllerFacing; }
    const glm::vec2& getHeadControllerFacingMovingAverage() const { return _headControllerFacingMovingAverage; }
    void setHeadControllerFacingMovingAverage(glm::vec2 currentHeadControllerFacing) { _headControllerFacingMovingAverage = currentHeadControllerFacing; }
    float getCurrentStandingHeight() const { return _currentStandingHeight; }
    void setCurrentStandingHeight(float newMode) { _currentStandingHeight = newMode; }
    const glm::quat getAverageHeadRotation() const { return _averageHeadRotation; }
    void setAverageHeadRotation(glm::quat rotation) { _averageHeadRotation = rotation; }
    bool getResetMode() const { return _resetMode; }
    void setResetMode(bool hasBeenReset) { _resetMode = hasBeenReset; }

    void setControllerPoseInSensorFrame(controller::Action action, const controller::Pose& pose);
    controller::Pose getControllerPoseInSensorFrame(controller::Action action) const;
    controller::Pose getControllerPoseInWorldFrame(controller::Action action) const;
    controller::Pose getControllerPoseInAvatarFrame(controller::Action action) const;

    bool hasDriveInput() const;

    /**jsdoc
    * Function returns list of avatar entities
    * @function MyAvatar.getAvatarEntitiesVariant()
    * @returns {object[]}
    */
    Q_INVOKABLE QVariantList getAvatarEntitiesVariant();
    void clearAvatarEntities();
    void removeWearableAvatarEntities();

    /**jsdoc
     * @function MyAvatar.isFlying
     * @returns {boolean} 
     */
    Q_INVOKABLE bool isFlying();

    /**jsdoc
     * @function MyAvatar.isInAir
     * @returns {boolean} 
     */
    Q_INVOKABLE bool isInAir();

    /**jsdoc
     * @function MyAvatar.setFlyingEnabled
     * @param {boolean} enabled
     */
    Q_INVOKABLE void setFlyingEnabled(bool enabled);

    /**jsdoc
     * @function MyAvatar.getFlyingEnabled
     * @returns {boolean}
     */
    Q_INVOKABLE bool getFlyingEnabled();

    /**jsdoc
     * @function MyAvatar.setFlyingDesktopPref
     * @param {boolean} enabled
     */
    Q_INVOKABLE void setFlyingDesktopPref(bool enabled);

    /**jsdoc
     * @function MyAvatar.getFlyingDesktopPref
     * @returns {boolean}
     */
    Q_INVOKABLE bool getFlyingDesktopPref();

    /**jsdoc
     * @function MyAvatar.setFlyingDesktopPref
     * @param {boolean} enabled
     */
    Q_INVOKABLE void setFlyingHMDPref(bool enabled);

    /**jsdoc
     * @function MyAvatar.getFlyingDesktopPref
     * @returns {boolean}
     */
    Q_INVOKABLE bool getFlyingHMDPref();


    /**jsdoc
     * @function MyAvatar.getAvatarScale
     * @returns {number}
     */
    Q_INVOKABLE float getAvatarScale();

    /**jsdoc
     * @function MyAvatar.setAvatarScale
     * @param {number} scale
     */
    Q_INVOKABLE void setAvatarScale(float scale);


    /**jsdoc
     * @function MyAvatar.setCollisionsEnabled
     * @param {boolean} enabled
     */
    Q_INVOKABLE void setCollisionsEnabled(bool enabled);

    /**jsdoc
     * @function MyAvatar.getCollisionsEnabled
     * @returns {boolean} 
     */
    Q_INVOKABLE bool getCollisionsEnabled();

    /**jsdoc
    * @function MyAvatar.getCollisionCapsule
    * @returns {object}
    */
    Q_INVOKABLE QVariantMap getCollisionCapsule() const;

    /**jsdoc
     * @function MyAvatar.setCharacterControllerEnabled
     * @param {boolean} enabled
     * @deprecated
     */
    Q_INVOKABLE void setCharacterControllerEnabled(bool enabled); // deprecated

    /**jsdoc
     * @function MyAvatar.getCharacterControllerEnabled
     * @returns {boolean} 
     * @deprecated
     */
    Q_INVOKABLE bool getCharacterControllerEnabled(); // deprecated

    virtual glm::quat getAbsoluteJointRotationInObjectFrame(int index) const override;
    virtual glm::vec3 getAbsoluteJointTranslationInObjectFrame(int index) const override;

    // all calibration matrices are in absolute sensor space.
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
    // results are in sensor frame (-z forward)
    glm::mat4 deriveBodyFromHMDSensor() const;

    glm::mat4 getSpine2RotationRigSpace() const;

    glm::vec3 computeCounterBalance();

    // derive avatar body position and orientation from using the current HMD Sensor location in relation to the previous
    // location of the base of support of the avatar.
    // results are in sensor frame (-z foward)
    glm::mat4 deriveBodyUsingCgModel();

    /**jsdoc
     * @function MyAvatar.isUp
     * @param {Vec3} direction
     * @returns {boolean} 
     */
    Q_INVOKABLE bool isUp(const glm::vec3& direction) { return glm::dot(direction, _worldUpDirection) > 0.0f; }; // true iff direction points up wrt avatar's definition of up.

    /**jsdoc
     * @function MyAvatar.isDown
     * @param {Vec3} direction
     * @returns {boolean} 
     */
    Q_INVOKABLE bool isDown(const glm::vec3& direction) { return glm::dot(direction, _worldUpDirection) < 0.0f; };

    void setUserHeight(float value);
    float getUserHeight() const;
    float getUserEyeHeight() const;

    virtual SpatialParentTree* getParentTree() const override;

    const QUuid& getSelfID() const { return AVATAR_SELF_ID; }

    void setIsInWalkingState(bool isWalking);
    bool getIsInWalkingState() const;
    void setWalkSpeed(float value);
    float getWalkSpeed() const;
    void setWalkBackwardSpeed(float value);
    float getWalkBackwardSpeed() const;
    void setSprintSpeed(float value);
    float getSprintSpeed() const;

    QVector<QString> getScriptUrls();

    bool isReadyForPhysics() const;

    float computeStandingHeightMode(const controller::Pose& head);
    glm::quat computeAverageHeadRotation(const controller::Pose& head);

    virtual void setAttachmentData(const QVector<AttachmentData>& attachmentData) override;
    virtual QVector<AttachmentData> getAttachmentData() const override;

    virtual QVariantList getAttachmentsVariant() const override;
    virtual void setAttachmentsVariant(const QVariantList& variant) override;

public slots:

    /**jsdoc
     * Increase the avatar's scale by five percent, up to a minimum scale of <code>1000</code>.
     * @function MyAvatar.increaseSize
     * @example <caption>Reset your avatar's size to default then grow it 5 times.</caption>
     * MyAvatar.resetSize();
     *
     * for (var i = 0; i < 5; i++){
     *     print ("Growing by 5 percent");
     *     MyAvatar.increaseSize();
     * }
     */
    void increaseSize();

    /**jsdoc
     * Decrease the avatar's scale by five percent, down to a minimum scale of <code>0.25</code>.
     * @function MyAvatar.decreaseSize
     * @example <caption>Reset your avatar's size to default then shrink it 5 times.</caption>
     * MyAvatar.resetSize();
     *
     * for (var i = 0; i < 5; i++){
     *     print ("Shrinking by 5 percent");
     *     MyAvatar.decreaseSize();
     * }
     */
    void decreaseSize();

    /**jsdoc
     * Reset the avatar's scale back to the default scale of <code>1.0</code>.
     * @function MyAvatar.resetSize
     */
    void resetSize();

    /**jsdoc
     * @function MyAvatar.animGraphLoaded
     */
    void animGraphLoaded();

    /**jsdoc
     * @function MyAvatar.setGravity
     * @param {number} gravity
     */
    void setGravity(float gravity);

    /**jsdoc
     * @function MyAvatar.getGravity
     * @returns {number} 
     */
    float getGravity();

    /**jsdoc
     * Move the avatar to a new position and/or orientation in the domain, while taking into account Avatar leg-length.
     * @function MyAvatar.goToFeetLocation
     * @param {Vec3} position - The new position for the avatar, in world coordinates.
     * @param {boolean} [hasOrientation=false] - Set to <code>true</code> to set the orientation of the avatar.
     * @param {Quat} [orientation=Quat.IDENTITY] - The new orientation for the avatar.
     * @param {boolean} [shouldFaceLocation=false] - Set to <code>true</code> to position the avatar a short distance away from
     *      the new position and orientate the avatar to face the position.
     */

    void goToFeetLocation(const glm::vec3& newPosition,
        bool hasOrientation, const glm::quat& newOrientation,
        bool shouldFaceLocation);

    /**jsdoc
     * Move the avatar to a new position and/or orientation in the domain.
     * @function MyAvatar.goToLocation
     * @param {Vec3} position - The new position for the avatar, in world coordinates.
     * @param {boolean} [hasOrientation=false] - Set to <code>true</code> to set the orientation of the avatar.
     * @param {Quat} [orientation=Quat.IDENTITY] - The new orientation for the avatar.
     * @param {boolean} [shouldFaceLocation=false] - Set to <code>true</code> to position the avatar a short distance away from 
     * @param {boolean} [withSafeLanding=true] - Set to <code>false</code> MyAvatar::safeLanding will not be called (used when teleporting).
     *     the new position and orientate the avatar to face the position.
     */
    void goToLocation(const glm::vec3& newPosition,
                      bool hasOrientation = false, const glm::quat& newOrientation = glm::quat(),
                      bool shouldFaceLocation = false, bool withSafeLanding = true);
    /**jsdoc
     * @function MyAvatar.goToLocation
     * @param {object} properties
     */
    void goToLocation(const QVariant& properties);

    /**jsdoc
     * @function MyAvatar.goToLocationAndEnableCollisions
     * @param {Vec3} position
     */
    void goToLocationAndEnableCollisions(const glm::vec3& newPosition);

    /**jsdoc
     * @function MyAvatar.safeLanding
     * @param {Vec3} position
     * @returns {boolean} 
     */
    bool safeLanding(const glm::vec3& position);


    /**jsdoc
     * @function MyAvatar.restrictScaleFromDomainSettings
     * @param {objecct} domainSettingsObject
     */
    void restrictScaleFromDomainSettings(const QJsonObject& domainSettingsObject);

    /**jsdoc
     * @function MyAvatar.clearScaleRestriction
     */
    void clearScaleRestriction();


    /**jsdoc
     * @function MyAvatar.addThrust
     * @param {Vec3} thrust
     */
    //  Set/Get update the thrust that will move the avatar around
    void addThrust(glm::vec3 newThrust) { _thrust += newThrust; };

    /**jsdoc
     * @function MyAvatar.getThrust
     * @returns {vec3} 
     */
    glm::vec3 getThrust() { return _thrust; };

    /**jsdoc
     * @function MyAvatar.setThrust
     * @param {Vec3} thrust
     */
    void setThrust(glm::vec3 newThrust) { _thrust = newThrust; }


    /**jsdoc
     * @function MyAvatar.updateMotionBehaviorFromMenu
     */
    Q_INVOKABLE void updateMotionBehaviorFromMenu();

    /**jsdoc
    * @function MyAvatar.setToggleHips
    * @param {boolean} enabled
    */
    void setToggleHips(bool followHead);
    /**jsdoc
    * @function MyAvatar.setEnableDebugDrawBaseOfSupport
    * @param {boolean} enabled
    */
    void setEnableDebugDrawBaseOfSupport(bool isEnabled);
    /**jsdoc
     * @function MyAvatar.setEnableDebugDrawDefaultPose
     * @param {boolean} enabled
     */
    void setEnableDebugDrawDefaultPose(bool isEnabled);
    /**jsdoc
     * @function MyAvatar.setEnableDebugDrawAnimPose
     * @param {boolean} enabled
     */
    void setEnableDebugDrawAnimPose(bool isEnabled);
    /**jsdoc
     * @function MyAvatar.setEnableDebugDrawPosition
     * @param {boolean} enabled
     */
    void setEnableDebugDrawPosition(bool isEnabled);
    /**jsdoc
     * @function MyAvatar.setEnableDebugDrawHandControllers
     * @param {boolean} enabled
     */
    void setEnableDebugDrawHandControllers(bool isEnabled);
    /**jsdoc
     * @function MyAvatar.setEnableDebugDrawSensorToWorldMatrix
     * @param {boolean} enabled
     */
    void setEnableDebugDrawSensorToWorldMatrix(bool isEnabled);
    /**jsdoc
     * @function MyAvatar.setEnableDebugDrawIKTargets
     * @param {boolean} enabled
     */
    void setEnableDebugDrawIKTargets(bool isEnabled);
    /**jsdoc
     * @function MyAvatar.setEnableDebugDrawIKConstraints
     * @param {boolean} enabled
     */
    void setEnableDebugDrawIKConstraints(bool isEnabled);
    /**jsdoc
     * @function MyAvatar.setEnableDebugDrawIKChains
     * @param {boolean} enabled
     */
    void setEnableDebugDrawIKChains(bool isEnabled);
    /**jsdoc
     * @function MyAvatar.setEnableDebugDrawDetailedCollision
     * @param {boolean} enabled
     */
    void setEnableDebugDrawDetailedCollision(bool isEnabled);

    /**jsdoc
     * Get whether or not your avatar mesh is visible.
     * @function MyAvatar.getEnableMeshVisible
     * @returns {boolean} <code>true</code> if your avatar's mesh is visible, otherwise <code>false</code>.
     */
    bool getEnableMeshVisible() const override;

    /**jsdoc
     * Set whether or not your avatar mesh is visible.
     * @function MyAvatar.setEnableMeshVisible
     * @param {boolean} visible - <code>true</code> to set your avatar mesh visible; <code>false</code> to set it invisible.
     * @example <caption>Make your avatar invisible for 10s.</caption>
     * MyAvatar.setEnableMeshVisible(false);
     * Script.setTimeout(function () {
     *     MyAvatar.setEnableMeshVisible(true);
     * }, 10000);
     */
    virtual void setEnableMeshVisible(bool isEnabled) override;

    /**jsdoc
     * @function MyAvatar.setEnableInverseKinematics
     * @param {boolean} enabled
     */
    void setEnableInverseKinematics(bool isEnabled);


    /**jsdoc
     * @function MyAvatar.getAnimGraphOverrideUrl
     * @returns {string} 
     */
    QUrl getAnimGraphOverrideUrl() const;  // thread-safe

    /**jsdoc
     * @function MyAvatar.setAnimGraphOverrideUrl
     * @param {string} url
     */
    void setAnimGraphOverrideUrl(QUrl value);  // thread-safe

    /**jsdoc
     * @function MyAvatar.getAnimGraphUrl
     * @returns {string} 
     */
    QUrl getAnimGraphUrl() const;  // thread-safe

    /**jsdoc
     * @function MyAvatar.setAnimGraphUrl
     * @param {string} url
     */
    void setAnimGraphUrl(const QUrl& url);  // thread-safe

    /**jsdoc
     * @function MyAvatar.getPositionForAudio
     * @returns {Vec3} 
     */
    glm::vec3 getPositionForAudio();

    /**jsdoc
     * @function MyAvatar.getOrientationForAudio
     * @returns {Quat} 
     */
    glm::quat getOrientationForAudio();

    /**jsdoc
     * @function MyAvatar.setModelScale
     * @param {number} scale
     */
    virtual void setModelScale(float scale) override;

signals:

    /**jsdoc
     * @function MyAvatar.audioListenerModeChanged
     * @returns {Signal} 
     */
    void audioListenerModeChanged();

    /**jsdoc
     * @function MyAvatar.transformChanged
     * @returns {Signal} 
     */
    void transformChanged();

    /**jsdoc
     * @function MyAvatar.newCollisionSoundURL
     * @param {string} url
     * @returns {Signal} 
     */
    void newCollisionSoundURL(const QUrl& url);

    /**jsdoc
     * Triggered when the avatar collides with an entity.
     * @function MyAvatar.collisionWithEntity
     * @param {Collision} collision
     * @returns {Signal}
     * @example <caption>Report each time your avatar collides with an entity.</caption>
     * MyAvatar.collisionWithEntity.connect(function (collision) {
     *     print("Your avatar collided with an entity.");
     * });
     */
    void collisionWithEntity(const Collision& collision);

    /**jsdoc
     * Triggered when collisions with avatar enabled or disabled
     * @function MyAvatar.collisionsEnabledChanged
     * @param {boolean} enabled
     * @returns {Signal}
     */
    void collisionsEnabledChanged(bool enabled);

    /**jsdoc
     * Triggered when avatar's animation url changes
     * @function MyAvatar.animGraphUrlChanged
     * @param {url} url
     * @returns {Signal}
     */
    void animGraphUrlChanged(const QUrl& url);

    /**jsdoc
     * @function MyAvatar.energyChanged
     * @param {number} energy
     * @returns {Signal} 
     */
    void energyChanged(float newEnergy);

    /**jsdoc
     * @function MyAvatar.positionGoneTo
     * @returns {Signal} 
     */
    // FIXME: Better name would be goneToLocation().
    void positionGoneTo();

    /**jsdoc
     * @function MyAvatar.onLoadComplete
     * @returns {Signal} 
     */
    void onLoadComplete();

    /**jsdoc
     * @function MyAvatar.wentAway
     * @returns {Signal} 
     */
    void wentAway();

    /**jsdoc
     * @function MyAvatar.wentActive
     * @returns {Signal} 
     */
    void wentActive();

    /**jsdoc
     * @function MyAvatar.skeletonChanged
     * @returns {Signal} 
     */
    void skeletonChanged();

    /**jsdoc
     * @function MyAvatar.dominantHandChanged
     * @param {string} hand
     * @returns {Signal} 
     */
    void dominantHandChanged(const QString& hand);

    /**jsdoc
     * @function MyAvatar.sensorToWorldScaleChanged
     * @param {number} scale
     * @returns {Signal} 
     */
    void sensorToWorldScaleChanged(float sensorToWorldScale);

    /**jsdoc
     * @function MyAvatar.attachmentsChanged
     * @returns {Signal} 
     */
    void attachmentsChanged();

    /**jsdoc
     * @function MyAvatar.scaleChanged
     * @returns {Signal} 
     */
    void scaleChanged();

    /**jsdoc
     * Triggered when hand touch is globally enabled or disabled
     * @function MyAvatar.shouldDisableHandTouchChanged
     * @param {boolean} shouldDisable
     * @returns {Signal}
     */
    void shouldDisableHandTouchChanged(bool shouldDisable);

    /**jsdoc
     * Triggered when hand touch is enabled or disabled for an specific entity
     * @function MyAvatar.disableHandTouchForIDChanged
     * @param {Uuid} entityID - ID of the entity that will enable hand touch effect
     * @param {boolean} disable
     * @returns {Signal}
     */
    void disableHandTouchForIDChanged(const QUuid& entityID, bool disable);

private slots:
    void leaveDomain();
    void updateCollisionCapsuleCache();

protected:
    virtual void beParentOfChild(SpatiallyNestablePointer newChild) const override;
    virtual void forgetChild(SpatiallyNestablePointer newChild) const override;
    virtual void recalculateChildCauterization() const override;

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
    void setHasScriptedBlendshapes(bool hasScriptedBlendshapes);
    bool getHasScriptedBlendshapes() const override { return _hasScriptedBlendShapes; }
    void setHasProceduralBlinkFaceMovement(bool hasProceduralBlinkFaceMovement);
    bool getHasProceduralBlinkFaceMovement() const override { return _headData->getHasProceduralBlinkFaceMovement(); }
    void setHasProceduralEyeFaceMovement(bool hasProceduralEyeFaceMovement);
    bool getHasProceduralEyeFaceMovement() const override { return _headData->getHasProceduralEyeFaceMovement(); }
    void setHasAudioEnabledFaceMovement(bool hasAudioEnabledFaceMovement);
    bool getHasAudioEnabledFaceMovement() const override { return _headData->getHasAudioEnabledFaceMovement(); }
    void setRotationRecenterFilterLength(float length);
    float getRotationRecenterFilterLength() const { return _rotationRecenterFilterLength; }
    void setRotationThreshold(float angleRadians);
    float getRotationThreshold() const { return _rotationThreshold; }
    void setEnableStepResetRotation(bool stepReset) { _stepResetRotationEnabled = stepReset; }
    bool getEnableStepResetRotation() const { return _stepResetRotationEnabled; }
    void setEnableDrawAverageFacing(bool drawAverage) { _drawAverageFacingEnabled = drawAverage; }
    bool getEnableDrawAverageFacing() const { return _drawAverageFacingEnabled; }
    bool isMyAvatar() const override { return true; }
    virtual int parseDataFromBuffer(const QByteArray& buffer) override;
    virtual glm::vec3 getSkeletonPosition() const override;
    int _skeletonModelChangeCount { 0 };

    void saveAvatarScale();

    glm::vec3 getScriptedMotorVelocity() const { return _scriptedMotorVelocity; }
    float getScriptedMotorTimescale() const { return _scriptedMotorTimescale; }
    QString getScriptedMotorFrame() const;
    QString getScriptedMotorMode() const;
    void setScriptedMotorVelocity(const glm::vec3& velocity);
    void setScriptedMotorTimescale(float timescale);
    void setScriptedMotorFrame(QString frame);
    void setScriptedMotorMode(QString mode);

    // Attachments
    virtual void attach(const QString& modelURL, const QString& jointName = QString(),
                        const glm::vec3& translation = glm::vec3(), const glm::quat& rotation = glm::quat(),
                        float scale = 1.0f, bool isSoft = false,
                        bool allowDuplicates = false, bool useSaved = true) override;

    virtual void detachOne(const QString& modelURL, const QString& jointName = QString()) override;
    virtual void detachAll(const QString& modelURL, const QString& jointName = QString()) override;
    
    // Attachments/Avatar Entity
    void attachmentDataToEntityProperties(const AttachmentData& data, EntityItemProperties& properties);
    AttachmentData entityPropertiesToAttachmentData(const EntityItemProperties& properties) const;
    bool findAvatarEntity(const QString& modelURL, const QString& jointName, QUuid& entityID);

    bool cameraInsideHead(const glm::vec3& cameraPosition) const;

    void updateEyeContactTarget(float deltaTime);

    // These are made private for MyAvatar so that you will use the "use" methods instead
    virtual void setSkeletonModelURL(const QUrl& skeletonModelURL) override;

    virtual void updatePalms() override {}
    void lateUpdatePalms();

    void clampTargetScaleToDomainLimits();
    void clampScaleChangeToDomainLimits(float desiredScale);
    glm::mat4 computeCameraRelativeHandControllerMatrix(const glm::mat4& controllerSensorMatrix) const;

    std::array<float, MAX_DRIVE_KEYS> _driveKeys;
    std::bitset<MAX_DRIVE_KEYS> _disabledDriveKeys;

    bool _enableFlying { false };
    bool _flyingPrefDesktop { true };
    bool _flyingPrefHMD { false };
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
    int _scriptedMotorMode;
    quint32 _motionBehaviors;
    QString _collisionSoundURL;

    SharedSoundPointer _collisionSound;

    MyCharacterController _characterController;
    int32_t _previousCollisionGroup { BULLET_COLLISION_GROUP_MY_AVATAR };

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
    QString _dominantHand { DOMINANT_RIGHT_HAND };

    const float ROLL_CONTROL_DEAD_ZONE_DEFAULT = 8.0f; // degrees
    const float ROLL_CONTROL_RATE_DEFAULT = 114.0f; // degrees / sec

    bool _hmdRollControlEnabled { true };
    float _hmdRollControlDeadZone { ROLL_CONTROL_DEAD_ZONE_DEFAULT };
    float _hmdRollControlRate { ROLL_CONTROL_RATE_DEFAULT };
    std::atomic<bool> _hasScriptedBlendShapes { false };
    std::atomic<float> _rotationRecenterFilterLength { 4.0f };
    std::atomic<float> _rotationThreshold { 0.5235f };  // 30 degrees in radians
    std::atomic<bool> _stepResetRotationEnabled { true };
    std::atomic<bool> _drawAverageFacingEnabled { false };

    // working copy -- see AvatarData for thread-safe _sensorToWorldMatrixCache, used for outward facing access
    glm::mat4 _sensorToWorldMatrix { glm::mat4() };

    // cache of the current HMD sensor position and orientation in sensor space.
    glm::mat4 _hmdSensorMatrix;
    glm::quat _hmdSensorOrientation;
    glm::vec3 _hmdSensorPosition;
    // cache head controller pose in sensor space
    glm::vec2 _headControllerFacing;  // facing vector in xz plane (sensor space)
    glm::vec2 _headControllerFacingMovingAverage { 0.0f, 0.0f };   // facing vector in xz plane (sensor space)
    glm::quat _averageHeadRotation { 0.0f, 0.0f, 0.0f, 1.0f };

    glm::vec2 _hipToHandController { 0.0f, -1.0f };  // spine2 facing vector in xz plane (avatar space)

    float _currentStandingHeight { 0.0f };
    bool _resetMode { true };
    RingBufferHistory<int> _recentModeReadings;

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
        bool shouldActivateHorizontalCG(MyAvatar& myAvatar) const;
        void prePhysicsUpdate(MyAvatar& myAvatar, const glm::mat4& bodySensorMatrix, const glm::mat4& currentBodyMatrix, bool hasDriveInput);
        glm::mat4 postPhysicsUpdate(const MyAvatar& myAvatar, const glm::mat4& currentBodyMatrix);
        bool getForceActivateRotation() const;
        void setForceActivateRotation(bool val);
        bool getForceActivateVertical() const;
        void setForceActivateVertical(bool val);
        bool getForceActivateHorizontal() const;
        void setForceActivateHorizontal(bool val);
        bool getToggleHipsFollowing() const;
        void setToggleHipsFollowing(bool followHead);
        std::atomic<bool> _forceActivateRotation { false };
        std::atomic<bool> _forceActivateVertical { false };
        std::atomic<bool> _forceActivateHorizontal { false };
        std::atomic<bool> _toggleHipsFollowing { true };
    };
    FollowHelper _follow;

    bool _goToPending { false };
    bool _physicsSafetyPending { false };
    bool _goToSafe { true };
    glm::vec3 _goToPosition;
    glm::quat _goToOrientation;

    std::unordered_set<int> _headBoneSet;
    std::unordered_set<SpatiallyNestablePointer> _cauterizedChildrenOfHead;
    bool _prevShouldDrawHead;
    bool _rigEnabled { true };

    bool _enableDebugDrawBaseOfSupport { false };
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
    mutable std::mutex _disableHandTouchMutex;

    bool _centerOfGravityModelEnabled { true };
    bool _hmdLeanRecenterEnabled { true };
    bool _sprint { false };

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

    void updateChildCauterization(SpatiallyNestablePointer object, bool cauterize);

    // max unscaled forward movement speed
    ThreadSafeValueCache<float> _walkSpeed { DEFAULT_AVATAR_MAX_WALKING_SPEED };
    ThreadSafeValueCache<float> _walkBackwardSpeed { DEFAULT_AVATAR_MAX_WALKING_BACKWARD_SPEED };
    ThreadSafeValueCache<float> _sprintSpeed { AVATAR_SPRINT_SPEED_SCALAR };
    float _walkSpeedScalar { AVATAR_WALK_SPEED_SCALAR };
    bool _isInWalkingState { false };

    // load avatar scripts once when rig is ready
    bool _shouldLoadScripts { false };

    bool _haveReceivedHeightLimitsFromDomain { false };
    int _disableHandTouchCount { 0 };
};

QScriptValue audioListenModeToScriptValue(QScriptEngine* engine, const AudioListenerMode& audioListenerMode);
void audioListenModeFromScriptValue(const QScriptValue& object, AudioListenerMode& audioListenerMode);

QScriptValue driveKeysToScriptValue(QScriptEngine* engine, const MyAvatar::DriveKeys& driveKeys);
void driveKeysFromScriptValue(const QScriptValue& object, MyAvatar::DriveKeys& driveKeys);

bool isWearableEntity(const EntityItemPointer& entity);

#endif // hifi_MyAvatar_h
