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
#include <glad/glad.h>

#include <VrApi_Types.h>

namespace ovr {

struct Framebuffer {
public:
    void updateLayer(int eye, ovrLayerProjection2& layer, const ovrMatrix4f* projectionMatrix = nullptr) const;
    void create(const glm::uvec2& size);
    void advance();
    void destroy();
    void bind(GLenum target = GL_DRAW_FRAMEBUFFER);
    void invalidate(GLenum target = GL_DRAW_FRAMEBUFFER);
    void drawBuffers(ovrEye eye) const;

    const glm::uvec2& size() const { return _size; }

private:
    uint32_t _fbo{ 0 };
    glm::uvec2 _size;
    struct SwapChainInfo {
        int length{ -1 };
        int index{ -1 };
        bool validTexture{ false };
        ovrTextureSwapChain* swapChain{ nullptr };

        void create(const glm::uvec2& size);
        void destroy();
        void advance();
        void bind(GLenum target, GLenum attachment);
    };

    SwapChainInfo _swapChainInfos[VRAPI_FRAME_LAYER_EYE_MAX];
};

}  // namespace ovr