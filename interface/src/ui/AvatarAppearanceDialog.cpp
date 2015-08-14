//
//  AvatarAppearanceDialog.cpp
//  interface/src/ui
//
//  Created by Stojce Slavkovski on 2/20/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QFileDialog>
#include <QFont>

#include <AudioClient.h>
#include <avatar/AvatarManager.h>
#include <devices/Faceshift.h>
#include <NetworkingConstants.h>

#include "Application.h"
#include "MainWindow.h"
#include "LODManager.h"
#include "Menu.h"
#include "AvatarAppearanceDialog.h"
#include "Snapshot.h"
#include "UserActivityLogger.h"
#include "UIUtil.h"
#include "ui/DialogsManager.h"
#include "ui/PreferencesDialog.h"

AvatarAppearanceDialog::AvatarAppearanceDialog(QWidget* parent) :
    QDialog(parent) {
        
    setAttribute(Qt::WA_DeleteOnClose);

    ui.setupUi(this);

    loadAvatarAppearance();

    connect(ui.defaultButton, &QPushButton::clicked, this, &AvatarAppearanceDialog::accept);

    connect(ui.buttonBrowseHead, &QPushButton::clicked, this, &AvatarAppearanceDialog::openHeadModelBrowser);
    connect(ui.buttonBrowseBody, &QPushButton::clicked, this, &AvatarAppearanceDialog::openBodyModelBrowser);
    connect(ui.buttonBrowseFullAvatar, &QPushButton::clicked, this, &AvatarAppearanceDialog::openFullAvatarModelBrowser);

    connect(ui.useSeparateBodyAndHead, &QRadioButton::clicked, this, &AvatarAppearanceDialog::useSeparateBodyAndHead);
    connect(ui.useFullAvatar, &QRadioButton::clicked, this, &AvatarAppearanceDialog::useFullAvatar);
    
    connect(Application::getInstance(), &Application::headURLChanged, this, &AvatarAppearanceDialog::headURLChanged);
    connect(Application::getInstance(), &Application::bodyURLChanged, this, &AvatarAppearanceDialog::bodyURLChanged);
    connect(Application::getInstance(), &Application::fullAvatarURLChanged, this, &AvatarAppearanceDialog::fullAvatarURLChanged);

    auto myAvatar = DependencyManager::get<AvatarManager>()->getMyAvatar();
    
    ui.bodyNameLabel->setText("Body - " + myAvatar->getBodyModelName());
    ui.headNameLabel->setText("Head - " + myAvatar->getHeadModelName());
    ui.fullAvatarNameLabel->setText("Full Avatar - " + myAvatar->getFullAvartarModelName());

    UIUtil::scaleWidgetFontSizes(this);
}

void AvatarAppearanceDialog::useSeparateBodyAndHead(bool checked) {
    QUrl headURL(ui.faceURLEdit->text());
    QUrl bodyURL(ui.skeletonURLEdit->text());
    DependencyManager::get<AvatarManager>()->getMyAvatar()->useHeadAndBodyURLs(headURL, bodyURL);
    setUseFullAvatar(!checked);
}

void AvatarAppearanceDialog::useFullAvatar(bool checked) {
    QUrl fullAvatarURL(ui.fullAvatarURLEdit->text());
    DependencyManager::get<AvatarManager>()->getMyAvatar()->useFullAvatarURL(fullAvatarURL);
    setUseFullAvatar(checked);
}

void AvatarAppearanceDialog::setUseFullAvatar(bool useFullAvatar) {
    _useFullAvatar = useFullAvatar;
    ui.faceURLEdit->setEnabled(!_useFullAvatar);
    ui.skeletonURLEdit->setEnabled(!_useFullAvatar);
    ui.fullAvatarURLEdit->setEnabled(_useFullAvatar);
    
    ui.useFullAvatar->setChecked(_useFullAvatar);
    ui.useSeparateBodyAndHead->setChecked(!_useFullAvatar);

    QPointer<PreferencesDialog> prefs = DependencyManager::get<DialogsManager>()->getPreferencesDialog();
    if (prefs) {  // Preferences dialog may have been closed
        prefs->avatarDescriptionChanged();
    }
}

void AvatarAppearanceDialog::headURLChanged(const QString& newValue, const QString& modelName) {
    ui.faceURLEdit->setText(newValue);
    setUseFullAvatar(false);
    ui.headNameLabel->setText("Head - " + modelName);
}

void AvatarAppearanceDialog::bodyURLChanged(const QString& newValue, const QString& modelName) {
    ui.skeletonURLEdit->setText(newValue);
    setUseFullAvatar(false);
    ui.bodyNameLabel->setText("Body - " + modelName);
}

void AvatarAppearanceDialog::fullAvatarURLChanged(const QString& newValue, const QString& modelName) {
    ui.fullAvatarURLEdit->setText(newValue);
    setUseFullAvatar(true);
    ui.fullAvatarNameLabel->setText("Full Avatar - " + modelName);
}

void AvatarAppearanceDialog::accept() {
    saveAvatarAppearance();

    QPointer<PreferencesDialog> prefs = DependencyManager::get<DialogsManager>()->getPreferencesDialog();
    if (prefs) {  // Preferences dialog may have been closed
        prefs->avatarDescriptionChanged();
    }

    close();
    delete _marketplaceWindow;
    _marketplaceWindow = NULL;
}

void AvatarAppearanceDialog::setHeadUrl(QString modelUrl) {
    ui.faceURLEdit->setText(modelUrl);
}

void AvatarAppearanceDialog::setSkeletonUrl(QString modelUrl) {
    ui.skeletonURLEdit->setText(modelUrl);
}

void AvatarAppearanceDialog::openFullAvatarModelBrowser() {
    auto MARKETPLACE_URL = NetworkingConstants::METAVERSE_SERVER_URL.toString() + "/marketplace?category=avatars";
    auto WIDTH = 900;
    auto HEIGHT = 700;
    if (!_marketplaceWindow) {
        _marketplaceWindow = new WebWindowClass("Marketplace", MARKETPLACE_URL, WIDTH, HEIGHT, false);
    }
    _marketplaceWindow->setVisible(true);
}

void AvatarAppearanceDialog::openHeadModelBrowser() {
    auto MARKETPLACE_URL = NetworkingConstants::METAVERSE_SERVER_URL.toString() + "/marketplace?category=avatars";
    auto WIDTH = 900;
    auto HEIGHT = 700;
    if (!_marketplaceWindow) {
        _marketplaceWindow = new WebWindowClass("Marketplace", MARKETPLACE_URL, WIDTH, HEIGHT, false);
    }
    _marketplaceWindow->setVisible(true);
}

void AvatarAppearanceDialog::openBodyModelBrowser() {
    auto MARKETPLACE_URL = NetworkingConstants::METAVERSE_SERVER_URL.toString() + "/marketplace?category=avatars";
    auto WIDTH = 900;
    auto HEIGHT = 700;
    if (!_marketplaceWindow) {
        _marketplaceWindow = new WebWindowClass("Marketplace", MARKETPLACE_URL, WIDTH, HEIGHT, false);
    }
    _marketplaceWindow->setVisible(true);
}

void AvatarAppearanceDialog::resizeEvent(QResizeEvent *resizeEvent) {
    
    // keep buttons panel at the bottom
    ui.buttonsPanel->setGeometry(0,
                                 size().height() - ui.buttonsPanel->height(),
                                 size().width(),
                                 ui.buttonsPanel->height());
    
    // set width and height of srcollarea to match bottom panel and width
    ui.scrollArea->setGeometry(ui.scrollArea->geometry().x(), ui.scrollArea->geometry().y(),
                               size().width(),
                               size().height() - ui.buttonsPanel->height() - ui.scrollArea->geometry().y());
    
}

void AvatarAppearanceDialog::loadAvatarAppearance() {
    
    MyAvatar* myAvatar = DependencyManager::get<AvatarManager>()->getMyAvatar();

    _useFullAvatar = myAvatar->getUseFullAvatar();
    _fullAvatarURLString = myAvatar->getFullAvatarURLFromPreferences().toString();
    _headURLString = myAvatar->getHeadURLFromPreferences().toString();
    _bodyURLString = myAvatar->getBodyURLFromPreferences().toString();

    ui.fullAvatarURLEdit->setText(_fullAvatarURLString);
    ui.faceURLEdit->setText(_headURLString);
    ui.skeletonURLEdit->setText(_bodyURLString);
    setUseFullAvatar(_useFullAvatar);
}

void AvatarAppearanceDialog::saveAvatarAppearance() {
    
    MyAvatar* myAvatar = DependencyManager::get<AvatarManager>()->getMyAvatar();

    QUrl headURL(ui.faceURLEdit->text());
    QString headURLString = headURL.toString();

    QUrl bodyURL(ui.skeletonURLEdit->text());
    QString bodyURLString = bodyURL.toString();

    QUrl fullAvatarURL(ui.fullAvatarURLEdit->text());
    QString fullAvatarURLString = fullAvatarURL.toString();

    bool somethingChanged = 
        _useFullAvatar != myAvatar->getUseFullAvatar() ||
        fullAvatarURLString != myAvatar->getFullAvatarURLFromPreferences().toString() ||
        headURLString != myAvatar->getHeadURLFromPreferences().toString() ||
        bodyURLString != myAvatar->getBodyURLFromPreferences().toString();
        
    if (somethingChanged) {
        if (_useFullAvatar) {
            myAvatar->useFullAvatarURL(fullAvatarURL);
        } else {
            myAvatar->useHeadAndBodyURLs(headURL, bodyURL);
        }
    }
}
