#line 2
// Created by inigo quilez - iq/2014
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.

// { 2d cell id, distance to border, distnace to center )
vec4 hexagon(vec2 p) {
    vec2 q = vec2(p.x * 2.0 * 0.5773503, p.y + p.x * 0.5773503);

    vec2 pi = floor(q);
    vec2 pf = fract(q);

    float v = mod(pi.x + pi.y, 3.0);

    float ca = step(1.0, v);
    float cb = step(2.0, v);
    vec2 ma = step(pf.xy, pf.yx);

    // distance to borders
    float e = dot(ma,
            1.0 - pf.yx + ca * (pf.x + pf.y - 1.0) + cb * (pf.yx - 2.0 * pf.xy));

    // distance to center           
    p = vec2(q.x + floor(0.5 + p.y / 1.5), 4.0 * p.y / 3.0) * 0.5 + 0.5;
    float f = length((fract(p) - 0.5) * vec2(1.0, 0.85));

    return vec4(pi + ca - cb * ma, e, f);
}


float hash1(vec2 p) {
    float n = dot(p, vec2(127.1, 311.7));
    return fract(sin(n) * 43758.5453);
}

vec4 getProceduralColor() {
    vec2 uv = _position.xz + 0.5;
    vec2 pos = _position.xz * 40.0;
    // gray
    vec4 h = hexagon(8.0 * pos + 0.5);
    float n = snoise(vec3(0.3 * h.xy + iGlobalTime * 0.1, iGlobalTime));
    vec3 col = 0.15 + 0.15 * hash1(h.xy + 1.2) * vec3(1.0);
    col *= smoothstep(0.10, 0.11, h.z);
    col *= smoothstep(0.10, 0.11, h.w);
    col *= 1.0 + 0.15 * sin(40.0 * h.z);
    col *= 0.75 + 0.5 * h.z * n;
    col *= pow(16.0 * uv.x * (1.0 - uv.x) * uv.y * (1.0 - uv.y), 0.1);
    return vec4(col, 1.0);
}
