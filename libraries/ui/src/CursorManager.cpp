//
//  Created by Bradley Austin Davis on 2015/06/08
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "CursorManager.h"

#include <QCursor>
#include <QWidget>
#include <QUrl>

#include <PathUtils.h>

namespace Cursor {

    void Instance::setIcon(uint16_t icon) {
        _icon = icon;
    }

    uint16_t Instance::getIcon() const {
        return _icon;
    }


    class MouseInstance : public Instance {
        Source getType() const override {
            return Source::MOUSE;
        }
    };

    static QMap<uint16_t, QString> ICONS;
    static uint16_t _customIconId = Icon::USER_BASE;

    Manager::Manager() {
        ICONS[Icon::DEFAULT] = PathUtils::resourcesPath() + "images/arrow.png";
        ICONS[Icon::LINK] = PathUtils::resourcesPath() + "images/link.png";
    }

    Manager& Manager::instance() {
        static Manager instance;
        return instance;
    }

    QList<uint16_t> Manager::registeredIcons() const {
        return ICONS.keys();
    }

    uint8_t Manager::getCount() {
        return 1;
    }

    Instance* Manager::getCursor(uint8_t index) {
        Q_ASSERT(index < getCount());
        static MouseInstance mouseInstance;
        if (index == 0) {
            return &mouseInstance;
        }
        return nullptr;
    }

    uint16_t Manager::registerIcon(const QString& path) {
        ICONS[_customIconId] = path;
        return _customIconId++;
    }

    const QString& Manager::getIconImage(uint16_t icon) {
        Q_ASSERT(ICONS.count(icon));
        return ICONS[icon];
    }

    float Manager::getScale() {
        return _scale;
    }

    void Manager::setScale(float scale) {
        _scale = scale;
    }

}
