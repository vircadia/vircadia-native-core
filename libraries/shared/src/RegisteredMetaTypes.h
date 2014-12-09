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

#include <QtScript/QScriptEngine>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "SharedUtil.h"

class QColor;
class QUrl;

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

QScriptValue qRectToScriptValue(QScriptEngine* engine, const QRect& rect);
void qRectFromScriptValue(const QScriptValue& object, QRect& rect);

QScriptValue xColorToScriptValue(QScriptEngine* engine, const xColor& color);
void xColorFromScriptValue(const QScriptValue &object, xColor& color);

QScriptValue qColorToScriptValue(QScriptEngine* engine, const QColor& color);
void qColorFromScriptValue(const QScriptValue& object, QColor& color);

QScriptValue qURLToScriptValue(QScriptEngine* engine, const QUrl& url);
void qURLFromScriptValue(const QScriptValue& object, QUrl& url);

class PickRay {
public:
    PickRay() : origin(0.0f), direction(0.0f)  { }
    PickRay(const glm::vec3& origin, const glm::vec3 direction) : origin(origin), direction(direction) {}
    glm::vec3 origin;
    glm::vec3 direction;
};
Q_DECLARE_METATYPE(PickRay)
QScriptValue pickRayToScriptValue(QScriptEngine* engine, const PickRay& pickRay);
void pickRayFromScriptValue(const QScriptValue& object, PickRay& pickRay);

class Collision {
public:
    Collision() : contactPoint(0.0f), penetration(0.0f) { }
    Collision(const glm::vec3& contactPoint, const glm::vec3& penetration) : 
                        contactPoint(contactPoint), penetration(penetration) { }
    glm::vec3 contactPoint;
    glm::vec3 penetration;
};
Q_DECLARE_METATYPE(Collision)
QScriptValue collisionToScriptValue(QScriptEngine* engine, const Collision& collision);
void collisionFromScriptValue(const QScriptValue &object, Collision& collision);

//Q_DECLARE_METATYPE(QUuid) // don't need to do this for QUuid since it's already a meta type
QScriptValue quuidToScriptValue(QScriptEngine* engine, const QUuid& uuid);
void quuidFromScriptValue(const QScriptValue& object, QUuid& uuid);

#endif // hifi_RegisteredMetaTypes_h
