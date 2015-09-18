#line 2

//////////////////////////////////
//
//   Available inputs
// 
// Uniforms: constant across the whole surface
// 
// float iGlobalTime;
// vec3 iWorldScale;
//
// Varyings: Per-pixel attributes that change for every pixel
// 
// vec3 _normal
// vec4 _position
// vec2 _texCoord0 // reserved for future use, currently always vec2(0)
// vec3 _color // reserved for future user, currently always vec3(1)
// 
/////////////////////////////////

//////////////////////////////////
//
//   Available functions
//
// All GLSL functions from GLSL version 4.10 and usable in fragment shaders
// See Page 8 of this document:  https://www.khronos.org/files/opengl41-quick-reference-card.pdf
//
// Additionally the snoise functions defined in WebGL-noise are available 
// See https://github.com/ashima/webgl-noise/tree/master/src
// 
// float snoise(vec2)
// float snoise(vec3)
// float snoise(vec4)
//

// Fade from black to white and back again
vec4 getProceduralColor() {
    return vec4(vec3(abs((sin(iGlobalTime * 3.14159) + 1.0) / 2.0)), 1); // vec4(color, 1.0);
}
