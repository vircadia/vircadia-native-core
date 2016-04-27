vec2 iResolution = iWorldScale.xy; 
vec2 iMouse = vec2(0);

const float PI = 3.14159265;

float time = iGlobalTime;
vec2 center = vec2(0.5, 0.5);
void mainImage( out vec4 fragColor, in vec2 fragCoord ) {
    vec2 position = (fragCoord.xy/iResolution.xy) + 0.5;
    float dist = pow(distance(position.xy, center), 3.);
    dist = dist / 1.0 + sin(time * 10)/100.0;
    vec3 color = vec3(dist, 0.0, dist);
    fragColor = vec4(color, 1.0);
}

vec4 getProceduralColor() {
    vec4 result;
    vec2 position = _position.xy;
    
    mainImage(result, position * iWorldScale.xy);
 
    return result;
}