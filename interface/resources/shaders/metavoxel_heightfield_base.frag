#version 120

//
//  metavoxel_heightfield_base.frag
//  fragment shader
//
//  Created by Andrzej Kapolka on 8/20/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// the diffuse texture
uniform sampler2D diffuseMap;

void main(void) {
    // compute the base color based on OpenGL lighting model
    gl_FragColor = gl_Color * texture2D(diffuseMap, gl_TexCoord[0].st);
}
