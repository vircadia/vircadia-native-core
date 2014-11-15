#version 120

//
//  spot_light.frag
//  fragment shader
//
//  Created by Andrzej Kapolka on 9/18/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// the diffuse texture
uniform sampler2D diffuseMap;

// the normal texture
uniform sampler2D normalMap;

// the specular texture
uniform sampler2D specularMap;

// the depth texture
uniform sampler2D depthMap;

// the distance to the near clip plane
uniform float near;

// scale factor for depth: (far - near) / far
uniform float depthScale;

// offset for depth texture coordinates
uniform vec2 depthTexCoordOffset;

// scale for depth texture coordinates
uniform vec2 depthTexCoordScale;

// the radius (hard cutoff) of the light effect
uniform float radius;

void main(void) {
    // get the depth and exit early if it doesn't pass the test
    vec2 texCoord = gl_TexCoord[0].st / gl_TexCoord[0].q;
    float depth = texture2D(depthMap, texCoord).r;
    if (depth < gl_FragCoord.z) {
        discard;
    }
    // compute the view space position using the depth
    float z = near / (depth * depthScale - 1.0);
    vec4 position = vec4((depthTexCoordOffset + texCoord * depthTexCoordScale) * z, z, 1.0);
    
    // get the normal from the map
    vec4 normal = texture2D(normalMap, texCoord);
    vec4 normalizedNormal = normalize(normal * 2.0 - vec4(1.0, 1.0, 1.0, 2.0));
    
    // compute the base color based on OpenGL lighting model
    vec4 lightVector = gl_LightSource[1].position - position;
    float lightDistance = length(lightVector);
    lightVector = lightVector / lightDistance;
    float diffuse = dot(normalizedNormal, lightVector);
    float facingLight = step(0.0, diffuse);
    vec4 baseColor = texture2D(diffuseMap, texCoord) * (gl_FrontLightProduct[1].ambient +
        gl_FrontLightProduct[1].diffuse * (diffuse * facingLight));
    
    // compute attenuation based on spot angle, distance, etc.
    float cosSpotAngle = max(-dot(lightVector.xyz, gl_LightSource[1].spotDirection), 0.0);
    float attenuation = step(lightDistance, radius) * step(gl_LightSource[1].spotCosCutoff, cosSpotAngle) *
        pow(cosSpotAngle, gl_LightSource[1].spotExponent) / dot(vec3(gl_LightSource[1].constantAttenuation,
            gl_LightSource[1].linearAttenuation, gl_LightSource[1].quadraticAttenuation),
                vec3(1.0, lightDistance, lightDistance * lightDistance));
                
    // add base to specular, modulate by attenuation
    float specular = facingLight * max(0.0, dot(normalize(lightVector - normalize(vec4(position.xyz, 0.0))),
        normalizedNormal));    
    vec4 specularColor = texture2D(specularMap, texCoord);
    gl_FragColor = vec4((baseColor.rgb + pow(specular, specularColor.a * 128.0) * specularColor.rgb) * attenuation, 0.0);
}
