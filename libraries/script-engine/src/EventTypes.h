//
//  EventTypes.h
//  libraries/script-engine/src
//
//  Created by Brad Hefta-Gaub on 1/28/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_EventTypes_h
#define hifi_EventTypes_h

#include <glm/gtc/quaternion.hpp>

#include <QtScript/QScriptEngine>

#include <QWheelEvent>

class SpatialEvent {
public:
    SpatialEvent();
    SpatialEvent(const SpatialEvent& other);

    glm::vec3 locTranslation;
    glm::quat locRotation;
    glm::vec3 absTranslation;
    glm::quat absRotation;

private:
};

Q_DECLARE_METATYPE(SpatialEvent)

void registerEventTypes(QScriptEngine* engine);


QScriptValue spatialEventToScriptValue(QScriptEngine* engine, const SpatialEvent& event);
void spatialEventFromScriptValue(const QScriptValue& object, SpatialEvent& event);

#endif // hifi_EventTypes_h
