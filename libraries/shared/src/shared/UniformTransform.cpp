//
//  Created by Bradley Austin Davis on 2015/11/09
//  Copyright 2013-2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "UniformTransform.h"

#include "JSONHelpers.h"

#include <QtCore/QJsonValue>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>

#include <glm/gtc/matrix_transform.hpp>

const float UniformTransform::DEFAULT_SCALE = 1.0f;

std::shared_ptr<UniformTransform> UniformTransform::parseJson(const QJsonValue& basis) {
    std::shared_ptr<UniformTransform> result = std::make_shared<UniformTransform>();
    result->fromJson(basis);
    return result;
}

static const QString JSON_TRANSLATION = QStringLiteral("translation");
static const QString JSON_ROTATION = QStringLiteral("rotation");
static const QString JSON_SCALE = QStringLiteral("scale");

void UniformTransform::fromJson(const QJsonValue& basisValue) {
    if (!basisValue.isObject()) {
        return;
    }
    QJsonObject basis = basisValue.toObject();
    if (basis.contains(JSON_ROTATION)) {
        rotation = quatFromJsonValue(basis[JSON_ROTATION]);
    }
    if (basis.contains(JSON_TRANSLATION)) {
        translation = vec3FromJsonValue(basis[JSON_TRANSLATION]);
    }
    if (basis.contains(JSON_SCALE)) {
        scale = (float)basis[JSON_SCALE].toDouble();
    }
}

glm::mat4 toMat4(const UniformTransform& transform) {
    return glm::translate(glm::mat4(), transform.translation) * glm::mat4_cast(transform.rotation);
}

UniformTransform fromMat4(const glm::mat4& m) {
    UniformTransform result;
    result.translation = vec3(m[3]);
    result.rotation = glm::quat_cast(m);
    return result;
}

UniformTransform UniformTransform::relativeTransform(const UniformTransform& worldTransform) const {
    UniformTransform result = fromMat4(glm::inverse(toMat4(*this)) * toMat4(worldTransform));
    result.scale = scale / worldTransform.scale;
    return result;
}

UniformTransform UniformTransform::worldTransform(const UniformTransform& relativeTransform) const {
    UniformTransform result = fromMat4(toMat4(*this) * toMat4(relativeTransform));
    result.scale = relativeTransform.scale * scale;
    return result;
}

QJsonObject UniformTransform::toJson() const {
    QJsonObject result;
    auto json = toJsonValue(translation);
    if (!json.isNull()) {
        result[JSON_TRANSLATION] = json;
    }
    json = toJsonValue(rotation);
    if (!json.isNull()) {
        result[JSON_ROTATION] = json;
    }
    if (scale != DEFAULT_SCALE) {
        result[JSON_SCALE] = scale;
    }
    return result;
}
