//
//  RegisteredMetaTypes.h
//  libraries/shared/src
//
//  Created by Stephen Birarda on 10/3/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_RegisteredMetaTypes_h
#define hifi_RegisteredMetaTypes_h

#include <glm/glm.hpp>

#include <QtScript/QScriptEngine>

#include "CollisionInfo.h"
#include "SharedUtil.h"

class QColor;

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

QScriptValue qColorToScriptValue(QScriptEngine* engine, const QColor& color);
void qColorFromScriptValue(const QScriptValue& object, QColor& color);

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

#endif // hifi_RegisteredMetaTypes_h
