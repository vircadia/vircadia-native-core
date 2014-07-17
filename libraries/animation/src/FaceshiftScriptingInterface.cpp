//
//  FaceshiftScriptingInterface.cpp
//  interface/src/scripting
//
//  Created by Ben Arnold on 7/38/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "FaceshiftScriptingInterface.h"

FaceshiftScriptingInterface* FaceshiftScriptingInterface::getInstance() {
    static FaceshiftScriptingInterface sharedInstance;
    return &sharedInstance;
}

bool FaceshiftScriptingInterface::isConnectedOrConnecting() const {
    return Application::getInstance()->getFaceshift()->isConnectedOrConnecting();
}

bool FaceshiftScriptingInterface::isActive() const {
    return Application::getInstance()->getFaceshift()->isActive();
}

const glm::vec3& FaceshiftScriptingInterface::getHeadAngularVelocity() const {
    return Application::getInstance()->getFaceshift()->getHeadAngularVelocity();
}

// these pitch/yaw angles are in degrees
float FaceshiftScriptingInterface::getEyeGazeLeftPitch() const {
    return Application::getInstance()->getFaceshift()->getEyeGazeLeftPitch();
}
float FaceshiftScriptingInterface::getEyeGazeLeftYaw() const {
    return Application::getInstance()->getFaceshift()->getEyeGazeLeftYaw();
}

float FaceshiftScriptingInterface::getEyeGazeRightPitch() const { 
    return Application::getInstance()->getFaceshift()->getEyeGazeRightPitch();
}
float FaceshiftScriptingInterface::getEyeGazeRightYaw() const { 
    return Application::getInstance()->getFaceshift()->getEyeGazeRightYaw();
}

float FaceshiftScriptingInterface::getLeftBlink() const { 
    return Application::getInstance()->getFaceshift()->getLeftBlink();
}

float FaceshiftScriptingInterface::getRightBlink() const {
    return Application::getInstance()->getFaceshift()->getRightBlink();
}

float FaceshiftScriptingInterface::getLeftEyeOpen() const {
    return Application::getInstance()->getFaceshift()->getLeftEyeOpen();
}

float FaceshiftScriptingInterface::getRightEyeOpen() const {
    return Application::getInstance()->getFaceshift()->getRightEyeOpen();
}

float FaceshiftScriptingInterface::getBrowDownLeft() const { 
    return Application::getInstance()->getFaceshift()->getBrowDownLeft();
}

float FaceshiftScriptingInterface::getBrowDownRight() const { 
    return Application::getInstance()->getFaceshift()->getBrowDownRight();
}

float FaceshiftScriptingInterface::getBrowUpCenter() const { 
    return Application::getInstance()->getFaceshift()->getBrowUpCenter();
}

float FaceshiftScriptingInterface::getBrowUpLeft() const {
    return Application::getInstance()->getFaceshift()->getBrowUpLeft();
}

float FaceshiftScriptingInterface::getBrowUpRight() const { 
    return Application::getInstance()->getFaceshift()->getBrowUpRight();
}

float FaceshiftScriptingInterface::getMouthSize() const {
    return Application::getInstance()->getFaceshift()->getMouthSize();
}

float FaceshiftScriptingInterface::getMouthSmileLeft() const { 
    return Application::getInstance()->getFaceshift()->getMouthSmileLeft();
}

float FaceshiftScriptingInterface::getMouthSmileRight() const {
    return Application::getInstance()->getFaceshift()->getMouthSmileRight();
}

void FaceshiftScriptingInterface::update() {
    Application::getInstance()->getFaceshift()->update();
}

void FaceshiftScriptingInterface::reset() {
    Application::getInstance()->getFaceshift()->reset();
}

void FaceshiftScriptingInterface::updateFakeCoefficients(float leftBlink, float rightBlink, float browUp,
                                                         float jawOpen, QVector<float>& coefficients) const {
    Application::getInstance()->getFaceshift()->updateFakeCoefficients(leftBlink, rightBlink, browUp, jawOpen, coefficients);
}