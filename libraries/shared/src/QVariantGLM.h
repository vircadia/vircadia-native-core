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

QVariantList vec3ToQList(const glm::vec3& g);
QVariantList quatToQList(const glm::quat& g);

QVariantMap vec3ToQMap(const glm::vec3& glmVector);
QVariantMap quatToQMap(const glm::quat& glmQuat);

glm::vec3 qListToVec3(const QVariant& q);
glm::quat qListToQuat(const QVariant& q);

glm::vec3 qMapToVec3(const QVariant& q);
glm::quat qMapToQuat(const QVariant& q);
glm::mat4 qMapToMat4(const QVariant& q);
