//
//  RegisteredMetaTypes.cpp
//  libraries/shared/src
//
//  Created by Stephen Birarda on 10/3/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QColor>
#include <QUrl>
#include <QUuid>
#include <QRect>
#include <glm/gtc/quaternion.hpp>

#include "RegisteredMetaTypes.h"

static int vec4MetaTypeId = qRegisterMetaType<glm::vec4>();
static int vec3MetaTypeId = qRegisterMetaType<glm::vec3>();
static int qVectorVec3MetaTypeId = qRegisterMetaType<QVector<glm::vec3>>();
static int qVectorQuatMetaTypeId = qRegisterMetaType<QVector<glm::quat>>();
static int qVectorBoolMetaTypeId = qRegisterMetaType<QVector<bool>>();
static int vec2MetaTypeId = qRegisterMetaType<glm::vec2>();
static int quatMetaTypeId = qRegisterMetaType<glm::quat>();
static int xColorMetaTypeId = qRegisterMetaType<xColor>();
static int pickRayMetaTypeId = qRegisterMetaType<PickRay>();
static int collisionMetaTypeId = qRegisterMetaType<Collision>();
static int qMapURLStringMetaTypeId = qRegisterMetaType<QMap<QUrl,QString>>();

void registerMetaTypes(QScriptEngine* engine) {
    qScriptRegisterMetaType(engine, vec4toScriptValue, vec4FromScriptValue);
    qScriptRegisterMetaType(engine, vec3toScriptValue, vec3FromScriptValue);
    qScriptRegisterMetaType(engine, qVectorVec3ToScriptValue, qVectorVec3FromScriptValue);
    qScriptRegisterMetaType(engine, qVectorQuatToScriptValue, qVectorQuatFromScriptValue);
    qScriptRegisterMetaType(engine, qVectorBoolToScriptValue, qVectorBoolFromScriptValue);
    qScriptRegisterMetaType(engine, qVectorFloatToScriptValue, qVectorFloatFromScriptValue);
    qScriptRegisterMetaType(engine, vec2toScriptValue, vec2FromScriptValue);
    qScriptRegisterMetaType(engine, quatToScriptValue, quatFromScriptValue);
    qScriptRegisterMetaType(engine, qRectToScriptValue, qRectFromScriptValue);
    qScriptRegisterMetaType(engine, xColorToScriptValue, xColorFromScriptValue);
    qScriptRegisterMetaType(engine, qColorToScriptValue, qColorFromScriptValue);
    qScriptRegisterMetaType(engine, qURLToScriptValue, qURLFromScriptValue);
    qScriptRegisterMetaType(engine, pickRayToScriptValue, pickRayFromScriptValue);
    qScriptRegisterMetaType(engine, collisionToScriptValue, collisionFromScriptValue);
    qScriptRegisterMetaType(engine, quuidToScriptValue, quuidFromScriptValue);
    qScriptRegisterMetaType(engine, qSizeFToScriptValue, qSizeFFromScriptValue);
    qScriptRegisterMetaType(engine, aaCubeToScriptValue, aaCubeFromScriptValue);
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
    if (vec3.x != vec3.x || vec3.y != vec3.y || vec3.z != vec3.z) {
        // if vec3 contains a NaN don't try to convert it
        return obj;
    }
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

QScriptValue qVectorVec3ToScriptValue(QScriptEngine* engine, const QVector<glm::vec3>& vector) {
    QScriptValue array = engine->newArray();
    for (int i = 0; i < vector.size(); i++) {
        array.setProperty(i, vec3toScriptValue(engine, vector.at(i)));
    }
    return array;
}

QScriptValue quatToScriptValue(QScriptEngine* engine, const glm::quat &quat) {
    QScriptValue obj = engine->newObject();
    if (quat.x != quat.x || quat.y != quat.y || quat.z != quat.z || quat.w != quat.w) {
        // if quat contains a NaN don't try to convert it
        return obj;
    }
    obj.setProperty("x", quat.x);
    obj.setProperty("y", quat.y);
    obj.setProperty("z", quat.z);
    obj.setProperty("w", quat.w);
    return obj;
}

void quatFromScriptValue(const QScriptValue &object, glm::quat &quat) {
    quat.x = object.property("x").toVariant().toFloat();
    quat.y = object.property("y").toVariant().toFloat();
    quat.z = object.property("z").toVariant().toFloat();
    quat.w = object.property("w").toVariant().toFloat();
}

QScriptValue qVectorQuatToScriptValue(QScriptEngine* engine, const QVector<glm::quat>& vector) {
    QScriptValue array = engine->newArray();
    for (int i = 0; i < vector.size(); i++) {
        array.setProperty(i, quatToScriptValue(engine, vector.at(i)));
    }
    return array;
}

QScriptValue qVectorBoolToScriptValue(QScriptEngine* engine, const QVector<bool>& vector) {
    QScriptValue array = engine->newArray();
    for (int i = 0; i < vector.size(); i++) {
        array.setProperty(i, vector.at(i));
    }
    return array;
}

QVector<float> qVectorFloatFromScriptValue(const QScriptValue& array) {
    if(!array.isArray()) {
        return QVector<float>();
    }
    QVector<float> newVector;
    int length = array.property("length").toInteger();
    newVector.reserve(length);
    for (int i = 0; i < length; i++) {
        if(array.property(i).isNumber()) {
            newVector << array.property(i).toNumber();
        }
    }
    
    return newVector;
}

QVector<QUuid> qVectorQUuidFromScriptValue(const QScriptValue& array) {
    if (!array.isArray()) {
        return QVector<QUuid>(); 
    }
    QVector<QUuid> newVector;
    int length = array.property("length").toInteger();
    newVector.reserve(length);
    for (int i = 0; i < length; i++) {
        QString uuidAsString = array.property(i).toString();
        QUuid fromString(uuidAsString);
        newVector << fromString;
    }
    return newVector;
}

QScriptValue qVectorFloatToScriptValue(QScriptEngine* engine, const QVector<float>& vector) {
    QScriptValue array = engine->newArray();
    for (int i = 0; i < vector.size(); i++) {
        float num = vector.at(i);
        array.setProperty(i, QScriptValue(num));
    }
    return array;
}

void qVectorFloatFromScriptValue(const QScriptValue& array, QVector<float>& vector) {
    int length = array.property("length").toInteger();
    
    for (int i = 0; i < length; i++) {
        vector << array.property(i).toVariant().toFloat();
    }
}
//
QVector<glm::vec3> qVectorVec3FromScriptValue(const QScriptValue& array){
    QVector<glm::vec3> newVector;
    int length = array.property("length").toInteger();
    
    for (int i = 0; i < length; i++) {
        glm::vec3 newVec3 = glm::vec3();
        vec3FromScriptValue(array.property(i), newVec3);
        newVector << newVec3;
    }
    return newVector;
}

void qVectorVec3FromScriptValue(const QScriptValue& array, QVector<glm::vec3>& vector ) {
    int length = array.property("length").toInteger();

    for (int i = 0; i < length; i++) {
        glm::vec3 newVec3 = glm::vec3();
        vec3FromScriptValue(array.property(i), newVec3);
        vector << newVec3;
    }
}

QVector<glm::quat> qVectorQuatFromScriptValue(const QScriptValue& array){
    QVector<glm::quat> newVector;
    int length = array.property("length").toInteger();

    for (int i = 0; i < length; i++) {
        glm::quat newQuat = glm::quat();
        quatFromScriptValue(array.property(i), newQuat);
        newVector << newQuat;
    }
    return newVector;
}

void qVectorQuatFromScriptValue(const QScriptValue& array, QVector<glm::quat>& vector ) {
    int length = array.property("length").toInteger();

    for (int i = 0; i < length; i++) {
        glm::quat newQuat = glm::quat();
        quatFromScriptValue(array.property(i), newQuat);
        vector << newQuat;
    }
}

QVector<bool> qVectorBoolFromScriptValue(const QScriptValue& array){
    QVector<bool> newVector;
    int length = array.property("length").toInteger();

    for (int i = 0; i < length; i++) {
        newVector << array.property(i).toBool();
    }
    return newVector;
}

void qVectorBoolFromScriptValue(const QScriptValue& array, QVector<bool>& vector ) {
    int length = array.property("length").toInteger();

    for (int i = 0; i < length; i++) {
        vector << array.property(i).toBool();
    }
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

QScriptValue qRectToScriptValue(QScriptEngine* engine, const QRect& rect) {
    QScriptValue obj = engine->newObject();
    obj.setProperty("x", rect.x());
    obj.setProperty("y", rect.y());
    obj.setProperty("width", rect.width());
    obj.setProperty("height", rect.height());
    return obj;
}

void qRectFromScriptValue(const QScriptValue &object, QRect& rect) {
    rect.setX(object.property("x").toVariant().toInt());
    rect.setY(object.property("y").toVariant().toInt());
    rect.setWidth(object.property("width").toVariant().toInt());
    rect.setHeight(object.property("height").toVariant().toInt());
}

QScriptValue xColorToScriptValue(QScriptEngine *engine, const xColor& color) {
    QScriptValue obj = engine->newObject();
    obj.setProperty("red", color.red);
    obj.setProperty("green", color.green);
    obj.setProperty("blue", color.blue);
    return obj;
}

void xColorFromScriptValue(const QScriptValue &object, xColor& color) {
    if (!object.isValid()) {
        return;
    }
    if (object.isNumber()) {
        color.red = color.green = color.blue = (uint8_t)object.toUInt32();
    } else if (object.isString()) {
        QColor qcolor(object.toString());
        if (qcolor.isValid()) {
            color.red = (uint8_t)qcolor.red();
            color.blue = (uint8_t)qcolor.blue();
            color.green = (uint8_t)qcolor.green();
        }
    } else {
        color.red = object.property("red").toVariant().toInt();
        color.green = object.property("green").toVariant().toInt();
        color.blue = object.property("blue").toVariant().toInt();
    }
}

QScriptValue qColorToScriptValue(QScriptEngine* engine, const QColor& color) {
    QScriptValue object = engine->newObject();
    object.setProperty("red", color.red());
    object.setProperty("green", color.green());
    object.setProperty("blue", color.blue());
    object.setProperty("alpha", color.alpha());
    return object;
}

QScriptValue aaCubeToScriptValue(QScriptEngine* engine, const AACube& aaCube) {
    QScriptValue obj = engine->newObject();
    const glm::vec3& corner = aaCube.getCorner();
    obj.setProperty("x", corner.x);
    obj.setProperty("y", corner.y);
    obj.setProperty("z", corner.z);
    obj.setProperty("scale", aaCube.getScale());
    return obj;
}

void aaCubeFromScriptValue(const QScriptValue &object, AACube& aaCube) {
    glm::vec3 corner;
    corner.x = object.property("x").toVariant().toFloat();
    corner.y = object.property("y").toVariant().toFloat();
    corner.z = object.property("z").toVariant().toFloat();
    float scale = object.property("scale").toVariant().toFloat();

    aaCube.setBox(corner, scale);
}

void qColorFromScriptValue(const QScriptValue& object, QColor& color) {
    if (object.isNumber()) {
        color.setRgb(object.toUInt32());
    
    } else if (object.isString()) {
        color.setNamedColor(object.toString());
            
    } else {
        QScriptValue alphaValue = object.property("alpha");
        color.setRgb(object.property("red").toInt32(), object.property("green").toInt32(), object.property("blue").toInt32(),
            alphaValue.isNumber() ? alphaValue.toInt32() : 255);
    }
}

QScriptValue qURLToScriptValue(QScriptEngine* engine, const QUrl& url) {
    return url.toString();
}

void qURLFromScriptValue(const QScriptValue& object, QUrl& url) {
    url = object.toString();
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
        auto x = originValue.property("x");
        auto y = originValue.property("y");
        auto z = originValue.property("z");
        if (x.isValid() && y.isValid() && z.isValid()) {
            pickRay.origin.x = x.toVariant().toFloat();
            pickRay.origin.y = y.toVariant().toFloat();
            pickRay.origin.z = z.toVariant().toFloat();
        }
    }
    QScriptValue directionValue = object.property("direction");
    if (directionValue.isValid()) {
        auto x = directionValue.property("x");
        auto y = directionValue.property("y");
        auto z = directionValue.property("z");
        if (x.isValid() && y.isValid() && z.isValid()) {
            pickRay.direction.x = x.toVariant().toFloat();
            pickRay.direction.y = y.toVariant().toFloat();
            pickRay.direction.z = z.toVariant().toFloat();
        }
    }
}

QScriptValue collisionToScriptValue(QScriptEngine* engine, const Collision& collision) {
    QScriptValue obj = engine->newObject();
    obj.setProperty("type", collision.type);
    obj.setProperty("idA", quuidToScriptValue(engine, collision.idA));
    obj.setProperty("idB", quuidToScriptValue(engine, collision.idB));
    obj.setProperty("penetration", vec3toScriptValue(engine, collision.penetration));
    obj.setProperty("contactPoint", vec3toScriptValue(engine, collision.contactPoint));
    obj.setProperty("velocityChange", vec3toScriptValue(engine, collision.velocityChange));
    return obj;
}

void collisionFromScriptValue(const QScriptValue &object, Collision& collision) {
    // TODO: implement this when we know what it means to accept collision events from JS
}

QScriptValue quuidToScriptValue(QScriptEngine* engine, const QUuid& uuid) {
    if (uuid.isNull()) {
        return QScriptValue::NullValue;
    }
    QScriptValue obj(uuid.toString());
    return obj;
}

void quuidFromScriptValue(const QScriptValue& object, QUuid& uuid) {
    if (object.isNull()) {
        uuid = QUuid();
        return;
    }
    QString uuidAsString = object.toVariant().toString();
    QUuid fromString(uuidAsString);
    uuid = fromString;
}

QScriptValue qSizeFToScriptValue(QScriptEngine* engine, const QSizeF& qSizeF) {
    QScriptValue obj = engine->newObject();
    obj.setProperty("width", qSizeF.width());
    obj.setProperty("height", qSizeF.height());
    return obj;
}

void qSizeFFromScriptValue(const QScriptValue& object, QSizeF& qSizeF) {
    qSizeF.setWidth(object.property("width").toVariant().toFloat());
    qSizeF.setHeight(object.property("height").toVariant().toFloat());
}
