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
    
    const unsigned NUM_DEGREES = 360;
    
    
    for(int star = 0; star < limit; ++star) {
        float azimuth, altitude;
        azimuth = ((float)rand() / (float) RAND_MAX) * NUM_DEGREES;
        altitude = (((float)rand() / (float) RAND_MAX) * NUM_DEGREES / 2) - NUM_DEGREES / 4;

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