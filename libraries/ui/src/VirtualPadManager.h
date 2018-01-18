//
//  Created by Gabriel Calero & Cristian Duarte on 01/16/2018
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#include <stdint.h>
#include <DependencyManager.h>

#include <GLMHelpers.h>

namespace VirtualPad {
    class Instance {
    public:
        virtual bool isBeingTouched();
        virtual void setBeingTouched(bool touched);
        virtual void setFirstTouch(glm::vec2 point);
        virtual glm::vec2 getFirstTouch();
        virtual void setCurrentTouch(glm::vec2 point);
        virtual glm::vec2 getCurrentTouch();
    private:
        bool _isBeingTouched;
        glm::vec2 _firstTouch;
        glm::vec2 _currentTouch;
    };

    class Manager : public QObject, public Dependency {
        SINGLETON_DEPENDENCY

        Manager();
        Manager(const Manager& other) = delete;
    public:
        static Manager& instance();
        Instance* getLeftVirtualPad();
        bool isEnabled();
        void enable(bool enable);
    private:
        Instance _leftVPadInstance;
        bool _enabled;
    };
}


