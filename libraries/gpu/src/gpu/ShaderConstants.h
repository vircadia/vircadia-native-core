// <!
//  Created by Bradley Austin Davis on 2018/05/25
//  Copyright 2013-2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
// !>

// <@if not GPU_SHADER_CONSTANTS_H@>
// <@def GPU_SHADER_CONSTANTS_H@>

// Hack comment to absorb the extra '//' scribe prepends

#ifndef GPU_SHADER_CONSTANTS_H
#define GPU_SHADER_CONSTANTS_H

#define GPU_BUFFER_TRANSFORM_CAMERA 15
#define GPU_BUFFER_TEXTURE_TABLE0 16
#define GPU_BUFFER_TEXTURE_TABLE1 17
#define GPU_BUFFER_CAMERA_CORRECTION 18

#define GPU_TEXTURE_TRANSFORM_OBJECT 31

#define GPU_STORAGE_TRANSFORM_OBJECT 7

#define GPU_ATTR_POSITION 0
#define GPU_ATTR_NORMAL 1
#define GPU_ATTR_COLOR 2
#define GPU_ATTR_TEXCOORD0 3
#define GPU_ATTR_TANGENT 4
#define GPU_ATTR_SKIN_CLUSTER_INDEX 5
#define GPU_ATTR_SKIN_CLUSTER_WEIGHT 6
#define GPU_ATTR_TEXCOORD1 7
#define GPU_ATTR_TEXCOORD2 8
#define GPU_ATTR_TEXCOORD3 9
#define GPU_ATTR_TEXCOORD4 10
#define GPU_ATTR_STEREO_SIDE 14
#define GPU_ATTR_DRAW_CALL_INFO 15

// OSX seems to have an issue using 14 as an attribute location for passing from the vertex to the fragment shader
#define GPU_ATTR_V2F_STEREO_SIDE 8

#define GPU_UNIFORM_COLOR 101
#define GPU_UNIFORM_TEXCOORD_RECT 102
#define GPU_UNIFORM_EXTRA0 110
#define GPU_UNIFORM_EXTRA1 111
#define GPU_UNIFORM_EXTRA2 112
#define GPU_UNIFORM_EXTRA3 113
#define GPU_UNIFORM_EXTRA4 114
#define GPU_UNIFORM_EXTRA5 115
#define GPU_UNIFORM_EXTRA6 116
#define GPU_UNIFORM_EXTRA7 117
#define GPU_UNIFORM_EXTRA8 118
#define GPU_UNIFORM_EXTRA9 119

// <!

namespace gpu { namespace slot {

namespace buffer {
enum Buffer {
    CameraTransform = GPU_BUFFER_TRANSFORM_CAMERA,
    TextureTable0 = GPU_BUFFER_TEXTURE_TABLE0,
    TextureTable1 = GPU_BUFFER_TEXTURE_TABLE1,
    CameraCorrection = GPU_BUFFER_CAMERA_CORRECTION,
};
} // namespace buffer

namespace texture {
enum Texture {
    ObjectTransforms = GPU_TEXTURE_TRANSFORM_OBJECT,
}; 
} // namespace texture

namespace storage {
enum Storage {
    ObjectTransforms = GPU_STORAGE_TRANSFORM_OBJECT,
}; 
} // namespace storage

namespace attr {
enum Attribute {
    Position = GPU_ATTR_POSITION,
    Normal = GPU_ATTR_NORMAL,
    Color = GPU_ATTR_COLOR,
    TexCoord0 = GPU_ATTR_TEXCOORD0,
    Tangent = GPU_ATTR_TANGENT,
    SkinClusterIndex = GPU_ATTR_SKIN_CLUSTER_INDEX,
    SkinClusterWeight = GPU_ATTR_SKIN_CLUSTER_WEIGHT,
    TexCoord1 = GPU_ATTR_TEXCOORD1,
    TexCoord2 = GPU_ATTR_TEXCOORD2,
    TexCoord3 = GPU_ATTR_TEXCOORD3,
    TexCoord4 = GPU_ATTR_TEXCOORD4,
    StereoSide = GPU_ATTR_STEREO_SIDE,
    DrawCallInfo = GPU_ATTR_DRAW_CALL_INFO,
};
} // namespace attr

namespace uniform {
enum Uniform {
    Color = GPU_UNIFORM_COLOR,
    TexCoordRect = GPU_UNIFORM_TEXCOORD_RECT,
    Extra0 = GPU_UNIFORM_EXTRA0,
    Extra1 = GPU_UNIFORM_EXTRA1,
    Extra2 = GPU_UNIFORM_EXTRA2,
    Extra3 = GPU_UNIFORM_EXTRA3,
    Extra4 = GPU_UNIFORM_EXTRA4,
    Extra5 = GPU_UNIFORM_EXTRA5,
    Extra6 = GPU_UNIFORM_EXTRA6,
    Extra7 = GPU_UNIFORM_EXTRA7,
    Extra8 = GPU_UNIFORM_EXTRA8,
    Extra9 = GPU_UNIFORM_EXTRA9,
};
} // namespace uniform

} } // namespace gpu::slot

// !>

// Hack Comment
#endif // GPU_SHADER_CONSTANTS_H

// <@if 1@>
// Trigger Scribe include
// <@endif@> <!def that !>

// <@endif@>

// Hack Comment
