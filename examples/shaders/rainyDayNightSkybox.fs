// Created by inigo quilez - iq/2013
// Turbulence and Day/Night cycle added by Michael Olson - OMGparticles/2015
// rain effect adapted from Rainy London by David Hoskins. - https://www.shadertoy.com/view/XdSGDc
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.
#line 6
const float PI = 3.14159;
uniform float rotationSpeed = 0.005;
uniform float gridLevel = 0.0;
uniform float constellationLevel = 0.0;
uniform float constellationBoundaryLevel = 0.0;

// Axial tilt
const float axialTilt = 0.408407;
const vec2 atsc = vec2(sin(axialTilt), cos(axialTilt));
const mat3 axialTiltMatrix = mat3(
        vec3(atsc.y, -atsc.x, 0.0),
        vec3(atsc.x, atsc.y, 0.0),
        vec3(0.0, 0.0, 1.0));

vec2 directionToSpherical(in vec3 d) {
    return vec2(atan(d.x, -d.z), asin(d.y)) * -1.0f;
}

vec2 directionToUv(in vec3 d) {
    vec2 uv = directionToSpherical(d);
    uv /= PI;
    uv.x /= 2.0;
    uv += 0.5;
    return uv;
}

vec3 stars3(in vec3 d) {
    //return rd;
    vec3 dir = d * axialTiltMatrix;
    
    float theta = rotationSpeed * iGlobalTime * 4.0;
    vec2 sc = vec2(sin(theta), cos(theta));
    dir = dir * mat3( vec3(sc.y, 0.0, sc.x),
                    vec3(0.0, 1.0, 0.0),
                    vec3(-sc.x, 0.0, sc.y));
    
    vec2 uv = directionToUv(dir);
    vec3 starColor = texture2D( iChannel0, uv).xyz;
    starColor = pow(starColor, vec3(0.75));
    
    if (gridLevel  > 0.0) {
        starColor += texture2D( iChannel1, uv).xyz * gridLevel;
    }
    return starColor;
}

//const vec3 vNightColor   = vec3(.15, 0.3, 0.6);
uniform vec3 uDayColor     = vec3(0.9,0.7,0.7);
const vec3 vNightColor   = vec3(1.0, 1.0, 1.0);
const vec3 vHorizonColor = vec3(0.6, 0.3, 0.4);
const vec3 vSunColor     = vec3(1.0,0.8,0.6);
const vec3 vSunRimColor  = vec3(1.0,0.66,0.33);

vec4 render( in vec3 ro, in vec3 rd )
{
    float fSunSpeed = (rotationSpeed * 10.0 * iGlobalTime) + PI / 2.0 * 3.0;
    vec3 sundir = normalize( vec3(cos(fSunSpeed),sin(fSunSpeed),0.0) );
    sundir  = sundir * axialTiltMatrix;
    float sun = clamp( dot(sundir,rd), 0.0, 1.0 );
    
    float fSunHeight = sundir.y;
    
    // below this height will be full night color
    float fNightHeight = -0.8;
    // above this height will be full day color
    float fDayHeight   = 0.3;
    
    float fHorizonLength = fDayHeight - fNightHeight;
    float fInverseHL = 1.0 / fHorizonLength;
    float fHalfHorizonLength = fHorizonLength / 2.0;
    float fInverseHHL = 1.0 / fHalfHorizonLength;
    float fMidPoint = fNightHeight + fHalfHorizonLength;
    
    float fNightContrib = clamp((fSunHeight - fMidPoint) * (-fInverseHHL), 0.0, 1.0);
    float fHorizonContrib = -clamp(abs((fSunHeight - fMidPoint) * (-fInverseHHL)), 0.0, 1.0) + 1.0;
    float fDayContrib = clamp((fSunHeight - fMidPoint) * ( fInverseHHL), 0.0, 1.0);
    
    // sky color
    vec3 vSkyColor = vec3(0.0);
    vSkyColor += mix(vec3(0.0),   vNightColor, fNightContrib);   // Night
    vSkyColor += mix(vec3(0.0), vHorizonColor, fHorizonContrib); // Horizon
    vSkyColor += mix(vec3(0.0),     uDayColor, fDayContrib);     // Day
    vec3 col = vSkyColor;
    
    // atmosphere brighter near horizon
    col -= clamp(rd.y, 0.0, 0.5);
    
    // draw sun
    col += 0.4 * vSunRimColor * pow( sun,    4.0 );
    col += 1.0 * vSunColor    * pow( sun, 2000.0 );
    
    // stars
    float fStarContrib = clamp((fSunHeight - fDayHeight) * (-fInverseHL), 0.0, 1.0);
    if (fStarContrib > 0.0) {
        vec3 vStarDir = rd;
        col = mix(col, stars3(vStarDir), fStarContrib);
    }

    // Ten layers of rain sheets...
    float rainBrightness = 0.15;
    vec2 q = vec2(atan(rd.x, -rd.z), asin(rd.y));
	float dis = 1;
    int sheets = 12;
	for (int i = 0; i < sheets; i++) {
        float f = pow(dis, .45) + 0.25;
        vec2 st =  f * (q * vec2(2.0, .05) + vec2(-iGlobalTime*.01 + q.y * .05, iGlobalTime * 0.12));
        f = texture2D(iChannel2, st * .5, -59.0).x;
        f += texture2D(iChannel2, st*.284, -99.0).y;
        f = clamp(pow(abs(f)*.5, 29.0) * 140.0, 0.00, q.y*.4+.05);
        vec3 bri = vec3(rainBrightness);
        col += bri*f;
		dis += 3.5;
	}

    return vec4(clamp(col, 0.0, 1.0), 1.0);
}

vec3 getSkyboxColor() {
    return render( vec3(0.0), normalize(_normal) ).rgb;
}
