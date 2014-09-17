#version 120

//
//  model_normal_map.frag
//  fragment shader
//
//  Created by Andrzej Kapolka on 10/29/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// the maximum number of local lights to apply
const int MAX_LOCAL_LIGHTS = 2;

// the color of each local light
uniform vec4 localLightColors[MAX_LOCAL_LIGHTS];

// the direction of each local light
uniform vec4 localLightDirections[MAX_LOCAL_LIGHTS];

// the diffuse texture
uniform sampler2D diffuseMap;

// the normal map texture
uniform sampler2D normalMap;

// the interpolated position
varying vec4 interpolatedPosition;

// the interpolated normal
varying vec4 interpolatedNormal;

// the interpolated tangent
varying vec4 interpolatedTangent;

void main(void) {
    vec3 normalizedNormal = normalize(vec3(interpolatedNormal));
    vec3 normalizedTangent = normalize(vec3(interpolatedTangent));
    vec3 normalizedBitangent = normalize(cross(normalizedNormal, normalizedTangent));
    vec3 localNormal = vec3(texture2D(normalMap, gl_TexCoord[0].st)) * 2.0 - vec3(1.0, 1.0, 1.0);

    // add up the local lights
    vec4 viewNormal = vec4(normalizedTangent * localNormal.x +
        normalizedBitangent * localNormal.y + normalizedNormal * localNormal.z, 0.0);
    vec4 localLight = vec4(0.0, 0.0, 0.0, 0.0);
    for (int i = 0; i < MAX_LOCAL_LIGHTS; i++) {
        localLight += localLightColors[i] * max(0.0, dot(viewNormal, localLightDirections[i]));
    }
    
    // compute the base color based on OpenGL lighting model
    float diffuse = dot(viewNormal, gl_LightSource[0].position);
    float facingLight = step(0.0, diffuse);
    vec4 base = gl_Color * (gl_FrontLightModelProduct.sceneColor + gl_FrontLightProduct[0].ambient +
        gl_FrontLightProduct[0].diffuse * (diffuse * facingLight) + localLight);
        
    // compute the specular component (sans exponent)
    float specular = facingLight * max(0.0, dot(normalize(gl_LightSource[0].position -
        normalize(vec4(vec3(interpolatedPosition), 0.0))), viewNormal));
        
    // modulate texture by base color and add specular contribution
    gl_FragColor = vec4(base.rgb, gl_FrontMaterial.diffuse.a) * texture2D(diffuseMap, gl_TexCoord[0].st) +
        vec4(pow(specular, gl_FrontMaterial.shininess) * gl_FrontLightProduct[0].specular.rgb, 0.0);
}
