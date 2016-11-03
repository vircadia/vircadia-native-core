//
//  flowers.fs
//  examples/homeContent/plant
//
//  Created by Eric Levin on 3/7/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  This fragment shader is designed to shader a sphere to create the effect of a blooming flower
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html


#define TWO_PI 6.28318530718

uniform float iBloomPct = 0.2;
uniform vec3 iHSLColor = vec3(0.7, 0.5, 0.5);


float hue2rgb(float f1, float f2, float hue) {
    if (hue < 0.0)
        hue += 1.0;
    else if (hue > 1.0)
        hue -= 1.0;
    float res;
    if ((6.0 * hue) < 1.0)
        res = f1 + (f2 - f1) * 6.0 * hue;
    else if ((2.0 * hue) < 1.0)
        res = f2;
    else if ((3.0 * hue) < 2.0)
        res = f1 + (f2 - f1) * ((2.0 / 3.0) - hue) * 6.0;
    else
        res = f1;
    return res;
}

vec3 hsl2rgb(vec3 hsl) {
    vec3 rgb;
    
    if (hsl.y == 0.0) {
        rgb = vec3(hsl.z); // Luminance
    } else {
        float f2;
        
        if (hsl.z < 0.5)
            f2 = hsl.z * (1.0 + hsl.y);
        else
            f2 = hsl.z + hsl.y - hsl.y * hsl.z;
            
        float f1 = 2.0 * hsl.z - f2;
        
        rgb.r = hue2rgb(f1, f2, hsl.x + (1.0/3.0));
        rgb.g = hue2rgb(f1, f2, hsl.x);
        rgb.b = hue2rgb(f1, f2, hsl.x - (1.0/3.0));
    }   
    return rgb;
}

vec3 hsl2rgb(float h, float s, float l) {
    return hsl2rgb(vec3(h, s, l));
}

void mainImage( out vec4 fragColor, in vec2 fragCoord ) {
    vec2 st = fragCoord.xy/iWorldScale.xz;
    vec3 color = vec3(0.0, 0.0, 0.0);

    vec2 toCenter = vec2(0.5) - st;
    float angle = atan(toCenter.y, toCenter.x);
    float radius = length(toCenter) * 2.0;

    // Second check is so we discard the top half of the sphere
    if ( iBloomPct < radius || _position.y > 0) {
        discard;
    }

    // simulate ambient occlusion
    float brightness = pow(radius, 0.8);
    vec3 hslColor = iHSLColor + (abs(angle) * 0.02);
    hslColor.z = 0.15 + pow(radius, 2.);
    vec3 rgbColor = hsl2rgb(hslColor);
    fragColor = vec4(rgbColor, 1.0);
}




vec4 getProceduralColor() {
    vec4 result;
    vec2 position = _position.xz;
    position += 0.5;
    
    mainImage(result, position * iWorldScale.xz);
 
    return result;
}