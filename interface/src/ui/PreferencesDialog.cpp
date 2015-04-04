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
#include <devices/Faceshift.h>
#include <devices/SixenseManager.h>
#include <NetworkingConstants.h>

#include "Application.h"
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

    connect(ui.buttonBrowseHead, &QPushButton::clicked, this, &PreferencesDialog::openHeadModelBrowser);
    connect(ui.buttonBrowseBody, &QPushButton::clicked, this, &PreferencesDialog::openBodyModelBrowser);
    connect(ui.buttonBrowseFullAvatar, &QPushButton::clicked, this, &PreferencesDialog::openFullAvatarModelBrowser);

    connect(ui.buttonBrowseLocation, &QPushButton::clicked, this, &PreferencesDialog::openSnapshotLocationBrowser);
    connect(ui.buttonBrowseScriptsLocation, &QPushButton::clicked, this, &PreferencesDialog::openScriptsLocationBrowser);
    connect(ui.buttonReloadDefaultScripts, &QPushButton::clicked, Application::getInstance(), &Application::loadDefaultScripts);

    connect(ui.useSeparateBodyAndHead, &QRadioButton::clicked, this, &PreferencesDialog::useSeparateBodyAndHead);
    connect(ui.useFullAvatar, &QRadioButton::clicked, this, &PreferencesDialog::useFullAvatar);
    
    
    connect(Application::getInstance(), &Application::headURLChanged, this, &PreferencesDialog::headURLChanged);
    connect(Application::getInstance(), &Application::bodyURLChanged, this, &PreferencesDialog::bodyURLChanged);
    connect(Application::getInstance(), &Application::fullAvatarURLChanged, this, &PreferencesDialog::fullAvatarURLChanged);

    // move dialog to left side
    move(parentWidget()->geometry().topLeft());
    setFixedHeight(parentWidget()->size().height() - PREFERENCES_HEIGHT_PADDING);

    auto myAvatar = DependencyManager::get<AvatarManager>()->getMyAvatar();
    
    ui.bodyNameLabel->setText("Body - " + myAvatar->getBodyModelName());
    ui.headNameLabel->setText("Head - " + myAvatar->getHeadModelName());
    ui.fullAvatarNameLabel->setText("Full Avatar - " + myAvatar->getFullAvartarModelName());

    UIUtil::scaleWidgetFontSizes(this);
}

void PreferencesDialog::useSeparateBodyAndHead(bool checked) {
    setUseFullAvatar(!checked);

    QUrl headURL(ui.faceURLEdit->text());
    QUrl bodyURL(ui.skeletonURLEdit->text());
    
    DependencyManager::get<AvatarManager>()->getMyAvatar()->useHeadAndBodyURLs(headURL, bodyURL);
}

void PreferencesDialog::useFullAvatar(bool checked) {
    setUseFullAvatar(checked);
    QUrl fullAvatarURL(ui.fullAvatarURLEdit->text());
    DependencyManager::get<AvatarManager>()->getMyAvatar()->useFullAvatarURL(fullAvatarURL);
}

void PreferencesDialog::setUseFullAvatar(bool useFullAvatar) {
    _useFullAvatar = useFullAvatar;
    ui.faceURLEdit->setEnabled(!_useFullAvatar);
    ui.skeletonURLEdit->setEnabled(!_useFullAvatar);
    ui.fullAvatarURLEdit->setEnabled(_useFullAvatar);
    
    ui.useFullAvatar->setChecked(_useFullAvatar);
    ui.useSeparateBodyAndHead->setChecked(!_useFullAvatar);
}

void PreferencesDialog::headURLChanged(const QString& newValue, const QString& modelName) {
    ui.faceURLEdit->setText(newValue);
    setUseFullAvatar(false);
    ui.headNameLabel->setText("Head - " + modelName);
}

void PreferencesDialog::bodyURLChanged(const QString& newValue, const QString& modelName) {
    ui.skeletonURLEdit->setText(newValue);
    setUseFullAvatar(false);
    ui.bodyNameLabel->setText("Body - " + modelName);
}

void PreferencesDialog::fullAvatarURLChanged(const QString& newValue, const QString& modelName) {
    ui.fullAvatarURLEdit->setText(newValue);
    setUseFullAvatar(true);
    ui.fullAvatarNameLabel->setText("Full Avatar - " + modelName);
}

void PreferencesDialog::accept() {
    savePreferences();
    close();
    delete _marketplaceWindow;
    _marketplaceWindow = NULL;
}

void PreferencesDialog::setHeadUrl(QString modelUrl) {
    ui.faceURLEdit->setText(modelUrl);
}

void PreferencesDialog::setSkeletonUrl(QString modelUrl) {
    ui.skeletonURLEdit->setText(modelUrl);
}

void PreferencesDialog::openFullAvatarModelBrowser() {
    auto MARKETPLACE_URL = NetworkingConstants::METAVERSE_SERVER_URL.toString() + "/marketplace?category=avatars";
    auto WIDTH = 900;
    auto HEIGHT = 700;
    if (!_marketplaceWindow) {
        _marketplaceWindow = new WebWindowClass("Marketplace", MARKETPLACE_URL, WIDTH, HEIGHT, false);
    }
    _marketplaceWindow->setVisible(true);
}

void PreferencesDialog::openHeadModelBrowser() {
    auto MARKETPLACE_URL = NetworkingConstants::METAVERSE_SERVER_URL.toString() + "/marketplace?category=avatars";
    auto WIDTH = 900;
    auto HEIGHT = 700;
    if (!_marketplaceWindow) {
        _marketplaceWindow = new WebWindowClass("Marketplace", MARKETPLACE_URL, WIDTH, HEIGHT, false);
    }
    _marketplaceWindow->setVisible(true);
}

void PreferencesDialog::openBodyModelBrowser() {
    auto MARKETPLACE_URL = NetworkingConstants::METAVERSE_SERVER_URL.toString() + "/marketplace?category=avatars";
    auto WIDTH = 900;
    auto HEIGHT = 700;
    if (!_marketplaceWindow) {
        _marketplaceWindow = new WebWindowClass("Marketplace", MARKETPLACE_URL, WIDTH, HEIGHT, false);
    }
    _marketplaceWindow->setVisible(true);
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
    
    MyAvatar* myAvatar = DependencyManager::get<AvatarManager>()->getMyAvatar();
    Menu* menuInstance = Menu::getInstance();

    _displayNameString = myAvatar->getDisplayName();
    ui.displayNameEdit->setText(_displayNameString);


    _useFullAvatar = myAvatar->getUseFullAvatar();
    _fullAvatarURLString = myAvatar->getFullAvatarURLFromPreferences().toString();
    _headURLString = myAvatar->getHeadURLFromPreferences().toString();
    _bodyURLString = myAvatar->getBodyURLFromPreferences().toString();

    ui.fullAvatarURLEdit->setText(_fullAvatarURLString);
    ui.faceURLEdit->setText(_headURLString);
    ui.skeletonURLEdit->setText(_bodyURLString);
    setUseFullAvatar(_useFullAvatar);
    
    // TODO: load the names for the models.
    
    ui.sendDataCheckBox->setChecked(!menuInstance->isOptionChecked(MenuOption::DisableActivityLogger));

    ui.snapshotLocationEdit->setText(Snapshot::snapshotsLocation.get());

    ui.scriptsLocationEdit->setText(qApp->getScriptsLocation());

    ui.pupilDilationSlider->setValue(myAvatar->getHead()->getPupilDilation() *
                                     ui.pupilDilationSlider->maximum());
    
    auto faceshift = DependencyManager::get<Faceshift>();
    ui.faceshiftEyeDeflectionSider->setValue(faceshift->getEyeDeflection() *
                                             ui.faceshiftEyeDeflectionSider->maximum());
    
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

    ui.realWorldFieldOfViewSpin->setValue(DependencyManager::get<AvatarManager>()->getMyAvatar()->getRealWorldFieldOfView());

    ui.fieldOfViewSpin->setValue(qApp->getFieldOfView());
    
    ui.leanScaleSpin->setValue(myAvatar->getLeanScale());
    
    ui.avatarScaleSpin->setValue(myAvatar->getScale());
    
    ui.maxOctreePPSSpin->setValue(qApp->getOctreeQuery().getMaxOctreePacketsPerSecond());

    ui.oculusUIAngularSizeSpin->setValue(qApp->getApplicationOverlay().getOculusUIAngularSize());

    SixenseManager& sixense = SixenseManager::getInstance();
    ui.sixenseReticleMoveSpeedSpin->setValue(sixense.getReticleMoveSpeed());
    ui.invertSixenseButtonsCheckBox->setChecked(sixense.getInvertButtons());

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
    
    if (shouldDispatchIdentityPacket) {
        myAvatar->sendIdentityPacket();
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
    
    auto glCanvas = Application::getInstance()->getGLWidget();
    Application::getInstance()->resizeGL(glCanvas->width(), glCanvas->height());

    DependencyManager::get<AvatarManager>()->getMyAvatar()->setRealWorldFieldOfView(ui.realWorldFieldOfViewSpin->value());
    
    qApp->setFieldOfView(ui.fieldOfViewSpin->value());
    
    auto faceshift = DependencyManager::get<Faceshift>();
    faceshift->setEyeDeflection(ui.faceshiftEyeDeflectionSider->value() /
                                (float)ui.faceshiftEyeDeflectionSider->maximum());
    
    faceshift->setHostname(ui.faceshiftHostnameEdit->text());
    
    qApp->getOctreeQuery().setMaxOctreePacketsPerSecond(ui.maxOctreePPSSpin->value());

    qApp->getApplicationOverlay().setOculusUIAngularSize(ui.oculusUIAngularSizeSpin->value());
    
    SixenseManager& sixense = SixenseManager::getInstance();
    sixense.setReticleMoveSpeed(ui.sixenseReticleMoveSpeedSpin->value());
    sixense.setInvertButtons(ui.invertSixenseButtonsCheckBox->isChecked());

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

    Application::getInstance()->resizeGL(glCanvas->width(), glCanvas->height());

    // LOD items
    auto lodManager = DependencyManager::get<LODManager>();
    lodManager->setDesktopLODDecreaseFPS(ui.desktopMinimumFPSSpin->value());
    lodManager->setHMDLODDecreaseFPS(ui.hmdMinimumFPSSpin->value());
}
