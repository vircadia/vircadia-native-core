//
//  ScriptAvatarData.h
//  libraries/script-engine/src
//
//  Created by Zach Fox on 2017-04-10.
//  Copyright 2017 High Fidelity, Inc.
//  Copyright 2021 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ScriptAvatarData_h
#define hifi_ScriptAvatarData_h

#include <QtCore/QObject>

#include "AvatarData.h"

class ScriptAvatarData : public QObject {
    Q_OBJECT

    //
    // PHYSICAL PROPERTIES: POSITION AND ORIENTATION
    //
    Q_PROPERTY(glm::vec3 position READ getPosition)
    Q_PROPERTY(float scale READ getTargetScale)
    Q_PROPERTY(glm::vec3 handPosition READ getHandPosition)
    Q_PROPERTY(float bodyPitch READ getBodyPitch)
    Q_PROPERTY(float bodyYaw READ getBodyYaw)
    Q_PROPERTY(float bodyRoll READ getBodyRoll)
    Q_PROPERTY(glm::quat orientation READ getOrientation)
    Q_PROPERTY(glm::quat headOrientation READ getHeadOrientation)
    Q_PROPERTY(float headPitch READ getHeadPitch)
    Q_PROPERTY(float headYaw READ getHeadYaw)
    Q_PROPERTY(float headRoll READ getHeadRoll)
    //
    // PHYSICAL PROPERTIES: VELOCITY
    //
    Q_PROPERTY(glm::vec3 velocity READ getVelocity)
    Q_PROPERTY(glm::vec3 angularVelocity READ getAngularVelocity)

    //
    // IDENTIFIER PROPERTIES
    //
    Q_PROPERTY(QUuid sessionUUID READ getSessionUUID)
    Q_PROPERTY(QString displayName READ getDisplayName NOTIFY displayNameChanged)
    Q_PROPERTY(QString sessionDisplayName READ getSessionDisplayName NOTIFY sessionDisplayNameChanged)
    Q_PROPERTY(bool isReplicated READ getIsReplicated)
    Q_PROPERTY(bool lookAtSnappingEnabled READ getLookAtSnappingEnabled NOTIFY lookAtSnappingChanged)

    //
    // ATTACHMENT AND JOINT PROPERTIES
    //
    Q_PROPERTY(QString skeletonModelURL READ getSkeletonModelURLFromScript NOTIFY skeletonModelURLChanged)
    Q_PROPERTY(QVector<AttachmentData> attachmentData READ getAttachmentData)
    Q_PROPERTY(QStringList jointNames READ getJointNames)

    //
    // AUDIO PROPERTIES
    //
    Q_PROPERTY(float audioLoudness READ getAudioLoudness)
    Q_PROPERTY(float audioAverageLoudness READ getAudioAverageLoudness)

    //
    // MATRIX PROPERTIES
    //
    Q_PROPERTY(glm::mat4 sensorToWorldMatrix READ getSensorToWorldMatrix)
    Q_PROPERTY(glm::mat4 controllerLeftHandMatrix READ getControllerLeftHandMatrix)
    Q_PROPERTY(glm::mat4 controllerRightHandMatrix READ getControllerRightHandMatrix)

    Q_PROPERTY(bool hasPriority READ getHasPriority)

public:
    ScriptAvatarData(AvatarSharedPointer avatarData);

    //
    // PHYSICAL PROPERTIES: POSITION AND ORIENTATION
    //
    glm::vec3 getPosition() const;
    float getTargetScale() const;
    glm::vec3 getHandPosition() const;
    float getBodyPitch() const;
    float getBodyYaw() const;
    float getBodyRoll() const;
    glm::quat getOrientation() const;
    glm::quat getHeadOrientation() const;
    float getHeadPitch() const;
    float getHeadYaw() const;
    float getHeadRoll() const;
    //
    // PHYSICAL PROPERTIES: VELOCITY
    //
    glm::vec3 getVelocity() const;
    glm::vec3 getAngularVelocity() const;

    //
    // IDENTIFIER PROPERTIES
    //
    QUuid getSessionUUID() const;
    QString getDisplayName() const;
    QString getSessionDisplayName() const;
    bool getIsReplicated() const;
    bool getLookAtSnappingEnabled() const;

    //
    // ATTACHMENT AND JOINT PROPERTIES
    //
    QString getSkeletonModelURLFromScript() const;

    /*@jsdoc
     * Gets the pointing state of the hands to control where the laser emanates from. If the right index finger is pointing, the
     * laser emanates from the tip of that finger, otherwise it emanates from the palm.
     * @function ScriptAvatar.getHandState
     * @returns {HandState|number} The pointing state of the hand, or <code>-1</code> if the avatar data aren't available.
     */
    Q_INVOKABLE char getHandState() const;

    /*@jsdoc
     * Gets the rotation of a joint relative to its parent. For information on the joint hierarchy used, see
     * <a href="https://docs.vircadia.com/create/avatars/avatar-standards.html">Avatar Standards</a>.
     * @function ScriptAvatar.getJointRotation
     * @param {number} index - The index of the joint.
     * @returns {Quat} The rotation of the joint relative to its parent, or {@link Quat(0)|Quat.IDENTITY} if the avatar data
     *     aren't available.
     */
    Q_INVOKABLE glm::quat getJointRotation(int index) const;

    /*@jsdoc
     * Gets the translation of a joint relative to its parent, in model coordinates.
     * <p><strong>Warning:</strong> These coordinates are not necessarily in meters.</p>
     * <p>For information on the joint hierarchy used, see
     * <a href="https://docs.vircadia.com/create/avatars/avatar-standards.html">Avatar Standards</a>.</p>
     * @function ScriptAvatar.getJointTranslation
     * @param {number} index - The index of the joint.
     * @returns {Vec3} The translation of the joint relative to its parent, in model coordinates, or {@link Vec3(0)|Vec3.ZERO}
     *     if the avatar data aren't available.
     */
    Q_INVOKABLE glm::vec3 getJointTranslation(int index) const;

    /*@jsdoc
     * Gets the rotation of a joint relative to its parent. For information on the joint hierarchy used, see
     * <a href="https://docs.vircadia.com/create/avatars/avatar-standards.html">Avatar Standards</a>.
     * @function ScriptAvatar.getJointRotation
     * @param {string} name - The name of the joint.
     * @returns {Quat} The rotation of the joint relative to its parent, or {@link Quat(0)|Quat.IDENTITY} if the avatar data
     *     aren't available.
     */
    Q_INVOKABLE glm::quat getJointRotation(const QString& name) const;

    /*@jsdoc
     * Gets the translation of a joint relative to its parent, in model coordinates.
     * <p><strong>Warning:</strong> These coordinates are not necessarily in meters.</p>
     * <p>For information on the joint hierarchy used, see
     * <a href="https://docs.vircadia.com/create/avatars/avatar-standards.html">Avatar Standards</a>.</p>
     * @function ScriptAvatar.getJointTranslation
     * @param {number} name - The name of the joint.
     * @returns {Vec3} The translation of the joint relative to its parent, in model coordinates, or {@link Vec3(0)|Vec3.ZERO}
     *     if the avatar data aren't available.
     */
    Q_INVOKABLE glm::vec3 getJointTranslation(const QString& name) const;

    /*@jsdoc
     * Gets the rotations of all joints in the avatar. Each joint's rotation is relative to its parent joint.
     * @function ScriptAvatar.getJointRotations
     * @returns {Quat[]} The rotations of all joints relative to each's parent, or <code>[]</code> if the avatar data aren't
     *     available. The values are in the same order as the array returned by {@link ScriptAvatar.getJointNames}.
     */
    Q_INVOKABLE QVector<glm::quat> getJointRotations() const;

    /*@jsdoc
     * Gets the translations of all joints in the avatar. Each joint's translation is relative to its parent joint, in
     * model coordinates.
     * <p><strong>Warning:</strong> These coordinates are not necessarily in meters.</p>
     * @function ScriptAvatar.getJointTranslations
     * @returns {Vec3[]} The translations of all joints relative to each's parent, in model coordinates, or <code>[]</code> if
     *     the avatar data aren't available. The values are in the same order as the array returned by
     *     {@link ScriptAvatar.getJointNames}.
     */
    Q_INVOKABLE QVector<glm::vec3> getJointTranslations() const;

    /*@jsdoc
     * Checks that the data for a joint are valid.
     * @function ScriptAvatar.isJointDataValid
     * @param {number} index - The index of the joint.
     * @returns {boolean} <code>true</code> if the joint data are valid, <code>false</code> if not or the avatar data aren't
     *     available.
     */
    Q_INVOKABLE bool isJointDataValid(const QString& name) const;

    /*@jsdoc
     * Gets the joint index for a named joint. The joint index value is the position of the joint in the array returned by
     * {@linkScriptAvatar.getJointNames}.
     * @function ScriptAvatar.getJointIndex
     * @param {string} name - The name of the joint.
     * @returns {number} The index of the joint if valid and avatar data are available, otherwise <code>-1</code>.
     */
    Q_INVOKABLE int getJointIndex(const QString& name) const;

    /*@jsdoc
     * Gets the names of all the joints in the avatar.
     * @function ScriptAvatar.getJointNames
     * @returns {string[]} The joint names, or <code>[]</code> if the avatar data aren't available.
     */
    Q_INVOKABLE QStringList getJointNames() const;

    /*@jsdoc
     * Gets information about the models currently attached to the avatar.
     * @function ScriptAvatar.getAttachmentData
     * @returns {AttachmentData[]} Information about all models attached to the avatar, or <code>[]</code> if the avatar data
     *     aren't available.
     * @deprecated This function is deprecated and will be removed. Use avatar entities instead.
     */
    Q_INVOKABLE QVector<AttachmentData> getAttachmentData() const;

#if DEV_BUILD || PR_BUILD
    Q_INVOKABLE AvatarEntityMap getAvatarEntities() const;
#endif

    //
    // AUDIO PROPERTIES
    //
    float getAudioLoudness() const;
    float getAudioAverageLoudness() const;

    //
    // MATRIX PROPERTIES
    //
    glm::mat4 getSensorToWorldMatrix() const;
    glm::mat4 getControllerLeftHandMatrix() const;
    glm::mat4 getControllerRightHandMatrix() const;

    bool getHasPriority() const;

signals:

    /*@jsdoc
     * Triggered when the avatar's <code>displayName</code> property value changes.
     * @function ScriptAvatar.displayNameChanged
     * @returns {Signal}
     */
    void displayNameChanged();

    /*@jsdoc
     * Triggered when the avatar's <code>sessionDisplayName</code> property value changes.
     * @function ScriptAvatar.sessionDisplayNameChanged
     * @returns {Signal}
     */
    void sessionDisplayNameChanged();

    /*@jsdoc
     * Triggered when the avatar's model (i.e., <code>skeletonModelURL</code> property value) changes.
     * @function ScriptAvatar.skeletonModelURLChanged
     * @returns {Signal}
     */
    void skeletonModelURLChanged();

    /*@jsdoc
     * Triggered when the avatar's <code>lookAtSnappingEnabled</code> property value changes.
     * @function ScriptAvatar.lookAtSnappingChanged
     * @param {boolean} enabled - <code>true</code> if look-at snapping is enabled, <code>false</code> if not.
     * @returns {Signal}
     */
    void lookAtSnappingChanged(bool enabled);

public slots:

    /*@jsdoc
     * Gets the rotation of a joint relative to the avatar.
     * @function ScriptAvatar.getAbsoluteJointRotationInObjectFrame
     * @param {number} index - The index of the joint.
     * @returns {Quat} The rotation of the joint relative to the avatar, or {@link Quat(0)|Quat.IDENTITY} if the avatar data
     *     aren't available.
     */
    glm::quat getAbsoluteJointRotationInObjectFrame(int index) const;

    /*@jsdoc
     * Gets the translation of a joint relative to the avatar.
     * @function ScriptAvatar.getAbsoluteJointTranslationInObjectFrame
     * @param {number} index - The index of the joint.
     * @returns {Vec3} The translation of the joint relative to the avatar, or {@link Vec3(0)|Vec3.ZERO} if the avatar data
     *     aren't available.
     */
    glm::vec3 getAbsoluteJointTranslationInObjectFrame(int index) const;

protected:
    std::weak_ptr<AvatarData> _avatarData;
};

#endif // hifi_ScriptAvatarData_h
