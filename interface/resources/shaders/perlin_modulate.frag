#version 120

//
//  perlin_modulate.frag
//  fragment shader
//
//  Created by Andrzej Kapolka on 5/15/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

// the texture containing our permutations and normals
uniform sampler2D permutationNormalTexture;

// the noise frequency
const float frequency = 1024.0;

// the noise amplitude
const float amplitude = 0.1;

// the position in model space
varying vec3 position;

// returns the gradient at a single corner of our sampling cube
vec3 grad(vec3 location) {
    float p1 = texture2D(permutationNormalTexture, vec2(location.x / 256.0, 0.25)).r;
    float p2 = texture2D(permutationNormalTexture, vec2(p1 + location.y / 256.0, 0.25)).r;
    return texture2D(permutationNormalTexture, vec2(p2 + location.z / 256.0, 0.75)).xyz * 2.0 - vec3(1.0, 1.0, 1.0);
}

// returns the perlin noise value for the specified location
float perlin(vec3 location) {
    vec3 floors = floor(location);
    vec3 ceils = ceil(location);
    vec3 fff = grad(floors);
    vec3 ffc = grad(vec3(floors.x, floors.y, ceils.z));
    vec3 fcf = grad(vec3(floors.x, ceils.y, floors.z));
    vec3 fcc = grad(vec3(floors.x, ceils.y, ceils.z));
    vec3 cff = grad(vec3(ceils.x, floors.y, floors.z));
    vec3 cfc = grad(vec3(ceils.x, floors.y, ceils.z));
    vec3 ccf = grad(vec3(ceils.x, ceils.y, floors.z));
    vec3 ccc = grad(ceils);
    vec3 ffracts = fract(location);
    vec3 cfracts = ffracts - vec3(1.0, 1.0, 1.0);
    vec3 params = ffracts*ffracts*(3.0 - 2.0*ffracts);
    
    float fffv = dot(fff, ffracts);
    float ffcv = dot(ffc, vec3(ffracts.x, ffracts.y, cfracts.z));
    float fcfv = dot(fcf, vec3(ffracts.x, cfracts.y, ffracts.z));
    float fccv = dot(fcc, vec3(ffracts.x, cfracts.y, cfracts.z));
    float cffv = dot(cff, vec3(cfracts.x, ffracts.y, ffracts.z));
    float cfcv = dot(cfc, vec3(cfracts.x, ffracts.y, cfracts.z));
    float ccfv = dot(ccf, vec3(cfracts.x, cfracts.y, ffracts.z)); 
    float cccv = dot(ccc, cfracts);
    
    return mix(
        mix(mix(fffv, cffv, params.x), mix(fcfv, ccfv, params.x), params.y),
        mix(mix(ffcv, cfcv, params.x), mix(fccv, cccv, params.x), params.y),
        params.z);
}

void main(void) {
    gl_FragColor = vec4(gl_Color.rgb * (1.0 + amplitude*(perlin(position * frequency) - 1.0)), 1.0);
}
