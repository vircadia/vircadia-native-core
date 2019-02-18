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
 * {@link MyAvatar} API.
 *
 * @namespace Avatar
 *
 * @hifi-assignment-client
 *
 * @comment IMPORTANT: This group of properties is copied from AvatarData.h; they should NOT be edited here.
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
 *
 */

class ScriptableAvatar : public AvatarData, public Dependency {
    Q_OBJECT

    using Clock = std::chrono::system_clock;
    using TimePoint = Clock::time_point;

public:

    ScriptableAvatar();

    /**jsdoc
     * @function Avatar.startAnimation
     * @param {string} url
     * @param {number} [fps=30]
     * @param {number} [priority=1]
     * @param {boolean} [loop=false]
     * @param {boolean} [hold=false]
     * @param {number} [firstFrame=0]
     * @param {number} [lastFrame=3.403e+38]
     * @param {string[]} [maskedJoints=[]]
     */
    /// Allows scripts to run animations.
    Q_INVOKABLE void startAnimation(const QString& url, float fps = 30.0f, float priority = 1.0f, bool loop = false,
                                    bool hold = false, float firstFrame = 0.0f, float lastFrame = FLT_MAX, 
                                    const QStringList& maskedJoints = QStringList());

    /**jsdoc
     * @function Avatar.stopAnimation
     */
    Q_INVOKABLE void stopAnimation();

    /**jsdoc
     * @function Avatar.getAnimationDetails
     * @returns {Avatar.AnimationDetails}
     */
    Q_INVOKABLE AnimationDetails getAnimationDetails();

    /**jsdoc
     * ####### TODO: If this override changes the function use @override and do JSDoc here, otherwise @comment that uses base class's JSDoc.<br />
     * Get the names of all the joints in the current avatar.
     * @function Avatar.getJointNames
     * @returns {string[]} The joint names.
     * @example <caption>Report the names of all the joints in your current avatar.</caption>
     * print(JSON.stringify(Avatar.getJointNames()));
     */
    Q_INVOKABLE virtual QStringList getJointNames() const override;

    /**jsdoc
     * ####### TODO: If this override changes the function use @override and do JSDoc here, otherwise @comment that uses base class's JSDoc.<br />
     * Get the joint index for a named joint. The joint index value is the position of the joint in the array returned by
     * {@link Avatar.getJointNames}.
     * @function Avatar.getJointIndex
     * @param {string} name - The name of the joint.
     * @returns {number} The index of the joint.
     * @example <caption>Report the index of your avatar's left arm joint.</caption>
     * print(JSON.stringify(Avatar.getJointIndex("LeftArm"));
     */
    /// Returns the index of the joint with the specified name, or -1 if not found/unknown.
    Q_INVOKABLE virtual int getJointIndex(const QString& name) const override;

    virtual void setSkeletonModelURL(const QUrl& skeletonModelURL) override;

    int sendAvatarDataPacket(bool sendAll = false) override;

    virtual QByteArray toByteArrayStateful(AvatarDataDetail dataDetail, bool dropFaceTracking = false) override;

    void setHasProceduralBlinkFaceMovement(bool hasProceduralBlinkFaceMovement);
    bool getHasProceduralBlinkFaceMovement() const override { return _headData->getHasProceduralBlinkFaceMovement(); }
    void setHasProceduralEyeFaceMovement(bool hasProceduralEyeFaceMovement);
    bool getHasProceduralEyeFaceMovement() const override { return _headData->getHasProceduralEyeFaceMovement(); }
    void setHasAudioEnabledFaceMovement(bool hasAudioEnabledFaceMovement);
    bool getHasAudioEnabledFaceMovement() const override { return _headData->getHasAudioEnabledFaceMovement(); }

     /**jsdoc
     * ####### TODO: If this override changes the function use @override and do JSDoc here, otherwise @comment that uses base class's JSDoc.<br />
     * ####### Also need to resolve with MyAvatar.<br />
     * Potentially Very Expensive.  Do not use.
     * @function Avatar.getAvatarEntityData
     * @returns {object}
     */
    Q_INVOKABLE AvatarEntityMap getAvatarEntityData() const override;

    /**jsdoc
     * ####### TODO: If this override changes the function use @override and do JSDoc here, otherwise @comment that uses base class's JSDoc.
     * @function Avatar.setAvatarEntityData
     * @param {object} avatarEntityData
     */
    Q_INVOKABLE void setAvatarEntityData(const AvatarEntityMap& avatarEntityData) override;

    /**jsdoc
     * ####### TODO: If this override changes the function use @override and do JSDoc here, otherwise @comment that uses base class's JSDoc.
     * @function Avatar.updateAvatarEntity
     * @param {Uuid} entityID
     * @param {string} entityData
     */
    Q_INVOKABLE void updateAvatarEntity(const QUuid& entityID, const QByteArray& entityData) override;

public slots:
    /**jsdoc
     * @function Avatar.update
     */
    void update(float deltatime);

    /**jsdoc
    * @function Avatar.setJointMappingsFromNetworkReply
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
