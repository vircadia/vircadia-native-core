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

class ViewFrustum;
class OctreeRenderer;
namespace gpu {
class Batch;
class Context;
}

class RenderArgs {
public:
    enum RenderMode { DEFAULT_RENDER_MODE, SHADOW_RENDER_MODE, DIFFUSE_RENDER_MODE, NORMAL_RENDER_MODE, MIRROR_RENDER_MODE };

    enum RenderSide { MONO, STEREO_LEFT, STEREO_RIGHT };

    enum DebugFlags {
        RENDER_DEBUG_NONE = 0,
        RENDER_DEBUG_HULLS = 1,
        RENDER_DEBUG_SIMULATION_OWNERSHIP = 2,
    };
    
    RenderArgs(OctreeRenderer* renderer = nullptr,
               ViewFrustum* viewFrustum = nullptr,
               float sizeScale = 1.0f,
               int boundaryLevelAdjust = 0,
               RenderMode renderMode = DEFAULT_RENDER_MODE,
               RenderSide renderSide = MONO,
               DebugFlags debugFlags = RENDER_DEBUG_NONE,
               gpu::Batch* batch = nullptr,
               gpu::Context* context = nullptr,
               
               int elementsTouched = 0,
               int itemsRendered = 0,
               int itemsOutOfView = 0,
               int itemsTooSmall = 0,
               
               int meshesConsidered = 0,
               int meshesRendered = 0,
               int meshesOutOfView = 0,
               int meshesTooSmall = 0,
               
               int materialSwitches = 0,
               int trianglesRendered = 0,
               int quadsRendered = 0,
               
               int translucentMeshPartsRendered = 0,
               int opaqueMeshPartsRendered = 0) :
    _renderer(renderer),
    _viewFrustum(viewFrustum),
    _sizeScale(sizeScale),
    _boundaryLevelAdjust(boundaryLevelAdjust),
    _renderMode(renderMode),
    _renderSide(renderSide),
    _debugFlags(debugFlags),
    _batch(batch),
    
    _elementsTouched(elementsTouched),
    _itemsRendered(itemsRendered),
    _itemsOutOfView(itemsOutOfView),
    _itemsTooSmall(itemsTooSmall),
    
    _meshesConsidered(meshesConsidered),
    _meshesRendered(meshesRendered),
    _meshesOutOfView(meshesOutOfView),
    _meshesTooSmall(meshesTooSmall),
    
    _materialSwitches(materialSwitches),
    _trianglesRendered(trianglesRendered),
    _quadsRendered(quadsRendered),
    
    _translucentMeshPartsRendered(translucentMeshPartsRendered),
    _opaqueMeshPartsRendered(opaqueMeshPartsRendered) {
    }

    OctreeRenderer* _renderer;
    ViewFrustum* _viewFrustum;
    float _sizeScale;
    int _boundaryLevelAdjust;
    RenderMode _renderMode;
    RenderSide _renderSide;
    DebugFlags _debugFlags;
    gpu::Batch* _batch;
    gpu::Context* _context;

    int _elementsTouched;
    int _itemsRendered;
    int _itemsOutOfView;
    int _itemsTooSmall;

    int _meshesConsidered;
    int _meshesRendered;
    int _meshesOutOfView;
    int _meshesTooSmall;

    int _materialSwitches;
    int _trianglesRendered;
    int _quadsRendered;

    int _translucentMeshPartsRendered;
    int _opaqueMeshPartsRendered;
};

#endif // hifi_RenderArgs_h
