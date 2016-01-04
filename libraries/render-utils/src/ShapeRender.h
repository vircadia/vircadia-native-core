//
//  ShapeRender.h
//  render-utils/src
//
//  Created by Zach Pomerantz on 1/4/2015.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_render_utils_Shape_h
#define hifi_render_utils_Shape_h

#include <render/Shape.h>

class ShapeRender : public render::Shape {
    static model::MaterialPointer _collisionHullMaterial;

public:
    static void initPipeline();
    const render::PipelinePointer pickPipeline(RenderArgs* args, Key& key) override;

    static model::MaterialPointer getCollisionHullMaterial();
};

#endif // hifi_render_utils_Shape_h

