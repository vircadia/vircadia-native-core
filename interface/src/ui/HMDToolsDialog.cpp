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

#include <QFormLayout>
#include <QGuiApplication>
#include <QDialogButtonBox>

#include <QDesktopWidget>
#include <QPushButton>
#include <QString>
#include <QScreen>
#include <QWindow>

#include <plugins/PluginManager.h>
#include <display-plugins/DisplayPlugin.h>

#include "MainWindow.h"
#include "Menu.h"
#include "ui/DialogsManager.h"
#include "ui/HMDToolsDialog.h"


HMDToolsDialog::HMDToolsDialog(QWidget* parent) :
    QDialog(parent, Qt::Window | Qt::WindowCloseButtonHint | Qt::WindowStaysOnTopHint) ,
    _previousScreen(NULL),
    _hmdScreen(NULL),
    _hmdScreenNumber(-1),
    _switchModeButton(NULL),
    _debugDetails(NULL),
    _previousDialogScreen(NULL),
    _inHDMMode(false)
{
    // FIXME do we want to support more than one connected HMD?  It seems like a pretty corner case
    foreach(auto dp, PluginManager::getInstance()->getDisplayPlugins()) {
        // The first plugin is always the standard 2D display, by convention
        if (_defaultPluginName.isEmpty()) {
            _defaultPluginName = dp->getName();
            continue;
        }
        
        if (dp->isHmd()) {
            // Not all HMD's have corresponding screens
            if (dp->getHmdScreen() >= 0) {
                _hmdScreenNumber = dp->getHmdScreen();
            }
            _hmdPluginName = dp->getName();
            break;
        }
    }

    this->setWindowTitle("HMD Tools");

    // Create layouter
    QFormLayout* form = new QFormLayout();
    const int WIDTH = 350;

    // Add a button to enter
    _switchModeButton = new QPushButton("Enter HMD Mode");
    _switchModeButton->setFixedWidth(WIDTH);
    form->addRow("", _switchModeButton);
    connect(_switchModeButton,SIGNAL(clicked(bool)),this,SLOT(switchModeClicked(bool)));

    // Create a label with debug details...
    _debugDetails = new QLabel();
    _debugDetails->setText(getDebugDetails());
    const int HEIGHT = 100;
    _debugDetails->setFixedSize(WIDTH, HEIGHT);
    form->addRow("", _debugDetails);
    
    this->QDialog::setLayout(form);
    

    Application::getInstance()->getWindow()->activateWindow();

    // watch for our application window moving screens. If it does we want to update our screen details
    QWindow* mainWindow = Application::getInstance()->getWindow()->windowHandle();
    connect(mainWindow, &QWindow::screenChanged, this, &HMDToolsDialog::applicationWindowScreenChanged);

    // watch for our dialog window moving screens. If it does we want to enforce our rules about
    // what screens we're allowed on
    watchWindow(windowHandle());
    auto dialogsManager = DependencyManager::get<DialogsManager>();
    if (Application::getInstance()->getRunningScriptsWidget()) {
        watchWindow(Application::getInstance()->getRunningScriptsWidget()->windowHandle());
    }
    if (Application::getInstance()->getToolWindow()) {
        watchWindow(Application::getInstance()->getToolWindow()->windowHandle());
    }
    if (dialogsManager->getBandwidthDialog()) {
        watchWindow(dialogsManager->getBandwidthDialog()->windowHandle());
    }
    if (dialogsManager->getOctreeStatsDialog()) {
        watchWindow(dialogsManager->getOctreeStatsDialog()->windowHandle());
    }
    if (dialogsManager->getLodToolsDialog()) {
        watchWindow(dialogsManager->getLodToolsDialog()->windowHandle());
    }
    
    // when the application is about to quit, leave HDM mode
    connect(Application::getInstance(), SIGNAL(beforeAboutToQuit()), this, SLOT(aboutToQuit()));

    // keep track of changes to the number of screens
    connect(QApplication::desktop(), &QDesktopWidget::screenCountChanged, this, &HMDToolsDialog::screenCountChanged);
}

HMDToolsDialog::~HMDToolsDialog() {
    foreach(HMDWindowWatcher* watcher, _windowWatchers) {
        delete watcher;
    }
    _windowWatchers.clear();
}

void HMDToolsDialog::applicationWindowScreenChanged(QScreen* screen) {
    _debugDetails->setText(getDebugDetails());
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
    QScreen* mainWindowScreen = Application::getInstance()->getWindow()->windowHandle()->screen();
    results += "Application Main Window Screen: " + mainWindowScreen->name() + "\n";
    results += "Total Screens: " + QString::number(QApplication::desktop()->screenCount()) + "\n";

    return results;
}

void HMDToolsDialog::switchModeClicked(bool checked) {
    if (!_inHDMMode) {
        enterHDMMode();
    } else {
        leaveHDMMode();
    }
}

void HMDToolsDialog::enterHDMMode() {
    if (!_inHDMMode) {
        _switchModeButton->setText("Leave HMD Mode");
        _debugDetails->setText(getDebugDetails());
    
        // if we're on a single screen setup, then hide our tools window when entering HMD mode
        if (QApplication::desktop()->screenCount() == 1) {
            close();
        }

        Application::getInstance()->setActiveDisplayPlugin(_hmdPluginName);

        _inHDMMode = true;
        Application::getInstance()->getWindow()->activateWindow();
    }
}

void HMDToolsDialog::leaveHDMMode() {
    if (_inHDMMode) {
        _switchModeButton->setText("Enter HMD Mode");
        _debugDetails->setText(getDebugDetails());
        Application::getInstance()->setActiveDisplayPlugin(_defaultPluginName);
        Application::getInstance()->getWindow()->activateWindow();
        _inHDMMode = false;
    }
}

void HMDToolsDialog::reject() {
    // Just regularly close upon ESC
    close();
}

void HMDToolsDialog::closeEvent(QCloseEvent* event) {
    // TODO: consider if we want to prevent closing of this window with event->ignore();
    this->QDialog::closeEvent(event);
    emit closed();
}

void HMDToolsDialog::centerCursorOnWidget(QWidget* widget) {
    QWindow* window = widget->windowHandle();
    QScreen* screen = window->screen();
    QPoint windowCenter = window->geometry().center();
    QCursor::setPos(screen, windowCenter);
}

void HMDToolsDialog::showEvent(QShowEvent* event) {
    // center the cursor on the hmd tools dialog
    centerCursorOnWidget(this);
    if (qApp->isHMDMode()) {
        _inHDMMode = true;
        _switchModeButton->setText("Leave HMD Mode");
    } else {
        _inHDMMode = false;
        _switchModeButton->setText("Enter HMD Mode");
    }
}

void HMDToolsDialog::hideEvent(QHideEvent* event) {
    // center the cursor on the main application window
    centerCursorOnWidget(Application::getInstance()->getWindow());
}


void HMDToolsDialog::aboutToQuit() {
    if (_inHDMMode) {
        // FIXME this is ineffective because it doesn't trigger the menu to 
        // save the fact that VR Mode is not checked.
        leaveHDMMode();
    }
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


    if (_inHDMMode && _hmdScreenNumber != hmdScreenNumber) {
        qDebug() << "HMD Display changed WHILE IN HMD MODE";
        leaveHDMMode();
        
        // if there is a new best HDM screen then go back into HDM mode after done leaving
        if (hmdScreenNumber >= 0) {
            qDebug() << "Trying to go back into HDM Mode";
            const int SLIGHT_DELAY = 2000;
            QTimer::singleShot(SLIGHT_DELAY, this, SLOT(enterHDMMode()));
        }
    }
    _debugDetails->setText(getDebugDetails());
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
                QWindow* appWindow = Application::getInstance()->getWindow()->windowHandle();
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


