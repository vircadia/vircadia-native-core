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

// the specular texture
uniform sampler2D specularMap;

// the depth texture
uniform sampler2D depthMap;

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
    vec4 position = vec4((depthTexCoordOffset + gl_TexCoord[0].st * depthTexCoordScale) * z, z, 0.0);
    
    // get the normal from the map
    vec4 normal = texture2D(normalMap, gl_TexCoord[0].st);
    vec4 normalizedNormal = normal * 2.0 - vec4(1.0, 1.0, 1.0, 2.0);
    
    // compute the base color based on OpenGL lighting model
    float diffuse = dot(normalizedNormal, gl_LightSource[0].position);
    float facingLight = step(0.0, diffuse);
    vec4 baseColor = texture2D(diffuseMap, gl_TexCoord[0].st) * (gl_FrontLightModelProduct.sceneColor +
        gl_FrontLightProduct[0].ambient + gl_FrontLightProduct[0].diffuse * (diffuse * facingLight));
    
    // compute the specular multiplier (sans exponent)
    float specular = facingLight * max(0.0, dot(normalize(gl_LightSource[0].position - normalize(position)),
        normalizedNormal));    
    
    // add specular contribution
    vec4 specularColor = texture2D(specularMap, gl_TexCoord[0].st);
    gl_FragColor = vec4(baseColor.rgb + pow(specular, specularColor.a * 128.0) * specularColor.rgb, normal.a);
}
