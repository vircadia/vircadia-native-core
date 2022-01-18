//
//  Created by Sam Gondelman on 11/30/18
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "BillboardMode.h"

const char* billboardModeNames[] = {
    "none",
    "yaw",
    "full"
};

static const size_t BILLBOARD_MODE_NAMES = (sizeof(billboardModeNames) / sizeof(billboardModeNames[0]));
std::function<glm::quat(const glm::vec3&, const glm::quat&, BillboardMode, const glm::vec3&, bool)> BillboardModeHelpers::_getBillboardRotationOperator =
    [](const glm::vec3&, const glm::quat& rotation, BillboardMode, const glm::vec3&, bool) { return rotation; };
std::function<glm::vec3()> BillboardModeHelpers::_getPrimaryViewFrustumPositionOperator = []() { return glm::vec3(0.0f); };

void BillboardModeHelpers::setBillboardRotationOperator(std::function<glm::quat(const glm::vec3&, const glm::quat&,
                                                        BillboardMode, const glm::vec3&, bool)> getBillboardRotationOperator) {
    _getBillboardRotationOperator = getBillboardRotationOperator;
}

glm::quat BillboardModeHelpers::getBillboardRotation(const glm::vec3& position, const glm::quat& rotation, BillboardMode billboardMode,
                                                     const glm::vec3& frustumPos, bool rotate90x) {
    return _getBillboardRotationOperator(position, rotation, billboardMode, frustumPos, rotate90x);
}

void BillboardModeHelpers::setPrimaryViewFrustumPositionOperator(std::function<glm::vec3()> getPrimaryViewFrustumPositionOperator) {
    _getPrimaryViewFrustumPositionOperator = getPrimaryViewFrustumPositionOperator;
}

QString BillboardModeHelpers::getNameForBillboardMode(BillboardMode mode) {
    if (((int)mode <= 0) || ((int)mode >= (int)BILLBOARD_MODE_NAMES)) {
        mode = (BillboardMode)0;
    }

    return billboardModeNames[(int)mode];
}
