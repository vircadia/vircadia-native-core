//
//  HMDToolsDialog.cpp
//  interface/src/ui
//
//  Created by Brad Hefta-Gaub on 7/19/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QDesktopWidget>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGuiApplication>
#include <QLabel>
#include <QPushButton>
#include <QString>
#include <QScreen>
#include <QWindow>

#include <plugins/PluginManager.h>
#include <display-plugins/DisplayPlugin.h>

#include "Application.h"
#include "MainWindow.h"
#include "Menu.h"
#include "ui/DialogsManager.h"
#include "ui/HMDToolsDialog.h"

static const int WIDTH = 350;
static const int HEIGHT = 100;


HMDToolsDialog::HMDToolsDialog(QWidget* parent) :
    QDialog(parent, Qt::Window | Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowStaysOnTopHint)
{
    // FIXME do we want to support more than one connected HMD?  It seems like a pretty corner case
    foreach(auto displayPlugin, PluginManager::getInstance()->getDisplayPlugins()) {
        // The first plugin is always the standard 2D display, by convention
        if (_defaultPluginName.isEmpty()) {
            _defaultPluginName = displayPlugin->getName();
            continue;
        }
        
        if (displayPlugin->isHmd()) {
            // Not all HMD's have corresponding screens
            if (displayPlugin->getHmdScreen() >= 0) {
                _hmdScreenNumber = displayPlugin->getHmdScreen();
            }
            _hmdPluginName = displayPlugin->getName();
            break;
        }
    }

    setWindowTitle("HMD Tools");

    // Create layouter
    {
        QFormLayout* form = new QFormLayout();
        // Add a button to enter
        _switchModeButton = new QPushButton("Toggle HMD Mode");
        if (_hmdPluginName.isEmpty()) {
            _switchModeButton->setEnabled(false);
        }
        // Add a button to enter
        _switchModeButton->setFixedWidth(WIDTH);
        form->addRow("", _switchModeButton);
        // Create a label with debug details...
        _debugDetails = new QLabel();
        _debugDetails->setFixedSize(WIDTH, HEIGHT);
        form->addRow("", _debugDetails);
        setLayout(form);
    }

    qApp->getWindow()->activateWindow();

    // watch for our dialog window moving screens. If it does we want to enforce our rules about
    // what screens we're allowed on
    watchWindow(windowHandle());
    auto dialogsManager = DependencyManager::get<DialogsManager>();
    if (dialogsManager->getOctreeStatsDialog()) {
        watchWindow(dialogsManager->getOctreeStatsDialog()->windowHandle());
    }
    if (dialogsManager->getLodToolsDialog()) {
        watchWindow(dialogsManager->getLodToolsDialog()->windowHandle());
    }
    
    connect(_switchModeButton, &QPushButton::clicked, [this]{
        toggleHMDMode();
    });
    
    // when the application is about to quit, leave HDM mode
    connect(qApp, &Application::beforeAboutToQuit, [this]{
        // FIXME this is ineffective because it doesn't trigger the menu to
        // save the fact that VR Mode is not checked.
        leaveHMDMode();
    });

    connect(qApp, &Application::activeDisplayPluginChanged, [this]{
        updateUi();
    });

    // watch for our application window moving screens. If it does we want to update our screen details
    QWindow* mainWindow = qApp->getWindow()->windowHandle();
    connect(mainWindow, &QWindow::screenChanged, [this]{
        updateUi();
    });

    // keep track of changes to the number of screens
    connect(QApplication::desktop(), &QDesktopWidget::screenCountChanged, this, &HMDToolsDialog::screenCountChanged);
    
    updateUi();
}

HMDToolsDialog::~HMDToolsDialog() {
    foreach(HMDWindowWatcher* watcher, _windowWatchers) {
        delete watcher;
    }
    _windowWatchers.clear();
}

QString HMDToolsDialog::getDebugDetails() const {
    QString results;

    if (_hmdScreenNumber >= 0) {
        results += "HMD Screen: " + QGuiApplication::screens()[_hmdScreenNumber]->name() + "\n";
    } else {
        results += "HMD Screen Name: N/A\n";
    }

    int desktopPrimaryScreenNumber = QApplication::desktop()->primaryScreen();
    QScreen* desktopPrimaryScreen = QGuiApplication::screens()[desktopPrimaryScreenNumber];
    results += "Desktop's Primary Screen: " + desktopPrimaryScreen->name() + "\n";

    results += "Application Primary Screen: " + QGuiApplication::primaryScreen()->name() + "\n";
    QScreen* mainWindowScreen = qApp->getWindow()->windowHandle()->screen();
    results += "Application Main Window Screen: " + mainWindowScreen->name() + "\n";
    results += "Total Screens: " + QString::number(QApplication::desktop()->screenCount()) + "\n";

    return results;
}

void HMDToolsDialog::toggleHMDMode() {
    if (!qApp->isHMDMode()) {
        enterHMDMode();
    } else {
        leaveHMDMode();
    }
}

void HMDToolsDialog::enterHMDMode() {
    if (!qApp->isHMDMode()) {
        qApp->setActiveDisplayPlugin(_hmdPluginName);
        qApp->getWindow()->activateWindow();
    }
}

void HMDToolsDialog::leaveHMDMode() {
    if (qApp->isHMDMode()) {
        qApp->setActiveDisplayPlugin(_defaultPluginName);
        qApp->getWindow()->activateWindow();
    }
}

void HMDToolsDialog::reject() {
    // We don't want this window to be closable from a close icon, just from our "Leave HMD Mode" button
}

void HMDToolsDialog::closeEvent(QCloseEvent* event) {
    // We don't want this window to be closable from a close icon, just from our "Leave HMD Mode" button
    event->ignore();
}

void HMDToolsDialog::centerCursorOnWidget(QWidget* widget) {
    QWindow* window = widget->windowHandle();
    QScreen* screen = window->screen();
    QPoint windowCenter = window->geometry().center();
    QCursor::setPos(screen, windowCenter);
}

void HMDToolsDialog::updateUi() {
    _switchModeButton->setText(qApp->isHMDMode() ? "Leave HMD Mode" : "Enter HMD Mode");
    _debugDetails->setText(getDebugDetails());
}

void HMDToolsDialog::showEvent(QShowEvent* event) {
    // center the cursor on the hmd tools dialog
    centerCursorOnWidget(this);
    updateUi();
}

void HMDToolsDialog::hideEvent(QHideEvent* event) {
    // center the cursor on the main application window
    centerCursorOnWidget(qApp->getWindow());
}

void HMDToolsDialog::screenCountChanged(int newCount) {
    int hmdScreenNumber = -1;
    auto displayPlugins = PluginManager::getInstance()->getDisplayPlugins();
    foreach(auto dp, displayPlugins) {
        if (dp->isHmd()) {
            if (dp->getHmdScreen() >= 0) {
                hmdScreenNumber = dp->getHmdScreen();
            }
            break;
        }
    }

    if (qApp->isHMDMode() && _hmdScreenNumber != hmdScreenNumber) {
        qDebug() << "HMD Display changed WHILE IN HMD MODE";
        leaveHMDMode();
        
        // if there is a new best HDM screen then go back into HDM mode after done leaving
        if (hmdScreenNumber >= 0) {
            qDebug() << "Trying to go back into HMD Mode";
            const int SLIGHT_DELAY = 2000;
            QTimer::singleShot(SLIGHT_DELAY, [this]{
                enterHMDMode();
            });
        }
    }
}

void HMDToolsDialog::watchWindow(QWindow* window) {
    qDebug() << "HMDToolsDialog::watchWindow() window:" << window;
    if (window && !_windowWatchers.contains(window)) {
        HMDWindowWatcher* watcher = new HMDWindowWatcher(window, this);
        _windowWatchers[window] = watcher;
    }
}


HMDWindowWatcher::HMDWindowWatcher(QWindow* window, HMDToolsDialog* hmdTools) :
    _window(window),
    _hmdTools(hmdTools),
    _previousScreen(NULL)
{
    connect(window, &QWindow::screenChanged, this, &HMDWindowWatcher::windowScreenChanged);
    connect(window, &QWindow::xChanged, this, &HMDWindowWatcher::windowGeometryChanged);
    connect(window, &QWindow::yChanged, this, &HMDWindowWatcher::windowGeometryChanged);
    connect(window, &QWindow::widthChanged, this, &HMDWindowWatcher::windowGeometryChanged);
    connect(window, &QWindow::heightChanged, this, &HMDWindowWatcher::windowGeometryChanged);
}

HMDWindowWatcher::~HMDWindowWatcher() {
}


void HMDWindowWatcher::windowGeometryChanged(int arg) {
    _previousRect = _window->geometry();
    _previousScreen = _window->screen();
}

void HMDWindowWatcher::windowScreenChanged(QScreen* screen) {
    // if we have more than one screen, and a known hmdScreen then try to 
    // keep our dialog off of the hmdScreen
    if (QApplication::desktop()->screenCount() > 1) {
        int hmdScreenNumber = _hmdTools->_hmdScreenNumber;
        // we want to use a local variable here because we are not necesarily in HMD mode
        if (hmdScreenNumber >= 0) {
            QScreen* hmdScreen = QGuiApplication::screens()[hmdScreenNumber];
            if (screen == hmdScreen) {
                qDebug() << "HMD Tools: Whoa! What are you doing? You don't want to move me to the HMD Screen!";
        
                // try to pick a better screen
                QScreen* betterScreen = NULL;
        
                QScreen* lastApplicationScreen = _hmdTools->getLastApplicationScreen();
                QWindow* appWindow = qApp->getWindow()->windowHandle();
                QScreen* appScreen = appWindow->screen();

                if (_previousScreen && _previousScreen != hmdScreen) {
                    // first, if the previous dialog screen is not the HMD screen, then move it there.
                    betterScreen = _previousScreen;
                } else if (appScreen != hmdScreen) {
                    // second, if the application screen is not the HMD screen, then move it there.
                    betterScreen = appScreen;
                } else if (lastApplicationScreen && lastApplicationScreen != hmdScreen) {
                    // third, if the application screen is the HMD screen, we want to move it to
                    // the previous screen
                    betterScreen = lastApplicationScreen;
                } else {
                    // last, if we can't use the previous screen the use the primary desktop screen
                    int desktopPrimaryScreenNumber = QApplication::desktop()->primaryScreen();
                    QScreen* desktopPrimaryScreen = QGuiApplication::screens()[desktopPrimaryScreenNumber];
                    betterScreen = desktopPrimaryScreen;
                }

                if (betterScreen) {
                    _window->setScreen(betterScreen);
                    _window->setGeometry(_previousRect);
                }
            }
        }
    }
}


