const vec3 COLOR = vec3(0x00, 0xD8, 0x02) / vec3(0xFF);
const float CUTOFF = 0.65;
const float NOISE_MULT = 8.0;
const float NOISE_POWER = 1.0;

float noise4D(vec4 p) {
	return fract(sin(dot(p ,vec4(12.9898,78.233,126.7235, 593.2241))) * 43758.5453);
}

float worley4D(vec4 p) {
	float r = 3.0;
    vec4 f = floor(p);
    vec4 x = fract(p);
	for(int i = -1; i<=1; i++)
	{
		for(int j = -1; j<=1; j++)
		{
			for(int k = -1; k<=1; k++)
			{
                for (int l = -1; l <= 1; l++) {
                	vec4 q = vec4(float(i),float(j),float(k), float(l));
    				vec4 v = q + vec4(noise4D((q+f)*1.11), noise4D((q+f)*1.14), noise4D((q+f)*1.17), noise4D((q+f)*1.20)) - x;
    				float d = dot(v, v);
					r = min(r, d);
                }
			}
		}
	}
    return sqrt(r);
}


vec3 mainColor(vec3 direction) {
    float n = worley4D(vec4(direction * NOISE_MULT, iGlobalTime / 3.0));
    n = 1.0 - n;
    n = pow(n, NOISE_POWER);
    if (n < CUTOFF) {
        return vec3(0.0);
    }

    n = (n - CUTOFF) / (1.0 - CUTOFF);
	return COLOR * (1.0 - n);
}

vec3 getSkyboxColor() {
	return mainColor(normalize(_normal));
}
