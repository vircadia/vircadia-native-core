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
#include <NetworkingConstants.h>
#include <ScriptEngines.h>
#include <OffscreenUi.h>
#include <QtQml/QQmlContext>
#include <Preferences.h>

#include "Application.h"
#include "DialogsManager.h"
#include "MainWindow.h"
#include "LODManager.h"
#include "Menu.h"
#include "PreferencesDialog.h"
#include "Snapshot.h"
#include "UserActivityLogger.h"
#include "UIUtil.h"
#include "scripting/WebWindowClass.h"

void setupPreferences() {
    auto preferences = DependencyManager::get<Preferences>();

    MyAvatar* myAvatar = DependencyManager::get<AvatarManager>()->getMyAvatar();
    static const QString AVATAR_BASICS { "Avatar Basics" };
    {
        auto getter = [=]()->QString {return myAvatar->getDisplayName(); };
        auto setter = [=](const QString& value) { myAvatar->setDisplayName(value); };
        const QString label = "Avatar display name <font color=\"#909090\">(optional)</font>";
        auto preference = new EditPreference(AVATAR_BASICS, label, getter, setter);
        preference->setPlaceholderText("Not showing a name");
        preferences->addPreference(preference);
    }

    {
        auto getter = [=]()->QString {return myAvatar->getCollisionSoundURL(); };
        auto setter = [=](const QString& value) { myAvatar->setCollisionSoundURL(value); };
        const QString label = "Avatar collision sound URL <font color=\"#909090\">(optional)</font>";
        auto preference = new EditPreference(AVATAR_BASICS, label, getter, setter);
        preference->setPlaceholderText("Enter the URL of a sound to play when you bump into something");
        preferences->addPreference(preference);
    }

    {
        auto getter = [=]()->QString { return myAvatar->getFullAvatarURLFromPreferences().toString(); };
        auto setter = [=](const QString& value) { myAvatar->useFullAvatarURL(value, ""); };
        auto preference = new AvatarPreference(AVATAR_BASICS, "Appearance: ", getter, setter);
        preferences->addPreference(preference);
    }

    {
        auto getter = []()->QString { return Snapshot::snapshotsLocation.get(); };
        auto setter = [](const QString& value) { Snapshot::snapshotsLocation.set(value); };
        auto preference = new BrowsePreference("Snapshots", "Place my Snapshots here:", getter, setter);
        preferences->addPreference(preference);
    }

    // Scripts
    {
        auto getter = []()->QString { return DependencyManager::get<ScriptEngines>()->getScriptsLocation(); };
        auto setter = [](const QString& value) { DependencyManager::get<ScriptEngines>()->setScriptsLocation(value); };
        preferences->addPreference(new BrowsePreference("Scripts", "Load scripts from this directory:", getter, setter));
    }

    preferences->addPreference(new ButtonPreference("Scripts", "Load Default Scripts", [] {
        DependencyManager::get<ScriptEngines>()->loadDefaultScripts();
    }));

    {
        auto getter = []()->bool {return !Menu::getInstance()->isOptionChecked(MenuOption::DisableActivityLogger); };
        auto setter = [](bool value) { Menu::getInstance()->setIsOptionChecked(MenuOption::DisableActivityLogger, !value); };
        preferences->addPreference(new CheckPreference("Privacy", "Send Data", getter, setter));
    }
    
    static const QString LOD_TUNING("Level of Detail Tuning");
    CheckPreference* acuityToggle;
    {
        auto getter = []()->bool { return DependencyManager::get<LODManager>()->getUseAcuity(); };
        auto setter = [](bool value) { DependencyManager::get<LODManager>()->setUseAcuity(value); };
        preferences->addPreference(acuityToggle = new CheckPreference(LOD_TUNING, "Render based on visual acuity", getter, setter));
    }

    {
        auto getter = []()->float { return DependencyManager::get<LODManager>()->getDesktopLODDecreaseFPS(); };
        auto setter = [](float value) { DependencyManager::get<LODManager>()->setDesktopLODDecreaseFPS(value); };
        auto preference = new SpinnerPreference(LOD_TUNING, "Minimum Desktop FPS", getter, setter);
        preference->setMin(0);
        preference->setMax(120);
        preference->setStep(1);
        preference->setEnabler(acuityToggle);
        preferences->addPreference(preference);
    }

    {
        auto getter = []()->float { return DependencyManager::get<LODManager>()->getHMDLODDecreaseFPS(); };
        auto setter = [](float value) { DependencyManager::get<LODManager>()->setHMDLODDecreaseFPS(value); };
        auto preference = new SpinnerPreference(LOD_TUNING, "Minimum HMD FPS", getter, setter);
        preference->setMin(0);
        preference->setMax(120);
        preference->setStep(1);
        preference->setEnabler(acuityToggle);
        preferences->addPreference(preference);
    }

    {
        auto getter = []()->float { return 1.0f / DependencyManager::get<LODManager>()->getRenderDistanceInverseHighLimit(); };
        auto setter = [](float value) { DependencyManager::get<LODManager>()->setRenderDistanceInverseHighLimit(1.0f / value); };
        auto preference = new SpinnerPreference(LOD_TUNING, "Minimum Display Distance", getter, setter);
        preference->setMin(5);
        preference->setMax(32768);
        preference->setStep(1);
        preference->setEnabler(acuityToggle, true);
        preferences->addPreference(preference);
    }

    static const QString AVATAR_TUNING { "Avatar Tuning" };
    {
        auto getter = [=]()->float { return myAvatar->getRealWorldFieldOfView(); };
        auto setter = [=](float value) { myAvatar->setRealWorldFieldOfView(value); };
        auto preference = new SpinnerPreference(AVATAR_TUNING, "Real world vertical field of view (angular size of monitor)", getter, setter);
        preference->setMin(1);
        preference->setMax(180);
        preferences->addPreference(preference);
    }
    {
        auto getter = []()->float { return qApp->getFieldOfView(); };
        auto setter = [](float value) { qApp->setFieldOfView(value); };
        auto preference = new SpinnerPreference(AVATAR_TUNING, "Vertical field of view", getter, setter);
        preference->setMin(1);
        preference->setMax(180);
        preference->setStep(1);
        preferences->addPreference(preference);
    }
    {
        auto getter = [=]()->float { return myAvatar->getLeanScale(); };
        auto setter = [=](float value) { myAvatar->setLeanScale(value); };
        auto preference = new SpinnerPreference(AVATAR_TUNING, "Lean scale (applies to Faceshift users)", getter, setter);
        preference->setMin(0);
        preference->setMax(99.9f);
        preference->setDecimals(2);
        preference->setStep(1);
        preferences->addPreference(preference);
    }
    {
        auto getter = [=]()->float { return myAvatar->getUniformScale(); };
        auto setter = [=](float value) { myAvatar->setTargetScaleVerbose(value); }; // The hell?
        auto preference = new SpinnerPreference(AVATAR_TUNING, "Avatar scale <font color=\"#909090\">(default is 1.0)</font>", getter, setter);
        preference->setMin(0.01f);
        preference->setMax(99.9f);
        preference->setDecimals(2);
        preference->setStep(1);
        preferences->addPreference(preference);
    }
    {
        auto getter = [=]()->float { return myAvatar->getHead()->getPupilDilation(); };
        auto setter = [=](float value) { myAvatar->getHead()->setPupilDilation(value); }; 
        preferences->addPreference(new SliderPreference(AVATAR_TUNING, "Pupil dilation", getter, setter));
    }
    {
        auto getter = []()->float { return DependencyManager::get<DdeFaceTracker>()->getEyeClosingThreshold(); };
        auto setter = [](float value) { DependencyManager::get<DdeFaceTracker>()->setEyeClosingThreshold(value); };
        preferences->addPreference(new SliderPreference(AVATAR_TUNING, "Camera binary eyelid threshold", getter, setter));
    }
    {
        auto getter = []()->float { return FaceTracker::getEyeDeflection(); };
        auto setter = [](float value) { FaceTracker::setEyeDeflection(value); };
        preferences->addPreference(new SliderPreference(AVATAR_TUNING, "Face tracker eye deflection", getter, setter));
    }
    {
        auto getter = []()->QString { return DependencyManager::get<Faceshift>()->getHostname(); };
        auto setter = [](const QString& value) { DependencyManager::get<Faceshift>()->setHostname(value); };
        auto preference = new EditPreference(AVATAR_TUNING, "Faceshift hostname", getter, setter);
        preference->setPlaceholderText("localhost");
        preferences->addPreference(preference);
    }
    {
        auto getter = [=]()->QString { return myAvatar->getAnimGraphUrl().toString(); };
        auto setter = [=](const QString& value) { myAvatar->setAnimGraphUrl(value); };
        auto preference = new EditPreference(AVATAR_TUNING, "Avatar Animation JSON", getter, setter);
        preference->setPlaceholderText("default");
        preferences->addPreference(preference);
    }

    static const QString AUDIO("Audio");
    {
        auto getter = []()->bool {return DependencyManager::get<AudioClient>()->getReceivedAudioStream().getDynamicJitterBuffers(); };
        auto setter = [](bool value) { DependencyManager::get<AudioClient>()->getReceivedAudioStream().setDynamicJitterBuffers(value); };
        preferences->addPreference(new CheckPreference(AUDIO, "Enable Dynamic Jitter Buffers", getter, setter));
    }
    {
        auto getter = []()->float { return DependencyManager::get<AudioClient>()->getReceivedAudioStream().getDesiredJitterBufferFrames(); };
        auto setter = [](float value) { DependencyManager::get<AudioClient>()->getReceivedAudioStream().setStaticDesiredJitterBufferFrames(value); };
        
        auto preference = new SpinnerPreference(AUDIO, "Static Jitter Buffer Frames", getter, setter);
        preference->setMin(0);
        preference->setMax(10000);
        preference->setStep(1);
        preferences->addPreference(preference);
    }
    {
        auto getter = []()->float { return DependencyManager::get<AudioClient>()->getReceivedAudioStream().getMaxFramesOverDesired(); };
        auto setter = [](float value) { DependencyManager::get<AudioClient>()->getReceivedAudioStream().setMaxFramesOverDesired(value); };
        auto preference = new SpinnerPreference(AUDIO, "Max Frames Over Desired", getter, setter);
        preference->setMax(10000);
        preference->setStep(1);
        preferences->addPreference(preference);
    }
    {
        auto getter = []()->bool {return DependencyManager::get<AudioClient>()->getReceivedAudioStream().getUseStDevForJitterCalc(); };
        auto setter = [](bool value) { DependencyManager::get<AudioClient>()->getReceivedAudioStream().setUseStDevForJitterCalc(value); };
        preferences->addPreference(new CheckPreference(AUDIO, "Use Stddev for Dynamic Jitter Calc", getter, setter));
    }
    {
        auto getter = []()->float { return DependencyManager::get<AudioClient>()->getReceivedAudioStream().getWindowStarveThreshold(); };
        auto setter = [](float value) { DependencyManager::get<AudioClient>()->getReceivedAudioStream().setWindowStarveThreshold(value); };
        auto preference = new SpinnerPreference(AUDIO, "Window A Starve Threshold", getter, setter);
        preference->setMax(10000);
        preference->setStep(1);
        preferences->addPreference(preference);
    }
    {
        auto getter = []()->float { return DependencyManager::get<AudioClient>()->getReceivedAudioStream().getWindowSecondsForDesiredCalcOnTooManyStarves(); };
        auto setter = [](float value) { DependencyManager::get<AudioClient>()->getReceivedAudioStream().setWindowSecondsForDesiredCalcOnTooManyStarves(value); };
        auto preference = new SpinnerPreference(AUDIO, "Window A (raise desired on N starves) Seconds)", getter, setter);
        preference->setMax(10000);
        preference->setStep(1);
        preferences->addPreference(preference);
    }
    {
        auto getter = []()->float { return DependencyManager::get<AudioClient>()->getReceivedAudioStream().getWindowSecondsForDesiredReduction(); };
        auto setter = [](float value) { DependencyManager::get<AudioClient>()->getReceivedAudioStream().setWindowSecondsForDesiredReduction(value); };
        auto preference = new SpinnerPreference(AUDIO, "Window B (desired ceiling) Seconds", getter, setter);
        preference->setMax(10000);
        preference->setStep(1);
        preferences->addPreference(preference);
    }
    {
        auto getter = []()->bool {return DependencyManager::get<AudioClient>()->getReceivedAudioStream().getRepetitionWithFade(); };
        auto setter = [](bool value) { DependencyManager::get<AudioClient>()->getReceivedAudioStream().setRepetitionWithFade(value); };
        preferences->addPreference(new CheckPreference(AUDIO, "Repetition with Fade", getter, setter));
    }
    {
        auto getter = []()->float { return DependencyManager::get<AudioClient>()->getOutputBufferSize(); };
        auto setter = [](float value) { DependencyManager::get<AudioClient>()->setOutputBufferSize(value); };
        auto preference = new SpinnerPreference(AUDIO, "Output Buffer Size (frames)", getter, setter);
        preference->setMin(1);
        preference->setMax(20);
        preference->setStep(1);
        preferences->addPreference(preference);
    }
    {
        auto getter = []()->bool {return DependencyManager::get<AudioClient>()->getOutputStarveDetectionEnabled(); };
        auto setter = [](bool value) { DependencyManager::get<AudioClient>()->setOutputStarveDetectionEnabled(value); };
        auto preference = new CheckPreference(AUDIO, "Output Starve Detection (Automatic Buffer Size Increase)", getter, setter);
        preferences->addPreference(preference);
    }
    {
        auto getter = []()->float { return DependencyManager::get<AudioClient>()->getOutputStarveDetectionThreshold(); };
        auto setter = [](float value) { DependencyManager::get<AudioClient>()->setOutputStarveDetectionThreshold(value); };
        auto preference = new SpinnerPreference(AUDIO, "Output Starve Detection Threshold", getter, setter);
        preference->setMin(1);
        preference->setMax(500);
        preference->setStep(1);
        preferences->addPreference(preference);
    }
    {
        auto getter = []()->float { return DependencyManager::get<AudioClient>()->getOutputStarveDetectionPeriod(); };
        auto setter = [](float value) { DependencyManager::get<AudioClient>()->setOutputStarveDetectionPeriod(value); };
        auto preference = new SpinnerPreference(AUDIO, "Output Starve Detection Period (ms)", getter, setter);
        preference->setMin(1);
        preference->setMax((float)999999999);
        preference->setStep(1);
        preferences->addPreference(preference);
    }

    {
        auto getter = []()->float { return qApp->getMaxOctreePacketsPerSecond(); };
        auto setter = [](float value) { qApp->setMaxOctreePacketsPerSecond(value); };
        auto preference = new SpinnerPreference("Octree", "Max packets sent each second", getter, setter);
        preference->setMin(60);
        preference->setMax(6000);
        preference->setStep(10);
        preferences->addPreference(preference);
    }


    {
        auto getter = []()->float { return qApp->getApplicationCompositor().getHmdUIAngularSize(); };
        auto setter = [](float value) { qApp->getApplicationCompositor().setHmdUIAngularSize(value); };
        auto preference = new SpinnerPreference("HMD", "User Interface Horizontal Angular Size (degrees)", getter, setter);
        preference->setMin(30);
        preference->setMax(160);
        preference->setStep(1);
        preferences->addPreference(preference);
    }


    {
        auto getter = []()->float { return controller::InputDevice::getReticleMoveSpeed(); };
        auto setter = [](float value) { controller::InputDevice::setReticleMoveSpeed(value); };
        auto preference = new SpinnerPreference("Sixense Controllers", "Reticle Movement Speed", getter, setter);
        preference->setMin(0);
        preference->setMax(100);
        preference->setStep(1);
        preferences->addPreference(preference);
    }
}

const int PREFERENCES_HEIGHT_PADDING = 20;

PreferencesDialog::PreferencesDialog(QWidget* parent) :
    QDialog(parent)
{
    setAttribute(Qt::WA_DeleteOnClose);

    ui.setupUi(this);
    loadPreferences();

    ui.outputBufferSizeSpinner->setMinimum(MIN_AUDIO_OUTPUT_BUFFER_SIZE_FRAMES);
    ui.outputBufferSizeSpinner->setMaximum(MAX_AUDIO_OUTPUT_BUFFER_SIZE_FRAMES);

    connect(ui.buttonBrowseLocation, &QPushButton::clicked, this, &PreferencesDialog::openSnapshotLocationBrowser);
    connect(ui.buttonBrowseScriptsLocation, &QPushButton::clicked, this, &PreferencesDialog::openScriptsLocationBrowser);
    connect(ui.buttonReloadDefaultScripts, &QPushButton::clicked, [] {
        DependencyManager::get<ScriptEngines>()->loadDefaultScripts();
    });

    connect(ui.buttonChangeAppearance, &QPushButton::clicked, this, &PreferencesDialog::openFullAvatarModelBrowser);
    connect(ui.appearanceDescription, &QLineEdit::editingFinished, this, &PreferencesDialog::changeFullAvatarURL);
    connect(ui.useAcuityCheckBox, &QCheckBox::clicked, this, &PreferencesDialog::changeUseAcuity);

    connect(qApp, &Application::fullAvatarURLChanged, this, &PreferencesDialog::fullAvatarURLChanged);

    // move dialog to left side
    move(parentWidget()->geometry().topLeft());
    resize(sizeHint().width(), parentWidget()->size().height() - PREFERENCES_HEIGHT_PADDING);

    UIUtil::scaleWidgetFontSizes(this);
}

void PreferencesDialog::changeUseAcuity() {
    bool useAcuity = ui.useAcuityCheckBox->isChecked();
    ui.label_desktopMinimumFPSSpin->setEnabled(useAcuity);
    ui.desktopMinimumFPSSpin->setEnabled(useAcuity);
    ui.label_hmdMinimumFPSSpin->setEnabled(useAcuity);
    ui.hmdMinimumFPSSpin->setEnabled(useAcuity);
    ui.label_smallestReasonableRenderHorizon->setEnabled(!useAcuity);
    ui.smallestReasonableRenderHorizon->setEnabled(!useAcuity);
    Menu::getInstance()->getActionForOption(MenuOption::LodTools)->setEnabled(useAcuity);
    Menu::getInstance()->getSubMenuFromName(MenuOption::RenderResolution, Menu::getInstance()->getSubMenuFromName("Render", Menu::getInstance()->getMenu("Developer")))->setEnabled(useAcuity);
}
void PreferencesDialog::changeFullAvatarURL() {
    DependencyManager::get<AvatarManager>()->getMyAvatar()->useFullAvatarURL(ui.appearanceDescription->text(), "");
    this->fullAvatarURLChanged(ui.appearanceDescription->text(), "");
}

void PreferencesDialog::fullAvatarURLChanged(const QString& newValue, const QString& modelName) {
    ui.appearanceDescription->setText(newValue);
    const QString APPEARANCE_LABEL_TEXT("Appearance: ");
    ui.appearanceLabel->setText(APPEARANCE_LABEL_TEXT + modelName);
}

void PreferencesDialog::accept() {
    MyAvatar* myAvatar = DependencyManager::get<AvatarManager>()->getMyAvatar();

    // if there is an attempted change to the full avatar URL, apply it now
    if (QUrl(ui.appearanceDescription->text()) != myAvatar->getFullAvatarURLFromPreferences()) {
        changeFullAvatarURL();
    }

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
        _marketplaceWindow = new WebWindowClass("Marketplace", MARKETPLACE_URL, WIDTH, HEIGHT);
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

    ui.scriptsLocationEdit->setText(DependencyManager::get<ScriptEngines>()->getScriptsLocation());

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

    ui.avatarScaleSpin->setValue(myAvatar->getUniformScale());
    ui.avatarAnimationEdit->setText(myAvatar->getAnimGraphUrl().toString());

    ui.maxOctreePPSSpin->setValue(qApp->getMaxOctreePacketsPerSecond());

#if 0
    ui.oculusUIAngularSizeSpin->setValue(qApp->getApplicationCompositor().getHmdUIAngularSize());
#endif

    ui.sixenseReticleMoveSpeedSpin->setValue(controller::InputDevice::getReticleMoveSpeed());

    // LOD items
    auto lodManager = DependencyManager::get<LODManager>();
    ui.useAcuityCheckBox->setChecked(lodManager->getUseAcuity());
    ui.desktopMinimumFPSSpin->setValue(lodManager->getDesktopLODDecreaseFPS());
    ui.hmdMinimumFPSSpin->setValue(lodManager->getHMDLODDecreaseFPS());
    ui.smallestReasonableRenderHorizon->setValue(1.0f / lodManager->getRenderDistanceInverseHighLimit());
    changeUseAcuity();
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
        DependencyManager::get<ScriptEngines>()->setScriptsLocation(ui.scriptsLocationEdit->text());
    }

    myAvatar->getHead()->setPupilDilation(ui.pupilDilationSlider->value() / (float)ui.pupilDilationSlider->maximum());
    myAvatar->setLeanScale(ui.leanScaleSpin->value());
    myAvatar->setTargetScaleVerbose(ui.avatarScaleSpin->value());
    if (myAvatar->getAnimGraphUrl() != ui.avatarAnimationEdit->text()) { // If changed, destroy the old and start with the new
        myAvatar->setAnimGraphUrl(ui.avatarAnimationEdit->text());
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

    controller::InputDevice::setReticleMoveSpeed(ui.sixenseReticleMoveSpeedSpin->value());

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
    lodManager->setUseAcuity(ui.useAcuityCheckBox->isChecked());
    lodManager->setDesktopLODDecreaseFPS(ui.desktopMinimumFPSSpin->value());
    lodManager->setHMDLODDecreaseFPS(ui.hmdMinimumFPSSpin->value());
    lodManager->setRenderDistanceInverseHighLimit(1.0f / ui.smallestReasonableRenderHorizon->value());
}
