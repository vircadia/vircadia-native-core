vec2 iResolution = iWorldScale.xz; 

// from https://www.shadertoy.com/view/Xd2XWR

// Created by Daniel Burke - burito/2014
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.

// Inspiration from Dr Who (2005) S7E13 - The Name of the Doctor

vec2 rot(vec2 p, float a)
{
    float c = cos(a);
    float s = sin(a);
    return vec2(p.x*c + p.y*s,
                -p.x*s + p.y*c);
}

float circle(vec2 pos, float radius)
{
    return clamp(((1.0-abs(length(pos)-radius))-0.99)*100.0, 0.0, 1.0);
    
}

float circleFill(vec2 pos, float radius)
{
    return clamp(((1.0-(length(pos)-radius))-0.99)*100.0, 0.0, 1.0);   
}

// Thanks Iñigo Quilez!
float line( in vec2 p, in vec2 a, in vec2 b )
{
    vec2 pa = -p - a;
    vec2 ba = b - a;
    float h = clamp( dot(pa,ba)/dot(ba,ba), 0.0, 1.0 );
    float d = length( pa - ba*h );
    
    return clamp(((1.0 - d)-0.99)*100.0, 0.0, 1.0);
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
                vec2 uv = fragCoord.xy / iResolution.xy;
    vec2 p = -1.0 + 2.0 * uv;
    p.x *= iResolution.x / iResolution.y;
 
    vec3 colour = vec3(0);
    vec3 white = vec3(1);
    
    
    
    float c = circle(p, 0.2);
    c += circle(p, 0.1);
    c += circle(p, 0.18);
    c += circleFill(p, 0.005);

//    c += circle(p, 1.3);
    c += circle(p, 1.0);
    if(p.x > 0.0)c += circle(p, 0.4);
    if(p.x > 0.0)c += circle(p, 0.42);
    if(p.x < 0.0)c += circle(p, 0.47);
    c += circleFill(p+vec2(0.47, 0.0), 0.02);
    c += circleFill(p+vec2(0.84147*0.47, 0.54030*0.47), 0.02);
    c += circleFill(p+vec2(0.84147*0.47, -0.54030*0.47), 0.02);
    c += circleFill(p+vec2(0.41614*0.47, 0.90929*0.47), 0.02);
    c += circleFill(p+vec2(0.41614*0.47, -0.90929*0.47), 0.02);
    
    float t = iGlobalTime;
    float t2 = t * -0.01;
    float t3 = t * 0.03;
    
    vec2 angle1 = vec2(sin(t), cos(t));
    vec2 a = angle1 * 0.7;
    
    t *= 0.5;
    vec2 angle2 = vec2(sin(t), cos(t));
    vec2 b = angle2 * 0.8;
    
    vec2 angle3 = vec2(sin(t2), cos(t2));
    vec2 d = b + angle3* 0.4;

    vec2 angle4 = vec2(sin(t3), cos(t3));
    vec2 e = angle4 * 0.9;

    vec2 angle5 = vec2(sin(t3+4.0), cos(t3+4.0));
    vec2 f = angle5 * 0.8;
    
    vec2 angle6 = vec2(sin(t*-0.1+5.0), cos(t*-0.1+5.0));
    vec2 h = angle6 * 0.8;
    
    

    
    
    float tt = t * 1.4;
    
    float tm = mod(tt, 0.5);
    float tmt = tt - tm;
    if( tm > 0.4) tmt += (tm-0.4)*5.0;
    vec2 tangle1 = vec2(sin(tmt), cos(tmt));

                tt *= 0.8;
    tm = mod(tt, 0.6);
    float tmt2 = tt - tm;
    if( tm > 0.2) tmt2 += (tm-0.2)*1.5;
    
    vec2 tangle2 = vec2(sin(tmt2*-4.0), cos(tmt2*-4.0));
   
    vec2 tangle3 = vec2(sin(tmt2), cos(tmt2));
    
    tt = t+3.0;
    tm = mod(tt, 0.2);
    tmt = tt - tm;
    if( tm > 0.1) tmt += (tm-0.1)*2.0;
    vec2 tangle4 = vec2(sin(-tmt), cos(-tmt)); tmt += 0.9;
    vec2 tangle41 = vec2(sin(-tmt), cos(-tmt)); tmt += 0.5;
    vec2 tangle42 = vec2(sin(-tmt), cos(-tmt)); tmt += 0.5;
    vec2 tangle43 = vec2(sin(-tmt), cos(-tmt)); tmt += 0.5;
    vec2 tangle44 = vec2(sin(-tmt), cos(-tmt)); tmt += 0.5;
    vec2 tangle45 = vec2(sin(-tmt), cos(-tmt));

    tt = iGlobalTime+0.001;
    tm = mod(tt, 1.0);
    tmt = tt - tm;
    if( tm > 0.9) tmt += (tm-0.9)*10.0;

    vec2 tangle51 = 0.17*vec2(sin(-tmt), cos(-tmt)); tmt += 1.0471975511965976;
    vec2 tangle52 = 0.17*vec2(sin(-tmt), cos(-tmt)); tmt += 1.0471975511965976;
    vec2 tangle53 = 0.17*vec2(sin(-tmt), cos(-tmt));
    
    c += line(p, tangle51, -tangle53);
    c += line(p, tangle52, tangle51);
    c += line(p, tangle53, tangle52);
    c += line(p, -tangle51, tangle53);
    c += line(p, -tangle52, -tangle51);
    c += line(p, -tangle53, -tangle52);

    c += circleFill(p+tangle51, 0.01);
    c += circleFill(p+tangle52, 0.01);
    c += circleFill(p+tangle53, 0.01);
    c += circleFill(p-tangle51, 0.01);
    c += circleFill(p-tangle52, 0.01);
    c += circleFill(p-tangle53, 0.01);
    
    
    
    c += circle(p+a, 0.2);
    c += circle(p+a, 0.14);
    c += circle(p+a, 0.1);
    c += circleFill(p+a, 0.04);
    c += circleFill(p+a+tangle3*0.2, 0.025);   
    
    
    c += circle(p+a, 0.14);


    c += circle(p+b, 0.2);
    c += circle(p+b, 0.03);
    c += circle(p+b, 0.15);
    c += circle(p+b, 0.45);
    c += circleFill(p+b+tangle1*0.05, 0.01);
    c += circleFill(p+b+tangle1*0.09, 0.02);
    c += circleFill(p+b+tangle1*0.15, 0.03);
    c += circle(p+b+tangle1*-0.15, 0.03);
    c += circle(p+b+tangle1*-0.07, 0.015);

    c += circle(p+d, 0.08);


    c += circle(p+e, 0.08);
    

    c += circle(p+f, 0.12);
    c += circle(p+f, 0.10);
    c += circleFill(p+f+tangle2*0.05, 0.01);
    c += circleFill(p+f+tangle2*0.10, 0.01);
    c += circle(p+f-tangle2*0.03, 0.01);
    c += circleFill(p+f+vec2(0.085), 0.005);
    c += circleFill(p+f, 0.005);

    
    vec2 g = tangle4 * 0.16;
    c += circle(p+h, 0.05);
    c += circle(p+h, 0.1);
    c += circle(p+h, 0.17);
    c += circle(p+h, 0.2);
    c += circleFill(p+h+tangle41 *0.16, 0.01);
    c += circleFill(p+h+tangle42 *0.16, 0.01);
    c += circleFill(p+h+tangle43 *0.16, 0.01);
    c += circleFill(p+h+tangle44 *0.16, 0.01);
    c += circleFill(p+h+tangle45 *0.16, 0.01);
    c += circleFill(p+h+angle1 *0.06, 0.02);
    c += circleFill(p+h+tangle43*-0.16, 0.01);
    
    
    c += line(p, vec2(0.0), a);
    c += circleFill(p+b, 0.005);
    c += circleFill(p+d, 0.005);
    c += circleFill(p+e, 0.005);

    c += line(p, b, a);
    c += line(p, d, e);
    c += line(p, b+tangle1*0.15, e);
    c += line(p, e, f+vec2(0.085));

    c += line(p, h+angle1*0.06, f);
    c += line(p, h+tangle43*-0.16, d);
    c += line(p, h+tangle42*0.16, e);
    
    
    // of course I'd write a line function that
    // doesn't handle perfectly vertical lines
    c += line(p, vec2(0.001, -0.5), vec2(0.0001, 0.5));
    c += circleFill(p+vec2(0.001, -0.5), 0.005);
    c += circleFill(p+vec2(0.001, 0.5), 0.005);
    
    c = clamp(c, 0.0, 1.0);
    colour = white * c;
    

    fragColor = vec4(colour, 1.0);
}


vec4 getProceduralColor() {
    vec4 result;
    vec2 position = _position.xz;
    position += 0.5;
    
    mainImage(result, position * iWorldScale.xz);
    return result;
}