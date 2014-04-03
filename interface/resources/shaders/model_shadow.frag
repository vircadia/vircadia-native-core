#version 120

//
//  model_shadow.frag
//  fragment shader
//
//  Created by Andrzej Kapolka on 3/24/14.
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

void main(void) {
    // fixed color for now (we may eventually want to use texture alpha)
    gl_FragColor = vec4(1.0, 1.0, 1.0, 1.0);
}
