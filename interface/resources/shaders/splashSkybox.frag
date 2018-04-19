
float noise3D(vec3 p)
{
    return fract(sin(dot(p ,vec3(12.9898,78.233,126.7235))) * 43758.5453);
}

float worley3D(vec3 p)
{                                        
    float r = 3.0;
    vec3 f = floor(p);
    vec3 x = fract(p);
    for(int i = -1; i<=1; i++)
    {
        for(int j = -1; j<=1; j++)
        {
            for(int k = -1; k<=1; k++)
            {
                vec3 q = vec3(float(i),float(j),float(k));
                vec3 v = q + vec3(noise3D((q+f)*1.11), noise3D((q+f)*1.14), noise3D((q+f)*1.17)) - x;
                float d = dot(v, v);
                r = min(r, d);
            }
        }
    }
    return sqrt(r);
}   


vec3 getSkyboxColor() {
    vec3 color = abs(normalize(_normal));
//    vec2 uv;
//    uv.x = 0.5 + atan(_normal.z, _normal.x);
//    uv.y = 0.5 - asin(_normal.y);
//    uv *= 20.0;
//    color.r = worley3D(vec3(uv, iGlobalTime));
    return color;
}