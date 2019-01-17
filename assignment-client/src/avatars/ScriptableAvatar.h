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
 * <p><strong>Note:</strong> In the examples, use "<code>Avatar</code>" instead of "<code>MyAvatar</code>".</p>
 * 
 * @namespace Avatar
 *
 * @hifi-assignment-client
 *
 * @property {Vec3} position
 * @property {number} scale
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
 * @borrows MyAvatar.getDomainMinScale as getDomainMinScale
 * @borrows MyAvatar.getDomainMaxScale as getDomainMaxScale
 * @borrows MyAvatar.canMeasureEyeHeight as canMeasureEyeHeight
 * @borrows MyAvatar.getEyeHeight as getEyeHeight
 * @borrows MyAvatar.getHeight as getHeight
 * @borrows MyAvatar.setHandState as setHandState
 * @borrows MyAvatar.getHandState as getHandState
 * @borrows MyAvatar.setRawJointData as setRawJointData
 * @borrows MyAvatar.setJointData as setJointData
 * @borrows MyAvatar.setJointRotation as setJointRotation
 * @borrows MyAvatar.setJointTranslation as setJointTranslation
 * @borrows MyAvatar.clearJointData as clearJointData
 * @borrows MyAvatar.isJointDataValid as isJointDataValid
 * @borrows MyAvatar.getJointRotation as getJointRotation
 * @borrows MyAvatar.getJointTranslation as getJointTranslation
 * @borrows MyAvatar.getJointRotations as getJointRotations
 * @borrows MyAvatar.getJointTranslations as getJointTranslations
 * @borrows MyAvatar.setJointRotations as setJointRotations
 * @borrows MyAvatar.setJointTranslations as setJointTranslations
 * @borrows MyAvatar.clearJointsData as clearJointsData
 * @borrows MyAvatar.getJointIndex as getJointIndex
 * @borrows MyAvatar.getJointNames as getJointNames
 * @borrows MyAvatar.setBlendshape as setBlendshape
 * @borrows MyAvatar.getAttachmentsVariant as getAttachmentsVariant
 * @borrows MyAvatar.setAttachmentsVariant as setAttachmentsVariant
 * @borrows MyAvatar.updateAvatarEntity as updateAvatarEntity
 * @borrows MyAvatar.clearAvatarEntity as clearAvatarEntity
 * @borrows MyAvatar.setForceFaceTrackerConnected as setForceFaceTrackerConnected
 * @borrows MyAvatar.getAttachmentData as getAttachmentData
 * @borrows MyAvatar.setAttachmentData as setAttachmentData
 * @borrows MyAvatar.attach as attach
 * @borrows MyAvatar.detachOne as detachOne
 * @borrows MyAvatar.detachAll as detachAll
 * @borrows MyAvatar.getAvatarEntityData as getAvatarEntityData
 * @borrows MyAvatar.setAvatarEntityData as setAvatarEntityData
 * @borrows MyAvatar.getSensorToWorldMatrix as getSensorToWorldMatrix
 * @borrows MyAvatar.getSensorToWorldScale as getSensorToWorldScale
 * @borrows MyAvatar.getControllerLeftHandMatrix as getControllerLeftHandMatrix
 * @borrows MyAvatar.getControllerRightHandMatrix as getControllerRightHandMatrix
 * @borrows MyAvatar.getDataRate as getDataRate
 * @borrows MyAvatar.getUpdateRate as getUpdateRate
 * @borrows MyAvatar.displayNameChanged as displayNameChanged
 * @borrows MyAvatar.sessionDisplayNameChanged as sessionDisplayNameChanged
 * @borrows MyAvatar.skeletonModelURLChanged as skeletonModelURLChanged
 * @borrows MyAvatar.lookAtSnappingChanged as lookAtSnappingChanged
 * @borrows MyAvatar.sessionUUIDChanged as sessionUUIDChanged
 * @borrows MyAvatar.sendAvatarDataPacket as sendAvatarDataPacket
 * @borrows MyAvatar.sendIdentityPacket as sendIdentityPacket
 * @borrows MyAvatar.setJointMappingsFromNetworkReply as setJointMappingsFromNetworkReply
 * @borrows MyAvatar.setSessionUUID as setSessionUUID
 * @borrows MyAvatar.getAbsoluteJointRotationInObjectFrame as getAbsoluteJointRotationInObjectFrame
 * @borrows MyAvatar.getAbsoluteJointTranslationInObjectFrame as getAbsoluteJointTranslationInObjectFrame
 * @borrows MyAvatar.setAbsoluteJointRotationInObjectFrame as setAbsoluteJointRotationInObjectFrame
 * @borrows MyAvatar.setAbsoluteJointTranslationInObjectFrame as setAbsoluteJointTranslationInObjectFrame
 * @borrows MyAvatar.getTargetScale as getTargetScale
 * @borrows MyAvatar.resetLastSent as resetLastSent
 */

class ScriptableAvatar : public AvatarData, public Dependency {
    Q_OBJECT
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
    * Get the names of all the joints in the current avatar.
    * @function MyAvatar.getJointNames
    * @returns {string[]} The joint names.
    * @example <caption>Report the names of all the joints in your current avatar.</caption>
    * print(JSON.stringify(MyAvatar.getJointNames()));
    */
    Q_INVOKABLE virtual QStringList getJointNames() const override;

    /**jsdoc
    * Get the joint index for a named joint. The joint index value is the position of the joint in the array returned by
    * {@link MyAvatar.getJointNames} or {@link Avatar.getJointNames}.
    * @function MyAvatar.getJointIndex
    * @param {string} name - The name of the joint.
    * @returns {number} The index of the joint.
    * @example <caption>Report the index of your avatar's left arm joint.</caption>
    * print(JSON.stringify(MyAvatar.getJointIndex("LeftArm"));
    */
    /// Returns the index of the joint with the specified name, or -1 if not found/unknown.
    Q_INVOKABLE virtual int getJointIndex(const QString& name) const override;

    virtual void setSkeletonModelURL(const QUrl& skeletonModelURL) override;

    virtual QByteArray toByteArrayStateful(AvatarDataDetail dataDetail, bool dropFaceTracking = false) override;

    void setHasProceduralBlinkFaceMovement(bool hasProceduralBlinkFaceMovement);
    bool getHasProceduralBlinkFaceMovement() const override { return _headData->getHasProceduralBlinkFaceMovement(); }
    void setHasProceduralEyeFaceMovement(bool hasProceduralEyeFaceMovement);
    bool getHasProceduralEyeFaceMovement() const override { return _headData->getHasProceduralEyeFaceMovement(); }
    void setHasAudioEnabledFaceMovement(bool hasAudioEnabledFaceMovement);
    bool getHasAudioEnabledFaceMovement() const override { return _headData->getHasAudioEnabledFaceMovement(); }

     /**jsdoc
     * Potentially Very Expensive.  Do not use.
     * @function Avatar.getAvatarEntityData
     * @returns {object}
     */
    Q_INVOKABLE AvatarEntityMap getAvatarEntityData() const override;

    /**jsdoc
    * @function MyAvatar.setAvatarEntityData
    * @param {object} avatarEntityData
    */
    Q_INVOKABLE void setAvatarEntityData(const AvatarEntityMap& avatarEntityData) override;

    /**jsdoc
     * @function MyAvatar.updateAvatarEntity
     * @param {Uuid} entityID
     * @param {string} entityData
     */
    Q_INVOKABLE void updateAvatarEntity(const QUuid& entityID, const QByteArray& entityData) override;

public slots:
    void update(float deltatime);

    /**jsdoc
    * @function MyAvatar.setJointMappingsFromNetworkReply
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
};

#endif // hifi_ScriptableAvatar_h
