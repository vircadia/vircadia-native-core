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
#include <QtCore/QUuid>

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
Q_DECLARE_METATYPE(QVector<glm::vec3>)
Q_DECLARE_METATYPE(QVector<float>)

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

QScriptValue qVectorVec3ToScriptValue(QScriptEngine* engine, const QVector<glm::vec3>& vector);
void qVectorVec3FromScriptValue(const QScriptValue& array, QVector<glm::vec3>& vector);
QVector<glm::vec3> qVectorVec3FromScriptValue(const QScriptValue& array);

QScriptValue qVectorFloatToScriptValue(QScriptEngine* engine, const QVector<float>& vector);
void qVectorFloatFromScriptValue(const QScriptValue& array, QVector<float>& vector);
QVector<float> qVectorFloatFromScriptValue(const QScriptValue& array);

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

enum ContactEventType {
    CONTACT_EVENT_TYPE_START,
    CONTACT_EVENT_TYPE_CONTINUE,
    CONTACT_EVENT_TYPE_END
};

class Collision {
public:
    Collision() : type(CONTACT_EVENT_TYPE_START), idA(), idB(), contactPoint(0.0f), penetration(0.0f), velocityChange(0.0f) { }
    Collision(ContactEventType cType, const QUuid& cIdA, const QUuid& cIdB, const glm::vec3& cPoint,
              const glm::vec3& cPenetration, const glm::vec3& velocityChange)
    :   type(cType), idA(cIdA), idB(cIdB), contactPoint(cPoint), penetration(cPenetration), velocityChange(velocityChange) { }

    ContactEventType type;
    QUuid idA;
    QUuid idB;
    glm::vec3 contactPoint;
    glm::vec3 penetration;
    glm::vec3 velocityChange;
};
Q_DECLARE_METATYPE(Collision)
QScriptValue collisionToScriptValue(QScriptEngine* engine, const Collision& collision);
void collisionFromScriptValue(const QScriptValue &object, Collision& collision);

//Q_DECLARE_METATYPE(QUuid) // don't need to do this for QUuid since it's already a meta type
QScriptValue quuidToScriptValue(QScriptEngine* engine, const QUuid& uuid);
void quuidFromScriptValue(const QScriptValue& object, QUuid& uuid);

//Q_DECLARE_METATYPE(QSizeF) // Don't need to to this becase it's arleady a meta type
QScriptValue qSizeFToScriptValue(QScriptEngine* engine, const QSizeF& qSizeF);
void qSizeFFromScriptValue(const QScriptValue& object, QSizeF& qSizeF);

#endif // hifi_RegisteredMetaTypes_h
