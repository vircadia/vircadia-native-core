//
// starfield/Controller.h
// interface
//
// Created by Tobias Schwinger on 3/29/13.
// Modified by Chris Barnard 10/16/13.
// Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__starfield__Controller__
#define __interface__starfield__Controller__

#include <time.h>

#include "starfield/Generator.h"
#include "starfield/data/InputVertex.h"
#include "starfield/renderer/Renderer.h"
#include "starfield/renderer/VertexOrder.h"

namespace starfield {
    class Controller {
    public:
        Controller() : _tileResolution(20), _renderer(0l) { }
        
        ~Controller() { delete _renderer; }
        
        bool computeStars(unsigned numStars, unsigned seed);
        bool setResolution(unsigned tileResolution);
        void render(float perspective, float angle, mat4 const& orientation, float alpha);
    private:
        void retile(unsigned numStars, unsigned tileResolution);

        void recreateRenderer(unsigned numStars, unsigned tileResolution);

        InputVertices _inputSequence;
        unsigned _tileResolution;
        unsigned _numStars;
        Renderer* _renderer;
    };
}
#endif
