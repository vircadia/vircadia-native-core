//
//  Controller.cpp
//  interface/src/starfield
//
//  Created by Chris Barnard on 10/16/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QElapsedTimer>

#include "starfield/Controller.h"

using namespace starfield;

bool Controller::computeStars(unsigned numStars, unsigned seed) {
    QElapsedTimer startTime;
    startTime.start();
    
    Generator::computeStarPositions(_inputSequence, numStars, seed);
    
    this->retile(numStars, _tileResolution);
    
    double NSEC_TO_MSEC = 1.0 / 1000000.0;
    double timeDiff = (double)startTime.nsecsElapsed() * NSEC_TO_MSEC;
    qDebug() << "Total time to retile and generate stars: " << timeDiff << "msec";
    
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
