//
//  ScriptableAvatar.h
//  assignment-client/src/avatars
//
//  Created by Clement on 7/22/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ScriptableAvatar_h
#define hifi_ScriptableAvatar_h

#include <AnimationCache.h>
#include <AnimSkeleton.h>
#include <AvatarData.h>
#include <ScriptEngine.h>
#include <EntityItem.h>

/**jsdoc
 * The <code>Avatar</code> API is used to manipulate scriptable avatars on the domain. This API is a subset of the 
 * {@link MyAvatar} API. To enable this API, set {@link Agent|Agent.isAvatatr} to <code>true</code>.
 *
 * <p>For Interface, client entity, and avatar scripts, see {@link MyAvatar}.</p>
 *
 * @namespace Avatar
 *
 * @hifi-assignment-client
 *
 * @comment IMPORTANT: This group of properties is copied from AvatarData.h; they should NOT be edited here.
 * @property {Vec3} position - The position of the avatar.
 * @property {number} scale=1.0 - The scale of the avatar. When setting, the value is limited to between <code>0.005</code>
 *     and <code>1000.0</code>. When getting, the value may temporarily be further limited by the domain's settings.
 * @property {number} density - The density of the avatar in kg/m<sup>3</sup>. The density is used to work out its mass in
 *     the application of physics. <em>Read-only.</em>
 * @property {Vec3} handPosition - A user-defined hand position, in world coordinates. The position moves with the avatar
 *    but is otherwise not used or changed by Interface.
 * @property {number} bodyYaw - The rotation left or right about an axis running from the head to the feet of the avatar.
 *     Yaw is sometimes called "heading".
 * @property {number} bodyPitch - The rotation about an axis running from shoulder to shoulder of the avatar. Pitch is
 *     sometimes called "elevation".
 * @property {number} bodyRoll - The rotation about an axis running from the chest to the back of the avatar. Roll is
 *     sometimes called "bank".
 * @property {Quat} orientation - The orientation of the avatar.
 * @property {Quat} headOrientation - The orientation of the avatar's head.
 * @property {number} headPitch - The rotation about an axis running from ear to ear of the avatar's head. Pitch is
 *     sometimes called "elevation".
 * @property {number} headYaw - The rotation left or right about an axis running from the base to the crown of the avatar's
 *     head. Yaw is sometimes called "heading".
 * @property {number} headRoll - The rotation about an axis running from the nose to the back of the avatar's head. Roll is
 *     sometimes called "bank".
 * @property {Vec3} velocity - The current velocity of the avatar.
 * @property {Vec3} angularVelocity - The current angular velocity of the avatar.
 * @property {number} audioLoudness - The instantaneous loudness of the audio input that the avatar is injecting into the
 *     domain.
 * @property {number} audioAverageLoudness - The rolling average loudness of the audio input that the avatar is injecting
 *     into the domain.
 * @property {string} displayName - The avatar's display name.
 * @property {string} sessionDisplayName - Sanitized, defaulted version of <code>displayName</code> that is defined by the
 *     avatar mixer rather than by Interface clients. The result is unique among all avatars present in the domain at the
 *     time.
 * @property {boolean} lookAtSnappingEnabled=true - If <code>true</code>, the avatar's eyes snap to look at another avatar's
 *     eyes if generally in the line of sight and the other avatar also has <code>lookAtSnappingEnabled == true</code>.
 * @property {string} skeletonModelURL - The URL of the avatar model's <code>.fst</code> file.
 * @property {AttachmentData[]} attachmentData - Information on the attachments worn by the avatar.<br />
 *     <strong>Deprecated:</strong> Use avatar entities instead.
 * @property {string[]} jointNames - The list of joints in the current avatar model. <em>Read-only.</em>
 * @property {Uuid} sessionUUID - Unique ID of the avatar in the domain. <em>Read-only.</em>
 * @property {Mat4} sensorToWorldMatrix - The scale, rotation, and translation transform from the user's real world to the
 *     avatar's size, orientation, and position in the virtual world. <em>Read-only.</em>
 * @property {Mat4} controllerLeftHandMatrix - The rotation and translation of the left hand controller relative to the
 *     avatar. <em>Read-only.</em>
 * @property {Mat4} controllerRightHandMatrix - The rotation and translation of the right hand controller relative to the
 *     avatar. <em>Read-only.</em>
 * @property {number} sensorToWorldScale - The scale that transforms dimensions in the user's real world to the avatar's
 *     size in the virtual world. <em>Read-only.</em>
 */

class ScriptableAvatar : public AvatarData, public Dependency {
    Q_OBJECT

    using Clock = std::chrono::system_clock;
    using TimePoint = Clock::time_point;

public:

    ScriptableAvatar();

    /**jsdoc
     * Starts playing an animation on the avatar.
     * @function Avatar.startAnimation
     * @param {string} url - The URL to the animation file. Animation files need to be .FBX format but only need to contain 
     *     the avatar skeleton and animation data.
     * @param {number} [fps=30] - The frames per second (FPS) rate for the animation playback. 30 FPS is normal speed.
     * @param {number} [priority=1] - <em>Not used.</em>
     * @param {boolean} [loop=false] - <code>true</code> if the animation should loop, <code>false</code> if it shouldn't.
     * @param {boolean} [hold=false] - <em>Not used.</em>
     * @param {number} [firstFrame=0] - The frame the animation should start at.
     * @param {number} [lastFrame=3.403e+38] - The frame the animation should stop at.
     * @param {string[]} [maskedJoints=[]] - The names of joints that should not be animated.
     */
    /// Allows scripts to run animations.
    Q_INVOKABLE void startAnimation(const QString& url, float fps = 30.0f, float priority = 1.0f, bool loop = false,
                                    bool hold = false, float firstFrame = 0.0f, float lastFrame = FLT_MAX, 
                                    const QStringList& maskedJoints = QStringList());

    /**jsdoc
     * Stops playing the current animation.
     * @function Avatar.stopAnimation
     */
    Q_INVOKABLE void stopAnimation();

    /**jsdoc
     * Gets the details of the current avatar animation that is being or was recently played.
     * @function Avatar.getAnimationDetails
     * @returns {Avatar.AnimationDetails} The current or recent avatar animation.
     */
    Q_INVOKABLE AnimationDetails getAnimationDetails();

    /**jsdoc
     * @comment Uses the base class's JSDoc.
     */
    Q_INVOKABLE virtual QStringList getJointNames() const override;

    /**jsdoc
     * @comment Uses the base class's JSDoc.
     */
    /// Returns the index of the joint with the specified name, or -1 if not found/unknown.
    Q_INVOKABLE virtual int getJointIndex(const QString& name) const override;

    virtual void setSkeletonModelURL(const QUrl& skeletonModelURL) override;

    /**jsdoc
     * @comment Uses the base class's JSDoc.
     */
    int sendAvatarDataPacket(bool sendAll = false) override;

    virtual QByteArray toByteArrayStateful(AvatarDataDetail dataDetail, bool dropFaceTracking = false) override;

    void setHasProceduralBlinkFaceMovement(bool hasProceduralBlinkFaceMovement);
    bool getHasProceduralBlinkFaceMovement() const override { return _headData->getHasProceduralBlinkFaceMovement(); }
    void setHasProceduralEyeFaceMovement(bool hasProceduralEyeFaceMovement);
    bool getHasProceduralEyeFaceMovement() const override { return _headData->getHasProceduralEyeFaceMovement(); }
    void setHasAudioEnabledFaceMovement(bool hasAudioEnabledFaceMovement);
    bool getHasAudioEnabledFaceMovement() const override { return _headData->getHasAudioEnabledFaceMovement(); }

    /**jsdoc
     * Gets the avatar entities as binary data.
     * <p><strong>Warning:</strong> Potentially a very expensive call. Do not use if possible.</p>
     * @function Avatar.getAvatarEntityData
     * @returns {AvatarEntityMap}
     */
    Q_INVOKABLE AvatarEntityMap getAvatarEntityData() const override;

    /**jsdoc
     * Sets the avatar entities from binary data.
     * <p><strong>Warning:</strong> Potentially an expensive call. Do not use if possible.</p>
     * @function Avatar.setAvatarEntityData
     * @param {AvatarEntityMap} avatarEntityData
     */
    Q_INVOKABLE void setAvatarEntityData(const AvatarEntityMap& avatarEntityData) override;

    /**jsdoc
     * @comment Uses the base class's JSDoc.
     */
    Q_INVOKABLE void updateAvatarEntity(const QUuid& entityID, const QByteArray& entityData) override;

public slots:
    /**jsdoc
     * @function Avatar.update
     * @param {number} deltaTime - Delta time.
     * @deprecated This function is deprecated and will be removed.
     */
    void update(float deltatime);

    /**jsdoc
     * @function Avatar.setJointMappingsFromNetworkReply
     * @deprecated This function is deprecated and will be removed.
     */
    void setJointMappingsFromNetworkReply();

private:
    AnimationPointer _animation;
    AnimationDetails _animationDetails;
    QStringList _maskedJoints;
    AnimationPointer _bind; // a sleazy way to get the skeleton, given the various library/cmake dependencies
    std::shared_ptr<AnimSkeleton> _animSkeleton;
    QHash<QString, int> _fstJointIndices; ///< 1-based, since zero is returned for missing keys
    QStringList _fstJointNames; ///< in order of depth-first traversal
    QUrl _skeletonFBXURL;
    mutable QScriptEngine _scriptEngine;
    std::map<QUuid, EntityItemPointer> _entities;

    /// Loads the joint indices, names from the FST file (if any)
    void updateJointMappings();

    quint64 _lastSendAvatarDataTime { 0 };

    TimePoint _nextTraitsSendWindow;
};

#endif // hifi_ScriptableAvatar_h
