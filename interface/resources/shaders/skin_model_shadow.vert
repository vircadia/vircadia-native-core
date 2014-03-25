#version 120

//
//  skin_model_shadow.vert
//  vertex shader
//
//  Created by Andrzej Kapolka on 3/24/14.
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

const int MAX_CLUSTERS = 128;
const int INDICES_PER_VERTEX = 4;

uniform mat4 clusterMatrices[MAX_CLUSTERS];

attribute vec4 clusterIndices;
attribute vec4 clusterWeights;

void main(void) {
    vec4 position = vec4(0.0, 0.0, 0.0, 0.0);
    for (int i = 0; i < INDICES_PER_VERTEX; i++) {
        mat4 clusterMatrix = clusterMatrices[int(clusterIndices[i])];
        float clusterWeight = clusterWeights[i];
        position += clusterMatrix * gl_Vertex * clusterWeight;
    }
    gl_Position = gl_ModelViewProjectionMatrix * position;
}
