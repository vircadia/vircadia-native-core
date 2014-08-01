#version 120

//
//  metavoxel_heightfield.frag
//  fragment shader
//
//  Created by Andrzej Kapolka on 7/28/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// the diffuse texture
uniform sampler2D diffuseMap;

// the interpolated normal
varying vec4 normal;

void main(void) {
    // compute the base color based on OpenGL lighting model
    vec4 base = gl_Color * (gl_FrontLightModelProduct.sceneColor + gl_FrontLightProduct[0].ambient +
        gl_FrontLightProduct[0].diffuse * max(0.0, dot(normalize(normal), gl_LightSource[0].position)));
    gl_FragColor = base * texture2D(diffuseMap, gl_TexCoord[0].st);
}
