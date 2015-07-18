//
//  Created by Bradley Austin Davis 2015/07/18
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once
#ifndef hifi_gpu_Light_h
#define hifi_gpu_Light_h

#include <GLMHelpers.h>

namespace gpu {

struct Light {
    Light() {}
    Light(const vec3& ambient, const vec3& diffuse, const vec4& position, const vec4& specular = vec4(), int shininess = 0) :
        _ambientColor(ambient), _diffuseColor(diffuse), _specularColor(specular), _position(position), _shininess(shininess) {

    }
    vec3 _ambientColor;
    vec3 _diffuseColor;
    vec4 _position;
    vec4 _specularColor;
    int _shininess{0};
};

}

#endif
