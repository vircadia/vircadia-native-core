#version 120

//
//  metavoxel_heighfield_splat.vert
//  vertex shader
//
//  Created by Andrzej Kapolka on 8/20/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// the height texture
uniform sampler2D heightMap;

// the texture that contains the texture indices
uniform sampler2D textureMap;

// the distance between height points in texture space
uniform vec2 heightScale;

// the scale between height and texture textures
uniform vec2 textureScale;

// the splat texture offset
uniform vec2 splatTextureOffset;

// the splat textures scales on the S axis
uniform vec4 splatTextureScalesS;

// the splat texture scales on the T axis
uniform vec4 splatTextureScalesT;

// the lower bounds of the values corresponding to the splat textures
uniform vec4 textureValueMinima;

// the upper bounds of the values corresponding to the splat textures
uniform vec4 textureValueMaxima;

// alpha values for the four splat textures
varying vec4 alphaValues;

void main(void) {
    // add the height to the position
    float height = texture2D(heightMap, gl_MultiTexCoord0.st).r;
    vec4 modelSpacePosition = gl_Vertex + vec4(0.0, height, 0.0, 0.0);
    gl_Position = gl_ModelViewProjectionMatrix * modelSpacePosition;
    
    // the zero height should be invisible
    gl_FrontColor = vec4(1.0, 1.0, 1.0, 1.0 - step(height, 0.0));
    
    // pass along the scaled/offset texture coordinates
    vec4 textureSpacePosition = vec4(modelSpacePosition.xz, 0.0, 1.0) + vec4(splatTextureOffset, 0.0, 0.0);
    gl_TexCoord[0] = textureSpacePosition * vec4(splatTextureScalesS[0], splatTextureScalesT[0], 0.0, 1.0);
    gl_TexCoord[1] = textureSpacePosition * vec4(splatTextureScalesS[1], splatTextureScalesT[1], 0.0, 1.0);
    gl_TexCoord[2] = textureSpacePosition * vec4(splatTextureScalesS[2], splatTextureScalesT[2], 0.0, 1.0);
    gl_TexCoord[3] = textureSpacePosition * vec4(splatTextureScalesS[3], splatTextureScalesT[3], 0.0, 1.0);
    
    // compute the alpha values for each texture
    float value = texture2D(textureMap, (gl_MultiTexCoord0.st - heightScale) * textureScale).r;
    vec4 valueVector = vec4(value, value, value, value);
    alphaValues = step(textureValueMinima, valueVector) * step(valueVector, textureValueMaxima);
}
