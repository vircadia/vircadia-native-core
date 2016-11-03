// srtuss, 2013

// collecting some design ideas for a new game project.
// no raymarching is used.

// if i could add a custom soundtrack, it'd use this one (essential for desired sensation)
// http://www.youtube.com/watch?v=1uFAu65tZpo

//#define GREEN_VERSION

// ** improved camera shaking
// ** cleaned up code
// ** added stuff to the gates

// *******************************************************************************************
// Please do NOT use this shader in your own productions/videos/games without my permission!
// If you'd still like to do so, please drop me a mail (stral@aon.at)
// *******************************************************************************************
float time = iGlobalTime;

vec2 rotate(vec2 p, float a) {
    return vec2(p.x * cos(a) - p.y * sin(a), p.x * sin(a) + p.y * cos(a));
}

float box(vec2 p, vec2 b, float r) {
    return length(max(abs(p) - b, 0.0)) - r;
}

// iq's ray-plane-intersection code
vec3 intersect(in vec3 o, in vec3 d, vec3 c, vec3 u, vec3 v)
{
    vec3 q = o - c;
    return vec3(
            dot(cross(u, v), q),
            dot(cross(q, u), d),
            dot(cross(v, q), d)) / dot(cross(v, u), d);
}

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
    return fract(vec2(sin(p.x * 591.32 + p.y * 154.077), cos(p.x * 391.32 + p.y * 49.077)));
}

float noise11(float p) {
    float fl = floor(p);
    return mix(rand11(fl), rand11(fl + 1.0), fract(p)); //smoothstep(0.0, 1.0, fract(p)));
}
float fbm11(float p) {
    return noise11(p) * 0.5 + noise11(p * 2.0) * 0.25 + noise11(p * 5.0) * 0.125;
}
vec3 noise31(float p) {
    return vec3(noise11(p), noise11(p + 18.952), noise11(p - 11.372)) * 2.0 - 1.0;
}

// something that looks a bit like godrays coming from the surface
float sky(vec3 p) {
    float a = atan(p.x, p.z);
    float t = time * 0.1;
    float v = rand11(floor(a * 4.0 + t)) * 0.5 + rand11(floor(a * 8.0 - t)) * 0.25
            + rand11(floor(a * 16.0 + t)) * 0.125;
    return v;
}

vec3 voronoi(in vec2 x)
{
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



vec3 getSkyboxColor() {
    vec3 rd = normalize(_normal);
    vec3 ro = vec3(0, 0, 1);
    float inten = 0.0;

    // background
    float sd = dot(rd, vec3(0.0, 1.0, 0.0));
    inten = pow(1.0 - abs(sd), 20.0) + pow(sky(rd), 5.0) * step(0.0, rd.y) * 0.2;

    vec3 its;
    float v, g;

    // voronoi floor layers
    for(int i = 0; i < 4; i ++)
    {
        float layer = float(i);
        its = intersect(ro, rd, vec3(0.0, -5.0 - layer * 5.0, 0.0), vec3(1.0, 0.0, 0.0), vec3(0.0, 0.0, 1.0));
        if(its.x > 0.0)
        {
            vec3 vo = voronoi((its.yz) * 0.05 + 8.0 * rand21(float(i)));
            v = exp(-100.0 * (vo.z - 0.02));

            float fx = 0.0;

            // add some special fx to lowest layer
            if(i == 3)
            {
                float crd = 0.0;                //fract(time * 0.2) * 50.0 - 25.0;
                float fxi = cos(vo.x * 0.2 + time * 1.5);//abs(crd - vo.x);
                fx = clamp(smoothstep(0.9, 1.0, fxi), 0.0, 0.9) * 1.0 * rand12(vo.xy);
                fx *= exp(-3.0 * vo.z) * 2.0;
            }
            inten += v * 0.1 + fx;
        }
    }

    inten *= 0.4 + (sin(time) * 0.5 + 0.5) * 0.6;

    vec3 ct = vec3(0.6, 0.8, 9.0);
    // find a color for the computed intensity
    vec3 col = pow(vec3(inten), ct);
    return col;
}
