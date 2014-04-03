#version 120

//
//  model_shadow.vert
//  vertex shader
//
//  Created by Andrzej Kapolka on 3/24/14.
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

void main(void) {
    // just use standard pipeline transform
    gl_Position = ftransform();
}
