//
//  QVariantGLM.cpp
//  libraries/shared/src
//
//  Created by Seth Alves on 3/9/15.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "QVariantGLM.h"
#include "OctalCode.h"

QVariantList glmToQList(const glm::vec3& g) {
    return QVariantList() << g[0] << g[1] << g[2];
}

QVariantList glmToQList(const glm::quat& g) {
    return QVariantList() << g.x << g.y << g.z << g.w;
}

QVariantList rgbColorToQList(const rgbColor& v) {
    return QVariantList() << (int)(v[0]) << (int)(v[1]) << (int)(v[2]);
}

QVariantMap glmToQMap(const glm::vec3& glmVector) {
    QVariantMap vectorAsVariantMap;
    vectorAsVariantMap["x"] = glmVector.x;
    vectorAsVariantMap["y"] = glmVector.y;
    vectorAsVariantMap["z"] = glmVector.z;
    return vectorAsVariantMap;
}

QVariantMap glmToQMap(const glm::quat& glmQuat) {
    QVariantMap quatAsVariantMap;
    quatAsVariantMap["x"] = glmQuat.x;
    quatAsVariantMap["y"] = glmQuat.y;
    quatAsVariantMap["z"] = glmQuat.z;
    quatAsVariantMap["w"] = glmQuat.w;
    return quatAsVariantMap;
}


glm::vec3 qListToGlmVec3(const QVariant& q) {
    QVariantList qList = q.toList();
    return glm::vec3(qList[RED_INDEX].toFloat(), qList[GREEN_INDEX].toFloat(), qList[BLUE_INDEX].toFloat());
}

glm::quat qListToGlmQuat(const QVariant& q) {
    QVariantList qList = q.toList();
    float x = qList[0].toFloat();
    float y = qList[1].toFloat();
    float z = qList[2].toFloat();
    float w = qList[3].toFloat();
    return glm::quat(w, x, y, z);
}

void qListtoRgbColor(const QVariant& q, rgbColor& returnValue) {
    QVariantList qList = q.toList();
    returnValue[RED_INDEX] = qList[RED_INDEX].toInt();
    returnValue[GREEN_INDEX] = qList[GREEN_INDEX].toInt();
    returnValue[BLUE_INDEX] = qList[BLUE_INDEX].toInt();
}


glm::vec3 qMapToGlmVec3(const QVariant& q) {
    QVariantMap qMap = q.toMap();
    if (qMap.contains("x") && qMap.contains("y") && qMap.contains("y")) {
        return glm::vec3(
                qMap["x"].toFloat(),
                qMap["y"].toFloat(),
                qMap["z"].toFloat()
            );
    } else {
        return glm::vec3();
    }
}

glm::quat qMapToGlmQuat(const QVariant& q) {
    QVariantMap qMap = q.toMap();
    if (qMap.contains("w") && qMap.contains("x") && qMap.contains("y") && qMap.contains("z")) {
        return glm::quat(
                qMap["w"].toFloat(),
                qMap["x"].toFloat(),
                qMap["y"].toFloat(),
                qMap["z"].toFloat()
            );
    } else {
        return glm::quat();
    }
}

glm::mat4 qMapToGlmMat4(const QVariant& q) {
    QVariantMap qMap = q.toMap();
    if (qMap.contains("r0c0") && qMap.contains("r1c0") && qMap.contains("r2c0") && qMap.contains("r3c0")
            && qMap.contains("r0c1") && qMap.contains("r1c1") && qMap.contains("r2c1") && qMap.contains("r3c1")
            && qMap.contains("r0c2") && qMap.contains("r1c2") && qMap.contains("r2c2") && qMap.contains("r3c2")
            && qMap.contains("r0c3") && qMap.contains("r1c3") && qMap.contains("r2c3") && qMap.contains("r3c3")) {
        return glm::mat4(
                qMap["r0c0"].toFloat(), qMap["r1c0"].toFloat(), qMap["r2c0"].toFloat(), qMap["r3c0"].toFloat(),
                qMap["r0c1"].toFloat(), qMap["r1c1"].toFloat(), qMap["r2c1"].toFloat(), qMap["r3c1"].toFloat(),
                qMap["r0c2"].toFloat(), qMap["r1c2"].toFloat(), qMap["r2c2"].toFloat(), qMap["r3c2"].toFloat(),
                qMap["r0c3"].toFloat(), qMap["r1c3"].toFloat(), qMap["r2c3"].toFloat(), qMap["r3c3"].toFloat()
            );
    } else {
        return glm::mat4();
    }
}
