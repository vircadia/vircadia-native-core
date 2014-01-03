//
// starfield/Controller.cpp
// interface
//
// Created by Chris Barnard on 10/16/13
// Portions of code based on earlier work by Tobias Schwinger.
// Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include "starfield/Controller.h"

using namespace starfield;

bool Controller::computeStars(unsigned numStars, unsigned seed) {
    timeval startTime;
    gettimeofday(&startTime, NULL);
    
    Generator::computeStarPositions(_inputSequence, numStars, seed);
    
    this->retile(numStars, _tileResolution);
    
    qDebug() << "Total time to generate stars: " << ((usecTimestampNow() - usecTimestamp(&startTime)) / 1000) << " msec\n";
    
    return true;
}

bool Controller::setResolution(unsigned tileResolution) {
    if (tileResolution <= 3) {
        return false;
    }

    if (tileResolution != _tileResolution) {

        this->retile(_numStars, tileResolution);

        return true;
    } else {
        return false;
    }
}

void Controller::render(float perspective, float angle, mat4 const& orientation, float alpha) {
    Renderer* renderer = _renderer;

    if (renderer != 0l) {
        renderer->render(perspective, angle, orientation, alpha);
    }
}

void Controller::retile(unsigned numStars, unsigned tileResolution) {
    Tiling tiling(tileResolution);
    VertexOrder scanner(tiling);
    radix2InplaceSort(_inputSequence.begin(), _inputSequence.end(), scanner);

    recreateRenderer(numStars, tileResolution);

    _tileResolution = tileResolution;
}

void Controller::recreateRenderer(unsigned numStars, unsigned tileResolution) {
    delete _renderer;
    _renderer = new Renderer(_inputSequence, numStars, tileResolution);
}
