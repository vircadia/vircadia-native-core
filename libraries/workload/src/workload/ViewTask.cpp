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


void AssignSpaceViews::run(const WorkloadContextPointer& renderContext, const Input& inputs) {
    // Just do what it says
    renderContext->_space->setViews(inputs);
}

void SetupViews::configure(const Config& config) {
    data = config.data;
}

void SetupViews::run(const WorkloadContextPointer& renderContext, const Input& inputs, Output& outputs) {
    // If views are frozen don't use the input
    if (!data.freezeViews) {
        _views = inputs;
    }

    auto& outViews = outputs;
    outViews.clear();

    // Filter the first view centerer on the avatar head if needed
    if (_views.size() >= 2) {
        if (data.useAvatarView) {
            outViews.push_back(_views[0]);
            outViews.insert(outViews.end(), _views.begin() + 2, _views.end());
        } else {
            outViews.insert(outViews.end(), _views.begin() + 1, _views.end());
        }
    } else {
        outViews = _views;
    }

    // Force frutum orientation horizontal if needed
    if (outViews.size() > 0 && data.forceViewHorizontal) {
        outViews[0].makeHorizontal();
    }

    // Force frutum orientation horizontal if needed
    if (outViews.size() > 0 && data.simulateSecondaryCamera) {
        auto view = outViews[0];
        auto secondaryDirectionFlat = glm::normalize(glm::vec3(view.direction.x, 0.0f, view.direction.z));
        auto secondaryDirection = glm::normalize(glm::vec3(secondaryDirectionFlat.z, 0.0f, -secondaryDirectionFlat.x));

        view.origin += -20.0f * secondaryDirection;
        view.direction = -secondaryDirection;

        outViews.insert(outViews.begin() + 1, view);
    }

    // Update regions based on the current config
    for (auto& v : outViews) {
        View::updateRegions(v, (float*) &data);
    }

    // outViews is ready to be used
}


