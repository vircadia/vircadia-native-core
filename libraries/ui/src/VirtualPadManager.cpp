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

    const float Manager::DPI = 534.0f;
    const float Manager::BASE_DIAMETER_PIXELS = 512.0f;
    const float Manager::BASE_MARGIN_PIXELS = 59.0f;
    const float Manager::STICK_RADIUS_PIXELS = 105.0f;
    const float Manager::JUMP_BTN_TRIMMED_RADIUS_PIXELS = 67.0f;
    const float Manager::JUMP_BTN_FULL_PIXELS = 164.0f;
    const float Manager::JUMP_BTN_BOTTOM_MARGIN_PIXELS = 80.0f;
    const float Manager::JUMP_BTN_RIGHT_MARGIN_PIXELS = 13.0f;

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

    void Manager::hide(bool hidden) {
        _hidden = hidden;
    }

    bool Manager::isHidden() {
        return _hidden;
    }

    int Manager::extraBottomMargin() {
        return _extraBottomMargin;
    }

    void Manager::setExtraBottomMargin(int margin) {
        _extraBottomMargin = margin;
    }

    glm::vec2 Manager::getJumpButtonPosition() {
        return _jumpButtonPosition;
    }

    void Manager::setJumpButtonPosition(glm::vec2 point) {
        _jumpButtonPosition = point;
    }

    Instance* Manager::getLeftVirtualPad() {
        return &_leftVPadInstance;
    }

    bool Instance::isShown() {
        return _shown;
    }

    void Instance::setShown(bool show) {
        _shown = show;
    }

}
