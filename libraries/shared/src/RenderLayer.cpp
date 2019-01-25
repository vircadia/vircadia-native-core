//
//  Created by Sam Gondelman on 1/3/19
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "RenderLayer.h"

const char* renderLayerNames[] = {
    "world",
    "front",
    "hud"
};

static const size_t RENDER_LAYER_NAMES = (sizeof(renderLayerNames) / sizeof(renderLayerNames[0]));

QString RenderLayerHelpers::getNameForRenderLayer(RenderLayer mode) {
    if (((int)mode <= 0) || ((int)mode >= (int)RENDER_LAYER_NAMES)) {
        mode = (RenderLayer)0;
    }

    return renderLayerNames[(int)mode];
}