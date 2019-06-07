//
//  ScriptAvatarData.h
//  libraries/script-engine/src
//
//  Created by Zach Fox on 2017-04-10.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ScriptAvatarData_h
#define hifi_ScriptAvatarData_h

#include <QtCore/QObject>

#include "AvatarData.h"

/**jsdoc
 * Information about an avatar.
 * @typedef {object} AvatarData
 * @property {Vec3} position - The avatar's position.
 * @property {number} scale - The target scale of the avatar without any restrictions on permissible values imposed by the 
 *     domain.
 * @property {Vec3} handPosition - A user-defined hand position, in world coordinates. The position moves with the avatar but 
 *    is otherwise not used or changed by Interface.
 * @property {number} bodyPitch - The pitch of the avatar's body, in degrees.
 * @property {number} bodyYaw - The yaw of the avatar's body, in degrees.
 * @property {number} bodyRoll - The roll of the avatar's body, in degrees.
 * @property {Quat} orientation - The orientation of the avatar's body.
 * @property {Quat} headOrientation - The orientation of the avatar's head.
 * @property {number} headPitch - The pitch of the avatar's head relative to the body, in degrees.
 * @property {number} headYaw - The yaw of the avatar's head relative to the body, in degrees.
 * @property {number} headRoll - The roll of the avatar's head relative to the body, in degrees.
 *
 * @property {Vec3} velocity - The linear velocity of the avatar.
 * @property {Vec3} angularVelocity - The angular velocity of the avatar.
 *
 * @property {Uuid} sessionUUID - The avatar's session ID.
 * @property {string} displayName - The avatar's display name.
 * @property {string} sessionDisplayName - The avatar's display name, sanitized and versioned, as defined by the avatar mixer. 
 *     It is unique among all avatars present in the domain at the time.
 * @property {boolean} isReplicated - <span class="important">Deprecated: This property is deprecated and will be 
 *     removed.</span>
 * @property {boolean} lookAtSnappingEnabled - <code>true</code> if the avatar's eyes snap to look at another avatar's eyes 
 *     when the other avatar is in the line of sight and also has <code>lookAtSnappingEnabled == true</code>.
 *
 * @property {string} skeletonModelURL - The avatar's FST file.
 * @property {AttachmentData[]} attachmentData - Information on the avatar's attachments.
 *     <p class="important">Deprecated: This property is deprecated and will be removed. Use avatar entities instead.</p>
 * @property {string[]} jointNames - The list of joints in the current avatar model.
 *
 * @property {number} audioLoudness - The instantaneous loudness of the audio input that the avatar is injecting into the 
 *     domain.
 * @property {number} audioAverageLoudness - The rolling average loudness of the audio input that the avatar is injecting into 
 *     the domain.
 *
 * @property {Mat4} sensorToWorldMatrix - The scale, rotation, and translation transform from the user's real world to the 
 *     avatar's size, orientation, and position in the virtual world.
 * @property {Mat4} controllerLeftHandMatrix - The rotation and translation of the left hand controller relative to the avatar.
 * @property {Mat4} controllerRightHandMatrix - The rotation and translation of the right hand controller relative to the 
 *     avatar.
 *
 * @property {boolean} hasPriority - <code>true</code> if the avatar is in a "hero" zone, <code>false</code> if it isn't. 
 */
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
    Q_INVOKABLE char getHandState() const;
    Q_INVOKABLE glm::quat getJointRotation(int index) const;
    Q_INVOKABLE glm::vec3 getJointTranslation(int index) const;
    Q_INVOKABLE glm::quat getJointRotation(const QString& name) const;
    Q_INVOKABLE glm::vec3 getJointTranslation(const QString& name) const;
    Q_INVOKABLE QVector<glm::quat> getJointRotations() const;
    Q_INVOKABLE QVector<glm::vec3> getJointTranslations() const;
    Q_INVOKABLE bool isJointDataValid(const QString& name) const;
    Q_INVOKABLE int getJointIndex(const QString& name) const;
    Q_INVOKABLE QStringList getJointNames() const;
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
    void displayNameChanged();
    void sessionDisplayNameChanged();
    void skeletonModelURLChanged();
    void lookAtSnappingChanged(bool enabled);

public slots:
    glm::quat getAbsoluteJointRotationInObjectFrame(int index) const;
    glm::vec3 getAbsoluteJointTranslationInObjectFrame(int index) const;

protected:
    std::weak_ptr<AvatarData> _avatarData;
};

#endif // hifi_ScriptAvatarData_h
