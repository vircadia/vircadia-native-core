//
//  Stars.h
//  interface/src
//
//  Created by Tobias Schwinger on 3/22/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Stars_h
#define hifi_Stars_h

#include <glm/glm.hpp>

namespace starfield { class Controller; }

// Starfield rendering component. 
class Stars  {
public:
    Stars();
    ~Stars();

    // Generate stars from random number
    // The numStars parameter sets the number of stars to generate.
    bool generate(unsigned numStars, unsigned seed);

    // Renders the starfield from a local viewer's perspective.
    // The parameters specifiy the field of view.
    void render(float fovY, float aspect, float nearZ, float alpha);

    // Sets the resolution for FOV culling.
    //
    // The parameter determines the number of tiles in azimuthal 
    // and altitudinal directions. 
    //
    // GPU resources are updated upon change in which case 'true'
    // is returned.
    bool setResolution(unsigned k);

    // Returns true when stars have been loaded
    bool isStarsLoaded() const { return _starsLoaded; };
private:
    // don't copy/assign
    Stars(Stars const&); // = delete;
    Stars& operator=(Stars const&); // delete;

    starfield::Controller* _controller;
    
    bool _starsLoaded;
};


#endif // hifi_Stars_h
