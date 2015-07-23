//
//  Created by Bradley Austin Davis on 2015/05/29
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "Basic2DWindowOpenGLDisplayPlugin.h"

#include <plugins/PluginContainer.h>
#include <QWindow>

const QString Basic2DWindowOpenGLDisplayPlugin::NAME("2D Display");

const QString MENU_PARENT = "View";
const QString MENU_NAME = "Display Options";
const QString MENU_PATH = MENU_PARENT + ">" + MENU_NAME;
const QString FULLSCREEN = "Fullscreen";

const QString& Basic2DWindowOpenGLDisplayPlugin::getName() const {
    return NAME;
}

void Basic2DWindowOpenGLDisplayPlugin::activate(PluginContainer* container) {
    container->addMenu(MENU_PATH);
    container->addMenuItem(MENU_PATH, FULLSCREEN,
        [this] (bool clicked) { this->setFullscreen(clicked); },
        true, false);
    MainWindowOpenGLDisplayPlugin::activate(container);
}

void Basic2DWindowOpenGLDisplayPlugin::deactivate(PluginContainer* container) {
    container->removeMenuItem(MENU_NAME, FULLSCREEN);
    container->removeMenu(MENU_PATH);
    MainWindowOpenGLDisplayPlugin::deactivate(container);
}

void Basic2DWindowOpenGLDisplayPlugin::setFullscreen(bool fullscreen) {
    // The following code block is useful on platforms that can have a visible
    // app menu in a fullscreen window.  However the OSX mechanism hides the
    // application menu for fullscreen apps, so the check is not required.
//#ifndef Q_OS_MAC
//    if (fullscreen) {
//        // Move menu to a QWidget floating above _glWidget so that show/hide doesn't adjust viewport.
//        _menuBarHeight = Menu::getInstance()->height();
//        Menu::getInstance()->setParent(_fullscreenMenuWidget);
//        Menu::getInstance()->setFixedWidth(_window->windowHandle()->screen()->size().width());
//        _fullscreenMenuWidget->show();
//    }
//    else {
//        // Restore menu to being part of MainWindow.
//        _fullscreenMenuWidget->hide();
//        _window->setMenuBar(Menu::getInstance());
//        _window->menuBar()->setMaximumHeight(QWIDGETSIZE_MAX);
//    }
//#endif

    // Work around Qt bug that prevents floating menus being shown when in fullscreen mode.
    // https://bugreports.qt.io/browse/QTBUG-41883
    // Known issue: Top-level menu items don't highlight when cursor hovers. This is probably a side-effect of the work-around.
    // TODO: Remove this work-around once the bug has been fixed and restore the following lines.
    //_window->setWindowState(fullscreen ? (_window->windowState() | Qt::WindowFullScreen) :
    //    (_window->windowState() & ~Qt::WindowFullScreen));
    auto window = this->getWindow();
    window->hide();
    if (fullscreen) {
        auto state = window->windowState() | Qt::WindowFullScreen;
        window->setWindowState(Qt::WindowState((int)state));
        window->setPosition(0, 0);
        // The next line produces the following warning in the log:
        // [WARNING][03 / 06 12:17 : 58] QWidget::setMinimumSize: (/ MainWindow) Negative sizes
        //   (0, -1) are not possible
        // This is better than the alternative which is to have the window slightly less than fullscreen with a visible line
        // of pixels for the remainder of the screen.
        //window->setContentsMargins(0, 0, 0, -1);
    }
    else {
        window->setWindowState(Qt::WindowState(window->windowState() & ~Qt::WindowFullScreen));
        //window->setContentsMargins(0, 0, 0, 0);
    }

    window->show();
}
