const vec3 BLUE = vec3(0.0, 0.0, 1.0);
const vec3 YELLOW = vec3(1.0, 1.0, 0.0);

uniform float iSpeed = 1.0;
uniform vec3 iSize = vec3(1.0, 1.0, 1.0);

vec3 getNoiseColor() {
    float intensity = 0.0;
    vec3 position = _position.xyz;
    //position = normalize(position);
    float time = iGlobalTime * iSpeed;
    for (int i = 0; i < 4; ++i) {
        float modifier = pow(2, i);
        vec3 noisePosition = position * iSize * 10.0 * modifier;
        float noise = snoise(vec4(noisePosition, time));
        noise /= modifier;
        intensity += noise;
    }
    intensity /= 2.0; intensity += 0.5;
    return (intensity * BLUE) + (1.0 - intensity) * YELLOW;
}

// Produce a lit procedural surface
float getProceduralColorsLit(inout vec3 diffuse, inout vec3 specular, inout float shininess) {
    vec3 noiseColor = getNoiseColor();
    diffuse = noiseColor;
    return 0.0;
}

// Produce an unlit procedural surface:  emulates old behavior
float getProceduralColorsUnlit(inout vec3 diffuse, inout vec3 specular, inout float shininess) {
    vec3 noiseColor = getNoiseColor();
    diffuse = vec3(1.0);
    specular = noiseColor;
    return 1.0;
}

float getProceduralColors(inout vec3 diffuse, inout vec3 specular, inout float shininess) {
    return getProceduralColorsLit(diffuse, specular, shininess);
}