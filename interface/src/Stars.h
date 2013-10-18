//
// Stars.h
// interface
//
// Created by Tobias Schwinger on 3/22/13.
// Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__Stars__
#define __interface__Stars__

#include <glm/glm.hpp>

namespace starfield { class Controller; }

// 
// Starfield rendering component.
// 
class Stars  {
    public:
        Stars();
        ~Stars();

        //
        // Generate stars from random number seed
        //
        // The numStars parameter sets the number of stars to generate.
        //
        bool generate(unsigned numStars, unsigned seed);

        // 
        // Renders the starfield from a local viewer's perspective.
        // The parameters specifiy the field of view.
        // 
        void render(float fovY, float aspect, float nearZ, float alpha);

        // 
        // Sets the resolution for FOV culling.
        //
        // The parameter determines the number of tiles in azimuthal 
        // and altitudinal directions. 
        //
        // GPU resources are updated upon change in which case 'true'
        // is returned.
        // 
        bool setResolution(unsigned k);
    
    
        //
        // Returns true when stars have been loaded
        //
        bool isStarsLoaded() const { return _starsLoaded; };
    private:
        // don't copy/assign
        Stars(Stars const&); // = delete;
        Stars& operator=(Stars const&); // delete;

        // variables

        starfield::Controller* _controller;
        
        bool _starsLoaded;
};


#endif

