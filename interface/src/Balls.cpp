//
//  Balls.cpp
//  hifi
//
//  Created by Philip on 4/25/13.
//
//  A cloud of spring-mass spheres to simulate the avatar body/skin.  Each ball
//  connects to as many as 4 neighbors, and executes motion according to a damped
//  spring, while responding physically to other avatars.   
//

#include <glm/glm.hpp>
#include "Util.h"
#include "sharedUtil.h"
#include "world.h"
#include "InterfaceConfig.h"
#include "Balls.h"

const float INITIAL_AREA = 0.2f;
const float BALL_RADIUS = 0.025f;
const glm::vec3 INITIAL_COLOR(0.62f, 0.74f, 0.91f);

Balls::Balls(int numberOfBalls) {
    _numberOfBalls = numberOfBalls;
    _balls = new Ball[_numberOfBalls];
    for (unsigned int i = 0; i < _numberOfBalls; ++i) {
        _balls[i].position = randVector() * INITIAL_AREA;
        _balls[i].targetPosition = _balls[i].position;
        _balls[i].velocity = glm::vec3(0, 0, 0);
        _balls[i].radius = BALL_RADIUS;
        for (unsigned int j = 0; j < NUMBER_SPRINGS; ++j) {
            _balls[i].links[j] = NULL;
          }
    }
    _color = INITIAL_COLOR;
    _origin = glm::vec3(0, 0, 0);
}

void Balls::moveOrigin(const glm::vec3& newOrigin) {
    glm::vec3 delta = newOrigin - _origin;
    if (glm::length(delta) > EPSILON) {
        _origin = newOrigin;
        for (unsigned int i = 0; i < _numberOfBalls; ++i) {
            _balls[i].targetPosition += delta;
        }
    }
}

const bool RENDER_SPRINGS = false;

void Balls::render() {
    
    //  Render Balls    NOTE:  This needs to become something other that GlutSpheres!
    glColor3fv(&_color.x);
    for (unsigned int i = 0; i < _numberOfBalls; ++i) {
        glPushMatrix();
        glTranslatef(_balls[i].position.x, _balls[i].position.y, _balls[i].position.z);
        glutSolidSphere(_balls[i].radius, 8, 8);
        glPopMatrix();
    }
    
    //  Render springs
    if (RENDER_SPRINGS) {
        glColor3f(0.74, 0.91, 0.62);
        for (unsigned int i = 0; i < _numberOfBalls; ++i) {
            glBegin(GL_LINES);
            for (unsigned int j = 0; j < NUMBER_SPRINGS; ++j) {
                if(_balls[i].links[j] > 0) {
                    glVertex3f(_balls[i].position.x,
                               _balls[i].position.y,
                               _balls[i].position.z);
                    glVertex3f(_balls[_balls[i].links[j]-1].position.x,
                               _balls[_balls[i].links[j]-1].position.y,
                               _balls[_balls[i].links[j]-1].position.z);
                }
            }
            glEnd();
        }
    }
     
}

const float CONSTANT_VELOCITY_DAMPING = 1.0f;
const float NOISE_SCALE = 0.00;
const float SPRING_FORCE = 1.0;
const float SPRING_DAMPING = 1.0;

void Balls::simulate(float deltaTime) {
    for (unsigned int i = 0; i < _numberOfBalls; ++i) {
        
        // Move particles
        _balls[i].position += _balls[i].velocity * deltaTime;
        _balls[i].targetPosition += _balls[i].velocity * deltaTime;
        
        // Drag: decay velocity
        _balls[i].velocity *= (1.f - CONSTANT_VELOCITY_DAMPING * deltaTime);
        
        // Add noise
        _balls[i].velocity += randVector() * NOISE_SCALE;
        
        // Approach target position
        for (unsigned int i = 0; i < _numberOfBalls; ++i) {
            _balls[i].position += randFloat() * deltaTime * (_balls[i].targetPosition - _balls[i].position);
        }

        // Spring Force
        
        /*
        for (unsigned int j = 0; j < NUMBER_SPRINGS; ++j) {
            if(_balls[i].links[j] > 0) {
                float separation = glm::distance(_balls[i].position,
                                                _balls[_balls[i].links[j]-1].position);
                _balls[i].velocity += glm::normalize(_balls[i].position -
                                                   _balls[_balls[i].links[j]-1].position) *
                                        deltaTime *
                                        SPRING_FORCE *
                                        (_balls[i].springLength[j] - separation);
                
                //_balls[i].velocity *= (1.f - SPRING_DAMPING*deltaTime);

            }
        } */
         
         
        

    }
}

