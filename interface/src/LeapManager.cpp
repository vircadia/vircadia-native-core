//
//  LeapManager.cpp
//  hifi
//
//  Created by Eric Johnston on 6/26/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include "LeapManager.h"
#include <Leap.h>
#include <dlfcn.h>     // needed for RTLD_LAZY
#include <sstream>

bool LeapManager::_libraryExists = false;
Leap::Controller* LeapManager::_controller = NULL;
HifiLeapListener* LeapManager::_listener = NULL;

class HifiLeapListener  : public Leap::Listener {
public:
    Leap::Frame lastFrame;
    std::vector<glm::vec3> fingerTips;
    std::vector<glm::vec3> fingerRoots;
    std::vector<glm::vec3> handPositions;
    std::vector<glm::vec3> handNormals;
    
    virtual void onFrame(const Leap::Controller& controller) {
#ifndef LEAP_STUBS
        Leap::Frame frame = controller.frame();
        int numFingers = frame.fingers().count();
        fingerTips.resize(numFingers);
        fingerRoots.resize(numFingers);
        for (int i = 0; i < numFingers; ++i) {
            const Leap::Finger& thisFinger = frame.fingers()[i];
            const Leap::Vector pos = thisFinger.stabilizedTipPosition();
            fingerTips[i] = glm::vec3(pos.x, pos.y, pos.z);

            const Leap::Vector root = pos - thisFinger.direction() * thisFinger.length();
            fingerRoots[i] = glm::vec3(root.x, root.y, root.z);
        }

        int numHands = frame.hands().count();
        handPositions.resize(numHands);
        handNormals.resize(numHands);
        for (int i = 0; i < numHands; ++i) {
            const Leap::Hand& thisHand = frame.hands()[i];
            const Leap::Vector pos = thisHand.palmPosition();
            handPositions[i] = glm::vec3(pos.x, pos.y, pos.z);
            
            const Leap::Vector norm = thisHand.palmNormal();
            handNormals[i] = glm::vec3(norm.x, norm.y, norm.z);
        }
        lastFrame = frame;
#endif
    }
    
};

void LeapManager::initialize() {
#ifndef LEAP_STUBS
    if (dlopen("/usr/lib/libLeap.dylib", RTLD_LAZY)) {
        _libraryExists = true;
        _controller = new Leap::Controller();
        _listener = new HifiLeapListener();
    }
#endif
}

void LeapManager::terminate() {
    delete _listener;
    delete _controller;
    _listener = NULL;
    _controller = NULL;
}

void LeapManager::nextFrame() {
    if (_listener && _controller)
        _listener->onFrame(*_controller);
}

const std::vector<glm::vec3>& LeapManager::getFingerTips() {
    if (_listener) {
        return _listener->fingerTips;
    }
    else {
        static std::vector<glm::vec3> empty;
        return empty;
    }
}

const std::vector<glm::vec3>& LeapManager::getFingerRoots() {
    if (_listener) {
        return _listener->fingerRoots;
    }
    else {
        static std::vector<glm::vec3> empty;
        return empty;
    }
}

const std::vector<glm::vec3>& LeapManager::getHandPositions() {
    if (_listener) {
        return _listener->handPositions;
    }
    else {
        static std::vector<glm::vec3> empty;
        return empty;
    }
}

const std::vector<glm::vec3>& LeapManager::getHandNormals() {
    if (_listener) {
        return _listener->handNormals;
    }
    else {
        static std::vector<glm::vec3> empty;
        return empty;
    }
}

std::string LeapManager::statusString() {
    std::stringstream leapString;
#ifndef LEAP_STUBS
    if (!_libraryExists)
        leapString << "Leap library at /usr/lib/libLeap.dylib does not exist.";
    else if (!_controller || !_listener || !_controller->devices().count())
        leapString << "Leap controller is not attached.";
    else {
        leapString << "Leap pointables: " << _listener->lastFrame.pointables().count();
        if (_listener->lastFrame.pointables().count() > 0) {
            Leap::Vector pos = _listener->lastFrame.pointables()[0].tipPosition();
            leapString << " pos: " << pos.x << " " << pos.y << " " << pos.z;
        }
    }
#endif
    return leapString.str();
}

