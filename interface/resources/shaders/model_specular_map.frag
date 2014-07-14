#version 120

//
//  model_specular_map.frag
//  fragment shader
//
//  Created by Andrzej Kapolka on 5/6/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
const int MAX_LOCAL_LIGHTS = 2;

uniform int numLocalLights;
uniform vec3 localLightDirections[MAX_LOCAL_LIGHTS];
uniform vec3 localLightColors[MAX_LOCAL_LIGHTS];

// the diffuse texture
uniform sampler2D diffuseMap;

// the specular texture
uniform sampler2D specularMap;

// the interpolated position in view space
varying vec4 position;

// the interpolated normal
varying vec4 normal;

void main(void) {
    // compute the base color based on OpenGL lighting model
    vec4 normalizedNormal = normalize(normal);
    float diffuse = dot(normalizedNormal, gl_LightSource[0].position);
    float facingLight = step(0.0, diffuse);
    
    // the local light that is always present
    vec4 totalLocalLight = vec4(0.0, 0.0, 0.0, 1.0);
    for (int i = 0; i < numLocalLights; i++) {
        float localDiffuse = dot(normalizedNormal, vec4(localLightDirections[i], 1.0));
        float localLight = step(0.0, localDiffuse);
        float localLightVal = localDiffuse * localLight;
        
        totalLocalLight += (localLightVal * vec4( localLightColors[i], 0.0));
    }
    
    vec4 base = gl_Color * (gl_FrontLightModelProduct.sceneColor + gl_FrontLightProduct[0].ambient +
        gl_FrontLightProduct[0].diffuse * (diffuse * facingLight) + totalLocalLight);

    // compute the specular component (sans exponent)
    float specular = facingLight * max(0.0, dot(normalize(gl_LightSource[0].position - normalize(vec4(position.xyz, 0.0))),
        normalizedNormal));
        
    // modulate texture by base color and add specular contribution
    gl_FragColor = base * texture2D(diffuseMap, gl_TexCoord[0].st) + vec4(pow(specular, gl_FrontMaterial.shininess) *
        gl_FrontLightProduct[0].specular.rgb * texture2D(specularMap, gl_TexCoord[0].st).rgb, 0.0);
        
}
