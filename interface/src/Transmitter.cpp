//
//  Transmitter.cpp
//  hifi
//
//  Created by Philip Rosedale on 5/20/13.
//
//

#include "Transmitter.h"
#include "InterfaceConfig.h"
#include "Util.h"
#include <cstring>
#include <glm/glm.hpp>

const float DELTA_TIME = 1.f / 60.f;
const float DECAY_RATE = 0.15f;

Transmitter::Transmitter() :
    _isConnected(false),
    _lastRotationRate(0,0,0),
    _lastAcceleration(0,0,0),
    _estimatedRotation(0,0,0)
{
    
}

void Transmitter::resetLevels() {
    _lastRotationRate *= 0.f;
    _estimatedRotation *= 0.f; 
}

void Transmitter::processIncomingData(unsigned char* packetData, int numBytes) {
    if (numBytes == 3 + sizeof(_lastRotationRate) +
        sizeof(_lastAcceleration)) {
        memcpy(&_lastRotationRate, packetData + 2, sizeof(_lastRotationRate));
        memcpy(&_lastAcceleration, packetData + 3 + sizeof(_lastAcceleration), sizeof(_lastAcceleration));
       
        //  Update estimated absolute position from rotation rates
        _estimatedRotation += _lastRotationRate * DELTA_TIME;
        
        //  Decay estimated absolute position to slowly return to zero regardless
        _estimatedRotation *= (1.f - DECAY_RATE * DELTA_TIME); 
    
        if (!_isConnected) {
            printf("Transmitter V2 Connected.\n");
            _isConnected = true;
            _estimatedRotation *= 0.0;
        }
    } else {
        printf("Transmitter V2 packet read error.\n");
    }
}

void Transmitter::renderLevels(int width, int height) {
    char val[50];
    const int LEVEL_CORNER_X = 10;
    const int LEVEL_CORNER_Y = 400;
    
    // Draw the numeric degree/sec values from the gyros
    sprintf(val, "Pitch Rate %4.1f", _lastRotationRate.x);
    drawtext(LEVEL_CORNER_X, LEVEL_CORNER_Y, 0.10, 0, 1.0, 1, val, 0, 1, 0);
    sprintf(val, "Yaw Rate   %4.1f", _lastRotationRate.y);
    drawtext(LEVEL_CORNER_X, LEVEL_CORNER_Y + 15, 0.10, 0, 1.0, 1, val, 0, 1, 0);
    sprintf(val, "Roll Rate  %4.1f", _lastRotationRate.z);
    drawtext(LEVEL_CORNER_X, LEVEL_CORNER_Y + 30, 0.10, 0, 1.0, 1, val, 0, 1, 0);
    sprintf(val, "Pitch      %4.3f", _estimatedRotation.x);
    drawtext(LEVEL_CORNER_X, LEVEL_CORNER_Y + 45, 0.10, 0, 1.0, 1, val, 0, 1, 0);
    sprintf(val, "Yaw        %4.3f", _estimatedRotation.y);
    drawtext(LEVEL_CORNER_X, LEVEL_CORNER_Y + 60, 0.10, 0, 1.0, 1, val, 0, 1, 0);
    sprintf(val, "Roll       %4.3f", _estimatedRotation.z);
    drawtext(LEVEL_CORNER_X, LEVEL_CORNER_Y + 75, 0.10, 0, 1.0, 1, val, 0, 1, 0);
    
    //  Draw the levels as horizontal lines
    const int LEVEL_CENTER = 150;
    const float ACCEL_VIEW_SCALING = 1.f;
    glLineWidth(2.0);
    glColor4f(1, 1, 1, 1);
    glBegin(GL_LINES);
    // Gyro rates
    glVertex2f(LEVEL_CORNER_X + LEVEL_CENTER, LEVEL_CORNER_Y - 3);
    glVertex2f(LEVEL_CORNER_X + LEVEL_CENTER + _lastRotationRate.x, LEVEL_CORNER_Y - 3);
    glVertex2f(LEVEL_CORNER_X + LEVEL_CENTER, LEVEL_CORNER_Y + 12);
    glVertex2f(LEVEL_CORNER_X + LEVEL_CENTER + _lastRotationRate.y, LEVEL_CORNER_Y + 12);
    glVertex2f(LEVEL_CORNER_X + LEVEL_CENTER, LEVEL_CORNER_Y + 27);
    glVertex2f(LEVEL_CORNER_X + LEVEL_CENTER + _lastRotationRate.z, LEVEL_CORNER_Y + 27);
    // Acceleration
    glVertex2f(LEVEL_CORNER_X + LEVEL_CENTER, LEVEL_CORNER_Y + 42);
    glVertex2f(LEVEL_CORNER_X + LEVEL_CENTER + (int)(_estimatedRotation.x * ACCEL_VIEW_SCALING),
               LEVEL_CORNER_Y + 42);
    glVertex2f(LEVEL_CORNER_X + LEVEL_CENTER, LEVEL_CORNER_Y + 57);
    glVertex2f(LEVEL_CORNER_X + LEVEL_CENTER + (int)(_estimatedRotation.y * ACCEL_VIEW_SCALING),
               LEVEL_CORNER_Y + 57);
    glVertex2f(LEVEL_CORNER_X + LEVEL_CENTER, LEVEL_CORNER_Y + 72);
    glVertex2f(LEVEL_CORNER_X + LEVEL_CENTER + (int)(_estimatedRotation.z * ACCEL_VIEW_SCALING),
               LEVEL_CORNER_Y + 72);
    
    glEnd();
    //  Draw green vertical centerline
    glColor4f(0, 1, 0, 0.5);
    glBegin(GL_LINES);
    glVertex2f(LEVEL_CORNER_X + LEVEL_CENTER, LEVEL_CORNER_Y - 6);
    glVertex2f(LEVEL_CORNER_X + LEVEL_CENTER, LEVEL_CORNER_Y + 30);
    glEnd();

    
}

