//
//  View.cpp
//  libraries/workload/src/workload
//
//  Created by Sam Gateau 2018.03.05
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "View.h"
#include <ViewFrustum.h>

using namespace workload;

void View::setFov(float angleRad) {
    float halfAngle = angleRad * 0.5f;

    fov_halfAngle_tan_cos_sin.x = halfAngle;
    fov_halfAngle_tan_cos_sin.y = tanf(halfAngle);
    fov_halfAngle_tan_cos_sin.z = cosf(halfAngle);
    fov_halfAngle_tan_cos_sin.w = sinf(halfAngle);
}

void View::makeHorizontal() {
    direction = glm::normalize(glm::vec3(direction.x, 0.0f, direction.z));
}

View View::evalFromFrustum(const ViewFrustum& frustum, const glm::vec3& offset) {
    View view;
    view.origin = frustum.getPosition() + offset;
    view.direction = frustum.getDirection();
    view.setFov(frustum.getFieldOfView());

    return view;
}

Sphere View::evalRegionSphere(const View& view, float originRadius, float maxDistance) {
    float radius = (maxDistance + originRadius) / 2.0f;
    float distanceToCenter = radius - originRadius;
    return Sphere(view.origin + view.direction * distanceToCenter, radius);
}

void View::updateRegionsDefault(View& view) {
    std::vector<float> config(Region::NUM_TRACKED_REGIONS * 2, 0.0f);

    float refFar = 10.0f;
    float refClose = 2.0f;
    for (int i = 0; i < (int)Region::NUM_TRACKED_REGIONS; i++) {
        float weight = i + 1.0f;
        config[i * 2] = refClose;
        config[i * 2 + 1] = refFar * weight;
        refFar *= 2.0f;
    }
    updateRegionsFromBackFrontDistances(view, config.data());
}

void View::updateRegionsFromBackFronts(View& view) {
    for (int i = 0; i < (int)Region::NUM_TRACKED_REGIONS; i++) {
        view.regions[i] = evalRegionSphere(view, view.regionBackFronts[i].x, view.regionBackFronts[i].y);
    }
}

void View::updateRegionsFromBackFrontDistances(View& view, const float* configDistances) {
    for (int i = 0; i < (int)Region::NUM_TRACKED_REGIONS; i++) {
        view.regionBackFronts[i] = glm::vec2(configDistances[i * 2], configDistances[i * 2 + 1]);
    }
    updateRegionsFromBackFronts(view);
}
