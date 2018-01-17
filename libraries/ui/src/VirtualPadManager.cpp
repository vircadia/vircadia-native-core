//
//  Created by Gabriel Calero & Cristian Duarte on 01/16/2018
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "VirtualPadManager.h"

namespace VirtualPad {

    bool Instance::isBeingTouched() {
        return _isBeingTouched;
    }

    void Instance::setBeingTouched(bool touched) {
        _isBeingTouched = touched;
    }

    void Instance::setFirstTouch(glm::vec2 point) {
        _firstTouch = point;
    }

    glm::vec2 Instance::getFirstTouch() {
        return _firstTouch;
    }

    void Instance::setCurrentTouch(glm::vec2 point) {
        _currentTouch = point;
    }

    glm::vec2 Instance::getCurrentTouch() {
        return _currentTouch;
    }

    Manager::Manager() {

    }

    Manager& Manager::instance() {
        static QSharedPointer<Manager> instance = DependencyManager::get<VirtualPad::Manager>();
        return *instance;
    }

    void Manager::enable(bool enable) {
        _enabled = enable;
    }

    bool Manager::isEnabled() {
        return _enabled;
    }

    Instance* Manager::getLeftVirtualPad() {
        return &_leftVPadInstance;
    }

}
