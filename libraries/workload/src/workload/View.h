//
//  View.h
//  libraries/workload/src/workload
//
//  Created by Sam Gateau 2018.03.05
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_workload_View_h
#define hifi_workload_View_h

#include <memory>
#include <vector>
#include <glm/glm.hpp>

#include "Region.h"

class ViewFrustum;

namespace workload {

using Sphere = glm::vec4;

class View {
public:
    View() = default;
    View(const View& view) = default;

    View(const glm::vec3& pos, float nearRadius, float midRadius, float farRadius) :
        origin(pos) {
        regions[Region::R1] = Sphere(pos, nearRadius);
        regions[Region::R2] = Sphere(pos, midRadius);
        regions[Region::R3] = Sphere(pos, farRadius);
    }

    glm::vec3 origin;
    float _padding { 1.0f };
    Sphere regions[(Region::NUM_CLASSIFICATIONS - 1)];

    static View evalFromFrustum(const ViewFrustum& frustum, float farDistance);
};

using Views = std::vector<View>;

} // namespace workload

#endif // hifi_workload_View_h