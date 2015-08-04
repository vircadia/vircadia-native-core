//
//  gputest_shaders.h
//  hifi
//
//  Created by Seiji Emery on 8/3/15.
//
//

#ifndef hifi_gputest_shaders_h
#define hifi_gputest_shaders_h


const std::string & standardVertexShader() {
    static std::string src = R"(
    
    in vec4 inPosition;
    in vec4 inNormal;
    in vec4 inColor;
    in vec4 inTexCoord0;
    in vec4 inTangent;
    in vec4 inSkinClusterIndex;
    in vec4 inSkinClusterWeight;
    in vec4 inTexCoord1;
    
    struct TransformObject {
        mat4 _model;
        mat4 _modelInverse;
    };
    
    struct TransformCamera {
        mat4 _view;
        mat4 _viewInverse;
        mat4 _projectionViewUntranslated;
        mat4 _projection;
        mat4 _projectionInverse;
        vec4 _viewport;
    };
    
    uniform transformObjectBuffer {
        TransformObject _object;
    };
    TransformObject getTransformObject() {
        return _object;
    }
    
    uniform transformCameraBuffer {
        TransformCamera _camera;
    };
    TransformCamera getTransformCamera() {
        return _camera;
    }
    
    
    const int MAX_TEXCOORDS = 2;
    
    uniform mat4 texcoordMatrices[MAX_TEXCOORDS];
    
    out vec4 _position;
    out vec3 _normal;
    out vec3 _color;
    out vec2 _texCoord0;
    
    )";
    return src;
}





const std::string & basicVertexShader () {
    static std::string src = R"(
    // Basic forward-rendered shading w/ a single directional light source.
    
    #version 410
    
    // I/O
    layout (location = 0) in vec3 vertexPosition;
    layout (location = 1) in vec3 vertexNormal;
    
    out vec3 outColor;
    
    // Light info
    uniform vec4 lightPosition;
    uniform vec3 kd;    // diffuse reflectivity
    uniform vec3 ld;    // light source intensity
    
    // Model transforms
    uniform mat4 modelViewMatrix;
    uniform mat3 normalMatrix;
    uniform mat4 projectionMatrix;
    uniform mat4 mvp;
    
    void main (void) {
        vec3 norm = normalize(normalMatrix * vertexNormal);
        vec4 eyePos = modelViewMatrix * vec4(vertexPosition, 0);
        vec3 s = normalize(vec3(lightPosition - eyePos));
        
        outColor = ld * kd * max(dot(s, norm), 0.0);
        
        gl_Position = mvp * vec4(vertexPosition, 1.0);
    }

    )";
    return src;
}

const std::string & basicFragmentShader () {
    static std::string src = R"(
    #version 410
    
    // Just pass interpolated color value along
    in vec3 outColor;
    
    layout (location = 0) out vec4 fragColor;
    
    void main(void) {
        fragColor = vec4(outColor, 1.0);
    }
    
    )";
    return src;
}
#endif
