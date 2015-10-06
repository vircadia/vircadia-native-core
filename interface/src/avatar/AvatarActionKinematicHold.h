//
//  AvatarActionKinematicHold.h
//  interface/src/avatar/
//
//  Created by Seth Alves 2015-10-2
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AvatarActionKinematicHold_h
#define hifi_AvatarActionKinematicHold_h

#include <QUuid>

#include <EntityItem.h>
#include <ObjectActionSpring.h>

class AvatarActionKinematicHold : public ObjectActionSpring {
public:
    AvatarActionKinematicHold(const QUuid& id, EntityItemPointer ownerEntity);
    virtual ~AvatarActionKinematicHold();

    virtual bool updateArguments(QVariantMap arguments);
    virtual QVariantMap getArguments();

    virtual void updateActionWorker(float deltaTimeStep);

    virtual void deserialize(QByteArray serializedArguments);

private:
    static const uint16_t holdVersion;

    glm::vec3 _relativePosition;
    glm::quat _relativeRotation;
    QString _hand;
    bool _mine = false;

    bool _previousSet = false;
    glm::vec3 _previousPositionalTarget;
    glm::quat _previousRotationalTarget;

    bool _setVelocity = false;
};

#endif // hifi_AvatarActionKinematicHold_h
