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
    QDialog(parent, Qt::Window | Qt::WindowCloseButtonHint | Qt::WindowStaysOnTopHint) 
{
    this->setWindowTitle("HMD Tools");

    // Create layouter
    QFormLayout* form = new QFormLayout();

    // Add a button to enter
    QPushButton* enterModeButton = new QPushButton("Enter HMD Mode");
    form->addRow("", enterModeButton);
    connect(enterModeButton,SIGNAL(clicked(bool)),this,SLOT(enterModeClicked(bool)));

    // Add a button to leave
    QPushButton* leaveModeButton = new QPushButton("Leave HMD Mode");
    form->addRow("", leaveModeButton);
    connect(leaveModeButton,SIGNAL(clicked(bool)),this,SLOT(leaveModeClicked(bool)));

    // Create a label with debug details...
    _debugDetails = new QLabel();
    _debugDetails->setText(getDebugDetails());
    const int WIDTH = 350;
    const int HEIGHT = 100;
    _debugDetails->setFixedSize(WIDTH, HEIGHT);
    form->addRow("", _debugDetails);
    
    this->QDialog::setLayout(form);

    _wasMoved = false;
    _previousRect = Application::getInstance()->getWindow()->rect();
    Application::getInstance()->getWindow()->activateWindow();

    QWindow* mainWindow = Application::getInstance()->getWindow()->windowHandle();
    connect(mainWindow, &QWindow::screenChanged, this, &HMDToolsDialog::applicationWindowScreenChanged);
}

HMDToolsDialog::~HMDToolsDialog() {
}

void HMDToolsDialog::applicationWindowScreenChanged(QScreen* screen) {
    _debugDetails->setText(getDebugDetails());
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

void HMDToolsDialog::enterModeClicked(bool checked) {
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
}

void HMDToolsDialog::activateWindowAfterEnterMode() {
    Application::getInstance()->getWindow()->activateWindow();

    // center the cursor on the main application window
    centerCursorOnWidget(Application::getInstance()->getWindow());
}


void HMDToolsDialog::leaveModeClicked(bool checked) {
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


