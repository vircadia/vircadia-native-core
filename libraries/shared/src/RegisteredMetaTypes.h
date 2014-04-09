//
//  RegisteredMetaTypes.h
//  hifi
//
//  Created by Stephen Birarda on 10/3/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//
//  Used to register meta-types with Qt so that they can be used as properties for objects exposed to our
//  Agent scripting.

#ifndef hifi_RegisteredMetaTypes_h
#define hifi_RegisteredMetaTypes_h

#include <glm/glm.hpp>

#include <QtScript/QScriptEngine>

#include "CollisionInfo.h"
#include "SharedUtil.h"

Q_DECLARE_METATYPE(glm::vec4)
Q_DECLARE_METATYPE(glm::vec3)
Q_DECLARE_METATYPE(glm::vec2)
Q_DECLARE_METATYPE(glm::quat)
Q_DECLARE_METATYPE(xColor)

void registerMetaTypes(QScriptEngine* engine);

QScriptValue vec4toScriptValue(QScriptEngine* engine, const glm::vec4& vec4);
void vec4FromScriptValue(const QScriptValue& object, glm::vec4& vec4);

QScriptValue vec3toScriptValue(QScriptEngine* engine, const glm::vec3 &vec3);
void vec3FromScriptValue(const QScriptValue &object, glm::vec3 &vec3);

QScriptValue vec2toScriptValue(QScriptEngine* engine, const glm::vec2 &vec2);
void vec2FromScriptValue(const QScriptValue &object, glm::vec2 &vec2);

QScriptValue quatToScriptValue(QScriptEngine* engine, const glm::quat& quat);
void quatFromScriptValue(const QScriptValue &object, glm::quat& quat);

QScriptValue xColorToScriptValue(QScriptEngine* engine, const xColor& color);
void xColorFromScriptValue(const QScriptValue &object, xColor& color);

class PickRay {
public:
    PickRay() : origin(0), direction(0)  { }; 
    glm::vec3 origin;
    glm::vec3 direction;
};
Q_DECLARE_METATYPE(PickRay)
QScriptValue pickRayToScriptValue(QScriptEngine* engine, const PickRay& pickRay);
void pickRayFromScriptValue(const QScriptValue& object, PickRay& pickRay);

Q_DECLARE_METATYPE(CollisionInfo)
QScriptValue collisionToScriptValue(QScriptEngine* engine, const CollisionInfo& collision);
void collisionFromScriptValue(const QScriptValue &object, CollisionInfo& collision);

#endif
