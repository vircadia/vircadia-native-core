//
//  LeapManager.cpp
//  hifi
//
//  Created by Eric Johnston on 6/26/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include "LeapManager.h"
#include "Avatar.h"
#include <Leap.h>
#include <dlfcn.h>     // needed for RTLD_LAZY
#include <sstream>

bool LeapManager::_libraryExists = false;
bool LeapManager::_doFakeFingers = false;
Leap::Controller* LeapManager::_controller = NULL;
HifiLeapListener* LeapManager::_listener = NULL;

class HifiLeapListener  : public Leap::Listener {
public:
    HifiLeapListener() {}
    virtual ~HifiLeapListener() {}
    
    Leap::Frame lastFrame;
    virtual void onFrame(const Leap::Controller& controller) {
#ifndef LEAP_STUBS
        lastFrame = controller.frame();
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

void LeapManager::nextFrame(Avatar& avatar) {
    // Apply the frame data directly to the avatar.
    Hand& hand = avatar.getHand();
    
    // If we actually get valid Leap data, this will be set to true;
    bool gotRealData = false;
    // First, deactivate everything.
    for (size_t i = 0; i < hand.getNumPalms(); ++i) {
        PalmData& palm = hand.getPalms()[i];
        palm.setActive(false);
        for (size_t f = 0; f < palm.getNumFingers(); ++f) {
            FingerData& finger = palm.getFingers()[f];
            finger.setActive(false);
        }
    }

    if (controllersExist()) {
        _listener->onFrame(*_controller);
    }

#ifndef LEAP_STUBS
    if (controllersExist()) {
        // Performance note:
        //   This is a first pass.
        //   Once all of this is stable, perfoamance may be improved ysing std::map or another
        //   associative container to match serialized Leap ID's with available fingers.
        //   That will make this code shorter and more efficient.
        
        // First, see which palms and fingers are still valid.
        Leap::Frame& frame = _listener->lastFrame;
        
        // Note that this is O(n^2) at worst, but n is very small.

        size_t numLeapHands = frame.hands().count();
        std::vector<PalmData*> palmAssignment(numLeapHands);
        // Look for matches
        for (size_t index = 0; index < numLeapHands; ++index) {
            palmAssignment[index] = NULL;
            Leap::Hand leapHand = frame.hands()[index];
            int id = leapHand.id();
            if (leapHand.isValid()) {
                for (size_t i = 0; i < hand.getNumPalms(); ++i) {
                    PalmData& palm = hand.getPalms()[i];
                    if (palm.getLeapID() == id) {
                        // Found hand with the same ID. We're set!
                        palmAssignment[index] = &palm;
                        palm.setActive(true);
                    }
                }
            }
        }
        // Fill empty slots
        for (size_t index = 0; index < numLeapHands; ++index) {
            if (palmAssignment[index] == NULL) {
                Leap::Hand leapHand = frame.hands()[index];
                if (leapHand.isValid()) {
                    for (size_t i = 0; i < hand.getNumPalms() && palmAssignment[index] == NULL; ++i) {
                        PalmData& palm = hand.getPalms()[i];
                        if (!palm.isActive()) {
                            // Found a free hand to use.
                            palmAssignment[index] = &palm;
                            palm.setActive(true);
                            palm.setLeapID(leapHand.id());
                        }
                    }
                }
            }
        }
        // Apply the assignments
        for (size_t index = 0; index < numLeapHands; ++index) {
            if (palmAssignment[index]) {
                Leap::Hand leapHand = frame.hands()[index];
                PalmData& palm = *(palmAssignment[index]);
                const Leap::Vector pos = leapHand.palmPosition();
                const Leap::Vector normal = leapHand.palmNormal();
                palm.setRawPosition(glm::vec3(pos.x, pos.y, pos.z));
                palm.setRawNormal(glm::vec3(normal.x, normal.y, normal.z));
            }
        }

        size_t numLeapFingers = frame.fingers().count();
        std::vector<FingerData*> fingerAssignment(numLeapFingers);
        // Look for matches
        for (size_t index = 0; index < numLeapFingers; ++index) {
            fingerAssignment[index] = NULL;
            Leap::Finger leapFinger = frame.fingers()[index];
            int id = leapFinger.id();
            if (leapFinger.isValid()) {
                for (size_t i = 0; i < hand.getNumPalms(); ++i) {
                    PalmData& palm = hand.getPalms()[i];
                    for (size_t f = 0; f < palm.getNumFingers(); ++f) {
                        FingerData& finger = palm.getFingers()[f];
                        if (finger.getLeapID() == id) {
                            // Found finger with the same ID. We're set!
                            fingerAssignment[index] = &finger;
                            finger.setActive(true);
                        }
                    }
                }
            }
        }
        // Fill empty slots
        for (size_t index = 0; index < numLeapFingers; ++index) {
            if (fingerAssignment[index] == NULL) {
                Leap::Finger leapFinger = frame.fingers()[index];
                if (leapFinger.isValid()) {
                    for (size_t i = 0; i < hand.getNumPalms() && fingerAssignment[index] == NULL; ++i) {
                        PalmData& palm = hand.getPalms()[i];
                        for (size_t f = 0; f < palm.getNumFingers() && fingerAssignment[index] == NULL; ++f) {
                            FingerData& finger = palm.getFingers()[f];
                            if (!finger.isActive()) {
                                // Found a free finger to use.
                                fingerAssignment[index] = &finger;
                                finger.setActive(true);
                                finger.setLeapID(leapFinger.id());
                            }
                        }
                    }
                }
            }
        }
        // Apply the assignments
        for (size_t index = 0; index < numLeapFingers; ++index) {
            if (fingerAssignment[index]) {
                Leap::Finger leapFinger = frame.fingers()[index];
                FingerData& finger = *(fingerAssignment[index]);
                const Leap::Vector tip = leapFinger.stabilizedTipPosition();
                const Leap::Vector root = tip - leapFinger.direction() * leapFinger.length();
                finger.setRawTipPosition(glm::vec3(tip.x, tip.y, tip.z));
                finger.setRawRootPosition(glm::vec3(root.x, root.y, root.z));
            }
        }
        gotRealData = true;
    }
#endif
    if (!gotRealData) {
        if (_doFakeFingers) {
            // There's no real Leap data and we need to fake it.
            for (size_t i = 0; i < hand.getNumPalms(); ++i) {
                static const glm::vec3 fakeHandOffsets[] = {
                    glm::vec3( -500.0f, 50.0f, 50.0f),
                    glm::vec3(    0.0f, 50.0f, 50.0f)
                };
                static const glm::vec3 fakeHandFingerMirrors[] = {
                    glm::vec3( -1.0f, 1.0f, 1.0f),
                    glm::vec3(  1.0f, 1.0f, 1.0f)
                };
                static const glm::vec3 fakeFingerPositions[] = {
                    glm::vec3( -60.0f, 0.0f, -40.0f),
                    glm::vec3( -20.0f, 0.0f, -60.0f),
                    glm::vec3(  20.0f, 0.0f, -60.0f),
                    glm::vec3(  60.0f, 0.0f, -40.0f),
                    glm::vec3( -50.0f, 0.0f,  30.0f)
                };

                PalmData& palm = hand.getPalms()[i];
                palm.setActive(true);
                // Simulated data
                
                palm.setRawPosition(glm::vec3( 0.0f, 0.0f, 0.0f) + fakeHandOffsets[i]);
                palm.setRawNormal(glm::vec3(0.0f, 1.0f, 0.0f));

                for (size_t f = 0; f < palm.getNumFingers(); ++f) {
                    FingerData& finger = palm.getFingers()[f];
                    finger.setActive(true);
                    const float tipScale = 1.5f;
                    const float rootScale = 0.75f;
                    glm::vec3 fingerPos = fakeFingerPositions[f] * fakeHandFingerMirrors[i];
                    finger.setRawTipPosition(fingerPos * tipScale + fakeHandOffsets[i]);
                    finger.setRawRootPosition(fingerPos * rootScale + fakeHandOffsets[i]);
                }
            }
        }
        else {
            // Just deactivate everything.
            for (size_t i = 0; i < hand.getNumPalms(); ++i) {
                PalmData& palm = hand.getPalms()[i];
                palm.setActive(false);
                for (size_t f = 0; f < palm.getNumFingers(); ++f) {
                    FingerData& finger = palm.getFingers()[f];
                    finger.setActive(false);
                }
            }
        }
    }
    hand.updateFingerTrails();
}

void LeapManager::enableFakeFingers(bool enable) {
    _doFakeFingers = enable;
}

bool LeapManager::controllersExist() {
#ifdef LEAP_STUBS
    return false;
#else
    return _listener && _controller && _controller->devices().count() > 0;
#endif
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

