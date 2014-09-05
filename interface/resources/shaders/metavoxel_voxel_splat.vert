#version 120

//
//  metavoxel_voxel_splat.vert
//  vertex shader
//
//  Created by Andrzej Kapolka on 9/4/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// the splat textures scales on the S axis
uniform vec4 splatTextureScalesS;

// the splat texture scales on the T axis
uniform vec4 splatTextureScalesT;

// the lower bounds of the values corresponding to the splat textures
uniform vec4 textureValueMinima;

// the upper bounds of the values corresponding to the splat textures
uniform vec4 textureValueMaxima;

// the materials to apply to the vertex
attribute vec4 materials;

// the weights of each material
attribute vec4 materialWeights;

// alpha values for the four splat textures
varying vec4 alphaValues;

void main(void) {
    // use the fixed-function position
    gl_Position = ftransform();
    
    // pass along the scaled/offset texture coordinates
    vec4 textureSpacePosition = vec4(gl_Vertex.xz, 0.0, 1.0);
    gl_TexCoord[0] = textureSpacePosition * vec4(splatTextureScalesS[0], splatTextureScalesT[0], 0.0, 1.0);
    gl_TexCoord[1] = textureSpacePosition * vec4(splatTextureScalesS[1], splatTextureScalesT[1], 0.0, 1.0);
    gl_TexCoord[2] = textureSpacePosition * vec4(splatTextureScalesS[2], splatTextureScalesT[2], 0.0, 1.0);
    gl_TexCoord[3] = textureSpacePosition * vec4(splatTextureScalesS[3], splatTextureScalesT[3], 0.0, 1.0);
    
    // compute the alpha values for each texture
    float value = materials[0];
    vec4 valueVector = vec4(value, value, value, value);
    alphaValues = step(textureValueMinima, valueVector) * step(valueVector, textureValueMaxima) * materialWeights[0];
    
    value = materials[1];
    valueVector = vec4(value, value, value, value);
    alphaValues += step(textureValueMinima, valueVector) * step(valueVector, textureValueMaxima) * materialWeights[1];
    
    value = materials[2];
    valueVector = vec4(value, value, value, value);
    alphaValues += step(textureValueMinima, valueVector) * step(valueVector, textureValueMaxima) * materialWeights[2];
    
    value = materials[3];
    valueVector = vec4(value, value, value, value);
    alphaValues += step(textureValueMinima, valueVector) * step(valueVector, textureValueMaxima) * materialWeights[3];
}
