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

QVariantList glmToQList(const glm::vec3& g) {
    return QVariantList() << g[0] << g[1] << g[2];
}

QVariantList glmToQList(const glm::quat& g) {
    return QVariantList() << g[0] << g[1] << g[2] << g[3];
}

glm::vec3 qListToGlmVec3(const QVariant q) {
    QVariantList qList = q.toList();
    return glm::vec3(qList[0].toFloat(), qList[1].toFloat(), qList[2].toFloat());
}

glm::quat qListToGlmQuat(const QVariant q) {
    QVariantList qList = q.toList();
    return glm::quat(qList[0].toFloat(), qList[1].toFloat(), qList[2].toFloat(), qList[3].toFloat());
}

