//
//  Generator.cpp
//  interface/src/starfield
//
//  Created by Chris Barnard on 10/13/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QElapsedTimer>

#include "starfield/Generator.h"

using namespace starfield;

const float Generator::STAR_COLORIZATION = 0.1f;
const float PI_OVER_180 = 3.14159265358979f / 180.0f;

void Generator::computeStarPositions(InputVertices& destination, unsigned limit, unsigned seed) {
    InputVertices* vertices = & destination;
    //_limit = limit;
    
    QElapsedTimer startTime;
    startTime.start();
    
    srand(seed);
    
    vertices->clear();
    vertices->reserve(limit);
    
    const unsigned MILKY_WAY_WIDTH = 12.0; // width in degrees of one half of the Milky Way
    const float MILKY_WAY_INCLINATION = 0.0f; // angle of Milky Way from horizontal in degrees
    const float MILKY_WAY_RATIO = 0.4f;
    const unsigned NUM_DEGREES = 360;
    
    for(int star = 0; star < floor(limit * (1 - MILKY_WAY_RATIO)); ++star) {
        float azimuth, altitude;
        azimuth = (((float)rand() / (float) RAND_MAX) * NUM_DEGREES) - (NUM_DEGREES / 2);
        altitude = (acos((2.0f * ((float)rand() / (float)RAND_MAX)) - 1.0f) / PI_OVER_180) + 90;
        vertices->push_back(InputVertex(azimuth, altitude, computeStarColor(STAR_COLORIZATION)));
    }
    
    for(int star = 0; star < ceil(limit * MILKY_WAY_RATIO); ++star) {
        float azimuth = ((float)rand() / (float) RAND_MAX) * NUM_DEGREES;
        float altitude = powf(randFloat()*0.5f, 2.0f)/0.25f * MILKY_WAY_WIDTH;
        if (randFloat() > 0.5f) {
            altitude *= -1.0f;
        }
        
        // we need to rotate the Milky Way band to the correct orientation in the sky
        // convert from spherical coordinates to cartesian, rotate the point and then convert back.
        // An improvement would be to convert all stars to cartesian at this point and not have to convert back.
        
        float tempX = sin(azimuth * PI_OVER_180) * cos(altitude * PI_OVER_180);
        float tempY = sin(altitude * PI_OVER_180);
        float tempZ = -cos(azimuth * PI_OVER_180) * cos(altitude * PI_OVER_180);
        
        float xangle = MILKY_WAY_INCLINATION * PI_OVER_180;
        float newX = (tempX * cos(xangle)) - (tempY * sin(xangle));
        float newY = (tempX * sin(xangle)) + (tempY * cos(xangle));
        float newZ = tempZ;
        
        azimuth = (atan2(newX,-newZ) + Radians::pi()) / PI_OVER_180;
        altitude = atan2(-newY, hypotf(newX, newZ)) / PI_OVER_180;
        
        vertices->push_back(InputVertex(azimuth, altitude, computeStarColor(STAR_COLORIZATION)));
    }
    
    double timeDiff = (double)startTime.nsecsElapsed() / 1000000.0; // ns to ms
    qDebug() << "Total time to generate stars: " << timeDiff << " msec";
}

// computeStarColor
// - Generate a star color.
//
// colorization can be a value between 0 and 1 specifying how colorful the resulting star color is.
//
// 0 = completely black & white
// 1 = very colorful
unsigned Generator::computeStarColor(float colorization) {
    unsigned char red, green, blue;
    if (randFloat() < 0.3f) {
        // A few stars are colorful
        red = 2 + (rand() % 254);
        green = 2 + round((red * (1 - colorization)) + ((rand() % 254) * colorization));
        blue = 2 + round((red * (1 - colorization)) + ((rand() % 254) * colorization));
    } else {
        // Most stars are dimmer and white
        red = green = blue = 2 + (rand() % 128);
    }
    return red | (green << 8) | (blue << 16);
}
