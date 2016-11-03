float aspect(vec2 v) {
    return v.x / v.y;
}

vec3 aspectCorrectedTexture() {
    vec2 uv;

    if (abs(_position.y) > 0.4999) {
        uv = _position.xz;
    } else if (abs(_position.z) > 0.4999) {
        uv = _position.xy;
    } else {
        uv = _position.yz;
    } 
    uv += 0.5;
    uv.y = 1.0 - uv.y;
    return texture(iChannel0, uv).rgb;
}

float getProceduralColors(inout vec3 diffuse, inout vec3 specular, inout float shininess) {
    specular = aspectCorrectedTexture();
    return 1.0;
}

