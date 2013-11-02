#version 120
 
attribute float voxelSizeIn;
varying float voxelSize;
uniform float viewportWidth;
uniform float viewportHeight;

void main(void) {
    gl_FrontColor = gl_Color;
    vec4 cornerOriginal = gl_Vertex;
    vec4 corner = gl_ModelViewProjectionMatrix * gl_Vertex;

    // I'm currently using the distance between the near bottom left corner and the far top right corner. That's not
    // ideal for all possible angles of viewing of the voxel. I really should be using a lookup table to determine which
    // corners to use. 
    //
    // The effect with the current approach is that some "projections" are smaller than they should be, and we get a 
    // thinning effect. However, this may not matter when we actually switch to using cubes and points in concert.
    //
    
    vec4 farCornerVertex = gl_Vertex;
    farCornerVertex += vec4(voxelSizeIn, voxelSizeIn, voxelSizeIn, 0.0);
    vec4 farCorner = gl_ModelViewProjectionMatrix * farCornerVertex;
    
    // math! If the w result is negative then the point is behind the viewer
    vec2 cornerOnScreen = vec2(corner.x / corner.w, corner.y / corner.w);
    if (corner.w < 0) {
        cornerOnScreen.x = -cornerOnScreen.x;
        cornerOnScreen.y = -cornerOnScreen.y;
    }

    vec2 farCornerOnScreen = vec2(farCorner.x / farCorner.w, farCorner.y / farCorner.w);
    if (farCorner.w < 0) {
        farCornerOnScreen.x = -farCornerOnScreen.x;
        farCornerOnScreen.y = -farCornerOnScreen.y;
    }

    float voxelScreenWidth = abs(farCornerOnScreen.x - cornerOnScreen.x) * viewportWidth / 2.0;
    float voxelScreenHeight = abs(farCornerOnScreen.y - cornerOnScreen.y) * viewportHeight / 2.0;
    float voxelScreenLength = sqrt(voxelScreenHeight * voxelScreenHeight + voxelScreenWidth * voxelScreenWidth);
    
    vec4 centerVertex = gl_Vertex;
    float halfSizeIn = voxelSizeIn / 2;
    centerVertex += vec4(halfSizeIn, halfSizeIn, halfSizeIn, 0.0);
    vec4 center = gl_ModelViewProjectionMatrix * centerVertex;
    
    gl_Position = center;
    gl_PointSize = voxelScreenLength;
}