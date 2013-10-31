//
// starfield/Generator.cpp
// interface
//
// Created by Chris Barnard on 10/13/13.
// Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include "starfield/Generator.h"

using namespace starfield;

const float Generator::STAR_COLORIZATION = 0.1;

void Generator::computeStarPositions(InputVertices& destination, unsigned limit, unsigned seed) {
    InputVertices* vertices = & destination;
    //_limit = limit;
    
    timeval startTime;
    gettimeofday(&startTime, NULL);
    
    srand(seed);
    
    vertices->clear();
    vertices->reserve(limit);
    
    const unsigned MILKY_WAY_WIDTH = 16.0; // width in degrees of one half of the Milky Way
    const float MILKY_WAY_INCLINATION = 30.0f; // angle of Milky Way from horizontal in degrees
    const float MILKY_WAY_RATIO = 0.6;
    const unsigned NUM_DEGREES = 360;
    
    for(int star = 0; star < floor(limit * (1 - MILKY_WAY_RATIO)); ++star) {
        float azimuth, altitude;
        azimuth = (((float)rand() / (float) RAND_MAX) * NUM_DEGREES) - (NUM_DEGREES / 2);
        altitude = (acos((2.0f * ((float)rand() / (float)RAND_MAX)) - 1.0f) / PI_OVER_180) + 90;
        
        vertices->push_back(InputVertex(azimuth, altitude, computeStarColor(STAR_COLORIZATION)));
    }
    
    for(int star = 0; star < ceil(limit * MILKY_WAY_RATIO); ++star) {
        float azimuth = ((float)rand() / (float) RAND_MAX) * NUM_DEGREES;
        float altitude = asin((float)rand() / (float) RAND_MAX * 2 - 1) * MILKY_WAY_WIDTH;
        
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
    
    qDebug("Took %llu msec to generate stars.\n", (usecTimestampNow() - usecTimestamp(&startTime)) / 1000);
}

// computeStarColor
// - Generate a star color.
//
// colorization can be a value between 0 and 1 specifying how colorful the resulting star color is.
//
// 0 = completely black & white
// 1 = very colorful
unsigned Generator::computeStarColor(float colorization) {
    unsigned char red = rand() % 256;
    unsigned char green = round((red * (1 - colorization)) + ((rand() % 256) * colorization));
    unsigned char blue = round((red * (1 - colorization)) + ((rand() % 256) * colorization));
    return red | green << 8 | blue << 16;
}