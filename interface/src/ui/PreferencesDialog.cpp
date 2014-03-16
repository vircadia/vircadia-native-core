//
//  PreferencesDialog.cpp
//  hifi
//
//  Created by Stojce Slavkovski on 2/22/14.
//
//

#include "PreferencesDialog.h"
#include "Application.h"
#include "Menu.h"

PreferencesDialog::PreferencesDialog(QWidget* parent, Qt::WindowFlags flags) : FramelessDialog(parent, flags) {
    ui.setupUi(this);
    setStyleSheet("styles/preferences.qss");
    loadPreferences();
}

void PreferencesDialog::accept() {
    savePreferences();
    close();
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
    
    ui.pupilDilationSlider->setValue(myAvatar->getHead()->getPupilDilation() *
                                     ui.pupilDilationSlider->maximum());
    
    ui.faceshiftEyeDeflectionSider->setValue(menuInstance->getFaceshiftEyeDeflection() *
                                             ui.faceshiftEyeDeflectionSider->maximum());
    
    ui.audioJitterSpin->setValue(menuInstance->getAudioJitterBufferSamples());
    
    ui.fieldOfViewSpin->setValue(menuInstance->getFieldOfView());
    
    ui.leanScaleSpin->setValue(myAvatar->getLeanScale());
    
    ui.avatarScaleSpin->setValue(myAvatar->getScale());
    
    ui.maxVoxelsSpin->setValue(menuInstance->getMaxVoxels());
    
    ui.maxVoxelsPPSSpin->setValue(menuInstance->getMaxVoxelPacketsPerSecond());
}

void PreferencesDialog::savePreferences() {
    
    MyAvatar* myAvatar = Application::getInstance()->getAvatar();
    bool shouldDispatchIdentityPacket = false;
    
    QString displayNameStr(ui.displayNameEdit->text());
    if (displayNameStr != _displayNameString) {
        myAvatar->setDisplayName(displayNameStr);
        shouldDispatchIdentityPacket = true;
    }
    
    QUrl faceModelURL(ui.faceURLEdit->text());
    if (faceModelURL.toString() != _faceURLString) {
        // change the faceModelURL in the profile, it will also update this user's BlendFace
        myAvatar->setFaceModelURL(faceModelURL);
        shouldDispatchIdentityPacket = true;
    }

    QUrl skeletonModelURL(ui.skeletonURLEdit->text());
    if (skeletonModelURL.toString() != _skeletonURLString) {
        // change the skeletonModelURL in the profile, it will also update this user's Body
        myAvatar->setSkeletonModelURL(skeletonModelURL);
        shouldDispatchIdentityPacket = true;
    }
    
    if (shouldDispatchIdentityPacket) {
        myAvatar->sendIdentityPacket();
    }

    myAvatar->getHead()->setPupilDilation(ui.pupilDilationSlider->value() / (float)ui.pupilDilationSlider->maximum());
    myAvatar->setLeanScale(ui.leanScaleSpin->value());
    myAvatar->setClampedTargetScale(ui.avatarScaleSpin->value());
    
    Application::getInstance()->getVoxels()->setMaxVoxels(ui.maxVoxelsSpin->value());
    Application::getInstance()->resizeGL(Application::getInstance()->getGLWidget()->width(),
                                         Application::getInstance()->getGLWidget()->height());

    Menu::getInstance()->setFaceshiftEyeDeflection(ui.faceshiftEyeDeflectionSider->value() /
                                                     (float)ui.faceshiftEyeDeflectionSider->maximum());
    Menu::getInstance()->setMaxVoxelPacketsPerSecond(ui.maxVoxelsPPSSpin->value());

    Menu::getInstance()->setAudioJitterBufferSamples(ui.audioJitterSpin->value());
    
}
