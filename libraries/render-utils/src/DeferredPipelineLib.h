//
//  DeferredPipelineLib.h
//  render-utils/src
//
//  Created by Zach Pomerantz on 1/4/2015.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_render_utils_DeferredPipelineLib_h
#define hifi_render_utils_DeferredPipelineLib_h

#include <render/ShapePipeline.h>
#include <model/Material.h>

class DeferredPipelineLib : public render::ShapePipelineLib {
public:
    DeferredPipelineLib();
    const PipelinePointer pickPipeline(RenderArgs* args, const Key& key) const override;

    static model::MaterialPointer getCollisionHullMaterial();

protected:
    static bool _isInitPipeline;
    static void initPipeline();

    static model::MaterialPointer _collisionHullMaterial;
};

#endif // hifi_render_utils_DeferredPipelineLib_h
