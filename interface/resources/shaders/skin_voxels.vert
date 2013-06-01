#version 120

//
//  skin_voxels.vert
//  vertex shader
//
//  Created by Andrzej Kapolka on 5/31/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

const int MAX_BONES = 32;
const int INDICES_PER_VERTEX = 4;

uniform mat4 boneMatrices[MAX_BONES];

attribute vec4 boneIndices;
attribute vec4 boneWeights;

void main(void) {
    vec4 position = vec4(0.0, 0.0, 0.0, 0.0);
    vec4 normal = vec4(0.0, 0.0, 0.0, 0.0);
    for (int i = 0; i < INDICES_PER_VERTEX; i++) {
        mat4 boneMatrix = boneMatrices[int(boneIndices[i])];
        float boneWeight = boneWeights[i];
        position += boneMatrix * gl_Vertex * boneWeight;
        normal += boneMatrix * vec4(gl_Normal, 0.0) * boneWeight;
    }
    position = gl_ModelViewProjectionMatrix * position;
    normal = normalize(gl_ModelViewMatrix * normal);
    
    gl_FrontColor = gl_Color * (gl_LightModel.ambient + gl_LightSource[0].ambient +
        gl_LightSource[0].diffuse * max(0.0, dot(normal, gl_LightSource[0].position)));
    gl_Position = position;
}
