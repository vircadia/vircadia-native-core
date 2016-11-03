//
//  AnimPose.h
//
//  Created by Anthony J. Thibault on 10/14/15.
//  Copyright (c) 2015 High Fidelity, Inc. All rights reserved.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AnimPose
#define hifi_AnimPose

#include <QtGlobal>
#include <QDebug>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

struct AnimPose {
    AnimPose() {}
    explicit AnimPose(const glm::mat4& mat);
    AnimPose(const glm::vec3& scaleIn, const glm::quat& rotIn, const glm::vec3& transIn) : scale(scaleIn), rot(rotIn), trans(transIn) {}
    static const AnimPose identity;

    glm::vec3 xformPoint(const glm::vec3& rhs) const;
    glm::vec3 xformVector(const glm::vec3& rhs) const;  // really slow

    glm::vec3 operator*(const glm::vec3& rhs) const; // same as xformPoint
    AnimPose operator*(const AnimPose& rhs) const;

    AnimPose inverse() const;
    AnimPose mirror() const;
    operator glm::mat4() const;

    glm::vec3 scale;
    glm::quat rot;
    glm::vec3 trans;
};

inline QDebug operator<<(QDebug debug, const AnimPose& pose) {
    debug << "AnimPose, trans = (" << pose.trans.x << pose.trans.y << pose.trans.z << "), rot = (" << pose.rot.x << pose.rot.y << pose.rot.z << pose.rot.w << "), scale = (" << pose.scale.x << pose.scale.y << pose.scale.z << ")";
    return debug;
}

using AnimPoseVec = std::vector<AnimPose>;

#endif
