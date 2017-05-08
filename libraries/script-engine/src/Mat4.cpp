//
//  Mat4.cpp
//  libraries/script-engine/src
//
//  Created by Anthony Thibault on 3/7/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QDebug>
#include <GLMHelpers.h>
#include <glm/gtx/string_cast.hpp>
#include "ScriptEngineLogging.h"
#include "ScriptEngine.h"
#include "Mat4.h"

glm::mat4 Mat4::multiply(const glm::mat4& m1, const glm::mat4& m2) const {
    return m1 * m2;
}

glm::mat4 Mat4::createFromRotAndTrans(const glm::quat& rot, const glm::vec3& trans) const {
    return ::createMatFromQuatAndPos(rot, trans);
}

glm::mat4 Mat4::createFromScaleRotAndTrans(const glm::vec3& scale, const glm::quat& rot, const glm::vec3& trans) const {
    return createMatFromScaleQuatAndPos(scale, rot, trans);
}

glm::mat4 Mat4::createFromColumns(const glm::vec4& col0, const glm::vec4& col1, const glm::vec4& col2, const glm::vec4& col3) const {
    return glm::mat4(col0, col1, col2, col3);
}

glm::vec3 Mat4::extractTranslation(const glm::mat4& m) const {
    return ::extractTranslation(m);
}

glm::quat Mat4::extractRotation(const glm::mat4& m) const {
    return ::glmExtractRotation(m);
}

glm::vec3 Mat4::extractScale(const glm::mat4& m) const {
    return ::extractScale(m);
}

glm::vec3 Mat4::transformPoint(const glm::mat4& m, const glm::vec3& point) const {
    return ::transformPoint(m, point);
}

glm::vec3 Mat4::transformVector(const glm::mat4& m, const glm::vec3& vector) const {
    return ::transformVectorFast(m, vector);
}

glm::mat4 Mat4::inverse(const glm::mat4& m) const {
    return glm::inverse(m);
}

glm::vec3 Mat4::getForward(const glm::mat4& m) const {
    return glm::vec3(-m[0][2], -m[1][2], -m[2][2]);
}

glm::vec3 Mat4::getRight(const glm::mat4& m) const {
    return glm::vec3(m[0][0], m[1][0], m[2][0]);
}

glm::vec3 Mat4::getUp(const glm::mat4& m) const {
    return glm::vec3(m[0][1], m[1][1], m[2][1]);
}

void Mat4::print(const QString& label, const glm::mat4& m, bool transpose) const {
    glm::dmat4 out = transpose ? glm::transpose(m) : m;
    QString message = QString("%1 %2").arg(qPrintable(label));
    message = message.arg(glm::to_string(out).c_str());
    qCDebug(scriptengine) << message;
    if (ScriptEngine* scriptEngine = qobject_cast<ScriptEngine*>(engine())) {
        scriptEngine->print(message);
    }
}
