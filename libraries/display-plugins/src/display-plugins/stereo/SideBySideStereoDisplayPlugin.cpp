//
//  Created by Bradley Austin Davis on 2015/05/29
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "SideBySideStereoDisplayPlugin.h"
#include <GLMHelpers.h>

const QString SideBySideStereoDisplayPlugin::NAME("3D TV - Side by Side Stereo");

SideBySideStereoDisplayPlugin::SideBySideStereoDisplayPlugin() {
}

glm::uvec2 SideBySideStereoDisplayPlugin::getRecommendedRenderSize() const {
    uvec2 result = WindowOpenGLDisplayPlugin::getRecommendedRenderSize();
    result.x *= 2;
    return result;
}


