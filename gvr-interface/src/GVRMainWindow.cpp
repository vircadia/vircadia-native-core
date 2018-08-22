//
//  GVRMainWindow.cpp
//  gvr-interface/src
//
//  Created by Stephen Birarda on 1/20/14.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "GVRMainWindow.h"

#include <QtGui/QKeyEvent>
#include <QtWidgets/QApplication>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QVBoxLayout>

#ifndef ANDROID

#include <QtWidgets/QDesktopWidget>

#elif defined(HAVE_LIBOVR)

#include <OVR_CAPI.h>

const float LIBOVR_DOUBLE_TAP_DURATION = 0.25f;
const float LIBOVR_LONG_PRESS_DURATION = 0.75f;

#endif

#include <AddressManager.h>

#include "InterfaceView.h"
#include "LoginDialog.h"
#include "RenderingClient.h"



GVRMainWindow::GVRMainWindow(QWidget* parent) :
    QMainWindow(parent),
#if defined(ANDROID) && defined(HAVE_LIBOVR)
    _backKeyState(LIBOVR_DOUBLE_TAP_DURATION, LIBOVR_LONG_PRESS_DURATION),
    _wasBackKeyDown(false),
#endif
    _mainLayout(NULL),
    _menuBar(NULL),
    _loginAction(NULL)
{
    
#ifndef ANDROID
    const int NOTE_4_WIDTH = 2560;
    const int NOTE_4_HEIGHT = 1440;
    setFixedSize(NOTE_4_WIDTH / 2, NOTE_4_HEIGHT / 2);
#endif
    
    setupMenuBar();
    
    QWidget* baseWidget = new QWidget(this);
    
    // setup a layout so we can vertically align to top
    _mainLayout = new QVBoxLayout(baseWidget);
    _mainLayout->setAlignment(Qt::AlignTop);
    
    // set the layout on the base widget
    baseWidget->setLayout(_mainLayout);
    
    setCentralWidget(baseWidget);
    
    // add the interface view
    new InterfaceView(baseWidget);
}

GVRMainWindow::~GVRMainWindow() {
    delete _menuBar;
}

void GVRMainWindow::keyPressEvent(QKeyEvent* event) {
#ifdef ANDROID
    if (event->key() == Qt::Key_Back) {
        // got the Android back key, hand off to OVR KeyState
        _backKeyState.HandleEvent(ovr_GetTimeInSeconds(), true, (_wasBackKeyDown ? 1 : 0));
        _wasBackKeyDown = true;
        return;
    }
#endif
    QWidget::keyPressEvent(event);
}

void GVRMainWindow::keyReleaseEvent(QKeyEvent* event) {
#ifdef ANDROID
    if (event->key() == Qt::Key_Back) {
        // release on the Android back key, hand off to OVR KeyState
        _backKeyState.HandleEvent(ovr_GetTimeInSeconds(), false, 0);
        _wasBackKeyDown = false;
    }
#endif
    QWidget::keyReleaseEvent(event);
}

void GVRMainWindow::setupMenuBar() {
    QMenu* fileMenu = new QMenu("File");
    QMenu* helpMenu = new QMenu("Help");
    
    _menuBar = new QMenuBar(0);
    
    _menuBar->addMenu(fileMenu);
    _menuBar->addMenu(helpMenu);
    
    QAction* goToAddress = new QAction("Go to Address", fileMenu);
    connect(goToAddress, &QAction::triggered, this, &GVRMainWindow::showAddressBar);
    fileMenu->addAction(goToAddress);
    
    _loginAction = new QAction("Login", fileMenu);
    fileMenu->addAction(_loginAction);
    
    // change the login action depending on our logged in/out state
    AccountManager& accountManager = AccountManager::getInstance();
    connect(&accountManager, &AccountManager::loginComplete, this, &GVRMainWindow::refreshLoginAction);
    connect(&accountManager, &AccountManager::logoutComplete, this, &GVRMainWindow::refreshLoginAction);
    
    // refresh the state now
    refreshLoginAction();
    
    QAction* aboutQt = new QAction("About Qt", helpMenu);
    connect(aboutQt, &QAction::triggered, qApp, &QApplication::aboutQt);
    helpMenu->addAction(aboutQt);
    
    setMenuBar(_menuBar);
}

void GVRMainWindow::showAddressBar() {
    // setup the address QInputDialog
    QInputDialog* addressDialog = new QInputDialog(this);
    addressDialog->setLabelText("Address");
    
    // add the address dialog to the main layout
    _mainLayout->addWidget(addressDialog);
    
    connect(addressDialog, &QInputDialog::textValueSelected, 
            DependencyManager::get<AddressManager>().data(), &AddressManager::handleLookupString);
}

void GVRMainWindow::showLoginDialog() {
    LoginDialog* loginDialog = new LoginDialog(this);
    
    // have the acccount manager handle credentials from LoginDialog
    AccountManager& accountManager = AccountManager::getInstance();
    connect(loginDialog, &LoginDialog::credentialsEntered, &accountManager, &AccountManager::requestAccessToken);
    connect(&accountManager, &AccountManager::loginFailed, this, &GVRMainWindow::showLoginFailure);
    
    _mainLayout->addWidget(loginDialog);
}

void GVRMainWindow::showLoginFailure() {
    QMessageBox::warning(this, "Login Failed",
                         "Could not log in with that username and password. Please try again!");
}

void GVRMainWindow::refreshLoginAction() {
    AccountManager& accountManager = AccountManager::getInstance();
    disconnect(_loginAction, &QAction::triggered, &accountManager, 0);
    
    if (accountManager.isLoggedIn()) {
        _loginAction->setText("Logout");
        connect(_loginAction, &QAction::triggered, &accountManager, &AccountManager::logout);
    } else {
        _loginAction->setText("Login");
        connect(_loginAction, &QAction::triggered, this, &GVRMainWindow::showLoginDialog);
    }
    
}
