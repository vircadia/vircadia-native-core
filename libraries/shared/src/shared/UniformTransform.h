//
//  Created by Bradley Austin Davis on 2015/11/09
//  Copyright 2013-2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_Shared_UniformTransform_h
#define hifi_Shared_UniformTransform_h

#include "../GLMHelpers.h"

class QJsonValue;

struct UniformTransform {
    static const float DEFAULT_SCALE;
    glm::vec3 translation;
    glm::quat rotation;
    float scale { DEFAULT_SCALE };

    UniformTransform() {}

    UniformTransform(const glm::vec3& translation, const glm::quat& rotation, const float& scale) 
        : translation(translation), rotation(rotation), scale(scale) {}

    UniformTransform relativeTransform(const UniformTransform& worldTransform) const;
    glm::vec3 relativeVector(const UniformTransform& worldTransform) const;

    UniformTransform worldTransform(const UniformTransform& relativeTransform) const;
    glm::vec3 worldVector(const UniformTransform& relativeTransform) const;

    QJsonObject toJson() const;
    void fromJson(const QJsonValue& json);

    static std::shared_ptr<UniformTransform> parseJson(const QJsonValue& json);
};

#endif
