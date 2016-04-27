#line 2

const vec3 RED = vec3(1.0, 0.0, 0.0);
const vec3 GREEN = vec3(0.0, 1.0, 0.0);
const vec3 BLUE = vec3(0.0, 0.0, 1.0);
const vec3 YELLOW = vec3(1.0, 1.0, 0.0);
const vec3 WHITE = vec3(1.0, 1.0, 1.0);

vec4 getProceduralColor() {
    float intensity = 0.0;  
    for (int i = 0; i < 2; ++i) {
        float modifier = pow(2, i);
        float noise = snoise(vec4(_position.xyz * 10.0 * modifier, iGlobalTime));
        noise /= modifier;
        intensity += noise;
    }
    intensity /= 2.0;
    intensity += 0.5;
    vec3 color = (intensity * BLUE) + (1.0 - intensity) * YELLOW;
    return vec4(color, 1.0); 
}

