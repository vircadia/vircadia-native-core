//
//  Created by Bradley Austin Davis on 2016/07/11
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

struct OverlayData {
    mat4 mvp;
    float alpha;
};

layout(std140) uniform overlayBuffer {
    OverlayData overlay;
};

mat4 mvp = overlay.mvp;

layout(location = 0) in vec3 Position;
layout(location = 3) in vec2 TexCoord;

out vec2 vTexCoord;

void main() {
  gl_Position = mvp * vec4(Position, 1);
  vTexCoord = TexCoord;
}
