#version 120
 
attribute float voxelSizeIn;
varying float voxelSize;

uniform float viewportWidth;
uniform float viewportHeight;
uniform vec3 cameraPosition;

// Bit codes for faces
const int NONE = 0;
const int RIGHT = 1;
const int LEFT = 2;
const int BOTTOM = 4;
const int BOTTOM_RIGHT = BOTTOM + RIGHT;
const int BOTTOM_LEFT = BOTTOM + LEFT;
const int TOP = 8;
const int TOP_RIGHT = TOP + RIGHT;
const int TOP_LEFT = TOP + LEFT;
const int NEAR = 16;
const int NEAR_RIGHT = NEAR + RIGHT;
const int NEAR_LEFT = NEAR + LEFT;
const int NEAR_BOTTOM = NEAR + BOTTOM;
const int NEAR_BOTTOM_RIGHT = NEAR + BOTTOM + RIGHT;
const int NEAR_BOTTOM_LEFT = NEAR + BOTTOM + LEFT;
const int NEAR_TOP = NEAR + TOP;
const int NEAR_TOP_RIGHT = NEAR + TOP + RIGHT;
const int NEAR_TOP_LEFT = NEAR + TOP + LEFT;
const int FAR = 32;
const int FAR_RIGHT = FAR + RIGHT;
const int FAR_LEFT = FAR + LEFT;
const int FAR_BOTTOM = FAR + BOTTOM;
const int FAR_BOTTOM_RIGHT = FAR + BOTTOM + RIGHT;
const int FAR_BOTTOM_LEFT = FAR + BOTTOM + LEFT;
const int FAR_TOP = FAR + TOP;
const int FAR_TOP_RIGHT = FAR + TOP + RIGHT;
const int FAR_TOP_LEFT = FAR + TOP + LEFT;

// If we know the position of the camera relative to the voxel, we can a priori know the vertices that make the visible hull
// polygon. This also tells us which two vertices are known to make the longest possible distance between any pair of these
// vertices for the projected polygon. This is a visibleFaces table based on this knowledge. 

void main(void) {
    // Note: the gl_Vertex in this case are in "world coordinates" meaning they've already been scaled to TREE_SCALE
    // this is also true for voxelSizeIn.
    vec4 bottomNearRight = gl_Vertex;
    vec4 topFarLeft = (gl_Vertex + vec4(voxelSizeIn, voxelSizeIn, voxelSizeIn, 0.0));

    int visibleFaces = NONE;

    // In order to use our visibleFaces "table" (implemented as if statements) below, we need to encode the 6-bit code to 
    // orient camera relative to the 6 defining faces of the voxel. Based on camera position relative to the bottomNearRight 
    // corner and the topFarLeft corner, we can calculate which hull and therefore which two vertices are furthest apart 
    // linearly once projected
    if (cameraPosition.x < bottomNearRight.x) {
        visibleFaces += RIGHT;
    }
    if (cameraPosition.x > topFarLeft.x) {
        visibleFaces += LEFT;
    }    
    if (cameraPosition.y < bottomNearRight.y) {
        visibleFaces += BOTTOM;
    }    
    if (cameraPosition.y > topFarLeft.y) {
        visibleFaces += TOP;
    }    
    if (cameraPosition.z < bottomNearRight.z) {
        visibleFaces += NEAR;
    }    
    if (cameraPosition.z > topFarLeft.z) {
        visibleFaces += FAR;
    }
    
    vec4 cornerAdjustOne;
    vec4 cornerAdjustTwo;
    
    if (visibleFaces == RIGHT) {
        cornerAdjustOne = vec4(0,0,0,0) * voxelSizeIn;
        cornerAdjustTwo = vec4(0,1,1,0) * voxelSizeIn;
    } else if (visibleFaces == LEFT) {
        cornerAdjustOne = vec4(1,0,0,0) * voxelSizeIn;
        cornerAdjustTwo = vec4(1,1,1,0) * voxelSizeIn;
    } else if (visibleFaces == BOTTOM) {
        cornerAdjustOne = vec4(0,0,0,0) * voxelSizeIn;
        cornerAdjustTwo = vec4(1,0,1,0) * voxelSizeIn;
    } else if (visibleFaces == TOP) {
        cornerAdjustOne = vec4(0,1,0,0) * voxelSizeIn;
        cornerAdjustTwo = vec4(1,1,1,0) * voxelSizeIn;
    } else if (visibleFaces == NEAR) {
        cornerAdjustOne = vec4(0,0,0,0) * voxelSizeIn;
        cornerAdjustTwo = vec4(1,1,0,0) * voxelSizeIn;
    } else if (visibleFaces == FAR) {
        cornerAdjustOne = vec4(0,0,1,0) * voxelSizeIn;
        cornerAdjustTwo = vec4(1,1,1,0) * voxelSizeIn;
    } else if (visibleFaces == NEAR_BOTTOM_LEFT ||
               visibleFaces == FAR_TOP ||
               visibleFaces == FAR_TOP_RIGHT) {
        cornerAdjustOne = vec4(0,1,0,0) * voxelSizeIn;
        cornerAdjustTwo = vec4(1,0,1,0) * voxelSizeIn;
    } else if (visibleFaces == FAR_TOP_LEFT ||
               visibleFaces == NEAR_RIGHT ||
               visibleFaces == NEAR_BOTTOM ||
               visibleFaces == NEAR_BOTTOM_RIGHT) {
        cornerAdjustOne = vec4(0,0,1,0) * voxelSizeIn;
        cornerAdjustTwo = vec4(1,1,0,0) * voxelSizeIn;
    } else if (visibleFaces == NEAR_TOP_RIGHT ||
               visibleFaces == FAR_LEFT ||
               visibleFaces == FAR_BOTTOM_LEFT ||
               visibleFaces == BOTTOM_RIGHT ||
               visibleFaces == TOP_LEFT) {
        cornerAdjustOne = vec4(1,0,0,0) * voxelSizeIn;
        cornerAdjustTwo = vec4(0,1,1,0) * voxelSizeIn;
        
    // Everything else... 
    //} else if (visibleFaces == BOTTOM_LEFT || 
    //           visibleFaces == TOP_RIGHT ||
    //           visibleFaces == NEAR_LEFT ||
    //           visibleFaces == FAR_RIGHT ||
    //           visibleFaces == NEAR_TOP ||
    //           visibleFaces == NEAR_TOP_LEFT ||
    //           visibleFaces == FAR_BOTTOM ||
    //           visibleFaces == FAR_BOTTOM_RIGHT) {
    } else {
        cornerAdjustOne = vec4(0,0,0,0) * voxelSizeIn;
        cornerAdjustTwo = vec4(1,1,1,0) * voxelSizeIn;
    }

    // Determine our corners
    vec4 cornerOne = gl_Vertex + cornerAdjustOne;
    vec4 cornerTwo = gl_Vertex + cornerAdjustTwo;

    // Find their model view projections
    vec4 cornerOneMVP = gl_ModelViewProjectionMatrix * cornerOne;
    vec4 cornerTwoMVP = gl_ModelViewProjectionMatrix * cornerTwo;

    // Map to x, y screen coordinates
    vec2 cornerOneScreen = vec2(cornerOneMVP.x / cornerOneMVP.w, cornerOneMVP.y / cornerOneMVP.w);
    if (cornerOneMVP.w < 0) {
        cornerOneScreen.x = -cornerOneScreen.x;
        cornerOneScreen.y = -cornerOneScreen.y;
    }

    vec2 cornerTwoScreen = vec2(cornerTwoMVP.x / cornerTwoMVP.w, cornerTwoMVP.y / cornerTwoMVP.w);
    if (cornerTwoMVP.w < 0) {
        cornerTwoScreen.x = -cornerTwoScreen.x;
        cornerTwoScreen.y = -cornerTwoScreen.y;
    }

    // Find the distance between them in pixels
    float voxelScreenWidth = abs(cornerOneScreen.x - cornerTwoScreen.x) * viewportWidth / 2.0;
    float voxelScreenHeight = abs(cornerOneScreen.y - cornerTwoScreen.y) * viewportHeight / 2.0;
    float voxelScreenLength = sqrt(voxelScreenHeight * voxelScreenHeight + voxelScreenWidth * voxelScreenWidth);
    
    // Find the center of the voxel
    vec4 centerVertex = gl_Vertex;
    float halfSizeIn = voxelSizeIn / 2;
    centerVertex += vec4(halfSizeIn, halfSizeIn, halfSizeIn, 0.0);
    vec4 center = gl_ModelViewProjectionMatrix * centerVertex;

    // Finally place the point at the center of the voxel, with a size equal to the maximum screen length    
    gl_Position = center;
    gl_PointSize = voxelScreenLength;
    gl_FrontColor = gl_Color;
}