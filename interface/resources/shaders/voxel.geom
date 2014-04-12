#version 120
#extension GL_ARB_geometry_shader4 : enable

//
//  voxel.geom
//  geometry shader
//
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

//
// VOXEL GEOMETRY SHADER
//
// Input: gl_VerticesIn/gl_PositionIn
//        GL_POINTS
//        Assumes vertex shader has not transformed coordinates
//        Each gl_PositionIn is the corner of voxel
//        varying voxelSize - which is the voxel size
//
// Note: In vertex shader doesn't do any transform. Therefore passing the 3D world coordinates xyz to us
// 
// Output: GL_TRIANGLE_STRIP
//
// Issues:
//          how do we need to handle lighting of these colors?? 
//          how do we handle normals?
//          check for size=0 and don't output the primitive
//

varying in float voxelSize[1];

const int VERTICES_PER_FACE = 4;
const int COORD_PER_VERTEX = 3;
const int COORD_PER_FACE = COORD_PER_VERTEX * VERTICES_PER_FACE;

void faceOfVoxel(vec4 corner, float scale, float[COORD_PER_FACE] facePoints, vec4 color, vec4 normal) {
    for (int v = 0; v < VERTICES_PER_FACE; v++ ) {
        vec4 vertex = corner;
        for (int c = 0; c < COORD_PER_VERTEX; c++ ) {
            int cIndex = c + (v * COORD_PER_VERTEX);
            vertex[c] += (facePoints[cIndex] * scale);
        }
        
        gl_FrontColor = color * (gl_LightModel.ambient + gl_LightSource[0].ambient +
            gl_LightSource[0].diffuse * max(0.0, dot(normal, gl_LightSource[0].position)));

        gl_Position = gl_ModelViewProjectionMatrix * vertex;
        EmitVertex();
    }
    EndPrimitive();
}


void main() {
    //increment variable
    int i;
    vec4 corner;
    float scale;

    float bottomFace[COORD_PER_FACE] = float[COORD_PER_FACE]( 0, 0, 0,   1, 0, 0,   0, 0, 1,   1, 0, 1  );
    float topFace[COORD_PER_FACE]    = float[COORD_PER_FACE]( 1, 1, 0,   0, 1, 0,   1, 1, 1,   0, 1, 1  );
    float rightFace[COORD_PER_FACE]  = float[COORD_PER_FACE]( 0, 0, 0,   0, 1, 0,   0, 0, 1,   0, 1, 1  );
    float leftFace[COORD_PER_FACE]   = float[COORD_PER_FACE]( 1, 0, 0,   1, 1, 0,   1, 0, 1,   1, 1, 1  );
    float frontFace[COORD_PER_FACE]  = float[COORD_PER_FACE]( 1, 0, 0,   0, 0, 0,   1, 1, 0,   0, 1, 0  );
    float backFace[COORD_PER_FACE]   = float[COORD_PER_FACE]( 1, 0, 1,   0, 0, 1,   1, 1, 1,   0, 1, 1  );

    vec4 bottomNormal = vec4(0.0,  -1, 0.0, 0.0);
    vec4 topNormal    = vec4(0.0,   1, 0.0, 0.0);
    vec4 rightNormal  = vec4( -1, 0.0, 0.0, 0.0);
    vec4 leftNormal   = vec4(  1, 0.0, 0.0, 0.0);
    vec4 frontNormal  = vec4(0.0, 0.0,  -1, 0.0);
    vec4 backNormal   = vec4(0.0, 0.0,   1, 0.0);
        
    for(i = 0; i < gl_VerticesIn; i++) {
        corner = gl_PositionIn[i];
        scale = voxelSize[i];
        faceOfVoxel(corner, scale, bottomFace, gl_FrontColorIn[i], bottomNormal);
        faceOfVoxel(corner, scale, topFace,    gl_FrontColorIn[i], topNormal   );
        faceOfVoxel(corner, scale, rightFace,  gl_FrontColorIn[i], rightNormal );
        faceOfVoxel(corner, scale, leftFace,   gl_FrontColorIn[i], leftNormal  );
        faceOfVoxel(corner, scale, frontFace,  gl_FrontColorIn[i], frontNormal );
        faceOfVoxel(corner, scale, backFace,   gl_FrontColorIn[i], backNormal  );
    }
}