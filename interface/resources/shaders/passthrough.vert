#version 120
 
attribute float voxelSizeIn;
varying float voxelSize;

void main(void) {
    gl_FrontColor = gl_Color;
    gl_Position = gl_Vertex; // don't call ftransform(), because we do projection in geometry shader
    voxelSize = voxelSizeIn;
}