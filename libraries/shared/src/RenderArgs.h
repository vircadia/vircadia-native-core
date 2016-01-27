//
//  RenderArgs.h
//  libraries/shared
//
//  Created by Brad Hefta-Gaub on 10/29/14.
//  Copyright 2013-2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_RenderArgs_h
#define hifi_RenderArgs_h

#include <functional>
#include <memory>
#include "GLMHelpers.h"


class AABox;
class OctreeRenderer;
class ViewFrustum;

namespace gpu {
class Batch;
class Context;
class Texture;
class Framebuffer;
}

namespace render {
class ShapePipeline;
}

class RenderDetails {
public:
    enum Type {
        OPAQUE_ITEM,
        SHADOW_ITEM,
        TRANSLUCENT_ITEM,
        OTHER_ITEM
    };
    
    struct Item {
        size_t _considered = 0;
        size_t _rendered = 0;
        int _outOfView = 0;
        int _tooSmall = 0;
    };
    
    int _materialSwitches = 0;
    int _trianglesRendered = 0;
    
    Item _opaque;
    Item _shadow;
    Item _translucent;
    Item _other;
    
    Item& edit(Type type) {
        switch (type) {
            case OPAQUE_ITEM:
                return _opaque;
            case SHADOW_ITEM:
                return _shadow;
            case TRANSLUCENT_ITEM:
                return _translucent;
            case OTHER_ITEM:
            default:
                return _other;
        }
    }
};

class RenderArgs {
public:
    enum RenderMode { DEFAULT_RENDER_MODE, SHADOW_RENDER_MODE, DIFFUSE_RENDER_MODE, NORMAL_RENDER_MODE, MIRROR_RENDER_MODE };
    enum RenderSide { MONO, STEREO_LEFT, STEREO_RIGHT };
    enum DebugFlags {
        RENDER_DEBUG_NONE = 0,
        RENDER_DEBUG_HULLS = 1
    };

    RenderArgs(std::shared_ptr<gpu::Context> context = nullptr,
               OctreeRenderer* renderer = nullptr,
               ViewFrustum* viewFrustum = nullptr,
               float sizeScale = 1.0f,
               int boundaryLevelAdjust = 0,
               RenderMode renderMode = DEFAULT_RENDER_MODE,
               RenderSide renderSide = MONO,
               DebugFlags debugFlags = RENDER_DEBUG_NONE,
               gpu::Batch* batch = nullptr) :
    _context(context),
    _renderer(renderer),
    _viewFrustum(viewFrustum),
    _sizeScale(sizeScale),
    _boundaryLevelAdjust(boundaryLevelAdjust),
    _renderMode(renderMode),
    _renderSide(renderSide),
    _debugFlags(debugFlags),
    _batch(batch) {
    }

    std::shared_ptr<gpu::Context> _context = nullptr;
    std::shared_ptr<gpu::Framebuffer> _blitFramebuffer = nullptr;
    std::shared_ptr<render::ShapePipeline> _pipeline = nullptr;
    OctreeRenderer* _renderer = nullptr;
    ViewFrustum* _viewFrustum = nullptr;
    glm::ivec4 _viewport{ 0.0f, 0.0f, 1.0f, 1.0f };
    glm::vec3 _boomOffset{ 0.0f, 0.0f, 1.0f };
    float _sizeScale = 1.0f;
    int _boundaryLevelAdjust = 0;
    RenderMode _renderMode = DEFAULT_RENDER_MODE;
    RenderSide _renderSide = MONO;
    DebugFlags _debugFlags = RENDER_DEBUG_NONE;
    gpu::Batch* _batch = nullptr;
    
    std::shared_ptr<gpu::Texture> _whiteTexture;

    RenderDetails _details;
};

#endif // hifi_RenderArgs_h
