

#define TWO_PI 6.28318530718

uniform float iBloomPct = 0.2;
uniform float hueAngleRange = 20.0;
uniform float hueOffset = 0.5;


vec3 hsb2rgb( in vec3 c ){
    vec3 rgb = clamp(abs(mod(c.x*6.0+vec3(0.0,4.0,2.0),
                             6.0)-3.0)-1.0, 
                     0.0, 
                     1.0 );
    rgb = rgb*rgb*(3.0-2.0*rgb);
    return c.z * mix( vec3(1.0), rgb, c.y);
}


void mainImage( out vec4 fragColor, in vec2 fragCoord ) {
    vec2 st = fragCoord.xy/iWorldScale.xz;
    vec3 color = vec3(0.0, 0.0, 0.0);

    vec2 toCenter = vec2(0.5) - st;
    float angle = atan(toCenter.y, toCenter.x);
    float radius = length(toCenter) * 2.0;

    // Second check is so we discard the top half of the sphere
    if ( iBloomPct < radius || _position.y > 0) {
        discard;
    }

    // simulate ambient occlusion
    float brightness = pow(radius, 0.8);
    float hueTimeOffset = sin(iGlobalTime * 0.01) + hueOffset;
    color = hsb2rgb(vec3( abs(angle/hueAngleRange) + hueTimeOffset, 0.7, brightness));
  

    fragColor = vec4(color, 1.0);
}




vec4 getProceduralColor() {
    vec4 result;
    vec2 position = _position.xz;
    position += 0.5;
    
    mainImage(result, position * iWorldScale.xz);
 
    return result;
}