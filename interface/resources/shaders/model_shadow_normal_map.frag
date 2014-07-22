#version 120

//
//  model_shadow_normal_map.frag
//  fragment shader
//
//  Created by Andrzej Kapolka on 5/23/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// the maximum number of local lights to apply
const int MAX_LOCAL_LIGHTS = 2;

// the diffuse texture
uniform sampler2D diffuseMap;

// the normal map texture
uniform sampler2D normalMap;

// the shadow texture
uniform sampler2DShadow shadowMap;

// the inverse of the size of the shadow map
const float shadowScale = 1.0 / 2048.0;

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
    for (int i = 1; i <= MAX_LOCAL_LIGHTS; i++) {
        localLight += gl_FrontLightProduct[i].diffuse * max(0.0, dot(viewNormal, gl_LightSource[i].position));
    }
    
    // compute the base color based on OpenGL lighting model
    float diffuse = dot(viewNormal, gl_LightSource[0].position);
    float facingLight = step(0.0, diffuse) * 0.25 *
        (shadow2D(shadowMap, gl_TexCoord[1].stp + vec3(-shadowScale, -shadowScale, 0.0)).r +
        shadow2D(shadowMap, gl_TexCoord[1].stp + vec3(-shadowScale, shadowScale, 0.0)).r +
        shadow2D(shadowMap, gl_TexCoord[1].stp + vec3(shadowScale, -shadowScale, 0.0)).r +
        shadow2D(shadowMap, gl_TexCoord[1].stp + vec3(shadowScale, shadowScale, 0.0)).r);
    vec4 base = gl_Color * (gl_FrontLightModelProduct.sceneColor + gl_FrontLightProduct[0].ambient +
        gl_FrontLightProduct[0].diffuse * (diffuse * facingLight) + localLight);

    // compute the specular component (sans exponent)
    float specular = facingLight * max(0.0, dot(normalize(gl_LightSource[0].position -
        normalize(vec4(vec3(interpolatedPosition), 0.0))), viewNormal));
        
    // modulate texture by base color and add specular contribution
    gl_FragColor = base * texture2D(diffuseMap, gl_TexCoord[0].st) +
        vec4(pow(specular, gl_FrontMaterial.shininess) * gl_FrontLightProduct[0].specular.rgb, 0.0);
}
