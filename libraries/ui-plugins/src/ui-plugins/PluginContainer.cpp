//
//  Created by Bradley Austin Davis on 2015/08/08
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "PluginContainer.h"

#include <QtCore/QTimer>
#include <QtCore/QThread>
#include <QtGui/QScreen>
#include <QtGui/QWindow>
#include <QtWidgets/QApplication>

#include <ui/Menu.h>
#include <MainWindow.h>
#include <plugins/DisplayPlugin.h>

static PluginContainer* INSTANCE{ nullptr };

PluginContainer& PluginContainer::getInstance() {
    Q_ASSERT(INSTANCE);
    return *INSTANCE;
}

PluginContainer::PluginContainer() {
    Q_ASSERT(!INSTANCE);
    INSTANCE = this;
};

PluginContainer::~PluginContainer() {
    Q_ASSERT(INSTANCE == this);
    INSTANCE = nullptr;
};

struct MenuCache {
    QSet<QString> menus;
    struct Item {
        QString path;
        std::function<void(bool)> onClicked;
        bool checkable;
        bool checked;
        QString groupName;
    };
    QHash<QString, Item> items;
    std::map<QString, QActionGroup*> _exclusiveGroups;

    void addMenu(ui::Menu* menu, const QString& menuName) {
        if (!menu) {
            menus.insert(menuName);
            return;
        }

        flushCache(menu);
        menu->addMenu(menuName);
    }

    void removeMenu(ui::Menu* menu, const QString& menuName) {
        if (!menu) {
            menus.remove(menuName);
            return;
        }
        flushCache(menu);
        menu->removeMenu(menuName);
    }

    void addMenuItem(ui::Menu* menu, const QString& path, const QString& name, std::function<void(bool)> onClicked, bool checkable, bool checked, const QString& groupName) {
        if (!menu) {
            items[name] = Item{ path, onClicked, checkable, checked, groupName };
            return;
        }
        flushCache(menu);
        MenuWrapper* parentItem = menu->getMenu(path);
        QAction* action = menu->addActionToQMenuAndActionHash(parentItem, name);
        if (!groupName.isEmpty()) {
            QActionGroup* group{ nullptr };
            if (!_exclusiveGroups.count(groupName)) {
                group = _exclusiveGroups[groupName] = new QActionGroup(menu);
                group->setExclusive(true);
            } else {
                group = _exclusiveGroups[groupName];
            }
            group->addAction(action);
        }
        QObject::connect(action, &QAction::triggered, [=] {
            onClicked(action->isChecked());
        });
        action->setCheckable(checkable);
        action->setChecked(checked);
    }
    void removeMenuItem(ui::Menu* menu, const QString& menuName, const QString& menuItemName) {
        if (!menu) {
            items.remove(menuItemName);
            return;
        }
        flushCache(menu);
        menu->removeMenuItem(menuName, menuItemName);
    }

    bool isOptionChecked(ui::Menu* menu, const QString& name) {
        if (!menu) {
            return items.contains(name) && items[name].checked;
        }
        flushCache(menu);
        return menu->isOptionChecked(name);
    }

    void setIsOptionChecked(ui::Menu* menu, const QString& name, bool checked) {
        if (!menu) {
            if (items.contains(name)) {
                items[name].checked = checked;
            }
            return;
        }
        flushCache(menu);

    }

    void flushCache(ui::Menu* menu) {
        if (!menu) {
            return;
        }
        static bool flushed = false;
        if (flushed) {
            return;
        }
        flushed = true;
        for (const auto& menuName : menus) {
            addMenu(menu, menuName);
        }
        menus.clear();

        for (const auto& menuItemName : items.keys()) {
            const auto menuItem = items[menuItemName];
            addMenuItem(menu, menuItem.path, menuItemName, menuItem.onClicked, menuItem.checkable, menuItem.checked, menuItem.groupName);
        }
        items.clear();
    }
};


static MenuCache& getMenuCache() {
    static MenuCache cache;
    return cache;
}

void PluginContainer::addMenu(const QString& menuName) {
    getMenuCache().addMenu(getPrimaryMenu(), menuName);
}

void PluginContainer::removeMenu(const QString& menuName) {
    getMenuCache().removeMenu(getPrimaryMenu(), menuName);
}

void PluginContainer::addMenuItem(PluginType type, const QString& path, const QString& name, std::function<void(bool)> onClicked, bool checkable, bool checked, const QString& groupName) {
    getMenuCache().addMenuItem(getPrimaryMenu(), path, name, onClicked, checkable, checked, groupName);
    if (type == PluginType::DISPLAY_PLUGIN) {
        _currentDisplayPluginActions.push_back({ path, name });
    } else {
        _currentInputPluginActions.push_back({ path, name });
    }
}

void PluginContainer::removeMenuItem(const QString& menuName, const QString& menuItem) {
    getMenuCache().removeMenuItem(getPrimaryMenu(), menuName, menuItem);
}

bool PluginContainer::isOptionChecked(const QString& name) {
    return getMenuCache().isOptionChecked(getPrimaryMenu(), name);
}

void PluginContainer::setIsOptionChecked(const QString& path, bool checked) {
    getPrimaryMenu()->setIsOptionChecked(path, checked);
}



// FIXME there is a bug in the fullscreen setting, where leaving
// fullscreen does not restore the window frame, making it difficult
// or impossible to move or size the window.
// Additionally, setting fullscreen isn't hiding the menu on windows
// make it useless for stereoscopic modes.
void PluginContainer::setFullscreen(const QScreen* target, bool hideMenu) {
    auto _window = getPrimaryWindow();
    if (!_window->isFullScreen()) {
        _savedGeometry = _window->geometry();
    }
    if (nullptr == target) {
        // FIXME target the screen where the window currently is
        target = qApp->primaryScreen();
    }

    _window->setGeometry(target->availableGeometry());
    _window->windowHandle()->setScreen((QScreen*)target);
    _window->showFullScreen();

#ifndef Q_OS_MAC
    // also hide the QMainWindow's menuBar
    QMenuBar* menuBar = _window->menuBar();
    if (menuBar && hideMenu) {
        menuBar->setVisible(false);
    }
#endif
}

void PluginContainer::unsetFullscreen(const QScreen* avoid) {
    auto _window = getPrimaryWindow();
    _window->showNormal();

    QRect targetGeometry = _savedGeometry;
    if (avoid != nullptr) {
        QRect avoidGeometry = avoid->geometry();
        if (avoidGeometry.contains(targetGeometry.topLeft())) {
            QScreen* newTarget = qApp->primaryScreen();
            if (newTarget == avoid) {
                foreach(auto screen, qApp->screens()) {
                    if (screen != avoid) {
                        newTarget = screen;
                        break;
                    }
                }
            }
            targetGeometry = newTarget->availableGeometry();
        }
    }
#ifdef Q_OS_MAC
    QTimer* timer = new QTimer();
    timer->singleShot(2000, [=] {
        _window->setGeometry(targetGeometry);
        timer->deleteLater();
    });
#else
    _window->setGeometry(targetGeometry);
#endif

#ifndef Q_OS_MAC
    // also show the QMainWindow's menuBar
    QMenuBar* menuBar = _window->menuBar();
    if (menuBar) {
        menuBar->setVisible(true);
    }
#endif
}

/// settings interface
bool PluginContainer::getBoolSetting(const QString& settingName, bool defaultValue) {
    Setting::Handle<bool> settingValue(settingName, defaultValue);
    return settingValue.get();
}

void PluginContainer::setBoolSetting(const QString& settingName, bool value) {
    Setting::Handle<bool> settingValue(settingName, value);
    return settingValue.set(value);
}

void PluginContainer::flushMenuUpdates() {
    getMenuCache().flushCache(getPrimaryMenu());
}
