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

#include <QtCore/QUrl>
#include <QtCore/QUuid>
#include <QtCore/QRect>
#include <QtCore/QVariant>
#include <QtGui/QColor>
#include <QtGui/QVector2D>
#include <QtGui/QVector3D>
#include <QtGui/QQuaternion>
#include <glm/gtc/quaternion.hpp>
#include <QAbstractSocket>

#include "RegisteredMetaTypes.h"

int vec4MetaTypeId = qRegisterMetaType<glm::vec4>();
int vec3MetaTypeId = qRegisterMetaType<glm::vec3>();
int qVectorVec3MetaTypeId = qRegisterMetaType<QVector<glm::vec3>>();
int qVectorQuatMetaTypeId = qRegisterMetaType<QVector<glm::quat>>();
int qVectorBoolMetaTypeId = qRegisterMetaType<QVector<bool>>();
int vec2MetaTypeId = qRegisterMetaType<glm::vec2>();
int quatMetaTypeId = qRegisterMetaType<glm::quat>();
int xColorMetaTypeId = qRegisterMetaType<xColor>();
int pickRayMetaTypeId = qRegisterMetaType<PickRay>();
int collisionMetaTypeId = qRegisterMetaType<Collision>();
int qMapURLStringMetaTypeId = qRegisterMetaType<QMap<QUrl,QString>>();
int socketErrorMetaTypeId = qRegisterMetaType<QAbstractSocket::SocketError>();

void registerMetaTypes(QScriptEngine* engine) {
    qScriptRegisterMetaType(engine, mat4toScriptValue, mat4FromScriptValue);
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

QScriptValue mat4toScriptValue(QScriptEngine* engine, const glm::mat4& mat4) {
    QScriptValue obj = engine->newObject();
    obj.setProperty("r0c0", mat4[0][0]);
    obj.setProperty("r1c0", mat4[0][1]);
    obj.setProperty("r2c0", mat4[0][2]);
    obj.setProperty("r3c0", mat4[0][3]);
    obj.setProperty("r0c1", mat4[1][0]);
    obj.setProperty("r1c1", mat4[1][1]);
    obj.setProperty("r2c1", mat4[1][2]);
    obj.setProperty("r3c1", mat4[1][3]);
    obj.setProperty("r0c2", mat4[2][0]);
    obj.setProperty("r1c2", mat4[2][1]);
    obj.setProperty("r2c2", mat4[2][2]);
    obj.setProperty("r3c2", mat4[2][3]);
    obj.setProperty("r0c3", mat4[3][0]);
    obj.setProperty("r1c3", mat4[3][1]);
    obj.setProperty("r2c3", mat4[3][2]);
    obj.setProperty("r3c3", mat4[3][3]);
    return obj;
}

void mat4FromScriptValue(const QScriptValue& object, glm::mat4& mat4) {
    mat4[0][0] = object.property("r0c0").toVariant().toFloat();
    mat4[0][1] = object.property("r1c0").toVariant().toFloat();
    mat4[0][2] = object.property("r2c0").toVariant().toFloat();
    mat4[0][3] = object.property("r3c0").toVariant().toFloat();
    mat4[1][0] = object.property("r0c1").toVariant().toFloat();
    mat4[1][1] = object.property("r1c1").toVariant().toFloat();
    mat4[1][2] = object.property("r2c1").toVariant().toFloat();
    mat4[1][3] = object.property("r3c1").toVariant().toFloat();
    mat4[2][0] = object.property("r0c2").toVariant().toFloat();
    mat4[2][1] = object.property("r1c2").toVariant().toFloat();
    mat4[2][2] = object.property("r2c2").toVariant().toFloat();
    mat4[2][3] = object.property("r3c2").toVariant().toFloat();
    mat4[3][0] = object.property("r0c3").toVariant().toFloat();
    mat4[3][1] = object.property("r1c3").toVariant().toFloat();
    mat4[3][2] = object.property("r2c3").toVariant().toFloat();
    mat4[3][3] = object.property("r3c3").toVariant().toFloat();
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

QVariant vec3toVariant(const glm::vec3& vec3) {
    if (vec3.x != vec3.x || vec3.y != vec3.y || vec3.z != vec3.z) {
        // if vec3 contains a NaN don't try to convert it
        return QVariant();
    }
    QVariantMap result;
    result["x"] = vec3.x;
    result["y"] = vec3.y;
    result["z"] = vec3.z;
    return result;
}

QVariant vec4toVariant(const glm::vec4& vec4) {
    if (isNaN(vec4.x) || isNaN(vec4.y) || isNaN(vec4.z) || isNaN(vec4.w)) {
        // if vec4 contains a NaN don't try to convert it
        return QVariant();
    }
    QVariantMap result;
    result["x"] = vec4.x;
    result["y"] = vec4.y;
    result["z"] = vec4.z;
    result["w"] = vec4.w;
    return result;
}

QScriptValue qVectorVec3ToScriptValue(QScriptEngine* engine, const QVector<glm::vec3>& vector) {
    QScriptValue array = engine->newArray();
    for (int i = 0; i < vector.size(); i++) {
        array.setProperty(i, vec3toScriptValue(engine, vector.at(i)));
    }
    return array;
}


glm::vec3 vec3FromVariant(const QVariant& object, bool& valid) {
    glm::vec3 v;
    valid = false;
    if (!object.isValid() || object.isNull()) {
        return v;
    } else if (object.canConvert<float>()) {
        v = glm::vec3(object.toFloat());
        valid = true;
    } else if (object.canConvert<QVector3D>()) {
        auto qvec3 = qvariant_cast<QVector3D>(object);
        v.x = qvec3.x();
        v.y = qvec3.y();
        v.z = qvec3.z();
        valid = true;
    } else {
        auto map = object.toMap();
        auto x = map["x"];
        auto y = map["y"];
        auto z = map["z"];
        if (!x.isValid()) {
            x = map["width"];
        }
        if (!y.isValid()) {
            y = map["height"];
        }
        if (!y.isValid()) {
            z = map["depth"];
        }

        if (x.canConvert<float>() && y.canConvert<float>() && z.canConvert<float>()) {
            v.x = x.toFloat();
            v.y = y.toFloat();
            v.z = z.toFloat();
            valid = true;
        }
    }
    return v;
}

glm::vec3 vec3FromVariant(const QVariant& object) {
    bool valid = false;
    return vec3FromVariant(object, valid);
}

glm::vec4 vec4FromVariant(const QVariant& object, bool& valid) {
    glm::vec4 v;
    valid = false;
    if (!object.isValid() || object.isNull()) {
        return v;
    } else if (object.canConvert<float>()) {
        v = glm::vec4(object.toFloat());
        valid = true;
    } else if (object.canConvert<QVector4D>()) {
        auto qvec4 = qvariant_cast<QVector4D>(object);
        v.x = qvec4.x();
        v.y = qvec4.y();
        v.z = qvec4.z();
        v.w = qvec4.w();
        valid = true;
    } else {
        auto map = object.toMap();
        auto x = map["x"];
        auto y = map["y"];
        auto z = map["z"];
        auto w = map["w"];
        if (x.canConvert<float>() && y.canConvert<float>() && z.canConvert<float>() && w.canConvert<float>()) {
            v.x = x.toFloat();
            v.y = y.toFloat();
            v.z = z.toFloat();
            v.w = w.toFloat();
            valid = true;
        }
    }
    return v;
}

glm::vec4 vec4FromVariant(const QVariant& object) {
    bool valid = false;
    return vec4FromVariant(object, valid);
}

QScriptValue quatToScriptValue(QScriptEngine* engine, const glm::quat& quat) {
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

void quatFromScriptValue(const QScriptValue& object, glm::quat &quat) {
    quat.x = object.property("x").toVariant().toFloat();
    quat.y = object.property("y").toVariant().toFloat();
    quat.z = object.property("z").toVariant().toFloat();
    quat.w = object.property("w").toVariant().toFloat();

    // enforce normalized quaternion
    float length = glm::length(quat);
    if (length > FLT_EPSILON) {
        quat /= length;
    } else {
        quat = glm::quat();
    }
}

glm::quat quatFromVariant(const QVariant &object, bool& isValid) {
    glm::quat q;
    if (object.canConvert<QQuaternion>()) {
        auto qvec3 = qvariant_cast<QQuaternion>(object);
        q.x = qvec3.x();
        q.y = qvec3.y();
        q.z = qvec3.z();
        q.w = qvec3.scalar();

        // enforce normalized quaternion
        float length = glm::length(q);
        if (length > FLT_EPSILON) {
            q /= length;
        } else {
            q = glm::quat();
        }
        isValid = true;
    } else {
        auto map = object.toMap();
        q.x = map["x"].toFloat(&isValid);
        if (!isValid) {
            return glm::quat();
        }
        q.y = map["y"].toFloat(&isValid);
        if (!isValid) {
            return glm::quat();
        }
        q.z = map["z"].toFloat(&isValid);
        if (!isValid) {
            return glm::quat();
        }
        q.w = map["w"].toFloat(&isValid);
        if (!isValid) {
            return glm::quat();
        }
    }
    return q;
}

glm::quat quatFromVariant(const QVariant& object) {
    bool valid = false;
    return quatFromVariant(object, valid);
}

QVariant quatToVariant(const glm::quat& quat) {
    if (quat.x != quat.x || quat.y != quat.y || quat.z != quat.z) {
        // if vec3 contains a NaN don't try to convert it
        return QVariant();
    }
    QVariantMap result;
    result["x"] = quat.x;
    result["y"] = quat.y;
    result["z"] = quat.z;
    result["w"] = quat.w;
    return result;
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

QVariant vec2toVariant(const glm::vec2 &vec2) {
    if (vec2.x != vec2.x || vec2.y != vec2.y) {
        // if vec2 contains a NaN don't try to convert it
        return QVariant();
    }
    QVariantMap result;
    result["x"] = vec2.x;
    result["y"] = vec2.y;
    return result;
}

glm::vec2 vec2FromVariant(const QVariant &object, bool& isValid) {
    isValid = false;
    glm::vec2 result;
    if (object.canConvert<float>()) {
        result = glm::vec2(object.toFloat());
    } else if (object.canConvert<QVector2D>()) {
        auto qvec2 = qvariant_cast<QVector2D>(object);
        result.x = qvec2.x();
        result.y = qvec2.y();
    } else {
        auto map = object.toMap();
        auto x = map["x"];
        if (!x.isValid()) {
            x = map["width"];
        }
        auto y = map["y"];
        if (!y.isValid()) {
            y = map["height"];
        }
        if (x.isValid() && y.isValid()) {
            result.x = x.toFloat(&isValid);
            if (isValid) {
                result.y = y.toFloat(&isValid);
            }
        }
    }
    return result;
}

glm::vec2 vec2FromVariant(const QVariant &object) {
    bool valid;
    return vec2FromVariant(object, valid);
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

QVariant qRectToVariant(const QRect& rect) {
    QVariantMap obj;
    obj["x"] = rect.x();
    obj["y"] = rect.y();
    obj["width"] = rect.width();
    obj["height"] = rect.height();
    return obj;
}

QRect qRectFromVariant(const QVariant& objectVar, bool& valid) {
    QVariantMap object = objectVar.toMap();
    QRect rect;
    valid = false;
    rect.setX(object["x"].toInt(&valid));
    if (valid) {
        rect.setY(object["y"].toInt(&valid));
    }
    if (valid) {
        rect.setWidth(object["width"].toInt(&valid));
    }
    if (valid) {
        rect.setHeight(object["height"].toInt(&valid));
    }
    return rect;
}

QRect qRectFromVariant(const QVariant& object) {
    bool valid;
    return qRectFromVariant(object, valid);
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


QVariant xColorToVariant(const xColor& color) {
    QVariantMap obj;
    obj["red"] = color.red;
    obj["green"] = color.green;
    obj["blue"] = color.blue;
    return obj;
}

xColor xColorFromVariant(const QVariant &object, bool& isValid) {
    isValid = false;
    xColor color { 0, 0, 0 };
    if (!object.isValid()) {
        return color;
    }
    if (object.canConvert<int>()) {
        isValid = true;
        color.red = color.green = color.blue = (uint8_t)object.toInt();
    } else if (object.canConvert<QString>()) {
        QColor qcolor(object.toString());
        if (qcolor.isValid()) {
            isValid = true;
            color.red = (uint8_t)qcolor.red();
            color.blue = (uint8_t)qcolor.blue();
            color.green = (uint8_t)qcolor.green();
        }
    } else if (object.canConvert<QColor>()) {
        QColor qcolor = qvariant_cast<QColor>(object);
        if (qcolor.isValid()) {
            isValid = true;
            color.red = (uint8_t)qcolor.red();
            color.blue = (uint8_t)qcolor.blue();
            color.green = (uint8_t)qcolor.green();
        }
    } else {
        QVariantMap map = object.toMap();
        color.red = map["red"].toInt(&isValid);
        if (isValid) {
            color.green = map["green"].toInt(&isValid);
        }
        if (isValid) {
            color.blue = map["blue"].toInt(&isValid);
        }
    }
    return color;
}

xColor xColorFromVariant(const QVariant &object) {
    bool valid;
    return xColorFromVariant(object, valid);
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
