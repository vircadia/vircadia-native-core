//
//  DoInBatch.h
//  interface/src/gpu
//
//  Created by Brad Hefta-Gaub on 2015/09/23.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_gpu_DoInBatch_h
#define hifi_gpu_DoInBatch_h

#include <RenderArgs.h>

#include "Batch.h"
//#include "Framebuffer.h"
//#include "Pipeline.h"
//#include "Query.h"
//#include "Stream.h"
//#include "Texture.h"
//#include "Transform.h"

// FIXME - technically according to our coding standard Context.h should be before "Framebuffer.h" but it appears
// as if Context.h is not self contained and assumes other users have included other gpu headers.
#include "Context.h"

template<typename F>
void doInBatch(RenderArgs* args, F f) {
    static gpu::Batch::CacheState cacheState;
    gpu::Batch batch(cacheState);
    f(batch);
    args->_context->render(batch);
    cacheState = batch.getCacheState();
}


#endif



