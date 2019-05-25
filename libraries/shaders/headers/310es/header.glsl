#define BITFIELD highp int
#define LAYOUT(X) layout(X)
#define LAYOUT_STD140(X) layout(std140, X)
#ifdef VULKAN
    #define gl_InstanceID  gl_InstanceIndex
    #define gl_VertexID  gl_VertexIndex
#endif
#extension GL_EXT_texture_buffer : enable
#if defined(HAVE_EXT_clip_cull_distance) && !defined(VULKAN)
#extension GL_EXT_clip_cull_distance : enable
#endif
precision highp float;
precision highp samplerBuffer;
precision highp sampler2DShadow;
precision highp sampler2DArrayShadow;
precision lowp sampler2DArray;
