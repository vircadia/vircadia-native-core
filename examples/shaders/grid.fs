#line 2

// https://www.shadertoy.com/view/lss3WS

// srtuss, 2013

// collecting some design ideas for a new game project.
// no raymarching is used.

// if i could add a custom soundtrack, it'd use this one (essential for desired sensation)
// http://www.youtube.com/watch?v=1uFAu65tZpo

//#define GREEN_VERSION

// ** improved camera shaking
// ** cleaned up code
// ** added stuff to the gates

// srtuss, 2013
float time = iGlobalTime;

// some noise functions for fast developing
float rand11(float p) {
    return fract(sin(p * 591.32) * 43758.5357);
}

float rand12(vec2 p) {
    return fract(sin(dot(p.xy, vec2(12.9898, 78.233))) * 43758.5357);
}

vec2 rand21(float p) {
    return fract(vec2(sin(p * 591.32), cos(p * 391.32)));
}

vec2 rand22(in vec2 p)
{
    return fract(vec2(sin(p.x * 591.32 + p.y * 154.077),
                    cos(p.x * 391.32 + p.y * 49.077)));
}

vec3 voronoi(in vec2 x) {
    vec2 n = floor(x); // grid cell id
    vec2 f = fract(x);// grid internal position
    vec2 mg;// shortest distance...
    vec2 mr;// ..and second shortest distance
    float md = 8.0, md2 = 8.0;
    for(int j = -1; j <= 1; j ++)
    {
        for(int i = -1; i <= 1; i ++)
        {
            vec2 g = vec2(float(i), float(j)); // cell id
            vec2 o = rand22(n + g);// offset to edge point
            vec2 r = g + o - f;

            float d = max(abs(r.x), abs(r.y));// distance to the edge

            if(d < md)
            {   md2 = md; md = d; mr = r; mg = g;}
            else if(d < md2)
            {   md2 = d;}
        }
    }
    return vec3(n + mg, md2 - md);
}


vec4 getProceduralColor() {
    float inten = 0.0;
    vec3 its;
    float v, g;
    // voronoi floor layers
    for (int i = 0; i < 4; i++) {
        float layer = float(i);
        vec2 pos = _position.xz * 100.0;
        if (i == 2) {
            pos.x += time * 0.05;
        } else if (i == 1) {
            pos.y += time * 0.07;
        }
        vec3 vo = voronoi(pos + 8.0 * rand21(float(i)));

        v = exp(-100.0 * (vo.z - 0.02));
        float fx = 0.0;

        // add some special fx to lowest layer
        if (i == 3) {
            float crd = 0.0; //fract(time * 0.2) * 50.0 - 25.0;
            float fxi = cos(vo.x * 0.2 + time * 0.5); //abs(crd - vo.x);
            fx = clamp(smoothstep(0.9, 1.0, fxi), 0.0, 0.9) * 1.0 * rand12(vo.xy);
            fx *= exp(-3.0 * vo.z) * 2.0;
        }
        inten += v * 0.1 + fx;
    }
    inten *= 0.4 + (sin(time) * 0.5 + 0.5) * 0.6;
    vec3 cr = vec3(0.15, 2.0, 9.0);
    vec3 cg = vec3(2.0, 0.15, 9.0);
    vec3 cb = vec3(9.0, 2.0, 0.15);
    vec3 ct = vec3(9.0, 0.25, 0.3);
    vec3 cy = vec3(0.25, 0.3, 9.3);
    vec3 col = pow(vec3(inten), 1.5 * cy);
    return vec4(col, 1.0);
}
