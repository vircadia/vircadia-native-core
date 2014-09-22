#version 120

//
//  model_shadow.frag
//  fragment shader
//
//  Created by Andrzej Kapolka on 3/24/14.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

void main(void) {
    // fixed color for now (we may eventually want to use texture alpha)
    gl_FragColor = vec4(1.0, 1.0, 1.0, 0.0);
}
