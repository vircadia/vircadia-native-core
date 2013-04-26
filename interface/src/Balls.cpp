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

#include "Balls.h"

Balls::Balls(int numberOfBalls) {
    _numberOfBalls = numberOfBalls;
    _balls = new Ball[_numberOfBalls];
    for (unsigned int i = 0; i < _numberOfBalls; ++i) {
        _balls[i].position = glm::vec3(1.0 + randFloat()*0.5,
                                       0.5 + randFloat()*0.5,
                                       1.0 + randFloat()*0.5);
        _balls[i].radius = 0.02 + randFloat()*0.06;
        for (unsigned int j = 0; j < NUMBER_SPRINGS; ++j) {
            _balls[i].links[j] = rand() % (numberOfBalls + 1);
            if (_balls[i].links[j]-1 == i) { _balls[i].links[j] = 0; }
            _balls[i].springLength[j] = 0.5;
         }
    }
}

const bool RENDER_SPRINGS = true;

void Balls::render() {
    
    //  Render Balls    NOTE:  This needs to become something other that GlutSpheres!
    glColor3f(0.62,0.74,0.91);
    for (unsigned int i = 0; i < _numberOfBalls; ++i) {
        glPushMatrix();
        glTranslatef(_balls[i].position.x, _balls[i].position.y, _balls[i].position.z);
        glutSolidSphere(_balls[i].radius, 15, 15);
        glPopMatrix();
    }
    
    //  Render springs
    if (RENDER_SPRINGS) {
        glColor3f(0.74,0.91,0.62);
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
        
        // Drag: decay velocity
        _balls[i].velocity *= (1.f - CONSTANT_VELOCITY_DAMPING*deltaTime);
        
        // Add noise
        _balls[i].velocity += glm::vec3((randFloat() - 0.5) * NOISE_SCALE,
                                        (randFloat() - 0.5) * NOISE_SCALE,
                                        (randFloat() - 0.5) * NOISE_SCALE);
        
        // Spring Force
        
        
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
        }
         
         
        

    }
}

