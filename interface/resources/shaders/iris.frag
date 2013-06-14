#version 120

//
//  iris.frag
//  fragment shader
//
//  Created by Andrzej Kapolka on 6/13/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

// the iris texture
uniform sampler2D texture;

// the interpolated normal
varying vec4 normal;

void main(void) {
    // compute the specular component (sans exponent) based on the normal OpenGL lighting model
    float specular = max(0.0, dot(normalize(gl_LightSource[0].position + vec4(0.0, 0.0, 1.0, 0.0)), normalize(normal)));
    
    // modulate texture by diffuse color and add specular contribution
    gl_FragColor = vec4(gl_Color.rgb * texture2D(texture, gl_TexCoord[0].st).rgb +
        pow(specular, gl_FrontMaterial.shininess) * gl_FrontLightProduct[0].specular.rgb, 1.0);
}
