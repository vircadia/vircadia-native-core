//
//  RegisteredMetaTypes.cpp
//  hifi
//
//  Created by Stephen Birarda on 10/3/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//
//  Used to register meta-types with Qt so that they can be used as properties for objects exposed to our
//  Agent scripting.
//

#include "RegisteredMetaTypes.h"

static int vec4MetaTypeId = qRegisterMetaType<glm::vec4>();
static int vec3MetaTypeId = qRegisterMetaType<glm::vec3>();
static int vec2MetaTypeId = qRegisterMetaType<glm::vec2>();
static int quatMetaTypeId = qRegisterMetaType<glm::quat>();
static int xColorMetaTypeId = qRegisterMetaType<xColor>();
static int pickRayMetaTypeId = qRegisterMetaType<PickRay>();
static int collisionMetaTypeId = qRegisterMetaType<CollisionInfo>();

void registerMetaTypes(QScriptEngine* engine) {
    qScriptRegisterMetaType(engine, vec4toScriptValue, vec4FromScriptValue);
    qScriptRegisterMetaType(engine, vec3toScriptValue, vec3FromScriptValue);
    qScriptRegisterMetaType(engine, vec2toScriptValue, vec2FromScriptValue);
    qScriptRegisterMetaType(engine, quatToScriptValue, quatFromScriptValue);
    qScriptRegisterMetaType(engine, xColorToScriptValue, xColorFromScriptValue);
    qScriptRegisterMetaType(engine, pickRayToScriptValue, pickRayFromScriptValue);
    qScriptRegisterMetaType(engine, collisionToScriptValue, collisionFromScriptValue);
}

QScriptValue vec4toScriptValue(QScriptEngine* engine, const glm::vec4& vec4) {
    QScriptValue obj = engine->newObject();
    obj.setProperty("x", vec4.x);
    obj.setProperty("y", vec4.y);
    obj.setProperty("z", vec4.z);
    obj.setProperty("w", vec4.w);
    return obj;
}

void vec4FromScriptValue(const QScriptValue& object, glm::vec4& vec4) {
    vec4.x = object.property("x").toVariant().toFloat();
    vec4.y = object.property("y").toVariant().toFloat();
    vec4.z = object.property("z").toVariant().toFloat();
    vec4.w = object.property("w").toVariant().toFloat();
}

QScriptValue vec3toScriptValue(QScriptEngine* engine, const glm::vec3 &vec3) {
    QScriptValue obj = engine->newObject();
    obj.setProperty("x", vec3.x);
    obj.setProperty("y", vec3.y);
    obj.setProperty("z", vec3.z);
    return obj;
}

void vec3FromScriptValue(const QScriptValue &object, glm::vec3 &vec3) {
    vec3.x = object.property("x").toVariant().toFloat();
    vec3.y = object.property("y").toVariant().toFloat();
    vec3.z = object.property("z").toVariant().toFloat();
}

QScriptValue vec2toScriptValue(QScriptEngine* engine, const glm::vec2 &vec2) {
    QScriptValue obj = engine->newObject();
    obj.setProperty("x", vec2.x);
    obj.setProperty("y", vec2.y);
    return obj;
}

void vec2FromScriptValue(const QScriptValue &object, glm::vec2 &vec2) {
    vec2.x = object.property("x").toVariant().toFloat();
    vec2.y = object.property("y").toVariant().toFloat();
}

QScriptValue quatToScriptValue(QScriptEngine* engine, const glm::quat& quat) {
    QScriptValue obj = engine->newObject();
    obj.setProperty("x", quat.x);
    obj.setProperty("y", quat.y);
    obj.setProperty("z", quat.z);
    obj.setProperty("w", quat.w);
    return obj;
}

void quatFromScriptValue(const QScriptValue &object, glm::quat& quat) {
    quat.x = object.property("x").toVariant().toFloat();
    quat.y = object.property("y").toVariant().toFloat();
    quat.z = object.property("z").toVariant().toFloat();
    quat.w = object.property("w").toVariant().toFloat();
}

QScriptValue xColorToScriptValue(QScriptEngine *engine, const xColor& color) {
    QScriptValue obj = engine->newObject();
    obj.setProperty("red", color.red);
    obj.setProperty("green", color.green);
    obj.setProperty("blue", color.blue);
    return obj;
}

void xColorFromScriptValue(const QScriptValue &object, xColor& color) {
    color.red = object.property("red").toVariant().toInt();
    color.green = object.property("green").toVariant().toInt();
    color.blue = object.property("blue").toVariant().toInt();
}

QScriptValue pickRayToScriptValue(QScriptEngine* engine, const PickRay& pickRay) {
    QScriptValue obj = engine->newObject();
    QScriptValue origin = vec3toScriptValue(engine, pickRay.origin);
    obj.setProperty("origin", origin);
    QScriptValue direction = vec3toScriptValue(engine, pickRay.direction);
    obj.setProperty("direction", direction);
    return obj;
}

void pickRayFromScriptValue(const QScriptValue& object, PickRay& pickRay) {
    QScriptValue originValue = object.property("origin");
    if (originValue.isValid()) {
        pickRay.origin.x = originValue.property("x").toVariant().toFloat();
        pickRay.origin.y = originValue.property("y").toVariant().toFloat();
        pickRay.origin.z = originValue.property("z").toVariant().toFloat();
    }
    QScriptValue directionValue = object.property("direction");
    if (directionValue.isValid()) {
        pickRay.direction.x = directionValue.property("x").toVariant().toFloat();
        pickRay.direction.y = directionValue.property("y").toVariant().toFloat();
        pickRay.direction.z = directionValue.property("z").toVariant().toFloat();
    }
}

QScriptValue collisionToScriptValue(QScriptEngine* engine, const CollisionInfo& collision) {
    QScriptValue obj = engine->newObject();
    obj.setProperty("penetration", vec3toScriptValue(engine, collision._penetration));
    obj.setProperty("contactPoint", vec3toScriptValue(engine, collision._contactPoint));
    return obj;
}

void collisionFromScriptValue(const QScriptValue &object, CollisionInfo& collision) {
    // TODO: implement this when we know what it means to accept collision events from JS
}

