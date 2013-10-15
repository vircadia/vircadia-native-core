#version 120

//
//  skin_blendface.vert
//  vertex shader
//
//  Created by Andrzej Kapolka on 10/14/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

const int MAX_CLUSTERS = 32;
const int INDICES_PER_VERTEX = 4;

uniform mat4 clusterMatrices[MAX_CLUSTERS];

attribute vec4 clusterIndices;
attribute vec4 clusterWeights;

void main(void) {
    vec4 position = vec4(0.0, 0.0, 0.0, 0.0);
    vec4 normal = vec4(0.0, 0.0, 0.0, 0.0);
    for (int i = 0; i < INDICES_PER_VERTEX; i++) {
        mat4 clusterMatrix = clusterMatrices[int(clusterIndices[i])];
        float clusterWeight = clusterWeights[i];
        position += clusterMatrix * gl_Vertex * clusterWeight;
        normal += clusterMatrix * vec4(gl_Normal, 0.0) * clusterWeight;
    }
    position = gl_ModelViewProjectionMatrix * position;
    normal = normalize(gl_ModelViewMatrix * normal);
    
    // standard diffuse lighting
    gl_FrontColor = (gl_LightModel.ambient + gl_LightSource[0].ambient +
        gl_LightSource[0].diffuse * max(0.0, dot(normal, gl_LightSource[0].position)));
    
    // pass along the texture coordinate
    gl_TexCoord[0] = gl_MultiTexCoord0;
    
    gl_Position = position;
}
