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
#include <model/Material.h>

class ShapeRender : public render::Shape {
public:
    ShapeRender();
    static void initPipeline();
    const PipelinePointer pickPipeline(RenderArgs* args, const Key& key) const override;

    static model::MaterialPointer getCollisionHullMaterial();

protected:
    static model::MaterialPointer _collisionHullMaterial;
};

#endif // hifi_render_utils_Shape_h
