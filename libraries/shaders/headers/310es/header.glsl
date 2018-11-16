#version 310 es
#define GPU_GLES
#define GPU_GLES_310
#define BITFIELD highp int
#define LAYOUT(X) layout(X)
#define LAYOUT_STD140(X) layout(std140, X)
#ifdef VULKAN
    #define gl_InstanceID  gl_InstanceIndex
    #define gl_VertexID  gl_VertexIndex
#endif
#extension GL_EXT_texture_buffer : enable
precision highp float;
precision highp samplerBuffer;
precision highp sampler2DShadow;
precision highp sampler2DArrayShadow;
precision lowp sampler2DArray;
