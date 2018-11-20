// Replicate the default skybox texture

const int NUM_COLORS = 5;
const vec3 WHITISH = vec3(0.471, 0.725, 0.825);
const vec3 GREENISH = vec3(0.157, 0.529, 0.588);
const vec3 COLORS[NUM_COLORS] = vec3[](
    GREENISH,
    GREENISH,
    WHITISH,
    WHITISH,
    vec3(0.6, 0.275, 0.706)    // purple
);
const float PI = 3.14159265359;
const vec3 BLACK = vec3(0.0);
const vec3 SPACE_BLUE = vec3(0.0, 0.118, 0.392);
const float HORIZONTAL_OFFSET = 3.75;

vec3 getSkyboxColor() {
    vec2 horizontal = vec2(_normal.x, _normal.z);
    horizontal = normalize(horizontal);
    float theta = atan(horizontal.y, horizontal.x);
    theta = 0.5 * (theta / PI + 1.0);
    float index = theta * NUM_COLORS;
    index = mod(index + HORIZONTAL_OFFSET, NUM_COLORS);
    int index1 = int(index) % NUM_COLORS;
    int index2 = (index1 + 1) % NUM_COLORS;
    vec3 horizonColor = mix(COLORS[index1], COLORS[index2], index - index1);
    horizonColor = mix(horizonColor, SPACE_BLUE, smoothstep(0.0, 0.08, _normal.y));
    horizonColor = mix(horizonColor, BLACK, smoothstep(0.04, 0.15, _normal.y));
    horizonColor = mix(BLACK, horizonColor, smoothstep(-0.01, 0.0, _normal.y));
	return pow(horizonColor, vec3(0.4545));;
}
