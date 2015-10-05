//
//  EntityRig.h
//  libraries/animation/src/
//
//  Created by SethAlves on 2015-7-22.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_EntityRig_h
#define hifi_EntityRig_h

#include <QObject>

#include "Rig.h"

class EntityRig : public Rig {
    Q_OBJECT

 public:
    ~EntityRig() {}
    virtual void updateJointState(int index, glm::mat4 rootTransform);
    virtual void setHandPosition(int jointIndex, const glm::vec3& position, const glm::quat& rotation,
                                 float scale, float priority) {}
    virtual void setJointState(int index, bool valid, const glm::quat& rotation, const glm::vec3& translation, float priority);
};

#endif // hifi_EntityRig_h
