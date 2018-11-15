// <!
//  Created by Olivier Prat on 2018/09/18
//  Copyright 2013-2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
// !>

// <@if not RENDER_UTILS_SSAO_SHARED_H@>
// <@def RENDER_UTILS_SSAO_SHARED_H@>

// Hack comment to absorb the extra '//' scribe prepends

#ifndef RENDER_UTILS_SSAO_SHARED_H
#define RENDER_UTILS_SSAO_SHARED_H

#define SSAO_USE_QUAD_SPLIT	1
#define SSAO_BILATERAL_BLUR_USE_NORMAL 0

#define SSAO_DEPTH_KEY_SCALE 300.0

#if SSAO_USE_QUAD_SPLIT
#define SSAO_SPLIT_LOG2_COUNT  2
#else
#define SSAO_SPLIT_LOG2_COUNT  0
#endif
#define SSAO_SPLIT_COUNT  (1 << SSAO_SPLIT_LOG2_COUNT)

// glsl / C++ compatible source as interface for ambient occlusion
#ifdef __cplusplus
#   define SSAO_VEC4 glm::vec4
#   define SSAO_MAT4 glm::mat4
#else
#   define SSAO_VEC4 vec4
#   define SSAO_MAT4 mat4
#endif

struct AmbientOcclusionParams {
    SSAO_VEC4 _resolutionInfo;
    SSAO_VEC4 _radiusInfo;
    SSAO_VEC4 _ditheringInfo;
    SSAO_VEC4 _sampleInfo;
    SSAO_VEC4 _falloffInfo;
    SSAO_VEC4 _sideSizes[2];
};

struct AmbientOcclusionFrameParams {
    SSAO_VEC4 _angleInfo;
};

struct AmbientOcclusionBlurParams {
    SSAO_VEC4 _blurInfo;
    SSAO_VEC4 _blurAxis;
};

#endif // RENDER_UTILS_SHADER_CONSTANTS_H

// <@if 1@>
// Trigger Scribe include
// <@endif@> <!def that !>

// <@endif@>

// Hack Comment