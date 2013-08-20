#version 120

//
//  occlusion_blur.frag
//  fragment shader
//
//  Created by Andrzej Kapolka on 8/16/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

// the original texture
uniform sampler2D originalTexture;

void main(void) {
    float ds = dFdx(gl_TexCoord[0].s);
    float dt = dFdy(gl_TexCoord[0].t);
    vec4 sum = vec4(0.0, 0.0, 0.0, 0.0);
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            sum += texture2D(originalTexture, gl_TexCoord[0].st +
                vec2(ds, dt) * vec2(-2.0 + float(i), -2.0 + float(j)));
        }
    }
    gl_FragColor = sum / 16.0;
}
