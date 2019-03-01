#extension GL_ARB_shader_viewport_layer_array : require
#define GPU_SSBO_TRANSFORM_OBJECT
#define BITFIELD int
#define LAYOUT(X) layout(X)
#define LAYOUT_STD140(X) layout(std140, X)
#ifdef VULKAN
#define gl_InstanceID  gl_InstanceIndex
#define gl_VertexID  gl_VertexIndex
#endif
