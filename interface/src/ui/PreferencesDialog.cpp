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
#include <QFont>

#include <AudioClient.h>
#include <avatar/AvatarManager.h>
#include <devices/DdeFaceTracker.h>
#include <devices/Faceshift.h>
#include <input-plugins/SixenseManager.h> // TODO: This should be replaced with InputDevice/InputPlugin, or something similar
#include <NetworkingConstants.h>

#include "Application.h"
#include "DialogsManager.h"
#include "MainWindow.h"
#include "LODManager.h"
#include "Menu.h"
#include "PreferencesDialog.h"
#include "Snapshot.h"
#include "UserActivityLogger.h"
#include "UIUtil.h"


const int PREFERENCES_HEIGHT_PADDING = 20;

PreferencesDialog::PreferencesDialog(QWidget* parent) :
    QDialog(parent) {
        
    setAttribute(Qt::WA_DeleteOnClose);

    ui.setupUi(this);
    loadPreferences();

    ui.outputBufferSizeSpinner->setMinimum(MIN_AUDIO_OUTPUT_BUFFER_SIZE_FRAMES);
    ui.outputBufferSizeSpinner->setMaximum(MAX_AUDIO_OUTPUT_BUFFER_SIZE_FRAMES);

    connect(ui.buttonBrowseLocation, &QPushButton::clicked, this, &PreferencesDialog::openSnapshotLocationBrowser);
    connect(ui.buttonBrowseScriptsLocation, &QPushButton::clicked, this, &PreferencesDialog::openScriptsLocationBrowser);
    connect(ui.buttonReloadDefaultScripts, &QPushButton::clicked, qApp, &Application::loadDefaultScripts);

    connect(ui.buttonChangeAppearance, &QPushButton::clicked, this, &PreferencesDialog::openFullAvatarModelBrowser);
    connect(ui.appearanceDescription, &QLineEdit::textChanged, this, [this](const QString& url) {
        DependencyManager::get<AvatarManager>()->getMyAvatar()->useFullAvatarURL(url, "");
        this->fullAvatarURLChanged(url, "");
    });
    connect(qApp, &Application::fullAvatarURLChanged, this, &PreferencesDialog::fullAvatarURLChanged);

    // move dialog to left side
    move(parentWidget()->geometry().topLeft());
    setFixedHeight(parentWidget()->size().height() - PREFERENCES_HEIGHT_PADDING);

    UIUtil::scaleWidgetFontSizes(this);
}

void PreferencesDialog::fullAvatarURLChanged(const QString& newValue, const QString& modelName) {
    ui.appearanceDescription->setText(newValue);
    const QString APPEARANCE_LABEL_TEXT("Appearance: ");
    ui.appearanceLabel->setText(APPEARANCE_LABEL_TEXT + modelName);
}

void PreferencesDialog::accept() {
    MyAvatar* myAvatar = DependencyManager::get<AvatarManager>()->getMyAvatar();
    _lastGoodAvatarURL = myAvatar->getFullAvatarURLFromPreferences();
    _lastGoodAvatarName = myAvatar->getFullAvatarModelName();
    savePreferences();
    close();
    delete _marketplaceWindow;
    _marketplaceWindow = NULL;
}

void PreferencesDialog::restoreLastGoodAvatar() {
    const QString& url = _lastGoodAvatarURL.toString();
    fullAvatarURLChanged(url, _lastGoodAvatarName);
    DependencyManager::get<AvatarManager>()->getMyAvatar()->useFullAvatarURL(url, _lastGoodAvatarName);
}

void PreferencesDialog::reject() {
    restoreLastGoodAvatar();
    QDialog::reject();
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
void PreferencesDialog::openFullAvatarModelBrowser() {
    const auto MARKETPLACE_URL = NetworkingConstants::METAVERSE_SERVER_URL.toString() + "/marketplace?category=avatars";
    const auto WIDTH = 900;
    const auto HEIGHT = 700;
    if (!_marketplaceWindow) {
        _marketplaceWindow = new WebWindowClass("Marketplace", MARKETPLACE_URL, WIDTH, HEIGHT, false);
    }
    _marketplaceWindow->setVisible(true);

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
    
    MyAvatar* myAvatar = DependencyManager::get<AvatarManager>()->getMyAvatar();
    Menu* menuInstance = Menu::getInstance();

    _displayNameString = myAvatar->getDisplayName();
    ui.displayNameEdit->setText(_displayNameString);

    ui.collisionSoundURLEdit->setText(myAvatar->getCollisionSoundURL());

    _lastGoodAvatarURL = myAvatar->getFullAvatarURLFromPreferences();
    _lastGoodAvatarName = myAvatar->getFullAvatarModelName();
    fullAvatarURLChanged(_lastGoodAvatarURL.toString(), _lastGoodAvatarName);

    ui.sendDataCheckBox->setChecked(!menuInstance->isOptionChecked(MenuOption::DisableActivityLogger));

    ui.snapshotLocationEdit->setText(Snapshot::snapshotsLocation.get());

    ui.scriptsLocationEdit->setText(qApp->getScriptsLocation());

    ui.pupilDilationSlider->setValue(myAvatar->getHead()->getPupilDilation() *
                                     ui.pupilDilationSlider->maximum());
    
    auto dde = DependencyManager::get<DdeFaceTracker>();
    ui.ddeEyeClosingThresholdSlider->setValue(dde->getEyeClosingThreshold() * 
                                              ui.ddeEyeClosingThresholdSlider->maximum());

    ui.faceTrackerEyeDeflectionSider->setValue(FaceTracker::getEyeDeflection() *
                                               ui.faceTrackerEyeDeflectionSider->maximum());
    
    auto faceshift = DependencyManager::get<Faceshift>();
    ui.faceshiftHostnameEdit->setText(faceshift->getHostname());

    auto audio = DependencyManager::get<AudioClient>();
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

    ui.realWorldFieldOfViewSpin->setValue(myAvatar->getRealWorldFieldOfView());

    ui.fieldOfViewSpin->setValue(qApp->getFieldOfView());
    
    ui.leanScaleSpin->setValue(myAvatar->getLeanScale());
    
    ui.avatarScaleSpin->setValue(myAvatar->getScale());
    ui.avatarAnimationEdit->setText(myAvatar->getAnimGraphUrl());
    
    ui.maxOctreePPSSpin->setValue(qApp->getMaxOctreePacketsPerSecond());

#if 0
    ui.oculusUIAngularSizeSpin->setValue(qApp->getApplicationCompositor().getHmdUIAngularSize());
#endif

    ui.sixenseReticleMoveSpeedSpin->setValue(InputDevice::getReticleMoveSpeed());

    // LOD items
    auto lodManager = DependencyManager::get<LODManager>();
    ui.desktopMinimumFPSSpin->setValue(lodManager->getDesktopLODDecreaseFPS());
    ui.hmdMinimumFPSSpin->setValue(lodManager->getHMDLODDecreaseFPS());
}

void PreferencesDialog::savePreferences() {
    
    MyAvatar* myAvatar = DependencyManager::get<AvatarManager>()->getMyAvatar();

    bool shouldDispatchIdentityPacket = false;
    
    QString displayNameStr(ui.displayNameEdit->text());
    if (displayNameStr != _displayNameString) {
        myAvatar->setDisplayName(displayNameStr);
        UserActivityLogger::getInstance().changedDisplayName(displayNameStr);
        shouldDispatchIdentityPacket = true;
    }

    if (shouldDispatchIdentityPacket) {
        myAvatar->sendIdentityPacket();
    }
    
    myAvatar->setCollisionSoundURL(ui.collisionSoundURLEdit->text());

    // MyAvatar persists its own data. If it doesn't agree with what the user has explicitly accepted, set it back to old values.
    if (_lastGoodAvatarURL != myAvatar->getFullAvatarURLFromPreferences()) {
        restoreLastGoodAvatar();
    }

    if (!Menu::getInstance()->isOptionChecked(MenuOption::DisableActivityLogger)
        != ui.sendDataCheckBox->isChecked()) {
        Menu::getInstance()->triggerOption(MenuOption::DisableActivityLogger);
    }

    if (!ui.snapshotLocationEdit->text().isEmpty() && QDir(ui.snapshotLocationEdit->text()).exists()) {
        Snapshot::snapshotsLocation.set(ui.snapshotLocationEdit->text());
    }

    if (!ui.scriptsLocationEdit->text().isEmpty() && QDir(ui.scriptsLocationEdit->text()).exists()) {
        qApp->setScriptsLocation(ui.scriptsLocationEdit->text());
    }

    myAvatar->getHead()->setPupilDilation(ui.pupilDilationSlider->value() / (float)ui.pupilDilationSlider->maximum());
    myAvatar->setLeanScale(ui.leanScaleSpin->value());
    myAvatar->setClampedTargetScale(ui.avatarScaleSpin->value());
    if (myAvatar->getAnimGraphUrl() != ui.avatarAnimationEdit->text()) { // If changed, destroy the old and start with the new
        bool isEnabled = myAvatar->getEnableAnimGraph();
        myAvatar->setEnableAnimGraph(false);
        myAvatar->setAnimGraphUrl(ui.avatarAnimationEdit->text());
        if (isEnabled) {
            myAvatar->setEnableAnimGraph(true);
        }
    }

    myAvatar->setRealWorldFieldOfView(ui.realWorldFieldOfViewSpin->value());
    
    qApp->setFieldOfView(ui.fieldOfViewSpin->value());
    
    auto dde = DependencyManager::get<DdeFaceTracker>();
    dde->setEyeClosingThreshold(ui.ddeEyeClosingThresholdSlider->value() / 
                                (float)ui.ddeEyeClosingThresholdSlider->maximum());

    FaceTracker::setEyeDeflection(ui.faceTrackerEyeDeflectionSider->value() /
                                (float)ui.faceTrackerEyeDeflectionSider->maximum());
    
    auto faceshift = DependencyManager::get<Faceshift>();
    faceshift->setHostname(ui.faceshiftHostnameEdit->text());
    
    qApp->setMaxOctreePacketsPerSecond(ui.maxOctreePPSSpin->value());

    qApp->getApplicationCompositor().setHmdUIAngularSize(ui.oculusUIAngularSizeSpin->value());
    
    InputDevice::setReticleMoveSpeed(ui.sixenseReticleMoveSpeedSpin->value());

    auto audio = DependencyManager::get<AudioClient>();
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

    qApp->resizeGL();

    // LOD items
    auto lodManager = DependencyManager::get<LODManager>();
    lodManager->setDesktopLODDecreaseFPS(ui.desktopMinimumFPSSpin->value());
    lodManager->setHMDLODDecreaseFPS(ui.hmdMinimumFPSSpin->value());
}
