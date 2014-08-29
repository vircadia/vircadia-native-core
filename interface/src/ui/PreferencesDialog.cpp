//
//  PreferencesDialog.cpp
//  interface/src/ui
//
//  Created by Stojce Slavkovski on 2/20/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#include "Application.h"
#include "Menu.h"
#include "ModelsBrowser.h"
#include "PreferencesDialog.h"
#include "UserActivityLogger.h"

PreferencesDialog::PreferencesDialog() :
    QDialog(Application::getInstance()->getWindow()) {
        
    setAttribute(Qt::WA_DeleteOnClose);

    ui.setupUi(this);
    loadPreferences();

    connect(ui.buttonBrowseHead, &QPushButton::clicked, this, &PreferencesDialog::openHeadModelBrowser);
    connect(ui.buttonBrowseBody, &QPushButton::clicked, this, &PreferencesDialog::openBodyModelBrowser);
    connect(ui.buttonBrowseLocation, &QPushButton::clicked, this, &PreferencesDialog::openSnapshotLocationBrowser);
    connect(ui.buttonBrowseScriptsLocation, &QPushButton::clicked, this, &PreferencesDialog::openScriptsLocationBrowser);
    connect(ui.buttonReloadDefaultScripts, &QPushButton::clicked,
            Application::getInstance(), &Application::loadDefaultScripts);
}

void PreferencesDialog::accept() {
    savePreferences();
    close();
}

void PreferencesDialog::setHeadUrl(QString modelUrl) {
    ui.faceURLEdit->setText(modelUrl);
}

void PreferencesDialog::setSkeletonUrl(QString modelUrl) {
    ui.skeletonURLEdit->setText(modelUrl);
}

void PreferencesDialog::openHeadModelBrowser() {
    ModelsBrowser modelBrowser(HEAD_MODEL);
    connect(&modelBrowser, &ModelsBrowser::selected, this, &PreferencesDialog::setHeadUrl);
    modelBrowser.browse();
}

void PreferencesDialog::openBodyModelBrowser() {
    ModelsBrowser modelBrowser(SKELETON_MODEL);
    connect(&modelBrowser, &ModelsBrowser::selected, this, &PreferencesDialog::setSkeletonUrl);
    modelBrowser.browse();
}

void PreferencesDialog::openSnapshotLocationBrowser() {
    QString dir = QFileDialog::getExistingDirectory(this, tr("Snapshots Location"),
                                                    QStandardPaths::writableLocation(QStandardPaths::DesktopLocation),
                                                    QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (!dir.isNull() && !dir.isEmpty()) {
        ui.snapshotLocationEdit->setText(dir);
    }
}

void PreferencesDialog::openScriptsLocationBrowser() {
    QString dir = QFileDialog::getExistingDirectory(this, tr("Scripts Location"),
                                                    ui.scriptsLocationEdit->text(),
                                                    QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (!dir.isNull() && !dir.isEmpty()) {
        ui.scriptsLocationEdit->setText(dir);
    }
}

void PreferencesDialog::resizeEvent(QResizeEvent *resizeEvent) {
    
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

void PreferencesDialog::loadPreferences() {
    
    MyAvatar* myAvatar = Application::getInstance()->getAvatar();
    Menu* menuInstance = Menu::getInstance();

    _displayNameString = myAvatar->getDisplayName();
    ui.displayNameEdit->setText(_displayNameString);

    _faceURLString = myAvatar->getHead()->getFaceModel().getURL().toString();
    ui.faceURLEdit->setText(_faceURLString);

    _skeletonURLString = myAvatar->getSkeletonModel().getURL().toString();
    ui.skeletonURLEdit->setText(_skeletonURLString);
    
    ui.sendDataCheckBox->setChecked(!menuInstance->isOptionChecked(MenuOption::DisableActivityLogger));

    ui.snapshotLocationEdit->setText(menuInstance->getSnapshotsLocation());

    ui.scriptsLocationEdit->setText(menuInstance->getScriptsLocation());

    ui.pupilDilationSlider->setValue(myAvatar->getHead()->getPupilDilation() *
                                     ui.pupilDilationSlider->maximum());
    
    ui.faceshiftEyeDeflectionSider->setValue(menuInstance->getFaceshiftEyeDeflection() *
                                             ui.faceshiftEyeDeflectionSider->maximum());
    
    const InboundAudioStream::Settings& streamSettings = menuInstance->getReceivedAudioStreamSettings();

    ui.dynamicJitterBuffersCheckBox->setChecked(streamSettings._dynamicJitterBuffers);
    ui.staticDesiredJitterBufferFramesSpin->setValue(streamSettings._staticDesiredJitterBufferFrames);
    ui.maxFramesOverDesiredSpin->setValue(streamSettings._maxFramesOverDesired);
    ui.useStdevForJitterCalcCheckBox->setChecked(streamSettings._useStDevForJitterCalc);
    ui.windowStarveThresholdSpin->setValue(streamSettings._windowStarveThreshold);
    ui.windowSecondsForDesiredCalcOnTooManyStarvesSpin->setValue(streamSettings._windowSecondsForDesiredCalcOnTooManyStarves);
    ui.windowSecondsForDesiredReductionSpin->setValue(streamSettings._windowSecondsForDesiredReduction);
    ui.repetitionWithFadeCheckBox->setChecked(streamSettings._repetitionWithFade);

    ui.realWorldFieldOfViewSpin->setValue(menuInstance->getRealWorldFieldOfView());

    ui.fieldOfViewSpin->setValue(menuInstance->getFieldOfView());
    
    ui.leanScaleSpin->setValue(myAvatar->getLeanScale());
    
    ui.avatarScaleSpin->setValue(myAvatar->getScale());
    
    ui.maxVoxelsSpin->setValue(menuInstance->getMaxVoxels());
    
    ui.maxVoxelsPPSSpin->setValue(menuInstance->getMaxVoxelPacketsPerSecond());

    ui.oculusUIAngularSizeSpin->setValue(menuInstance->getOculusUIAngularSize());

    ui.sixenseReticleMoveSpeedSpin->setValue(menuInstance->getSixenseReticleMoveSpeed());

    ui.invertSixenseButtonsCheckBox->setChecked(menuInstance->getInvertSixenseButtons());

}

void PreferencesDialog::savePreferences() {
    
    MyAvatar* myAvatar = Application::getInstance()->getAvatar();
    bool shouldDispatchIdentityPacket = false;
    
    QString displayNameStr(ui.displayNameEdit->text());
    if (displayNameStr != _displayNameString) {
        myAvatar->setDisplayName(displayNameStr);
        UserActivityLogger::getInstance().changedDisplayName(displayNameStr);
        shouldDispatchIdentityPacket = true;
    }
    
    QUrl faceModelURL(ui.faceURLEdit->text());
    if (faceModelURL.toString() != _faceURLString) {
        // change the faceModelURL in the profile, it will also update this user's BlendFace
        myAvatar->setFaceModelURL(faceModelURL);
        UserActivityLogger::getInstance().changedModel("head", faceModelURL.toString());
        shouldDispatchIdentityPacket = true;
    }

    QUrl skeletonModelURL(ui.skeletonURLEdit->text());
    if (skeletonModelURL.toString() != _skeletonURLString) {
        // change the skeletonModelURL in the profile, it will also update this user's Body
        myAvatar->setSkeletonModelURL(skeletonModelURL);
        UserActivityLogger::getInstance().changedModel("skeleton", skeletonModelURL.toString());
        shouldDispatchIdentityPacket = true;
    }
    
    if (shouldDispatchIdentityPacket) {
        myAvatar->sendIdentityPacket();
        Application::getInstance()->bumpSettings();
    }
    
    if (!Menu::getInstance()->isOptionChecked(MenuOption::DisableActivityLogger)
        != ui.sendDataCheckBox->isChecked()) {
        Menu::getInstance()->triggerOption(MenuOption::DisableActivityLogger);
    }

    if (!ui.snapshotLocationEdit->text().isEmpty() && QDir(ui.snapshotLocationEdit->text()).exists()) {
        Menu::getInstance()->setSnapshotsLocation(ui.snapshotLocationEdit->text());
    }

    if (!ui.scriptsLocationEdit->text().isEmpty() && QDir(ui.scriptsLocationEdit->text()).exists()) {
        Menu::getInstance()->setScriptsLocation(ui.scriptsLocationEdit->text());
    }

    myAvatar->getHead()->setPupilDilation(ui.pupilDilationSlider->value() / (float)ui.pupilDilationSlider->maximum());
    myAvatar->setLeanScale(ui.leanScaleSpin->value());
    myAvatar->setClampedTargetScale(ui.avatarScaleSpin->value());
    
    Application::getInstance()->getVoxels()->setMaxVoxels(ui.maxVoxelsSpin->value());
    Application::getInstance()->resizeGL(Application::getInstance()->getGLWidget()->width(),
                                         Application::getInstance()->getGLWidget()->height());

    Menu::getInstance()->setRealWorldFieldOfView(ui.realWorldFieldOfViewSpin->value());
    
    Menu::getInstance()->setFieldOfView(ui.fieldOfViewSpin->value());

    Menu::getInstance()->setFaceshiftEyeDeflection(ui.faceshiftEyeDeflectionSider->value() /
                                                     (float)ui.faceshiftEyeDeflectionSider->maximum());
    Menu::getInstance()->setMaxVoxelPacketsPerSecond(ui.maxVoxelsPPSSpin->value());

    Menu::getInstance()->setOculusUIAngularSize(ui.oculusUIAngularSizeSpin->value());

    Menu::getInstance()->setSixenseReticleMoveSpeed(ui.sixenseReticleMoveSpeedSpin->value());

    Menu::getInstance()->setInvertSixenseButtons(ui.invertSixenseButtonsCheckBox->isChecked());

    InboundAudioStream::Settings streamSettings;
    streamSettings._dynamicJitterBuffers = ui.dynamicJitterBuffersCheckBox->isChecked();
    streamSettings._staticDesiredJitterBufferFrames = ui.staticDesiredJitterBufferFramesSpin->value();
    streamSettings._maxFramesOverDesired = ui.maxFramesOverDesiredSpin->value();
    streamSettings._useStDevForJitterCalc = ui.useStdevForJitterCalcCheckBox->isChecked();
    streamSettings._windowStarveThreshold = ui.windowStarveThresholdSpin->value();
    streamSettings._windowSecondsForDesiredCalcOnTooManyStarves = ui.windowSecondsForDesiredCalcOnTooManyStarvesSpin->value();
    streamSettings._windowSecondsForDesiredReduction = ui.windowSecondsForDesiredReductionSpin->value();
    streamSettings._repetitionWithFade = ui.repetitionWithFadeCheckBox->isChecked();

    Menu::getInstance()->setReceivedAudioStreamSettings(streamSettings);
    Application::getInstance()->getAudio()->setReceivedAudioStreamSettings(streamSettings);

    Application::getInstance()->resizeGL(Application::getInstance()->getGLWidget()->width(),
                                         Application::getInstance()->getGLWidget()->height());
}
