#version 120

//
//  skin_model_normal_map.vert
//  vertex shader
//
//  Created by Andrzej Kapolka on 10/29/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

const int MAX_CLUSTERS = 128;
const int INDICES_PER_VERTEX = 4;

uniform mat4 clusterMatrices[MAX_CLUSTERS];

// the tangent vector
attribute vec3 tangent;

attribute vec4 clusterIndices;
attribute vec4 clusterWeights;

// the interpolated normal
varying vec4 interpolatedNormal;

// the interpolated tangent
varying vec4 interpolatedTangent;

void main(void) {
    vec4 position = vec4(0.0, 0.0, 0.0, 0.0);
    interpolatedNormal = vec4(0.0, 0.0, 0.0, 0.0);
    interpolatedTangent = vec4(0.0, 0.0, 0.0, 0.0);
    for (int i = 0; i < INDICES_PER_VERTEX; i++) {
        mat4 clusterMatrix = clusterMatrices[int(clusterIndices[i])];
        float clusterWeight = clusterWeights[i];
        position += clusterMatrix * gl_Vertex * clusterWeight;
        interpolatedNormal += clusterMatrix * vec4(gl_Normal, 0.0) * clusterWeight;
        interpolatedTangent += clusterMatrix * vec4(tangent, 0.0) * clusterWeight;
    }
    position = gl_ModelViewProjectionMatrix * position;
    interpolatedNormal = gl_ModelViewMatrix * interpolatedNormal;
    interpolatedTangent = gl_ModelViewMatrix * interpolatedTangent;
    
    // pass along the vertex color
    gl_FrontColor = vec4(1.0, 1.0, 1.0, 1.0);
    
    // and the texture coordinates
    gl_TexCoord[0] = gl_MultiTexCoord0;
    
    gl_Position = position;
}
