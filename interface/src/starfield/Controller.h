//
//  Controller.h
//  interface/src/starfield
//
//  Created by Tobias Schwinger on 3/29/13.
//  Modified by Chris Barnard on 10/16/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Controller_h
#define hifi_Controller_h

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
#endif // hifi_Controller_h
