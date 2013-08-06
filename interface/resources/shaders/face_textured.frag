#version 120

//
//  face_textured.frag
//  fragment shader
//
//  Created by Andrzej Kapolka on 8/6/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

// the texture coordinate vector from left to right
uniform vec2 texCoordRight;

// the texture coordinate vector from bottom to the top
uniform vec2 texCoordUp;

// the permutation/normal texture
uniform sampler2D permutationNormalTexture;

// the depth texture
uniform sampler2D depthTexture;

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
    // compute normal from adjacent depth values
    float left = texture2D(depthTexture, gl_TexCoord[0].st - texCoordRight * 0.01).r;
    float right = texture2D(depthTexture, gl_TexCoord[0].st + texCoordRight * 0.01).r;
    float bottom = texture2D(depthTexture, gl_TexCoord[0].st - texCoordUp * 0.01).r;
    float top = texture2D(depthTexture, gl_TexCoord[0].st + texCoordUp * 0.01).r;
    vec3 normal = normalize(gl_NormalMatrix * vec3(left - right, top - bottom, -0.05));
    
    // compute the specular component (sans exponent) based on the normal OpenGL lighting model
    float specular = max(0.0, dot(normalize(gl_LightSource[0].position.xyz + vec3(0.0, 0.0, 1.0)), normal));
    
    vec3 color = mix(vec3(1.0, 1.0, 1.0), vec3(0.75, 0.75, 0.75),
        sin(dot(position, vec3(25.0, 25.0, 25.0)) + 2.0 * perlin(position * 10.0)));
    
    // standard lighting
    gl_FragColor = vec4(color * ( gl_LightModel.ambient.rgb + /* gl_LightSource[0].ambient.rgb + */
        gl_LightSource[0].diffuse.rgb * max(0.0, dot(normal, gl_LightSource[0].position.xyz))) +
            pow(specular, gl_FrontMaterial.shininess) * gl_FrontLightProduct[0].specular.rgb, gl_Color.a);
}
