//
// starfield/Generator.h
// interface
//
// Created by Chris Barnard on 10/13/13.
// Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__starfield__Generator__
#define __interface__starfield__Generator__

#include <locale.h>
#include <time.h>

#include "Config.h"
#include "SharedUtil.h"

#include "starfield/data/InputVertex.h"

namespace starfield {
    
    class Generator {
        
    public:
        Generator() {}
        ~Generator() {}

        static void computeStarPositions(InputVertices& destination, unsigned limit, unsigned seed);
        static unsigned computeStarColor(float colorization);
        
    private:
        static const float STAR_COLORIZATION;
        
    };
    
}
#endif