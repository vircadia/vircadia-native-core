#version 120

//
//  grid.frag
//  fragment shader
//
//  Created by Andrzej Kapolka on 1/21/14.
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

void main(void) {
    gl_FragColor = vec4(gl_Color.rgb, exp(-0.25 / gl_FragCoord.w));
}
