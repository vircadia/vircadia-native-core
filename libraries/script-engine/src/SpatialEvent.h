//
//  SpatialEvent.h
//  script-engine/src
//
//  Created by Stephen Birarda on 2014-10-27.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/// @addtogroup ScriptEngine
/// @{

#ifndef hifi_SpatialEvent_h
#define hifi_SpatialEvent_h

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <qscriptengine.h>

/// [unused] Represents a spatial event to the scripting engine
class SpatialEvent {
public:
    SpatialEvent();
    SpatialEvent(const SpatialEvent& other);
    
    static QScriptValue toScriptValue(QScriptEngine* engine, const SpatialEvent& event);
    static void fromScriptValue(const QScriptValue& object, SpatialEvent& event);
    
    glm::vec3 locTranslation;
    glm::quat locRotation;
    glm::vec3 absTranslation;
    glm::quat absRotation;
};

Q_DECLARE_METATYPE(SpatialEvent)

#endif // hifi_SpatialEvent_h

/// @}
