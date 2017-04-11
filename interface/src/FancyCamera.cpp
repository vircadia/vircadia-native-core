//
//  Camera.cpp
//  interface/src
//
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

//#include <glm/gtx/quaternion.hpp>

//#include <SharedUtil.h>
//#include <EventTypes.h>

#include "Application.h"
#include "FancyCamera.h"
//#include "Menu.h"
//#include "Util.h"


QUuid FancyCamera::getCameraEntity() const {
    if (_cameraEntity != nullptr) {
        return _cameraEntity->getID();
    }
    return QUuid();
};

void FancyCamera::setCameraEntity(QUuid entityID) {
    _cameraEntity = qApp->getEntities()->getTree()->findEntityByID(entityID);
}
