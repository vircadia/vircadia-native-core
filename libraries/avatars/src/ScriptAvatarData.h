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
    Q_PROPERTY(QString sessionDisplayName READ getSessionDisplayName)
    Q_PROPERTY(bool isReplicated READ getIsReplicated)
    Q_PROPERTY(bool lookAtSnappingEnabled READ getLookAtSnappingEnabled NOTIFY lookAtSnappingChanged)

    //
    // ATTACHMENT AND JOINT PROPERTIES
    //
    Q_PROPERTY(QString skeletonModelURL READ getSkeletonModelURLFromScript)
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
    
signals:
    void displayNameChanged();
    void lookAtSnappingChanged(bool enabled);

public slots:
    glm::quat getAbsoluteJointRotationInObjectFrame(int index) const;
    glm::vec3 getAbsoluteJointTranslationInObjectFrame(int index) const;

protected:
    std::weak_ptr<AvatarData> _avatarData;
};

#endif // hifi_ScriptAvatarData_h
