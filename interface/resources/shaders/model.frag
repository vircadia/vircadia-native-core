#version 120

//
//  model.frag
//  fragment shader
//
//  Created by Andrzej Kapolka on 10/14/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

// the diffuse texture
uniform sampler2D diffuseMap;

// the interpolated normal
varying vec4 normal;

void main(void) {
    // compute the base color based on OpenGL lighting model
    vec4 normalizedNormal = normalize(normal);
    vec4 base = gl_Color * (gl_FrontLightModelProduct.sceneColor + gl_FrontLightProduct[0].ambient +
        gl_FrontLightProduct[0].diffuse * max(0.0, dot(normalizedNormal, gl_LightSource[0].position)));

    // compute the specular component (sans exponent)
    float specular = max(0.0, dot(normalize(gl_LightSource[0].position + vec4(0.0, 0.0, 1.0, 0.0)), normalizedNormal));
    
    // modulate texture by base color and add specular contribution
    gl_FragColor = base * texture2D(diffuseMap, gl_TexCoord[0].st) +
        pow(specular, gl_FrontMaterial.shininess) * gl_FrontLightProduct[0].specular;
}
