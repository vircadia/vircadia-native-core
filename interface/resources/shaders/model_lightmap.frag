#version 120

//
//  model_lightmap.frag
//  fragment shader
//
//  Created by Samuel Gateau on 11/19/14.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// the diffuse texture
uniform sampler2D diffuseMap;

// the emissive map texture and parameters
uniform sampler2D emissiveMap;
uniform vec2 emissiveParams;

// the alpha threshold
uniform float alphaThreshold;

// the interpolated normal
varying vec4 normal;

// the interpolated texcoord1
varying vec2 interpolatedTexcoord1;

void main(void) {
    // set the diffuse, normal, specular data
    vec4 diffuse = texture2D(diffuseMap, gl_TexCoord[0].st);
    vec4 emissive = texture2D(emissiveMap, interpolatedTexcoord1.st);
    gl_FragData[0] = vec4(gl_Color.rgb * diffuse.rgb, mix(gl_Color.a, 1.0 - gl_Color.a, step(diffuse.a, alphaThreshold)));
    gl_FragData[1] = normalize(normal) * 0.5 + vec4(0.5, 0.5, 0.5, 0.5);
    gl_FragData[2] = vec4((vec3(emissiveParams.x) + emissiveParams.y * emissive.rgb), gl_FrontMaterial.shininess / 128.0);
}
