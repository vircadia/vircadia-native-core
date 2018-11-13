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

QVariantList vec3ToQList(const glm::vec3& g) {
    return QVariantList() << g[0] << g[1] << g[2];
}

QVariantList quatToQList(const glm::quat& g) {
    return QVariantList() << g.x << g.y << g.z << g.w;
}

QVariantMap vec3ToQMap(const glm::vec3& glmVector) {
    QVariantMap vectorAsVariantMap;
    vectorAsVariantMap["x"] = glmVector.x;
    vectorAsVariantMap["y"] = glmVector.y;
    vectorAsVariantMap["z"] = glmVector.z;
    return vectorAsVariantMap;
}

QVariantMap quatToQMap(const glm::quat& glmQuat) {
    QVariantMap quatAsVariantMap;
    quatAsVariantMap["x"] = glmQuat.x;
    quatAsVariantMap["y"] = glmQuat.y;
    quatAsVariantMap["z"] = glmQuat.z;
    quatAsVariantMap["w"] = glmQuat.w;
    return quatAsVariantMap;
}


glm::vec3 qListToVec3(const QVariant& q) {
    QVariantList qList = q.toList();
    return glm::vec3(qList[RED_INDEX].toFloat(), qList[GREEN_INDEX].toFloat(), qList[BLUE_INDEX].toFloat());
}

glm::quat qListToQuat(const QVariant& q) {
    QVariantList qList = q.toList();
    float x = qList[0].toFloat();
    float y = qList[1].toFloat();
    float z = qList[2].toFloat();
    float w = qList[3].toFloat();
    return glm::quat(w, x, y, z);
}

glm::vec3 qMapToVec3(const QVariant& q) {
    QVariantMap qMap = q.toMap();
    if (qMap.contains("x") && qMap.contains("y") && qMap.contains("z")) {
        return glm::vec3(
                qMap["x"].toFloat(),
                qMap["y"].toFloat(),
                qMap["z"].toFloat()
            );
    } else {
        return glm::vec3();
    }
}

glm::quat qMapToQuat(const QVariant& q) {
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

glm::mat4 qMapToMat4(const QVariant& q) {
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
