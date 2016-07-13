//
//  Created by Bradley Austin Davis on 2016/07/11
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#version 410 core

uniform vec4 color = vec4(1.0, 1.0, 1.0, 1.0);

layout(location = 0) in vec3 inLineDistance;

out vec4 FragColor;

void main() {
    vec2 d = inLineDistance.xy;
    d.y = abs(d.y);
    d.x = abs(d.x);
    if (d.x > 1.0) {
        d.x = (d.x - 1.0) / 0.02;
    } else { 
        d.x = 0.0; 
    }
    float alpha = 1.0 - length(d);
    if (alpha <= 0.0) {
        discard;
    }
    alpha = pow(alpha, 10.0);
    if (alpha < 0.05) {
        discard;
    }
    FragColor = vec4(color.rgb, alpha);
}
