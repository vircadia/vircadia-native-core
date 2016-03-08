//
//  Created by Bradley Austin Davis on 2015/06/08
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#include <stdint.h>

#include <GLMHelpers.h>

namespace Cursor {
    enum class Source {
        MOUSE,
        UNKNOWN,
    };

    enum Icon {
        DEFAULT,
        LINK,
        GRAB,

        // Add new system cursors here

        // User cursors will have ids over this value
        USER_BASE = 0xFF,
    };

    class Instance {
    public:
        virtual Source getType() const = 0;
        virtual void setIcon(uint16_t icon);
        virtual uint16_t getIcon() const;
    private:
        uint16_t _icon;
    };

    class Manager {
        Manager();
        Manager(const Manager& other) = delete;
    public:
        static Manager& instance();
        uint8_t getCount();
        float getScale();
        void setScale(float scale);
        Instance* getCursor(uint8_t index = 0);
        uint16_t registerIcon(const QString& path);
        QList<uint16_t> registeredIcons() const;
        const QString& getIconImage(uint16_t icon);
    private:
        float _scale{ 1.0f };
    };
}


