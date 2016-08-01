//
//  Created by Bradley Austin Davis on 2016/07/11
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

uniform sampler2D sampler;

struct OverlayData {
    mat4 mvp;
    vec4 glowPoints;
    vec4 glowColors[2];
    vec4 resolutionRadiusAlpha;
};

layout(std140) uniform overlayBuffer {
    OverlayData overlay;
};

vec2 resolution = overlay.resolutionRadiusAlpha.xy;
float radius = overlay.resolutionRadiusAlpha.z;
float alpha = overlay.resolutionRadiusAlpha.w;
vec4 glowPoints = overlay.glowPoints;
vec4 glowColors[2] = overlay.glowColors;

in vec3 vPosition;
in vec2 vTexCoord;

out vec4 FragColor;

float easeInOutCubic(float f) {
    const float d = 1.0;
    const float b = 0.0;
    const float c = 1.0;
    float t = f;
    if ((t /= d / 2.0) < 1.0) return c / 2.0 * t * t * t + b;
    return c / 2.0 * ((t -= 2.0) * t * t + 2.0) + b;
}

void main() {
    FragColor = texture(sampler, vTexCoord);

    vec2 aspect = resolution;
    aspect /= resolution.x;

    float glowIntensity = 0.0;
    float dist1 = distance(vTexCoord * aspect, glowPoints.xy * aspect);
    float dist2 = distance(vTexCoord * aspect, glowPoints.zw * aspect);
    float dist = min(dist1, dist2);
    vec3 glowColor = glowColors[0].rgb;
    if (dist2 < dist1) {
        glowColor = glowColors[1].rgb;
    }

    if (dist <= radius) {
       glowIntensity = 1.0 - (dist / radius);
       glowColor.rgb = pow(glowColor, vec3(1.0 - glowIntensity));
       glowIntensity = easeInOutCubic(glowIntensity);
       glowIntensity = pow(glowIntensity, 0.5);
    }

    if (alpha <= 0.0) {
        if (glowIntensity <= 0.0) {
            discard;
        }

        FragColor = vec4(glowColor, glowIntensity);
        return;
    }

    FragColor.rgb = mix(FragColor.rgb, glowColor.rgb, glowIntensity);
    FragColor.a *= alpha;
}