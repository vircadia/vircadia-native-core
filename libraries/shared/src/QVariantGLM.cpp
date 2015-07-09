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
