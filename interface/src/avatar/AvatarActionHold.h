//
//  AvatarActionHold.h
//  interface/src/avatar/
//
//  Created by Seth Alves 2015-6-9
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AvatarActionHold_h
#define hifi_AvatarActionHold_h

#include <QUuid>

#include <EntityItem.h>
#include <ObjectActionSpring.h>

class AvatarActionHold : public ObjectActionSpring {
public:
    AvatarActionHold(const QUuid& id, EntityItemPointer ownerEntity);
    virtual ~AvatarActionHold();

    virtual bool updateArguments(QVariantMap arguments);
    virtual QVariantMap getArguments();

    virtual void updateActionWorker(float deltaTimeStep);

    QByteArray serialize() const;
    virtual void deserialize(QByteArray serializedArguments);

    virtual bool shouldSuppressLocationEdits() { return true; }

private:
    static const uint16_t holdVersion;

    glm::vec3 _relativePosition;
    glm::quat _relativeRotation;
    QString _hand;
    QUuid _holderID;

    void doKinematicUpdate(float deltaTimeStep);
    bool _kinematic { false };
    bool _kinematicSetVelocity { false };
    bool _previousSet { false };
    glm::vec3 _previousPositionalTarget;
    glm::quat _previousRotationalTarget;
};

#endif // hifi_AvatarActionHold_h
