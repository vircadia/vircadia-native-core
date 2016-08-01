//
//  Created by Bradley Austin Davis on 2016/07/11
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

precision highp float;

struct TransformCamera {
    mat4 _view;
    mat4 _viewInverse;
    mat4 _projectionViewUntranslated;
    mat4 _projection;
    mat4 _projectionInverse;
    vec4 _viewport;
    vec4 _stereoInfo;
};

layout(std140) uniform transformCameraBuffer {
    TransformCamera _camera;
};

TransformCamera getTransformCamera() {
    return _camera;
}

vec3 getEyeWorldPos() {
    return _camera._viewInverse[3].xyz;
}


bool cam_isStereo() {
    return _camera._stereoInfo.x > 0.0;
}

float cam_getStereoSide() {
    return _camera._stereoInfo.y;
}


struct Reprojection {
    mat4 rotation;
};

layout(std140) uniform reprojectionBuffer {
    Reprojection reprojection;
};

layout(location = 0) in vec4 inPosition;

noperspective out vec2 varTexCoord0;


void main(void) {
    // standard transform
    TransformCamera cam = getTransformCamera();
    vec2 uv = inPosition.xy;
    uv.x /= 2.0;
    vec4 pos = inPosition;
    pos *= 2.0;
    pos -= 1.0;
    if (cam_getStereoSide() > 0.0) {
        uv.x += 0.5;
    }
    if (reprojection.rotation != mat4(1)) {
        vec4 eyeSpace = _camera._projectionInverse * pos; 
        eyeSpace /= eyeSpace.w;

        // Convert to a noramlized ray 
        vec3 ray = eyeSpace.xyz;
        ray = normalize(ray);
        
        // Adjust the ray by the rotation
        ray = mat3(inverse(reprojection.rotation)) * ray;

        // Project back on to the texture plane
        ray *= eyeSpace.z / ray.z;
        eyeSpace.xyz = ray;
        
        // Move back into NDC space
        eyeSpace = _camera._projection * eyeSpace;
        eyeSpace /= eyeSpace.w;
        eyeSpace.z = 0.0;
        pos = eyeSpace;
    }
    gl_Position = pos;
    varTexCoord0 = uv;
}
