//
//  AvatarActionFarGrab.h
//  interface/src/avatar/
//
//  Created by Seth Alves 2017-4-14
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AvatarActionFarGrab_h
#define hifi_AvatarActionFarGrab_h

#include <EntityItem.h>
#include <ObjectActionSpring.h>

class AvatarActionFarGrab : public ObjectActionSpring {
public:
    AvatarActionFarGrab(const QUuid& id, EntityItemPointer ownerEntity);
    virtual ~AvatarActionFarGrab();

    QByteArray serialize() const override;
    virtual void deserialize(QByteArray serializedArguments) override;
};

#endif // hifi_AvatarActionFarGrab_h
