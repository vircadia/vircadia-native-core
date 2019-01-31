//
//  Created by Bradley Austin Davis on 2018/10/14
//  Copyright 2013-2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once
#ifndef hifi_gpu_FrameIO_h
#define hifi_gpu_FrameIO_h

#include "Forward.h"
#include "Format.h"

#include <functional>

namespace gpu {

using TextureCapturer = std::function<void(const std::string&, const TexturePointer&, uint16 layer)>;
using TextureLoader = std::function<void(const std::string&, const TexturePointer&, uint16 layer)>;
void writeFrame(const std::string& filename, const FramePointer& frame, const TextureCapturer& capturer = nullptr);
FramePointer readFrame(const std::string& filename, uint32_t externalTexture, const TextureLoader& loader = nullptr);

using IndexOptimizer = std::function<void(Primitive, uint32_t faceCount, uint32_t indexCount, uint32_t* indices )>;
void optimizeFrame(const std::string& filename, const IndexOptimizer& optimizer);

}  // namespace gpu

#endif
