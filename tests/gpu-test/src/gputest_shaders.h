//
//  gputest_shaders.h
//  hifi
//
//  Created by Seiji Emery on 8/3/15.
//
//

#ifndef hifi_gputest_shaders_h
#define hifi_gputest_shaders_h

const std::string & basicVertexShader () {
    static std::string src = R"(
    
//    attribute vec3 position;
//    attribute vec3 normal;
    
    varying vec3 normal;
    
    void main (void) {
//        gl_Position = gl_ModelViewProjectionMatrix * vec4(position.xyz, 1.0);
        gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
        normal = gl_Normal;
    }

    )";
    return src;
}

const std::string & basicFragmentShader () {
    static std::string src = R"(
    #version 400
    
    varying vec3 normal;
    
    void main(void) {
        gl_FragColor.rgb = vec3(0.7, 0.2, 0.5) + gl_FragCoord.xyz * 0.2;
        
        vec3 diffuse = vec3(0.7, 0.2, 0.5);
        vec3 light_normal = vec3(0.5, -0.5, 0.7);
        
        gl_FragColor.rgb = diffuse * dot(light_normal, normal);
    }
    
    )";
    return src;
}
#endif
