//
//  ViewTask.cpp
//  libraries/workload/src/workload
//
//  Created by Sam Gateau 2018.03.05
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "ViewTask.h"

using namespace workload;


void SetupViews::configure(const Config& config) {
    data = config.data;
}

void SetupViews::run(const WorkloadContextPointer& renderContext, const Input& inputs) {
    // If views are frozen don't use the input
    if (!data.freezeViews) {
        _views = inputs;
    }

    // Filter the first view centerer on the avatar head if needed
    Views usedViews;
    if (_views.size() >= 2) {
        if (data.useAvatarView) {
            usedViews.push_back(_views[0]);
            usedViews.insert(usedViews.end(), _views.begin() + 2, _views.end());
        } else {
            usedViews.insert(usedViews.end(), _views.begin() + 1, _views.end());
        }
    } else {
        usedViews = _views;
    }

    // Force frutum orientation horizontal if needed
    if (usedViews.size() > 0 && data.forceViewHorizontal) {
        usedViews[0].makeHorizontal();
    }

    // Force frutum orientation horizontal if needed
    if (usedViews.size() > 0 && data.simulateSecondaryCamera) {
        auto view = usedViews[0];
        auto secondaryDirectionFlat = glm::normalize(glm::vec3(view.direction.x, 0.0f, view.direction.z));
        auto secondaryDirection = glm::normalize(glm::vec3(secondaryDirectionFlat.z, 0.0f, -secondaryDirectionFlat.x));

        view.origin += -30.0f * secondaryDirection;
        view.direction = -secondaryDirectionFlat;

        usedViews.insert(usedViews.begin() + 1, view);
    }

    // Update regions based on the current config
    for (auto& v : usedViews) {
        View::updateRegions(v, (float*) &data);
    }

    // Views are setup, assign to the Space
    renderContext->_space->setViews(usedViews);
}

