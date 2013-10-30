#version 120

//
//  model_normal_map.frag
//  fragment shader
//
//  Created by Andrzej Kapolka on 10/29/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

// the diffuse texture
uniform sampler2D diffuseMap;

// the normal map texture
uniform sampler2D normalMap;

// the interpolated normal
varying vec4 interpolatedNormal;

// the interpolated tangent
varying vec4 interpolatedTangent;

void main(void) {
    vec3 normalizedNormal = normalize(vec3(interpolatedNormal));
    vec3 normalizedTangent = normalize(vec3(interpolatedTangent));
    vec3 normalizedBitangent = normalize(cross(normalizedNormal, normalizedTangent));
    vec3 localNormal = vec3(texture2D(normalMap, gl_TexCoord[0].st)) * 2.0 - vec3(1.0, 1.0, 1.0);

    // compute the base color based on OpenGL lighting model
    vec4 viewNormal = vec4(normalizedTangent * localNormal.x +
        normalizedBitangent * localNormal.y + normalizedNormal * localNormal.z, 0.0);
    vec4 base = gl_Color * (gl_FrontLightModelProduct.sceneColor + gl_FrontLightProduct[0].ambient +
        gl_FrontLightProduct[0].diffuse * max(0.0, dot(viewNormal, gl_LightSource[0].position)));

    // compute the specular component (sans exponent)
    float specular = max(0.0, dot(normalize(gl_LightSource[0].position + vec4(0.0, 0.0, 1.0, 0.0)), viewNormal));
    
    // modulate texture by base color and add specular contribution
    gl_FragColor = base * texture2D(diffuseMap, gl_TexCoord[0].st) +
        pow(specular, gl_FrontMaterial.shininess) * gl_FrontLightProduct[0].specular;
}
