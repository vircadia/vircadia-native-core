//
//  Tv3dDisplayPlugin.cpp
//
//  Created by Bradley Austin Davis on 2014/04/13.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Tv3dDisplayPlugin.h"

const QString Tv3dDisplayPlugin::NAME("Tv3dDisplayPlugin");

const QString & Tv3dDisplayPlugin::getName() {
    return NAME;
}


void Tv3dDisplayPlugin::overrideOffAxisFrustum(
    float& left, float& right, float& bottom, float& top,
    float& nearVal, float& farVal,
    glm::vec4& nearClipPlane, glm::vec4& farClipPlane) const {

#if 0
    if (_activeEye) {
        left = _activeEye->left;
        right = _activeEye->right;
        bottom = _activeEye->bottom;
        top = _activeEye->top;
    }
#endif

}


#if 0
    } else if (TV3DManager::isConnected()) {

        TV3DManager::display(_myCamera);

    } else {
#endif
