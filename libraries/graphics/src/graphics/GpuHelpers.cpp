//
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
//

#include "GpuHelpers.h"

/*@jsdoc
 * <p>The interpretation of mesh elements.</p>
 * <table>
 *   <thead>
 *     <tr><th>Value</th><th>Description</th></tr>
 *   </thead>
 *   <tbody>
 *     <tr><td><code>"points"</code></td><td>Points.</td></tr>
 *     <tr><td><code>"lines"</code></td><td>Lines.</td></tr>
 *     <tr><td><code>"line_strip"</code></td><td>Line strip.</td></tr>
 *     <tr><td><code>"triangles"</code></td><td>Triangles.</td></tr>
 *     <tr><td><code>"triangle_strip"</code></td><td>Triangle strip.</td></tr>
 *     <tr><td><code>"quads"</code></td><td>Quads.</td></tr>
 *     <tr><td><code>"quad_strip"</code></td><td>Quad strip.</td></tr>
 *   </tbody>
 * </table>
 * @typedef {string} Graphics.MeshTopology
 */
namespace graphics {
    DebugEnums<Mesh::Topology> TOPOLOGIES{
       { Mesh::Topology::POINTS, "points" },
       { Mesh::Topology::LINES, "lines" },
       { Mesh::Topology::LINE_STRIP, "line_strip" },
       { Mesh::Topology::TRIANGLES, "triangles" },
       { Mesh::Topology::TRIANGLE_STRIP, "triangle_strip" },
       { Mesh::Topology::QUADS, "quads" },
       { Mesh::Topology::QUAD_STRIP, "quad_strip" },
       { Mesh::Topology::NUM_TOPOLOGIES, "num_topologies" },
    };
}
namespace gpu {
    DebugEnums<Type> TYPES{
        { Type::FLOAT, "float" },
        { Type::INT32, "int32" },
        { Type::UINT32, "uint32" },
        { Type::HALF, "half" },
        { Type::INT16, "int16" },
        { Type::UINT16, "uint16" },
        { Type::INT8, "int8" },
        { Type::UINT8, "uint8" },
        { Type::NINT32, "nint32" },
        { Type::NUINT32, "nuint32" },
        { Type::NINT16, "nint16" },
        { Type::NUINT16, "nuint16" },
        { Type::NINT8, "nint8" },
        { Type::NUINT8, "nuint8" },
        { Type::NUINT2, "nuint2" },
        { Type::NINT2_10_10_10, "nint2_10_10_10" },
        { Type::COMPRESSED, "compressed" },
        { Type::NUM_TYPES, "num_types" },
    };
    DebugEnums<Dimension> DIMENSIONS{
        { Dimension::SCALAR, "scalar" },
        { Dimension::VEC2, "vec2" },
        { Dimension::VEC3, "vec3" },
        { Dimension::VEC4, "vec4" },
        { Dimension::MAT2, "mat2" },
        { Dimension::MAT3, "mat3" },
        { Dimension::MAT4, "mat4" },
        { Dimension::TILE4x4, "tile4x4" },
        { Dimension::NUM_DIMENSIONS, "num_dimensions" },
    };
    DebugEnums<Semantic> SEMANTICS{
        { Semantic::RAW, "raw" },

        { Semantic::RED, "red" },
        { Semantic::RGB, "rgb" },
        { Semantic::RGBA, "rgba" },
        { Semantic::BGRA, "bgra" },

        { Semantic::XY, "xy" },
        { Semantic::XYZ, "xyz" },
        { Semantic::XYZW, "xyzw" },
        { Semantic::QUAT, "quat" },
        { Semantic::UV, "uv" },
        { Semantic::INDEX, "index" },
        { Semantic::PART, "part" },

        { Semantic::DEPTH, "depth" },
        { Semantic::STENCIL, "stencil" },
        { Semantic::DEPTH_STENCIL, "depth_stencil" },

        { Semantic::SRED, "sred" },
        { Semantic::SRGB, "srgb" },
        { Semantic::SRGBA, "srgba" },
        { Semantic::SBGRA, "sbgra" },

        { Semantic::_FIRST_COMPRESSED, "_first_compressed" },

        { Semantic::COMPRESSED_BC1_SRGB, "compressed_bc1_srgb" },
        { Semantic::COMPRESSED_BC1_SRGBA, "compressed_bc1_srgba" },
        { Semantic::COMPRESSED_BC3_SRGBA, "compressed_bc3_srgba" },
        { Semantic::COMPRESSED_BC4_RED, "compressed_bc4_red" },
        { Semantic::COMPRESSED_BC5_XY, "compressed_bc5_xy" },
        { Semantic::COMPRESSED_BC6_RGB, "compressed_bc6_rgb" },
        { Semantic::COMPRESSED_BC7_SRGBA, "compressed_bc7_srgba" },

        { Semantic::COMPRESSED_ETC2_RGB, "compressed_etc2_rgb" },
        { Semantic::COMPRESSED_ETC2_SRGB, "compressed_etc2_srgb" },
        { Semantic::COMPRESSED_ETC2_RGB_PUNCHTHROUGH_ALPHA, "compressed_etc2_rgb_punchthrough_alpha" },
        { Semantic::COMPRESSED_ETC2_SRGB_PUNCHTHROUGH_ALPHA, "compressed_etc2_srgb_punchthrough_alpha" },
        { Semantic::COMPRESSED_ETC2_RGBA, "compressed_etc2_rgba" },
        { Semantic::COMPRESSED_ETC2_SRGBA, "compressed_etc2_srgba" },
        { Semantic::COMPRESSED_EAC_RED, "compressed_eac_red" },
        { Semantic::COMPRESSED_EAC_RED_SIGNED, "compressed_eac_red_signed" },
        { Semantic::COMPRESSED_EAC_XY, "compressed_eac_xy" },
        { Semantic::COMPRESSED_EAC_XY_SIGNED, "compressed_eac_xy_signed" },

        { Semantic::_LAST_COMPRESSED, "_last_compressed" },

        { Semantic::R11G11B10, "r11g11b10" },
        { Semantic::RGB9E5, "rgb9e5" },

        { Semantic::UNIFORM, "uniform" },
        { Semantic::UNIFORM_BUFFER, "uniform_buffer" },
        { Semantic::RESOURCE_BUFFER, "resource_buffer" },
        { Semantic::SAMPLER, "sampler" },
        { Semantic::SAMPLER_MULTISAMPLE, "sampler_multisample" },
        { Semantic::SAMPLER_SHADOW, "sampler_shadow" },


        { Semantic::NUM_SEMANTICS, "num_semantics" },
    };
    DebugEnums<Stream::InputSlot> SLOTS{
        { Stream::InputSlot::POSITION, "position" },
        { Stream::InputSlot::NORMAL, "normal" },
        { Stream::InputSlot::COLOR, "color" },
        { Stream::InputSlot::TEXCOORD0, "texcoord0" },
        { Stream::InputSlot::TEXCOORD, "texcoord" },
        { Stream::InputSlot::TANGENT, "tangent" },
        { Stream::InputSlot::SKIN_CLUSTER_INDEX, "skin_cluster_index" },
        { Stream::InputSlot::SKIN_CLUSTER_WEIGHT, "skin_cluster_weight" },
        { Stream::InputSlot::TEXCOORD1, "texcoord1" },
        { Stream::InputSlot::TEXCOORD2, "texcoord2" },
        { Stream::InputSlot::TEXCOORD3, "texcoord3" },
        { Stream::InputSlot::TEXCOORD4, "texcoord4" },
        { Stream::InputSlot::NUM_INPUT_SLOTS, "num_input_slots" },
        { Stream::InputSlot::DRAW_CALL_INFO, "draw_call_info" },
    };
}
