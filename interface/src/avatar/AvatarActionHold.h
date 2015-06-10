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
#include <ObjectAction.h>

class AvatarActionHold : public ObjectAction {
public:
    AvatarActionHold(QUuid id, EntityItemPointer ownerEntity);
    virtual ~AvatarActionHold();

    virtual bool updateArguments(QVariantMap arguments);
    virtual void updateActionWorker(float deltaTimeStep);

private:

    glm::vec3 _linearOffset;
    bool _linearOffsetSet;

    glm::quat _angularOffset;
    bool _angularOffsetSet;

    float _timeScale = 0.01;
};

#endif // hifi_AvatarActionHold_h
