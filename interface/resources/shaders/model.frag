#version 120

//
//  model.frag
//  fragment shader
//
//  Created by Andrzej Kapolka on 10/14/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// the diffuse texture
uniform sampler2D diffuseMap;

// the interpolated normal
varying vec4 normal;

void main(void) {
    // set the diffuse, normal, specular data
    vec4 diffuse = texture2D(diffuseMap, gl_TexCoord[0].st);
    gl_FragData[0] = vec4(gl_Color.rgb * diffuse.rgb, step(diffuse.a, 0.5));
    gl_FragData[1] = normalize(normal) * 0.5 + vec4(0.5, 0.5, 0.5, 1.0);
    gl_FragData[2] = vec4(gl_FrontMaterial.specular.rgb, gl_FrontMaterial.shininess / 128.0);
}
