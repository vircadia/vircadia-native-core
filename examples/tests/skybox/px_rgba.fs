//
//  px_rgba.fs
//  examples/tests/skybox
//
//  Created by Zach Pomerantz on 3/10/2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html


vec3 getSkyboxColor() {
    float red = (cos(iGlobalTime) + 1) / 2;
    vec3 color = vec3(red, 1.0, 1.0);

    color *= skybox.color.rgb;

    return color;
}

