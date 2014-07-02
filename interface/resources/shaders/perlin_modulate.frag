#version 120

//
//  perlin_modulate.frag
//  fragment shader
//
//  Created by Andrzej Kapolka on 5/15/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// implementation based on Ken Perlin's Improved Noise reference implementation (orig. in Java) at
// http://mrl.nyu.edu/~perlin/noise/

uniform sampler2D permutationTexture;

// the noise frequency
const float frequency = 256.0;
//const float frequency = 65536.0; // looks better with current TREE_SCALE, was 1024 when TREE_SCALE was either 512 or 128

// the noise amplitude
const float amplitude = 0.5;

// the position in model space
varying vec3 position;

// gradient based on gradients from cube edge centers rather than random from texture lookup
float randomEdgeGrad(int hash, vec3 position){
    int h = int(mod(hash, 16));
    float u = h < 8 ? position.x : position.y;
    float v = h < 4 ? position.y : h == 12 || h == 14 ? position.x : position.z;
    bool even = mod(hash, 2) == 0;
    bool four = mod(hash, 4) == 0;
    return (even ? u : -u) + (four ? v : -v);
}

// still have the option to lookup based on texture
float randomTextureGrad(int hash, vec3 position){
    float u = float(hash) / 256.0;
    vec3 g = -1 + 2 * texture2D(permutationTexture, vec2(u, 0.75)).rgb;
    return dot(position, g);
}

float improvedGrad(int hash, vec3 position){
// Untested whether texture lookup is faster than math, uncomment one line or the other to try out
// cube edge gradients versus random spherical gradients sent in texture.

//    return randomTextureGrad(hash, position);
    return randomEdgeGrad(hash, position);
}

// 5th order fade function to remove 2nd order discontinuties
vec3 fade3(vec3 t){
    return t * t * t * (t * (t * 6 - 15) + 10); 
}

int permutation(int index){
    float u = float(index) / 256.0;
    float t = texture2D(permutationTexture, vec2(u, 0.25)).r;
    return int(t * 256);
}

float improvedNoise(vec3 position){
    int X = int(mod(floor(position.x), 256));
    int Y = int(mod(floor(position.y), 256));
    int Z = int(mod(floor(position.z), 256));

    vec3 fracs = fract(position);

    vec3 fades = fade3(fracs);

    int A  = permutation(X + 0) + Y;
    int AA = permutation(A + 0) + Z;
    int AB = permutation(A + 1) + Z;
    int B  = permutation(X + 1) + Y;
    int BA = permutation(B + 0) + Z;
    int BB = permutation(B + 1) + Z;

    float gradAA0 = improvedGrad(permutation(AA + 0), vec3(fracs.x    , fracs.y    , fracs.z    ));
    float gradBA0 = improvedGrad(permutation(BA + 0), vec3(fracs.x - 1, fracs.y    , fracs.z    ));
    float gradAB0 = improvedGrad(permutation(AB + 0), vec3(fracs.x    , fracs.y - 1, fracs.z    ));
    float gradBB0 = improvedGrad(permutation(BB + 0), vec3(fracs.x - 1, fracs.y - 1, fracs.z    ));
    float gradAA1 = improvedGrad(permutation(AA + 1), vec3(fracs.x    , fracs.y    , fracs.z - 1));
    float gradBA1 = improvedGrad(permutation(BA + 1), vec3(fracs.x - 1, fracs.y    , fracs.z - 1));
    float gradAB1 = improvedGrad(permutation(AB + 1), vec3(fracs.x    , fracs.y - 1, fracs.z - 1));
    float gradBB1 = improvedGrad(permutation(BB + 1), vec3(fracs.x - 1, fracs.y - 1, fracs.z - 1));

    return mix(mix(mix(gradAA0, gradBA0, fades.x), mix(gradAB0, gradBB0, fades.x), fades.y), mix(mix(gradAA1, gradBA1, fades.x), mix(gradAB1, gradBB1, fades.x), fades.y), fades.z);
}

float turbulence(vec3 position, float power){
    return (1.0f / power) * improvedNoise(power * position);
}

float turb(vec3 position){
    return turbulence(position, 1)
    + turbulence(position, 2), 
    + turbulence(position, 4) 
    + turbulence(position, 8) 
    + turbulence(position, 16) 
    + turbulence(position, 32)
    + turbulence(position, 64)
    + turbulence(position, 128)
    ;
}


void main(void) {

    // get noise in range 0 .. 1
    float noise = clamp(0.5f + amplitude * turb(position * frequency), 0, 1); 

    // apply vertex lighting
    vec3 color = gl_Color.rgb * vec3(noise, noise, noise);
    gl_FragColor = vec4(color, 1);
}


/* old implementation
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
*/