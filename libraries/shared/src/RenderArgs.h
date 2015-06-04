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

class AABox;
class OctreeRenderer;
class ViewFrustum;
namespace gpu {
class Batch;
class Context;
}

struct RenderDetails {
    int _materialSwitches = 0;
    int _trianglesRendered = 0;
    int _quadsRendered = 0;
    
    int _translucentMeshPartsRendered = 0;
    int _opaqueMeshPartsRendered = 0;
};

class RenderArgs {
public:
    typedef std::function<bool(const RenderArgs* args, const AABox& bounds)> ShoudRenderFunctor;
    
    enum RenderMode { DEFAULT_RENDER_MODE, SHADOW_RENDER_MODE, DIFFUSE_RENDER_MODE, NORMAL_RENDER_MODE, MIRROR_RENDER_MODE };

    enum RenderSide { MONO, STEREO_LEFT, STEREO_RIGHT };

    enum DebugFlags {
        RENDER_DEBUG_NONE = 0,
        RENDER_DEBUG_HULLS = 1,
        RENDER_DEBUG_SIMULATION_OWNERSHIP = 2,
    };
    
    RenderArgs(gpu::Context* context = nullptr,
               OctreeRenderer* renderer = nullptr,
               ViewFrustum* viewFrustum = nullptr,
               float sizeScale = 1.0f,
               int boundaryLevelAdjust = 0,
               RenderMode renderMode = DEFAULT_RENDER_MODE,
               RenderSide renderSide = MONO,
               DebugFlags debugFlags = RENDER_DEBUG_NONE,
               gpu::Batch* batch = nullptr,
               ShoudRenderFunctor shouldRender = nullptr) :
    _context(context),
    _renderer(renderer),
    _viewFrustum(viewFrustum),
    _sizeScale(sizeScale),
    _boundaryLevelAdjust(boundaryLevelAdjust),
    _renderMode(renderMode),
    _renderSide(renderSide),
    _debugFlags(debugFlags),
    _batch(batch),
    _shouldRender(shouldRender) {
    }

    gpu::Context* _context = nullptr;
    OctreeRenderer* _renderer = nullptr;
    ViewFrustum* _viewFrustum = nullptr;
    float _sizeScale = 1.0f;
    int _boundaryLevelAdjust = 0;
    RenderMode _renderMode = DEFAULT_RENDER_MODE;
    RenderSide _renderSide = MONO;
    DebugFlags _debugFlags = RENDER_DEBUG_NONE;
    gpu::Batch* _batch = nullptr;
    ShoudRenderFunctor _shouldRender;
    
    RenderDetails _details;

    float _alphaThreshold = 0.5f;
};

#endif // hifi_RenderArgs_h
