//
//  Created by Bradley Austin Davis on 2016/07/26
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_gpu_Frame_h
#define hifi_gpu_Frame_h

#include <functional>

#include "Forward.h"
#include "Batch.h"
#include "Resource.h"

namespace gpu {

    class Frame {
    public:
        using Batches = std::vector<Batch>;
        using FramebufferRecycler = std::function<void(const FramebufferPointer&)>;
        using OverlayRecycler = std::function<void(const TexturePointer&)>;

        virtual ~Frame();
        void finish();
        void preRender();

        StereoState stereoState;
        uint32_t frameIndex{ 0 };
        /// The sensor pose used for rendering the frame, only applicable for HMDs
        Mat4 pose;
        /// The collection of batches which make up the frame
        Batches batches;
        /// The main thread updates to buffers that are applicable for this frame.
        BufferUpdates bufferUpdates;
        /// The destination framebuffer in which the results will be placed
        FramebufferPointer framebuffer;
        /// The destination texture containing the 2D overlay
        TexturePointer overlay;

        /// How to process the framebuffer when the frame dies.  MUST BE THREAD SAFE
        FramebufferRecycler framebufferRecycler;
    };

};


#endif
