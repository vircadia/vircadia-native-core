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
    enum RenderMode { DEFAULT_RENDER_MODE, SHADOW_RENDER_MODE, DIFFUSE_RENDER_MODE, NORMAL_RENDER_MODE };
    enum RenderSide { MONO, STEREO_LEFT, STEREO_RIGHT };

    OctreeRenderer* _renderer;
    ViewFrustum* _viewFrustum;
    float _sizeScale;
    int _boundaryLevelAdjust;
    RenderMode _renderMode;
    RenderSide _renderSide;

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
