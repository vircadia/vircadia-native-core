#version 120

//
//  directional_light.frag
//  fragment shader
//
//  Created by Andrzej Kapolka on 9/3/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// the diffuse texture
uniform sampler2D diffuseMap;

// the normal texture
uniform sampler2D normalMap;

// the depth texture
uniform sampler2D depthMap;

// the shadow texture
uniform sampler2DShadow shadowMap;

// the inverse of the size of the shadow map
uniform float shadowScale;

// the distance to the near clip plane
uniform float near;

// scale factor for depth: (far - near) / far
uniform float depthScale;

// offset for depth texture coordinates
uniform vec2 depthTexCoordOffset;

// scale for depth texture coordinates
uniform vec2 depthTexCoordScale;

void main(void) {
    // compute the view space position using the depth
    float z = near / (texture2D(depthMap, gl_TexCoord[0].st).r * depthScale - 1.0);
    vec4 position = vec4((depthTexCoordOffset + gl_TexCoord[0].st * depthTexCoordScale) * z, z, 1.0);
    
    // compute the corresponding texture coordinates
    vec3 shadowTexCoord = vec3(dot(gl_EyePlaneS[0], position), dot(gl_EyePlaneT[0], position), dot(gl_EyePlaneR[0], position));
    
    // compute the color based on OpenGL lighting model, use the alpha from the normal map
    vec4 normal = texture2D(normalMap, gl_TexCoord[0].st);
    float diffuse = dot(normal * 2.0 - vec4(1.0, 1.0, 1.0, 2.0), gl_LightSource[0].position);
    float facingLight = step(0.0, diffuse) * 0.25 *
        (shadow2D(shadowMap, shadowTexCoord + vec3(-shadowScale, -shadowScale, 0.0)).r +
        shadow2D(shadowMap, shadowTexCoord + vec3(-shadowScale, shadowScale, 0.0)).r +
        shadow2D(shadowMap, shadowTexCoord + vec3(shadowScale, -shadowScale, 0.0)).r +
        shadow2D(shadowMap, shadowTexCoord + vec3(shadowScale, shadowScale, 0.0)).r);
    vec4 baseColor = texture2D(diffuseMap, gl_TexCoord[0].st) * (gl_FrontLightModelProduct.sceneColor +
        gl_FrontLightProduct[0].ambient + gl_FrontLightProduct[0].diffuse * (diffuse * facingLight));
    gl_FragColor = vec4(baseColor.rgb, normal.a);
}
