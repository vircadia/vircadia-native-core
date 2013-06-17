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
#include "Log.h"

const float DELTA_TIME = 1.f / 60.f;
const float DECAY_RATE = 0.15f;

Transmitter::Transmitter() :
    _isConnected(false),
    _lastRotationRate(0,0,0),
    _lastAcceleration(0,0,0),
    _estimatedRotation(0,0,0),
    _lastReceivedPacket(NULL)
{
    
}

void Transmitter::checkForLostTransmitter() {
    //  If we are in motion, check for loss of transmitter packets
    if (glm::length(_estimatedRotation) > 0.f) {
        timeval now;
        gettimeofday(&now, NULL);
        const int TIME_TO_ASSUME_LOST_MSECS = 2000;
        int msecsSinceLast = diffclock(_lastReceivedPacket, &now);
        if (msecsSinceLast > TIME_TO_ASSUME_LOST_MSECS) {
            resetLevels();
            printLog("Transmitter signal lost.\n");
        }
    }
}
void Transmitter::resetLevels() {
    _lastRotationRate *= 0.f;
    _estimatedRotation *= 0.f; 
}

void Transmitter::processIncomingData(unsigned char* packetData, int numBytes) {
    const int PACKET_HEADER_SIZE = 1;                   //  Packet's first byte is 'T'
    const int ROTATION_MARKER_SIZE = 1;                 //  'R' = Rotation (clockwise about x,y,z)
    const int ACCELERATION_MARKER_SIZE = 1;             //  'A' = Acceleration (x,y,z)
    if (!_lastReceivedPacket) {
        _lastReceivedPacket = new timeval;
    }
    gettimeofday(_lastReceivedPacket, NULL);
    
    if (numBytes == PACKET_HEADER_SIZE + ROTATION_MARKER_SIZE + ACCELERATION_MARKER_SIZE
            + sizeof(_lastRotationRate) + sizeof(_lastAcceleration)
            + sizeof(_touchState.x) + sizeof(_touchState.y) + sizeof(_touchState.state)) {
        unsigned char* packetDataPosition = &packetData[PACKET_HEADER_SIZE + ROTATION_MARKER_SIZE];
        memcpy(&_lastRotationRate, packetDataPosition, sizeof(_lastRotationRate));
        packetDataPosition += sizeof(_lastRotationRate) + ACCELERATION_MARKER_SIZE;
        memcpy(&_lastAcceleration, packetDataPosition, sizeof(_lastAcceleration));
        packetDataPosition += sizeof(_lastAcceleration);
        memcpy(&_touchState.state, packetDataPosition, sizeof(_touchState.state));
        packetDataPosition += sizeof(_touchState.state);
        memcpy(&_touchState.x, packetDataPosition, sizeof(_touchState.x));
        packetDataPosition += sizeof(_touchState.x);
        memcpy(&_touchState.y, packetDataPosition, sizeof(_touchState.y));
        packetDataPosition += sizeof(_touchState.y);

        //  Update estimated absolute position from rotation rates
        _estimatedRotation += _lastRotationRate * DELTA_TIME;
            
        // Sensor Fusion!  Slowly adjust estimated rotation to be relative to gravity (average acceleration)
        const float GRAVITY_FOLLOW_RATE = 1.f;
        float rollAngle = angleBetween(glm::vec3(_lastAcceleration.x, _lastAcceleration.y, 0.f), glm::vec3(0,-1,0)) *
                          ((_lastAcceleration.x < 0.f) ? -1.f : 1.f);
        float pitchAngle = angleBetween(glm::vec3(0.f, _lastAcceleration.y, _lastAcceleration.z), glm::vec3(0,-1,0)) *
                          ((_lastAcceleration.z < 0.f) ? 1.f : -1.f);

        _estimatedRotation.x = (1.f - GRAVITY_FOLLOW_RATE * DELTA_TIME) * _estimatedRotation.x +
                                GRAVITY_FOLLOW_RATE * DELTA_TIME * pitchAngle;
        _estimatedRotation.z = (1.f - GRAVITY_FOLLOW_RATE * DELTA_TIME) * _estimatedRotation.z +
                                GRAVITY_FOLLOW_RATE * DELTA_TIME * rollAngle;
        
        //  Can't apply gravity fusion to Yaw, so decay estimated yaw to zero,
        //  presuming that the average yaw direction is toward screen
        _estimatedRotation.y *= (1.f - DECAY_RATE * DELTA_TIME);

        if (!_isConnected) {
            printLog("Transmitter Connected.\n");
            _isConnected = true;
            _estimatedRotation *= 0.0;
        }
    } else {
        printLog("Transmitter packet read error, %d bytes.\n", numBytes);
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

