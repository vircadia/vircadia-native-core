//
//  Created by Bradley Austin Davis on 2018/11/20
//  Copyright 2013-2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include <cstdint>
#include <glm/glm.hpp>

#include <VrApi_Types.h>

namespace ovr {

struct Framebuffer {
public:
    void updateLayer(int eye, ovrLayerProjection2& layer, const ovrMatrix4f* projectionMatrix = nullptr) const;
    void create(const glm::uvec2& size);
    void advance();
    void destroy();
    void bind();

    uint32_t _depth { 0 };
    uint32_t _fbo{ 0 };
    int _length{ -1 };
    int _index{ -1 };
    bool _validTexture{ false };
    glm::uvec2 _size;
    ovrTextureSwapChain* _swapChain{ nullptr };
};

}  // namespace ovr