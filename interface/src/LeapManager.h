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

class Avatar;
class HifiLeapListener;
namespace Leap {
    class Controller;
}

class LeapManager {
public:
    static void nextFrame(Avatar& avatar);      // called once per frame to get new Leap data
    static bool controllersExist();             // Returns true if there's at least one active Leap plugged in
    static void enableFakeFingers(bool enable); // put fake data in if there's no Leap plugged in
    static const std::vector<glm::vec3>& getFingerTips();
    static const std::vector<glm::vec3>& getFingerRoots();
    static const std::vector<glm::vec3>& getHandPositions();
    static const std::vector<glm::vec3>& getHandNormals();
    static std::string statusString();
    static void initialize();
    static void terminate();
    
private:
    static bool _libraryExists;            // The library is present, so we won't crash if we call it.
    static bool _doFakeFingers;
    static Leap::Controller* _controller;
    static HifiLeapListener* _listener;
};

#endif /* defined(__hifi__LeapManager__) */
