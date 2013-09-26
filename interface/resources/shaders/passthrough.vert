#version 120
 
attribute float voxelSizeIn;
varying float voxelSize;

void main(void) {
        gl_FrontColor = gl_Color; //vec4(1.0, 0.0, 0.0, 1.0);
        gl_Position = gl_ModelViewMatrix * gl_Vertex;// ftransform();
        voxelSize = voxelSizeIn;
}