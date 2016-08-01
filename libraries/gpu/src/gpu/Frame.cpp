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

Frame::~Frame() {
    if (framebuffer && framebufferRecycler) {
        framebufferRecycler(framebuffer);
        framebuffer.reset();
    }

    if (overlay && overlayRecycler) {
        overlayRecycler(overlay);
        overlay.reset();
    }
}

void Frame::finish() {
    std::unordered_set<Buffer*> seenBuffers;
    for (Batch& batch : batches) {
        for (auto& namedCallData : batch._namedData) {
            for (auto& buffer : namedCallData.second.buffers) {
                if (!buffer) {
                    continue;
                }
                if (!buffer->isDirty()) {
                    continue;
                }
                if (seenBuffers.count(buffer.get())) {
                    continue;
                }
                seenBuffers.insert(buffer.get());
                bufferUpdates.push_back({ buffer, buffer->getUpdate() });
            }
        }
        for (auto& bufferCacheItem : batch._buffers._items) {
            const BufferPointer& buffer = bufferCacheItem._data;
            if (!buffer) {
                continue;
            }
            if (!buffer->isDirty()) {
                continue;
            }
            if (seenBuffers.count(buffer.get())) {
                continue;
            }
            seenBuffers.insert(buffer.get());
            bufferUpdates.push_back({ buffer, buffer->getUpdate() });
        }
    }
}

void Frame::preRender() {
    for (auto& bufferUpdate : bufferUpdates) {
        const BufferPointer& buffer = bufferUpdate.first;
        const Buffer::Update& update = bufferUpdate.second;
        buffer->applyUpdate(update);
    }
    bufferUpdates.clear();
}
