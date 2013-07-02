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

bool LeapManager::_isInitialized = false;
bool LeapManager::_libraryExists = false;
Leap::Controller* LeapManager::_controller = 0;
HifiLeapListener* LeapManager::_listener = 0;

class HifiLeapListener  : public Leap::Listener {
public:
    Leap::Frame lastFrame;
    std::vector<glm::vec3> fingerPositions;
    
    virtual void onFrame(const Leap::Controller& controller) {
#ifndef LEAP_STUBS
        Leap::Frame frame = controller.frame();
        int numFingers = frame.fingers().count();
        fingerPositions.resize(numFingers);
        for (int i = 0; i < numFingers; ++i) {
            const Leap::Finger& thisFinger = frame.fingers()[i];
            const Leap::Vector pos = thisFinger.tipPosition();
            fingerPositions[i] = glm::vec3(pos.x, pos.y, pos.z);
        }
        lastFrame = frame;
#endif
    }
    
};

void LeapManager::initialize() {
    if (!_isInitialized) {
#ifndef LEAP_STUBS
        if (dlopen("/usr/lib/libLeap.dylib", RTLD_LAZY)) {
            _libraryExists = true;
            _controller = new Leap::Controller();
            _listener = new HifiLeapListener();
            _controller->addListener(*_listener);
        }
#endif
        _isInitialized = true;
    }
}

void LeapManager::nextFrame() {
    initialize();
    if (_listener && _controller)
        _listener->onFrame(*_controller);
}

const std::vector<glm::vec3>& LeapManager::getFingerPositions() {
    if (_listener)
        return _listener->fingerPositions;
    else {
        static std::vector<glm::vec3> empty;
        return empty;
    }
}

std::string LeapManager::statusString() {
    std::stringstream leapString;
#ifndef LEAP_STUBS
    if (_isInitialized) {
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
    }
#endif
    return leapString.str();
}

