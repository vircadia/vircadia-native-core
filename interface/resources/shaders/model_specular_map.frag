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

// the diffuse texture
uniform sampler2D diffuseMap;

// the specular texture
uniform sampler2D specularMap;

// the alpha threshold
uniform float alphaThreshold;

// the interpolated normal
varying vec4 normal;

void main(void) {
    // set the diffuse, normal, specular data
    vec4 diffuse = texture2D(diffuseMap, gl_TexCoord[0].st);
    gl_FragData[0] = vec4(gl_Color.rgb * diffuse.rgb, mix(gl_Color.a, 1.0 - gl_Color.a, step(diffuse.a, alphaThreshold)));
    gl_FragData[1] = normalize(normal) * 0.5 + vec4(0.5, 0.5, 0.5, 1.0);
    gl_FragData[2] = vec4(gl_FrontMaterial.specular.rgb * texture2D(specularMap, gl_TexCoord[0].st).rgb,
        gl_FrontMaterial.shininess / 128.0);    
}
