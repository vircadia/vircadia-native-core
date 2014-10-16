//
//  Hair.h
//  interface/src
//
//  Created by Philip on June 26, 2014
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Hair_h
#define hifi_Hair_h

#include <iostream>

#include <glm/glm.hpp>
#include <SharedUtil.h>

#include "GeometryUtil.h"
#include "InterfaceConfig.h"
#include "Util.h"

const int HAIR_CONSTRAINTS = 2; 

const int DEFAULT_HAIR_STRANDS = 20;
const int DEFAULT_HAIR_LINKS = 10;
const float DEFAULT_HAIR_RADIUS = 0.15f;
const float DEFAULT_HAIR_LINK_LENGTH = 0.04f;
const float DEFAULT_HAIR_THICKNESS = 0.025f;

class Hair {
public:
    Hair(int strands = DEFAULT_HAIR_STRANDS,
         int links = DEFAULT_HAIR_LINKS,
         float radius = DEFAULT_HAIR_RADIUS,
         float linkLength = DEFAULT_HAIR_LINK_LENGTH,
         float hairThickness = DEFAULT_HAIR_THICKNESS);
    void simulate(float deltaTime);
    void render();
    void setAcceleration(const glm::vec3& acceleration) { _acceleration = acceleration; }
    void setAngularVelocity(const glm::vec3& angularVelocity) { _angularVelocity = angularVelocity; }
    void setAngularAcceleration(const glm::vec3& angularAcceleration) { _angularAcceleration = angularAcceleration; }
    void setGravity(const glm::vec3& gravity) { _gravity = gravity; }
    void setLoudness(const float loudness) { _loudness = loudness; }
    
private:
    int _strands;
    int _links;
    float _linkLength;
    float _hairThickness;
    float _radius;
    glm::vec3* _hairPosition;
    glm::vec3* _hairOriginalPosition;
    glm::vec3* _hairLastPosition;
    glm::vec3* _hairQuadDelta;
    glm::vec3* _hairNormals;
    glm::vec3* _hairColors;
    int* _hairIsMoveable;
    int* _hairConstraints;     
    glm::vec3 _acceleration;
    glm::vec3 _angularVelocity;
    glm::vec3 _angularAcceleration;
    glm::vec3 _gravity;
    float _loudness;
    
   };

#endif // hifi_Hair_h
