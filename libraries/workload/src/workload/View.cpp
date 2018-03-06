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

View View::evalFromFrustum(const ViewFrustum& frustum) {
    View view;
    view.origin = frustum.getPosition();
    view.direction = frustum.getDirection();
    view.setFov(frustum.getFieldOfView());

    return view;
}

Sphere View::evalRegionSphere(const View& view, float originRadius, float maxDistance) {
    float radius = (maxDistance + originRadius) / 2.0;
    float center = radius - originRadius;
    return Sphere(view.origin + view.direction * center, radius);
}

void View::updateRegions(View& view) {
    float refFar = 10.0f;
    float refClose = 2.0f;
    for (int i = 0; i < Region::NUM_VIEW_REGIONS; i++) {
        float weight = i + 1.0f;
        view.regions[i] = evalRegionSphere(view, refClose * weight, refFar * weight);
        refFar *= 2.0f;
    }
}