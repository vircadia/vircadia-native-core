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
#include "Context.h"

// FIXME -- ideally we'd like this to be in Batch.h, but gcc was not happy with the first cut at that,
// it was complaining with error: member access into incomplete type 'element_type' (aka 'gpu::Context')
template<typename F>
void doInBatch(RenderArgs* args, F f) {
    static gpu::Batch::CacheState cacheState;
    gpu::Batch batch(cacheState);
    f(batch);
    args->_context->render(batch);
    cacheState = batch.getCacheState();
}


#endif



