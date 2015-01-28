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

#include <QFileDialog>

#include <devices/Faceshift.h>
#include <devices/SixenseManager.h>

#include "Application.h"
#include "Audio.h"
#include "MainWindow.h"
#include "Menu.h"
#include "ModelsBrowser.h"
#include "PreferencesDialog.h"
#include "Snapshot.h"
#include "UserActivityLogger.h"

const int PREFERENCES_HEIGHT_PADDING = 20;

PreferencesDialog::PreferencesDialog(QWidget* parent) :
    QDialog(parent) {
        
    setAttribute(Qt::WA_DeleteOnClose);

    ui.setupUi(this);
    loadPreferences();

    ui.outputBufferSizeSpinner->setMinimum(MIN_AUDIO_OUTPUT_BUFFER_SIZE_FRAMES);
    ui.outputBufferSizeSpinner->setMaximum(MAX_AUDIO_OUTPUT_BUFFER_SIZE_FRAMES);

    connect(ui.buttonBrowseHead, &QPushButton::clicked, this, &PreferencesDialog::openHeadModelBrowser);
    connect(ui.buttonBrowseBody, &QPushButton::clicked, this, &PreferencesDialog::openBodyModelBrowser);
    connect(ui.buttonBrowseLocation, &QPushButton::clicked, this, &PreferencesDialog::openSnapshotLocationBrowser);
    connect(ui.buttonBrowseScriptsLocation, &QPushButton::clicked, this, &PreferencesDialog::openScriptsLocationBrowser);
    connect(ui.buttonReloadDefaultScripts, &QPushButton::clicked,
            Application::getInstance(), &Application::loadDefaultScripts);
    // move dialog to left side
    move(parentWidget()->geometry().topLeft());
    setFixedHeight(parentWidget()->size().height() - PREFERENCES_HEIGHT_PADDING);
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

    ui.snapshotLocationEdit->setText(SettingHandles::snapshotsLocation.get());

    ui.scriptsLocationEdit->setText(qApp->getScriptsLocation());

    ui.pupilDilationSlider->setValue(myAvatar->getHead()->getPupilDilation() *
                                     ui.pupilDilationSlider->maximum());
    
    auto faceshift = DependencyManager::get<Faceshift>();
    ui.faceshiftEyeDeflectionSider->setValue(faceshift->getEyeDeflection() *
                                             ui.faceshiftEyeDeflectionSider->maximum());
    
    ui.faceshiftHostnameEdit->setText(faceshift->getHostname());
    
    auto audio = DependencyManager::get<Audio>();
    MixedProcessedAudioStream& stream = audio->getReceivedAudioStream();

    ui.dynamicJitterBuffersCheckBox->setChecked(stream.getDynamicJitterBuffers());
    ui.staticDesiredJitterBufferFramesSpin->setValue(stream.getDesiredJitterBufferFrames());
    ui.maxFramesOverDesiredSpin->setValue(stream.getMaxFramesOverDesired());
    ui.useStdevForJitterCalcCheckBox->setChecked(stream.getUseStDevForJitterCalc());
    ui.windowStarveThresholdSpin->setValue(stream.getWindowStarveThreshold());
    ui.windowSecondsForDesiredCalcOnTooManyStarvesSpin->setValue(
            stream.getWindowSecondsForDesiredCalcOnTooManyStarves());
    ui.windowSecondsForDesiredReductionSpin->setValue(stream.getWindowSecondsForDesiredReduction());
    ui.repetitionWithFadeCheckBox->setChecked(stream.getRepetitionWithFade());

    ui.outputBufferSizeSpinner->setValue(audio->getOutputBufferSize());

    ui.outputStarveDetectionCheckBox->setChecked(audio->getOutputStarveDetectionEnabled());
    ui.outputStarveDetectionThresholdSpinner->setValue(audio->getOutputStarveDetectionThreshold());
    ui.outputStarveDetectionPeriodSpinner->setValue(audio->getOutputStarveDetectionPeriod());

    ui.realWorldFieldOfViewSpin->setValue(qApp->getViewFrustum()->getRealWorldFieldOfView());

    ui.fieldOfViewSpin->setValue(qApp->getViewFrustum()->getFieldOfView());
    
    ui.leanScaleSpin->setValue(myAvatar->getLeanScale());
    
    ui.avatarScaleSpin->setValue(myAvatar->getScale());
    
    ui.maxOctreePPSSpin->setValue(qApp->getOctreeQuery().getMaxOctreePacketsPerSecond());

    ui.oculusUIAngularSizeSpin->setValue(qApp->getApplicationOverlay().getOculusUIAngularSize());

    SixenseManager& sixense = SixenseManager::getInstance();
    ui.sixenseReticleMoveSpeedSpin->setValue(sixense.getReticleMoveSpeed());
    ui.invertSixenseButtonsCheckBox->setChecked(sixense.getInvertButtons());

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
    QString faceModelURLString = faceModelURL.toString();
    if (faceModelURLString != _faceURLString) {
        if (faceModelURLString.isEmpty() || faceModelURLString.toLower().endsWith(".fst")) {
            // change the faceModelURL in the profile, it will also update this user's BlendFace
            myAvatar->setFaceModelURL(faceModelURL);
            UserActivityLogger::getInstance().changedModel("head", faceModelURLString);
            shouldDispatchIdentityPacket = true;
        } else {
            qDebug() << "ERROR: Head model not FST or blank - " << faceModelURLString;
        }
    }

    QUrl skeletonModelURL(ui.skeletonURLEdit->text());
    QString skeletonModelURLString = skeletonModelURL.toString();
    if (skeletonModelURLString != _skeletonURLString) {
        if (skeletonModelURLString.isEmpty() || skeletonModelURLString.toLower().endsWith(".fst")) {
            // change the skeletonModelURL in the profile, it will also update this user's Body
            myAvatar->setSkeletonModelURL(skeletonModelURL);
            UserActivityLogger::getInstance().changedModel("skeleton", skeletonModelURLString);
            shouldDispatchIdentityPacket = true;
        } else {
            qDebug() << "ERROR: Skeleton model not FST or blank - " << skeletonModelURLString;
        }
    }
    
    if (shouldDispatchIdentityPacket) {
        myAvatar->sendIdentityPacket();
    }
    
    if (!Menu::getInstance()->isOptionChecked(MenuOption::DisableActivityLogger)
        != ui.sendDataCheckBox->isChecked()) {
        Menu::getInstance()->triggerOption(MenuOption::DisableActivityLogger);
    }

    if (!ui.snapshotLocationEdit->text().isEmpty() && QDir(ui.snapshotLocationEdit->text()).exists()) {
        SettingHandles::snapshotsLocation.set(ui.snapshotLocationEdit->text());
    }

    if (!ui.scriptsLocationEdit->text().isEmpty() && QDir(ui.scriptsLocationEdit->text()).exists()) {
        qApp->setScriptsLocation(ui.scriptsLocationEdit->text());
    }

    myAvatar->getHead()->setPupilDilation(ui.pupilDilationSlider->value() / (float)ui.pupilDilationSlider->maximum());
    myAvatar->setLeanScale(ui.leanScaleSpin->value());
    myAvatar->setClampedTargetScale(ui.avatarScaleSpin->value());
    
    auto glCanvas = DependencyManager::get<GLCanvas>();
    Application::getInstance()->resizeGL(glCanvas->width(), glCanvas->height());

    qApp->getViewFrustum()->setRealWorldFieldOfView(ui.realWorldFieldOfViewSpin->value());
    
    qApp->getViewFrustum()->setFieldOfView(ui.fieldOfViewSpin->value());
    
    auto faceshift = DependencyManager::get<Faceshift>();
    faceshift->setEyeDeflection(ui.faceshiftEyeDeflectionSider->value() /
                                (float)ui.faceshiftEyeDeflectionSider->maximum());
    
    faceshift->setHostname(ui.faceshiftHostnameEdit->text());
    
    qApp->getOctreeQuery().setMaxOctreePacketsPerSecond(ui.maxOctreePPSSpin->value());

    qApp->getApplicationOverlay().setOculusUIAngularSize(ui.oculusUIAngularSizeSpin->value());
    
    SixenseManager& sixense = SixenseManager::getInstance();
    sixense.setReticleMoveSpeed(ui.sixenseReticleMoveSpeedSpin->value());
    sixense.setInvertButtons(ui.invertSixenseButtonsCheckBox->isChecked());

    auto audio = DependencyManager::get<Audio>();
    MixedProcessedAudioStream& stream = audio->getReceivedAudioStream();
    
    stream.setDynamicJitterBuffers(ui.dynamicJitterBuffersCheckBox->isChecked());
    stream.setStaticDesiredJitterBufferFrames(ui.staticDesiredJitterBufferFramesSpin->value());
    stream.setMaxFramesOverDesired(ui.maxFramesOverDesiredSpin->value());
    stream.setUseStDevForJitterCalc(ui.useStdevForJitterCalcCheckBox->isChecked());
    stream.setWindowStarveThreshold(ui.windowStarveThresholdSpin->value());
    stream.setWindowSecondsForDesiredCalcOnTooManyStarves(ui.windowSecondsForDesiredCalcOnTooManyStarvesSpin->value());
    stream.setWindowSecondsForDesiredReduction(ui.windowSecondsForDesiredReductionSpin->value());
    stream.setRepetitionWithFade(ui.repetitionWithFadeCheckBox->isChecked());

    QMetaObject::invokeMethod(audio.data(), "setOutputBufferSize", Q_ARG(int, ui.outputBufferSizeSpinner->value()));

    audio->setOutputStarveDetectionEnabled(ui.outputStarveDetectionCheckBox->isChecked());
    audio->setOutputStarveDetectionThreshold(ui.outputStarveDetectionThresholdSpinner->value());
    audio->setOutputStarveDetectionPeriod(ui.outputStarveDetectionPeriodSpinner->value());

    Application::getInstance()->resizeGL(glCanvas->width(), glCanvas->height());
}
