#line 2

float noise(vec3 v) {
    return snoise(vec4(v, iGlobalTime));
}

vec3 noise3_3(vec3 p) {
    float fx = noise(p);
    float fy = noise(p + vec3(1345.67, 0, 45.67));
    float fz = noise(p + vec3(0, 134.67, 3245.67));
    return vec3(fx, fy, fz);
}

vec4 getProceduralColor() {
    vec3 color = noise3_3(_position.xyz * 10.0);
    return vec4(color, 1.0);
}