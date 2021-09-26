//
//  Created by Sam Gondelman on 11/30/18.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_BillboardMode_h
#define hifi_BillboardMode_h

#include <functional>

#include "QString"

#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"

/*@jsdoc
 * <p>How an entity is billboarded.</p>
 * <table>
 *   <thead>
 *     <tr><th>Value</th><th>Description</th></tr>
 *   </thead>
 *   <tbody>
 *     <tr><td><code>"none"</code></td><td>The entity will not be billboarded.</td></tr>
 *     <tr><td><code>"yaw"</code></td><td>The entity will yaw, but not pitch, to face the camera. Its actual rotation will be 
 *       ignored.</td></tr>
 *     <tr><td><code>"full"</code></td><td>The entity will yaw and pitch to face the camera. Its actual rotation will be 
 *       ignored.</td></tr>
 *   </tbody>
 * </table>
 * @typedef {string} BillboardMode
 */

enum class BillboardMode {
    NONE = 0,
    YAW,
    FULL
};

class BillboardModeHelpers {
public:
    static QString getNameForBillboardMode(BillboardMode mode);

    static void setBillboardRotationOperator(std::function<glm::quat(const glm::vec3&, const glm::quat&,
        BillboardMode, const glm::vec3&, bool)> getBillboardRotationOperator);
    static glm::quat getBillboardRotation(const glm::vec3& position, const glm::quat& rotation, BillboardMode billboardMode,
        const glm::vec3& frustumPos, bool rotate90x = false);
    static void setPrimaryViewFrustumPositionOperator(std::function<glm::vec3()> getPrimaryViewFrustumPositionOperator);
    static glm::vec3 getPrimaryViewFrustumPosition() { return _getPrimaryViewFrustumPositionOperator(); }

private:
    static std::function<glm::quat(const glm::vec3&, const glm::quat&, BillboardMode, const glm::vec3&, bool)> _getBillboardRotationOperator;
    static std::function<glm::vec3()> _getPrimaryViewFrustumPositionOperator;
};

#endif // hifi_BillboardMode_h
