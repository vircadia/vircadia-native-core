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

    QMap<uint16_t, QString> Manager::ICON_NAMES {
        { Icon::SYSTEM, "SYSTEM", },
        { Icon::DEFAULT, "DEFAULT", },
        { Icon::LINK, "LINK", },
        { Icon::ARROW, "ARROW", },
        { Icon::RETICLE, "RETICLE", },
    };

    static uint16_t _customIconId = Icon::USER_BASE;

    Manager::Manager() {
        ICONS[Icon::SYSTEM] = PathUtils::resourcesPath() + "images/cursor-none.png";
        ICONS[Icon::DEFAULT] = PathUtils::resourcesPath() + "images/cursor-arrow.png";
        ICONS[Icon::LINK] = PathUtils::resourcesPath() + "images/cursor-link.png";
        ICONS[Icon::ARROW] = PathUtils::resourcesPath() + "images/cursor-arrow.png";
        ICONS[Icon::RETICLE] = PathUtils::resourcesPath() + "images/cursor-reticle.png";
    }

    Icon Manager::lookupIcon(const QString& name) {
        for (const auto& kv : ICON_NAMES.toStdMap()) {
            if (kv.second == name) {
                return static_cast<Icon>(kv.first);
            }
        }
        return Icon::DEFAULT;
    }
    const QString& Manager::getIconName(const Icon& icon) {
        return ICON_NAMES.count(icon) ? ICON_NAMES[icon] : ICON_NAMES[Icon::DEFAULT];
    }

    Manager& Manager::instance() {
        static QSharedPointer<Manager> instance = DependencyManager::get<Cursor::Manager>();
        return *instance;
    }

    QList<uint16_t> Manager::registeredIcons() const {
        return ICONS.keys();
    }

    uint8_t Manager::getCount() {
        return 1;
    }

    Instance* Manager::getCursor(uint8_t index) {
        Q_ASSERT(index < getCount());
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
