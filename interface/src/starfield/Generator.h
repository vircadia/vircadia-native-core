//
//  Generator.h
//  interface/src/starfield
//
//  Created by Chris Barnard on 10/13/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Generator_h
#define hifi_Generator_h

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
#endif // hifi_Generator_h
