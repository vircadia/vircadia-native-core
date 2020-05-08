//
//  Created by Brad Hefta-Gaub on 10/29/14.
//  Copyright 2013-2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_render_Args_h
#define hifi_render_Args_h

#include <functional>
#include <memory>
#include <stack>
#include <unordered_set>

#include <GLMHelpers.h>
#include <ViewFrustum.h>
#include <StencilMaskMode.h>
#include <UUIDHasher.h>

#include <gpu/Forward.h>
#include "Forward.h"

class AABox;

namespace render {
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


    class Args {
    public:
        enum RenderMode { DEFAULT_RENDER_MODE, SHADOW_RENDER_MODE, MIRROR_RENDER_MODE, SECONDARY_CAMERA_RENDER_MODE };
        enum DisplayMode { MONO, STEREO_MONITOR, STEREO_HMD };
        enum RenderMethod { DEFERRED, FORWARD };
        enum DebugFlags {
            RENDER_DEBUG_NONE = 0,
            RENDER_DEBUG_HULLS = 1
        };

        Args() {}

        Args(const gpu::ContextPointer& context,
            float sizeScale = 1.0f,
            int boundaryLevelAdjust = 0,
            float lodAngleHalfTan = 0.1f,
            RenderMode renderMode = DEFAULT_RENDER_MODE,
            DisplayMode displayMode = MONO,
            RenderMethod renderMethod = DEFERRED,
            DebugFlags debugFlags = RENDER_DEBUG_NONE,
            gpu::Batch* batch = nullptr) :
            _context(context),
            _sizeScale(sizeScale),
            _boundaryLevelAdjust(boundaryLevelAdjust),
            _lodAngleHalfTan(lodAngleHalfTan),
            _lodAngleHalfTanSq(lodAngleHalfTan * lodAngleHalfTan),
            _renderMode(renderMode),
            _displayMode(displayMode),
            _renderMethod(renderMethod),
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

        bool isStereo() const { return _displayMode != MONO; }

        std::shared_ptr<gpu::Context> _context;
        std::shared_ptr<gpu::Framebuffer> _blitFramebuffer;
        std::shared_ptr<render::ShapePipeline> _shapePipeline;
        std::stack<ViewFrustum> _viewFrustums;
        glm::ivec4 _viewport { 0.0f, 0.0f, 1.0f, 1.0f };
        glm::vec3 _boomOffset { 0.0f, 0.0f, 1.0f };

        float _sizeScale { 1.0f };
        int _boundaryLevelAdjust { 0 };
        float _lodAngleHalfTan{ 0.1f };
        float _lodAngleHalfTanSq{ _lodAngleHalfTan  * _lodAngleHalfTan };

        RenderMode _renderMode { DEFAULT_RENDER_MODE };
        DisplayMode _displayMode { MONO };
        RenderMethod _renderMethod { DEFERRED };
        DebugFlags _debugFlags { RENDER_DEBUG_NONE };
        gpu::Batch* _batch = nullptr;

        uint32_t _globalShapeKey { 0 };
        uint32_t _itemShapeKey { 0 };
        bool _enableTexturing { true };
        bool _enableBlendshape { true };
        bool _enableSkinning { true };

        bool _enableFade { false };

        RenderDetails _details;
        render::ScenePointer _scene;
        int8_t _cameraMode { -1 };

        std::function<void(gpu::Batch&, const gpu::TexturePointer&)> _hudOperator { nullptr };
        gpu::TexturePointer _hudTexture { nullptr };

        bool _takingSnapshot { false };
        StencilMaskMode _stencilMaskMode { StencilMaskMode::NONE };
        std::function<void(gpu::Batch&)> _stencilMaskOperator;
    };

}

using RenderArgs = render::Args;

#endif // hifi_render_Args_h
