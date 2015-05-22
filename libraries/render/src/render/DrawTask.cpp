//
//  DrawTask.cpp
//  render/src/render
//
//  Created by Sam Gateau on 5/21/15.
//  Copyright 20154 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "DrawTask.h"

using namespace render;


DrawSceneTask::~DrawSceneTask() {
}

void DrawSceneTask::run(const SceneContextPointer& sceneContext) {
    // sanity checks
    assert(sceneContext);
    if (!sceneContext->_scene) {
        return;
    }
    auto scene = sceneContext->_scene;

    auto itemBucketMap = scene->getMasterBucket();
    
    RenderArgs args;
    // render opaques
    auto& opaqueShapeItems = itemBucketMap[ItemFilter::Builder::opaqueShape()];
    
    for (auto id : opaqueShapeItems) {
        auto item = scene->getItem(id);
            item.render(&args);
    }
};

