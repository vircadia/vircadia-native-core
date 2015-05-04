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

class RenderArgs {
public:
    enum RenderMode { DEFAULT_RENDER_MODE, SHADOW_RENDER_MODE, DIFFUSE_RENDER_MODE, NORMAL_RENDER_MODE, MIRROR_RENDER_MODE };

    enum RenderSide { MONO, STEREO_LEFT, STEREO_RIGHT };

    enum DebugFlags {
        RENDER_DEBUG_NONE = 0,
        RENDER_DEBUG_HULLS = 1,
        RENDER_DEBUG_SIMULATION_OWNERSHIP = 2,
    };
    
    RenderArgs(OctreeRenderer* _renderer = nullptr,
               ViewFrustum* _viewFrustum = nullptr,
               float _sizeScale = 1.0f,
               int _boundaryLevelAdjust = 0,
               RenderMode _renderMode = DEFAULT_RENDER_MODE,
               RenderSide _renderSide = MONO,
               DebugFlags _debugFlags = RENDER_DEBUG_NONE,
               
               int _elementsTouched = 0,
               int _itemsRendered = 0,
               int _itemsOutOfView = 0,
               int _itemsTooSmall = 0,
               
               int _meshesConsidered = 0,
               int _meshesRendered = 0,
               int _meshesOutOfView = 0,
               int _meshesTooSmall = 0,
               
               int _materialSwitches = 0,
               int _trianglesRendered = 0,
               int _quadsRendered = 0,
               
               int _translucentMeshPartsRendered = 0,
               int _opaqueMeshPartsRendered = 0) {
    }

    OctreeRenderer* _renderer;
    ViewFrustum* _viewFrustum;
    float _sizeScale;
    int _boundaryLevelAdjust;
    RenderMode _renderMode;
    RenderSide _renderSide;
    DebugFlags _debugFlags;

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
