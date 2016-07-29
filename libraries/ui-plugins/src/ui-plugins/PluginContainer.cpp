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


void PluginContainer::addMenu(const QString& menuName) {
    getPrimaryMenu()->addMenu(menuName);
}

void PluginContainer::removeMenu(const QString& menuName) {
    getPrimaryMenu()->removeMenu(menuName);
}

QAction* PluginContainer::addMenuItem(PluginType type, const QString& path, const QString& name, std::function<void(bool)> onClicked, bool checkable, bool checked, const QString& groupName) {
    auto menu = getPrimaryMenu();
    MenuWrapper* parentItem = menu->getMenu(path);
    QAction* action = menu->addActionToQMenuAndActionHash(parentItem, name);
    if (!groupName.isEmpty()) {
        QActionGroup* group { nullptr };
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
    if (type == PluginType::DISPLAY_PLUGIN) {
        _currentDisplayPluginActions.push_back({ path, name });
    } else {
        _currentInputPluginActions.push_back({ path, name });
    }
    return action;
}

void PluginContainer::removeMenuItem(const QString& menuName, const QString& menuItem) {
    getPrimaryMenu()->removeMenuItem(menuName, menuItem);
}

bool PluginContainer::isOptionChecked(const QString& name) {
    return getPrimaryMenu()->isOptionChecked(name);
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

bool isRenderThread() {
    return QThread::currentThread() != qApp->thread();
    // FIXME causes a deadlock on switching display plugins
    auto displayPlugin = PluginContainer::getInstance().getActiveDisplayPlugin();
    return displayPlugin && displayPlugin->isRenderThread();
}