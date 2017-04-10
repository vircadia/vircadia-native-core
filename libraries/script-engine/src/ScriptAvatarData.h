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
#include <AvatarData.h>
#include <SpatiallyNestable.h>

class ScriptAvatarData : public QObject, public SpatiallyNestable {
    Q_OBJECT

    Q_PROPERTY(QUuid sessionUUID READ getSessionUUID)
    Q_PROPERTY(glm::vec3 position READ getPosition)
    Q_PROPERTY(float audioLoudness READ getAudioLoudness)
    Q_PROPERTY(float audioAverageLoudness READ getAudioAverageLoudness)
    Q_PROPERTY(QString displayName READ getDisplayName NOTIFY displayNameChanged)
    Q_PROPERTY(QString sessionDisplayName READ getSessionDisplayName)

public:
    ScriptAvatarData(AvatarSharedPointer avatarData);

    const QUuid getSessionUUID() const;

    using SpatiallyNestable::getPosition;
    virtual glm::vec3 getPosition() const override;

    const float getAudioLoudness();
    const float getAudioAverageLoudness();

    const QString getDisplayName();
    const QString getSessionDisplayName();

    int getFauxJointIndex(const QString& name) const;
    /// Returns the index of the joint with the specified name, or -1 if not found/unknown.
    Q_INVOKABLE virtual int getJointIndex(const QString& name) const;
    
signals:
    void displayNameChanged();

public slots:
    glm::quat getAbsoluteJointRotationInObjectFrame(int index) const;
    glm::vec3 getAbsoluteJointTranslationInObjectFrame(int index) const;

private:
    std::weak_ptr<AvatarData> _avatarData;
};

Q_DECLARE_METATYPE(AvatarSharedPointer)

QScriptValue avatarDataToScriptValue(QScriptEngine* engine, const AvatarSharedPointer& in);
void avatarDataFromScriptValue(const QScriptValue& object, AvatarSharedPointer& out);

#endif // hifi_ScriptAvatarData_h
