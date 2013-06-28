//
//  LeapManager.h
//  hifi
//
//  Created by Eric Johnston on 6/26/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __hifi__LeapManager__
#define __hifi__LeapManager__

#include <vector>
#include <glm/glm.hpp>
#include <string>

class HifiLeapListener;
namespace Leap {
    class Controller;
}

class LeapManager {
public:
    static void nextFrame();  // called once per frame to get new Leap data
    static const std::vector<glm::vec3>& getFingerPositions();
    static std::string statusString();
    
private:
    static void initialize();
    static bool _isInitialized;            // We've looked for the library and hooked it up if it's there.
    static bool _libraryExists;            // The library is present, so we won't crash if we call it.
    static Leap::Controller* _controller;
    static HifiLeapListener* _listener;
    



};

#endif /* defined(__hifi__LeapManager__) */
