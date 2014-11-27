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

#include <VoxelConstants.h>

#include "Menu.h"
#include "devices/OculusManager.h"
#include "ui/HMDToolsDialog.h"


HMDToolsDialog::HMDToolsDialog(QWidget* parent) :
    QDialog(parent, Qt::Window | Qt::WindowCloseButtonHint | Qt::WindowStaysOnTopHint) ,
    _switchModeButton(NULL),
    _debugDetails(NULL),
    _previousScreen(NULL),
    _hmdScreen(NULL),
    _previousDialogScreen(NULL),
    _inHDMMode(false)
{
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

    _wasMoved = false;
    _previousRect = Application::getInstance()->getWindow()->rect();
    Application::getInstance()->getWindow()->activateWindow();

    // watch for our application window moving screens. If it does we want to update our screen details
    QWindow* mainWindow = Application::getInstance()->getWindow()->windowHandle();
    connect(mainWindow, &QWindow::screenChanged, this, &HMDToolsDialog::applicationWindowScreenChanged);

    // watch for our dialog window moving screens. If it does we want to enforce our rules about
    // what screens we're allowed on
    QWindow* dialogWindow = windowHandle();
    connect(dialogWindow, &QWindow::screenChanged, this, &HMDToolsDialog::dialogWindowScreenChanged);
    connect(dialogWindow, &QWindow::xChanged, this, &HMDToolsDialog::dialogWindowGeometryChanged);
    connect(dialogWindow, &QWindow::yChanged, this, &HMDToolsDialog::dialogWindowGeometryChanged);
    connect(dialogWindow, &QWindow::widthChanged, this, &HMDToolsDialog::dialogWindowGeometryChanged);
    connect(dialogWindow, &QWindow::heightChanged, this, &HMDToolsDialog::dialogWindowGeometryChanged);
    
    // when the application is about to quit, leave HDM mode
    connect(Application::getInstance(), SIGNAL(aboutToQuit()), this, SLOT(aboutToQuit()));
    
}

HMDToolsDialog::~HMDToolsDialog() {
}

void HMDToolsDialog::applicationWindowScreenChanged(QScreen* screen) {
    _debugDetails->setText(getDebugDetails());
}

void HMDToolsDialog::dialogWindowGeometryChanged(int arg) {
    QWindow* dialogWindow = windowHandle();
    _previousDialogRect = rect();
    _previousDialogRect = QRect(dialogWindow->mapToGlobal(_previousDialogRect.topLeft()), 
                                dialogWindow->mapToGlobal(_previousDialogRect.bottomRight()));
    _previousDialogScreen = dialogWindow->screen();
}

void HMDToolsDialog::dialogWindowScreenChanged(QScreen* screen) {
    _debugDetails->setText(getDebugDetails());

    // if we have more than one screen, and a known hmdScreen then try to 
    // keep our dialog off of the hmdScreen
    if (QApplication::desktop()->screenCount() > 1) {
        int hmdScreenNumber = OculusManager::getHMDScreen();

        if (hmdScreenNumber >= 0) {
            QScreen* hmdScreen = QGuiApplication::screens()[hmdScreenNumber];
    
            if (screen == hmdScreen) {
                qDebug() << "HMD Tools: Whoa! What are you doing? You don't want to move me to the HMD Screen!";
                QWindow* dialogWindow = windowHandle();
        
                // try to pick a better screen
                QScreen* betterScreen = NULL;
        
                QWindow* appWindow = Application::getInstance()->getWindow()->windowHandle();
                QScreen* appScreen = appWindow->screen();

                if (_previousDialogScreen && _previousDialogScreen != hmdScreen) {
                    // first, if the previous dialog screen is not the HMD screen, then move it there.
                    betterScreen = appScreen;
                } else if (appScreen != hmdScreen) {
                    // second, if the application screen is not the HMD screen, then move it there.
                    betterScreen = appScreen;
                } else if (_previousScreen && _previousScreen != hmdScreen) {
                    // third, if the application screen is the HMD screen, we want to move it to
                    // the previous screen
                    betterScreen = _previousScreen;
                } else {
                    // last, if we can't use the previous screen the use the primary desktop screen
                    int desktopPrimaryScreenNumber = QApplication::desktop()->primaryScreen();
                    QScreen* desktopPrimaryScreen = QGuiApplication::screens()[desktopPrimaryScreenNumber];
                    betterScreen = desktopPrimaryScreen;
                }

                if (betterScreen) {
                    dialogWindow->setScreen(betterScreen);
                    dialogWindow->setGeometry(_previousDialogRect);
                }
            }
        }
    }
}

QString HMDToolsDialog::getDebugDetails() const {
    QString results;

    int hmdScreenNumber = OculusManager::getHMDScreen();
    if (hmdScreenNumber >= 0) {
        results += "HMD Screen: " + QGuiApplication::screens()[hmdScreenNumber]->name() + "\n";
    } else {
        results += "HMD Screen Name: Unknown\n";
    }

    int desktopPrimaryScreenNumber = QApplication::desktop()->primaryScreen();
    QScreen* desktopPrimaryScreen = QGuiApplication::screens()[desktopPrimaryScreenNumber];
    results += "Desktop's Primary Screen: " + desktopPrimaryScreen->name() + "\n";

    results += "Application Primary Screen: " + QGuiApplication::primaryScreen()->name() + "\n";
    QScreen* mainWindowScreen = Application::getInstance()->getWindow()->windowHandle()->screen();
    results += "Application Main Window Screen: " + mainWindowScreen->name() + "\n";

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

        int hmdScreen = OculusManager::getHMDScreen();
        qDebug() << "enterModeClicked().... hmdScreen:" << hmdScreen;
    
        if (hmdScreen >= 0) {
            QWindow* mainWindow = Application::getInstance()->getWindow()->windowHandle();
            _hmdScreen = QGuiApplication::screens()[hmdScreen];
        
            _previousRect = Application::getInstance()->getWindow()->rect();
            _previousRect = QRect(mainWindow->mapToGlobal(_previousRect.topLeft()), 
                                    mainWindow->mapToGlobal(_previousRect.bottomRight()));
            _previousScreen = mainWindow->screen();
            QRect rect = QApplication::desktop()->screenGeometry(hmdScreen);
            mainWindow->setScreen(_hmdScreen);
            mainWindow->setGeometry(rect);

            _wasMoved = true;
        }
    
    
        // if we're on a single screen setup, then hide our tools window when entering HMD mode
        if (QApplication::desktop()->screenCount() == 1) {
            close();
        }

        Application::getInstance()->setFullscreen(true);
        Application::getInstance()->setEnableVRMode(true);
    
        const int SLIGHT_DELAY = 500;
        QTimer::singleShot(SLIGHT_DELAY, this, SLOT(activateWindowAfterEnterMode()));
    
        _inHDMMode = true;
    }
}

void HMDToolsDialog::activateWindowAfterEnterMode() {
    Application::getInstance()->getWindow()->activateWindow();

    // center the cursor on the main application window
    centerCursorOnWidget(Application::getInstance()->getWindow());
}

void HMDToolsDialog::leaveHDMMode() {
    if (_inHDMMode) {
        _switchModeButton->setText("Enter HMD Mode");
        _debugDetails->setText(getDebugDetails());

        Application::getInstance()->setFullscreen(false);
        Application::getInstance()->setEnableVRMode(false);
        Application::getInstance()->getWindow()->activateWindow();

        if (_wasMoved) {
            QWindow* mainWindow = Application::getInstance()->getWindow()->windowHandle();
            mainWindow->setScreen(_previousScreen);
            mainWindow->setGeometry(_previousRect);
    
            const int SLIGHT_DELAY = 1500;
            QTimer::singleShot(SLIGHT_DELAY, this, SLOT(moveWindowAfterLeaveMode()));
        }
        _wasMoved = false;
        _inHDMMode = false;
    }
}

void HMDToolsDialog::moveWindowAfterLeaveMode() {
    QWindow* mainWindow = Application::getInstance()->getWindow()->windowHandle();
    mainWindow->setScreen(_previousScreen);
    mainWindow->setGeometry(_previousRect);
    Application::getInstance()->getWindow()->activateWindow();
    Application::getInstance()->resetSensors();
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
}

void HMDToolsDialog::hideEvent(QHideEvent* event) {
    // center the cursor on the main application window
    centerCursorOnWidget(Application::getInstance()->getWindow());
}


void HMDToolsDialog::aboutToQuit() {
    if (_inHDMMode) {
        leaveHDMMode();
    }
}


