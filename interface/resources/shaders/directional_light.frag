#version 120

//
//  directional_light.frag
//  fragment shader
//
//  Created by Andrzej Kapolka on 9/3/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// the diffuse texture
uniform sampler2D diffuseMap;

// the normal texture
uniform sampler2D normalMap;

void main(void) {
    // compute the base color based on OpenGL lighting model
    vec4 normal = texture2D(normalMap, gl_TexCoord[0].st);
    gl_FragColor = vec4((texture2D(diffuseMap, gl_TexCoord[0].st) * (gl_FrontLightModelProduct.sceneColor +
        gl_FrontLightProduct[0].ambient + gl_FrontLightProduct[0].diffuse * max(0.0, dot(normal * 2.0 -
            vec4(1.0, 1.0, 1.0, 2.0), gl_LightSource[0].position)))).rgb, normal.a);
}
