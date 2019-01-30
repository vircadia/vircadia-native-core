//
//  Created by Bradley Austin Davis on 2016/07/26
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Frame.h"
#include <unordered_set>

using namespace gpu;

Frame::Frame() {
    batches.reserve(16);
}

Frame::~Frame() {
    if (framebuffer && framebufferRecycler) {
        framebufferRecycler(framebuffer);
        framebuffer.reset();
    }

    bufferUpdates.clear();
}

void Frame::finish() {
    PROFILE_RANGE(render_gpu, __FUNCTION__);
    for (const auto& batch : batches) {
        batch->finishFrame(bufferUpdates);
    }
}

void Frame::preRender() {
    PROFILE_RANGE(render_gpu, __FUNCTION__);
    for (auto& update : bufferUpdates) {
        update.apply();
    }
    bufferUpdates.clear();
}
