#version 120
 
attribute float voxelSizeIn;
varying float voxelSize;
uniform float viewportWidth;
uniform float viewportHeight;

void main(void) {
    gl_FrontColor = gl_Color;
    vec4 cornerOriginal = gl_Vertex;
    vec4 corner = gl_ModelViewProjectionMatrix * gl_Vertex;

    vec4 farCornerVertex = gl_Vertex;
    farCornerVertex[0] += voxelSizeIn;
    farCornerVertex[1] += voxelSizeIn;
    farCornerVertex[2] += voxelSizeIn;
    vec4 farCorner = gl_ModelViewProjectionMatrix * farCornerVertex;

    float voxelScreenWidth = abs((farCorner.x / farCorner.w) - (corner.x / corner.w)) * viewportWidth / 2.0;
    float voxelScreenHeight = abs((farCorner.y / farCorner.w) - (corner.y / corner.w)) * viewportHeight / 2.0;
    float voxelScreenLength = sqrt(voxelScreenHeight * voxelScreenHeight + voxelScreenWidth * voxelScreenWidth);
    
    vec4 centerVertex = gl_Vertex;
    float halfSizeIn = voxelSizeIn / 2;
    centerVertex[0] += halfSizeIn;
    centerVertex[1] += halfSizeIn;
    centerVertex[2] += halfSizeIn;
    vec4 center = gl_ModelViewProjectionMatrix * centerVertex;
    
    gl_Position = center;
    gl_PointSize = voxelScreenLength;
}