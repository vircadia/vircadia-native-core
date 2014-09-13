#version 120

//
//  skin_model.vert
//  vertex shader
//
//  Created by Andrzej Kapolka on 10/14/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

const int MAX_CLUSTERS = 128;
const int INDICES_PER_VERTEX = 4;

uniform mat4 clusterMatrices[MAX_CLUSTERS];

attribute vec4 clusterIndices;
attribute vec4 clusterWeights;

// the interpolated normal
varying vec4 normal;

void main(void) {
    vec4 position = vec4(0.0, 0.0, 0.0, 0.0);
    normal = vec4(0.0, 0.0, 0.0, 0.0);
    for (int i = 0; i < INDICES_PER_VERTEX; i++) {
        mat4 clusterMatrix = clusterMatrices[int(clusterIndices[i])];
        float clusterWeight = clusterWeights[i];
        position += clusterMatrix * gl_Vertex * clusterWeight;
        normal += clusterMatrix * vec4(gl_Normal, 0.0) * clusterWeight;
    }
    
    normal = normalize(gl_ModelViewMatrix * normal);
    
    // pass along the vertex color
    gl_FrontColor = vec4(1.0, 1.0, 1.0, 1.0);
    
    // and the texture coordinates
    gl_TexCoord[0] = gl_MultiTexCoord0;
    
    gl_Position = gl_ModelViewProjectionMatrix * position;
}
