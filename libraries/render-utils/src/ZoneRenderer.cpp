//
//  ZoneRenderer.cpp
//  render/src/render-utils
//
//  Created by Sam Gateau on 4/4/2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "ZoneRenderer.h"

const render::Selection::Name ZoneRenderer::ZONES_SELECTION { "RankedZones" };


void ZoneRenderer::run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext, const Inputs& inputs) {
    auto& scene = sceneContext->_scene;
    RenderArgs* args = renderContext->args;

    auto zones = scene->getSelection(ZONES_SELECTION);


}


