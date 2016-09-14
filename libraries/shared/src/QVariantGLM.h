//
//  QVariantGLM.h
//  libraries/shared/src
//
//  Created by Seth Alves on 3/9/15.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QList>
#include <QVariant>
#include <QVariantList>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

#include "SharedUtil.h"

QVariantList glmToQList(const glm::vec3& g);
QVariantList glmToQList(const glm::quat& g);
QVariantList rgbColorToQList(const rgbColor& v);

QVariantMap glmToQMap(const glm::vec3& glmVector);
QVariantMap glmToQMap(const glm::quat& glmQuat);

glm::vec3 qListToGlmVec3(const QVariant& q);
glm::quat qListToGlmQuat(const QVariant& q);
void qListtoRgbColor(const QVariant& q, rgbColor& returnValue);

glm::vec3 qMapToGlmVec3(const QVariant& q);
glm::quat qMapToGlmQuat(const QVariant& q);
glm::mat4 qMapToGlmMat4(const QVariant& q);
