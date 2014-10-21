//
//  Hair.cpp
//  interface/src
//
//  Created by Philip on June 26, 2014
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
//  Creates single flexible verlet-integrated strands that can be used for hair/fur/grass

#include "Hair.h"

#include "Util.h"
#include "world.h"

const float HAIR_DAMPING = 0.99f;
const float CONSTRAINT_RELAXATION = 10.0f;
const float HAIR_ACCELERATION_COUPLING = 0.045f;
const float HAIR_ANGULAR_VELOCITY_COUPLING = 0.020f;
const float HAIR_ANGULAR_ACCELERATION_COUPLING = 0.003f;
const float HAIR_MAX_LINEAR_ACCELERATION = 4.0f;
const float HAIR_STIFFNESS = 0.00f;
const glm::vec3 HAIR_COLOR1(0.98f, 0.76f, 0.075f);
const glm::vec3 HAIR_COLOR2(0.912f, 0.184f, 0.101f);

Hair::Hair(int strands,
           int links,
           float radius,
           float linkLength,
           float hairThickness) :
    _strands(strands),
    _links(links),
    _linkLength(linkLength),
    _hairThickness(hairThickness),
    _radius(radius),
    _acceleration(0.0f),
    _angularVelocity(0.0f),
    _angularAcceleration(0.0f),
    _gravity(0.0f),
    _loudness(0.0f)
{
    _hairPosition = new glm::vec3[_strands * _links];
    _hairOriginalPosition = new glm::vec3[_strands * _links];
    _hairLastPosition = new glm::vec3[_strands * _links];
    _hairQuadDelta = new glm::vec3[_strands * _links];
    _hairNormals = new glm::vec3[_strands * _links];
    _hairColors = new glm::vec3[_strands * _links];
    _hairIsMoveable = new int[_strands * _links];
    _hairConstraints = new int[_strands * _links * HAIR_CONSTRAINTS];     // Hair can link to two others
    glm::vec3 thisVertex;
    for (int strand = 0; strand < _strands; strand++) {
        float strandAngle = randFloat() * PI;
        float azimuth;
        float elevation = PI_OVER_TWO - (randFloat() * 0.10f * PI);
        azimuth = PI_OVER_TWO;
        if (randFloat() < 0.5f) {
            azimuth *= -1.0f;
        }
        glm::vec3 thisStrand(sinf(azimuth) * cosf(elevation), sinf(elevation), -cosf(azimuth) * cosf(elevation));
        thisStrand *= _radius;
        
        for (int link = 0; link < _links; link++) {
            int vertexIndex = strand * _links + link;
            //  Clear constraints
            for (int link2 = 0; link2 < HAIR_CONSTRAINTS; link2++) {
                _hairConstraints[vertexIndex * HAIR_CONSTRAINTS + link2] = -1;
            }
            if (vertexIndex % _links == 0) {
                //  start of strand
                thisVertex = thisStrand;
            } else {
                thisVertex+= glm::normalize(thisStrand) * _linkLength;
                //  Set constraints to vertex before and maybe vertex after in strand
                _hairConstraints[vertexIndex * HAIR_CONSTRAINTS] = vertexIndex - 1;
                if (link < (_links - 1)) {
                    _hairConstraints[vertexIndex * HAIR_CONSTRAINTS + 1] = vertexIndex + 1;
                }
            }
            _hairOriginalPosition[vertexIndex] = _hairLastPosition[vertexIndex] = _hairPosition[vertexIndex] = thisVertex;
            
            _hairQuadDelta[vertexIndex] = glm::vec3(cos(strandAngle) * _hairThickness, 0.f, sin(strandAngle) * _hairThickness);
            _hairQuadDelta[vertexIndex] *= ((float)link / _links);
            _hairNormals[vertexIndex] = glm::normalize(randVector());
            if (randFloat() < elevation / PI_OVER_TWO) {
                _hairColors[vertexIndex] = HAIR_COLOR1 * ((float)(link + 1) / (float)_links);
            } else {
                _hairColors[vertexIndex] = HAIR_COLOR2 * ((float)(link + 1) / (float)_links);
            }
        }
    }
}

const float SOUND_THRESHOLD = 40.0f;

void Hair::simulate(float deltaTime) {
    deltaTime = glm::clamp(deltaTime, 0.0f, 1.0f / 30.0f);
    glm::vec3 acceleration = _acceleration;
    if (glm::length(acceleration) > HAIR_MAX_LINEAR_ACCELERATION) {
        acceleration = glm::normalize(acceleration) * HAIR_MAX_LINEAR_ACCELERATION;
    }
    
    for (int strand = 0; strand < _strands; strand++) {
        for (int link = 0; link < _links; link++) {
            int vertexIndex = strand * _links + link;
            if (vertexIndex % _links == 0) {
                //  Base Joint - no integration
            } else {
                //
                //  Vertlet Integration
                //
                //  Add velocity from last position, with damping
                glm::vec3 thisPosition = _hairPosition[vertexIndex];
                glm::vec3 diff = thisPosition - _hairLastPosition[vertexIndex];
                _hairPosition[vertexIndex] += diff * HAIR_DAMPING;
                
                //  Resolve collisions with sphere
                if (glm::length(_hairPosition[vertexIndex]) < _radius) {
                    _hairPosition[vertexIndex] += glm::normalize(_hairPosition[vertexIndex]) *
                    (_radius - glm::length(_hairPosition[vertexIndex]));
                }
                //  Add random thing driven by loudness
                float loudnessFactor = (_loudness > SOUND_THRESHOLD) ? logf(_loudness - SOUND_THRESHOLD) / 2000.0f : 0.0f;

                const float QUIESCENT_LOUDNESS = 0.0f;
                _hairPosition[vertexIndex] += randVector() * (QUIESCENT_LOUDNESS + loudnessFactor) * ((float)link / (float)_links);

                //  Add gravity
                const float SCALE_GRAVITY = 0.13f;
                _hairPosition[vertexIndex] += _gravity * deltaTime * SCALE_GRAVITY;
                
                //  Add linear acceleration
                _hairPosition[vertexIndex] -= acceleration * HAIR_ACCELERATION_COUPLING * deltaTime;
                
                //  Add stiffness to return to original position
                _hairPosition[vertexIndex] += (_hairOriginalPosition[vertexIndex] - _hairPosition[vertexIndex])
                * powf(1.f - (float)link / _links, 2.f) * HAIR_STIFFNESS;
                
                //  Add angular acceleration
                const float ANGULAR_VELOCITY_MIN = 0.001f;
                if (glm::length(_angularVelocity) > ANGULAR_VELOCITY_MIN) {
                    glm::vec3 yawVector = _hairPosition[vertexIndex];
                    glm::vec3 angularVelocity = _angularVelocity * HAIR_ANGULAR_VELOCITY_COUPLING;
                    glm::vec3 angularAcceleration = _angularAcceleration * HAIR_ANGULAR_ACCELERATION_COUPLING;
                    yawVector.y = 0.f;
                    if (glm::length(yawVector) > EPSILON) {
                        float radius = glm::length(yawVector);
                        yawVector = glm::normalize(yawVector);
                        float angle = atan2f(yawVector.x, -yawVector.z) + PI;
                        glm::vec3 delta = glm::vec3(-1.f, 0.f, 0.f) * glm::angleAxis(angle, glm::vec3(0, 1, 0));
                        _hairPosition[vertexIndex] -= delta * radius * (angularVelocity.y - angularAcceleration.y) * deltaTime;
                    }
                    glm::vec3 pitchVector = _hairPosition[vertexIndex];
                    pitchVector.x = 0.f;
                    if (glm::length(pitchVector) > EPSILON) {
                        float radius = glm::length(pitchVector);
                        pitchVector = glm::normalize(pitchVector);
                        float angle = atan2f(pitchVector.y, -pitchVector.z) + PI;
                        glm::vec3 delta = glm::vec3(0.0f, 1.0f, 0.f) * glm::angleAxis(angle, glm::vec3(1, 0, 0));
                        _hairPosition[vertexIndex] -= delta * radius * (angularVelocity.x - angularAcceleration.x) * deltaTime;
                    }
                    glm::vec3 rollVector = _hairPosition[vertexIndex];
                    rollVector.z = 0.f;
                    if (glm::length(rollVector) > EPSILON) {
                        float radius = glm::length(rollVector);
                        pitchVector = glm::normalize(rollVector);
                        float angle = atan2f(rollVector.x, rollVector.y) + PI;
                        glm::vec3 delta = glm::vec3(-1.0f, 0.0f, 0.f) * glm::angleAxis(angle, glm::vec3(0, 0, 1));
                        _hairPosition[vertexIndex] -= delta * radius * (angularVelocity.z - angularAcceleration.z) * deltaTime;
                    }
                }
                
                //  Impose link constraints
                for (int link = 0; link < HAIR_CONSTRAINTS; link++) {
                    if (_hairConstraints[vertexIndex * HAIR_CONSTRAINTS + link] > -1) {
                        //  If there is a constraint, try to enforce it
                        glm::vec3 vectorBetween = _hairPosition[_hairConstraints[vertexIndex * HAIR_CONSTRAINTS + link]] - _hairPosition[vertexIndex];
                        _hairPosition[vertexIndex] += glm::normalize(vectorBetween) * (glm::length(vectorBetween) - _linkLength) * CONSTRAINT_RELAXATION * deltaTime;
                    }
                }
                
                //  Store start position for next verlet pass
                _hairLastPosition[vertexIndex] = thisPosition;
            }
        }
    }
}

void Hair::render() {
    //
    //  Before calling this function, translate/rotate to the origin of the owning object
    //
    float loudnessFactor = (_loudness > SOUND_THRESHOLD) ? logf(_loudness - SOUND_THRESHOLD) / 16.0f : 0.0f;
    const int SPARKLE_EVERY = 5;
    const float HAIR_SETBACK = 0.0f;
    int sparkleIndex = (int) (randFloat() * SPARKLE_EVERY);
    glPushMatrix();
    glTranslatef(0.f, 0.f, HAIR_SETBACK);
    glBegin(GL_QUADS);
    for (int strand = 0; strand < _strands; strand++) {
        for (int link = 0; link < _links - 1; link++) {
            int vertexIndex = strand * _links + link;
            glm::vec3 thisColor = _hairColors[vertexIndex];
            if (sparkleIndex % SPARKLE_EVERY == 0) {
                thisColor.x += (1.f - thisColor.x) * loudnessFactor;
                thisColor.y += (1.f - thisColor.y) * loudnessFactor;
                thisColor.z += (1.f - thisColor.z) * loudnessFactor;
            }
            glColor3fv(&thisColor.x);
            glNormal3fv(&_hairNormals[vertexIndex].x);
            glVertex3f(_hairPosition[vertexIndex].x - _hairQuadDelta[vertexIndex].x,
                       _hairPosition[vertexIndex].y - _hairQuadDelta[vertexIndex].y,
                       _hairPosition[vertexIndex].z - _hairQuadDelta[vertexIndex].z);
            glVertex3f(_hairPosition[vertexIndex].x + _hairQuadDelta[vertexIndex].x,
                       _hairPosition[vertexIndex].y + _hairQuadDelta[vertexIndex].y,
                       _hairPosition[vertexIndex].z + _hairQuadDelta[vertexIndex].z);
            
            glVertex3f(_hairPosition[vertexIndex + 1].x + _hairQuadDelta[vertexIndex].x,
                       _hairPosition[vertexIndex + 1].y + _hairQuadDelta[vertexIndex].y,
                       _hairPosition[vertexIndex + 1].z + _hairQuadDelta[vertexIndex].z);
            glVertex3f(_hairPosition[vertexIndex + 1].x - _hairQuadDelta[vertexIndex].x,
                       _hairPosition[vertexIndex + 1].y - _hairQuadDelta[vertexIndex].y,
                       _hairPosition[vertexIndex + 1].z - _hairQuadDelta[vertexIndex].z);
            sparkleIndex++;
        }
    }
    glEnd();
    glPopMatrix();
}



