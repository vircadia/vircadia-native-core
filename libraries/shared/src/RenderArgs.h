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
#include <stack>

#include <GLMHelpers.h>
#include <ViewFrustum.h>



class AABox;
class OctreeRenderer;

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
        ITEM,
        SHADOW,
        OTHER
    };

    struct Item {
        int _considered = 0;
        int _outOfView = 0;
        int _tooSmall = 0;
        int _rendered = 0;
    };

    int _materialSwitches = 0;
    int _trianglesRendered = 0;

    Item _item;
    Item _shadow;
    Item _other;

    Item& edit(Type type) {
        switch (type) {
            case SHADOW:
                return _shadow;
            case ITEM:
                return _item;
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
               QSharedPointer<OctreeRenderer> renderer = QSharedPointer<OctreeRenderer>(nullptr),
               float sizeScale = 1.0f,
               int boundaryLevelAdjust = 0,
               RenderMode renderMode = DEFAULT_RENDER_MODE,
               RenderSide renderSide = MONO,
               DebugFlags debugFlags = RENDER_DEBUG_NONE,
               gpu::Batch* batch = nullptr) :
    _context(context),
    _renderer(renderer),
    _sizeScale(sizeScale),
    _boundaryLevelAdjust(boundaryLevelAdjust),
    _renderMode(renderMode),
    _renderSide(renderSide),
    _debugFlags(debugFlags),
    _batch(batch) {
    }

    bool hasViewFrustum() const { return _viewFrustums.size() > 0; }
    void setViewFrustum(const ViewFrustum& viewFrustum) {
        while (_viewFrustums.size() > 0) {
            _viewFrustums.pop();
        }
        _viewFrustums.push(viewFrustum);
    }
    const ViewFrustum& getViewFrustum() const { assert(_viewFrustums.size() > 0); return _viewFrustums.top(); }
    void pushViewFrustum(const ViewFrustum& viewFrustum) { _viewFrustums.push(viewFrustum); }
    void popViewFrustum() { _viewFrustums.pop(); }

    std::shared_ptr<gpu::Context> _context = nullptr;
    std::shared_ptr<gpu::Framebuffer> _blitFramebuffer = nullptr;
    std::shared_ptr<render::ShapePipeline> _pipeline = nullptr;
    QSharedPointer<OctreeRenderer> _renderer;
    std::stack<ViewFrustum> _viewFrustums;
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
