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
#include <glm/gtc/constants.hpp>

#include "Region.h"

class ViewFrustum;

namespace workload {

using Sphere = glm::vec4;

class View {
public:
    View() = default;
    View(const View& view) = default;

    // View attributes:

    // direction
    glm::vec3 direction{ 0.0f, 0.0f, -1.0f };

    // Max radius
    float maxRadius{ FLT_MAX };

    // Fov stores the half field of view angle, and tan/cos/sin ready to go, default is fov of 90deg
    glm::vec4 fov_halfAngle_tan_cos_sin { glm::quarter_pi<float>(), 1.0f, glm::root_two<float>() * 0.5f, glm::root_two<float>() * 0.5f};

    // Origin position
    glm::vec3 origin{ 0.0f };

    // Origin radius
    float originRadius{ 0.5f };

    // N regions distances
    glm::vec2 regionBackFronts[Region::NUM_TRACKED_REGIONS];

    // N regions spheres
    Sphere regions[Region::NUM_TRACKED_REGIONS];

    // Set fov properties from angle
    void setFov(float angleRad);

    // Helper function to force the direction in the XZ plane
    void makeHorizontal();

    static View evalFromFrustum(const ViewFrustum& frustum, const glm::vec3& offset = glm::vec3());
    static Sphere evalRegionSphere(const View& view, float originRadius, float maxDistance);

    static void updateRegionsDefault(View& view);
    static void updateRegionsFromBackFronts(View& view);
    static void updateRegionsFromBackFrontDistances(View& view, const float* configDistances);
};

using Views = std::vector<View>;

} // namespace workload

#endif // hifi_workload_View_h
