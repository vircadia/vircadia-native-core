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

// the diffuse texture
uniform sampler2D diffuseMap;

// the normal map texture
uniform sampler2D normalMap;

// the interpolated normal
varying vec4 interpolatedNormal;

// the interpolated tangent
varying vec4 interpolatedTangent;

void main(void) {
    // compute the view normal from the various bits
    vec3 normalizedNormal = normalize(vec3(interpolatedNormal));
    vec3 normalizedTangent = normalize(vec3(interpolatedTangent));
    vec3 normalizedBitangent = normalize(cross(normalizedNormal, normalizedTangent));
    vec3 localNormal = vec3(texture2D(normalMap, gl_TexCoord[0].st)) - vec3(0.5, 0.5, 0.5);
    vec4 viewNormal = vec4(normalizedTangent * localNormal.x +
        normalizedBitangent * localNormal.y + normalizedNormal * localNormal.z, 0.0);
    
    // set the diffuse, normal, specular data
    vec4 diffuse = texture2D(diffuseMap, gl_TexCoord[0].st);
    gl_FragData[0] = vec4(gl_Color.rgb * diffuse.rgb, mix(gl_Color.a, 1.0 - gl_Color.a, step(diffuse.a, 0.5)));
    gl_FragData[1] = viewNormal + vec4(0.5, 0.5, 0.5, 1.0);
    gl_FragData[2] = vec4(gl_FrontMaterial.specular.rgb, gl_FrontMaterial.shininess / 128.0);
}
