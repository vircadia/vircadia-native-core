float aspect(vec2 v) {
    return v.x / v.y;
}

vec3 indexedTexture() {
    vec2 uv = _position.xy;
    uv += 0.5;
    uv.y = 1.0 - uv.y;
    
    float targetAspect = iWorldScale.x / iWorldScale.y;
    float sourceAspect = aspect(iChannelResolution[0].xy);
    float aspectCorrection = sourceAspect / targetAspect;
    if (aspectCorrection > 1.0) {
        float offset = aspectCorrection - 1.0;
        float halfOffset = offset / 2.0;
        uv.y -= halfOffset;
        uv.y *= aspectCorrection;
    } else {
        float offset = 1.0 - aspectCorrection;
        float halfOffset = offset / 2.0;
        uv.x -= halfOffset;
        uv.x /= aspectCorrection;
    }
    
    if (any(lessThan(uv, vec2(0.0)))) {
        return vec3(0.0);
    }

    if (any(greaterThan(uv, vec2(1.0)))) {
        return vec3(0.0);
    }

    vec4 color = texture(iChannel0, uv);
    return color.rgb * max(0.5, sourceAspect) * max(0.9, fract(iWorldPosition.x));
}

float getProceduralColors(inout vec3 diffuse, inout vec3 specular, inout float shininess) {
    if (_position.z > -0.49) {
        discard;
    }

    specular = indexedTexture(); 
    return 1.0;
}