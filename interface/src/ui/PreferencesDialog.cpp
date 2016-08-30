//
//  Re-created Bradley Austin Davis on 2016/01/22
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "PreferencesDialog.h"

#include <AudioClient.h>
#include <avatar/AvatarManager.h>
#include <devices/DdeFaceTracker.h>
#include <devices/Faceshift.h>
#include <NetworkingConstants.h>
#include <ScriptEngines.h>
#include <OffscreenUi.h>
#include <Preferences.h>
#include <display-plugins/CompositorHelper.h>

#include "Application.h"
#include "DialogsManager.h"
#include "LODManager.h"
#include "Menu.h"
#include "Snapshot.h"
#include "UserActivityLogger.h"

#include "AmbientOcclusionEffect.h"
#include "AntialiasingEffect.h"
#include "RenderShadowTask.h"

void setupPreferences() {
    auto preferences = DependencyManager::get<Preferences>();

    MyAvatar* myAvatar = DependencyManager::get<AvatarManager>()->getMyAvatar();
    static const QString AVATAR_BASICS { "Avatar Basics" };
    {
        auto getter = [=]()->QString { return myAvatar->getDisplayName(); };
        auto setter = [=](const QString& value) { myAvatar->setDisplayName(value); };
        auto preference = new EditPreference(AVATAR_BASICS, "Avatar display name (optional)", getter, setter);
        preference->setPlaceholderText("Not showing a name");
        preferences->addPreference(preference);
    }

    {
        auto getter = [=]()->QString { return myAvatar->getCollisionSoundURL(); };
        auto setter = [=](const QString& value) { myAvatar->setCollisionSoundURL(value); };
        auto preference = new EditPreference(AVATAR_BASICS, "Avatar collision sound URL (optional)", getter, setter);
        preference->setPlaceholderText("Enter the URL of a sound to play when you bump into something");
        preferences->addPreference(preference);
    }

    {
        auto getter = [=]()->QString { return myAvatar->getFullAvatarURLFromPreferences().toString(); };
        auto setter = [=](const QString& value) { myAvatar->useFullAvatarURL(value, ""); };
        auto preference = new AvatarPreference(AVATAR_BASICS, "Appearance", getter, setter);
        preferences->addPreference(preference);
    }

    {
        auto getter = [=]()->bool { return myAvatar->getSnapTurn(); };
        auto setter = [=](bool value) { myAvatar->setSnapTurn(value); };
        preferences->addPreference(new CheckPreference(AVATAR_BASICS, "Snap turn when in HMD", getter, setter));
    }
    {
        auto getter = [=]()->bool { return myAvatar->getClearOverlayWhenMoving(); };
        auto setter = [=](bool value) { myAvatar->setClearOverlayWhenMoving(value); };
        preferences->addPreference(new CheckPreference(AVATAR_BASICS, "Clear overlays when moving", getter, setter));
    }

    // Snapshots
    static const QString SNAPSHOTS { "Snapshots" };
    {
        auto getter = []()->QString { return Snapshot::snapshotsLocation.get(); };
        auto setter = [](const QString& value) { Snapshot::snapshotsLocation.set(value); };
        auto preference = new BrowsePreference(SNAPSHOTS, "Put my snapshots here", getter, setter);
        preferences->addPreference(preference);
    }

    // Scripts
    {
        auto getter = []()->QString { return DependencyManager::get<ScriptEngines>()->getScriptsLocation(); };
        auto setter = [](const QString& value) { DependencyManager::get<ScriptEngines>()->setScriptsLocation(value); };
        preferences->addPreference(new BrowsePreference("Scripts", "Load scripts from this directory", getter, setter));
    }

    preferences->addPreference(new ButtonPreference("Scripts", "Load Default Scripts", [] {
        DependencyManager::get<ScriptEngines>()->loadDefaultScripts();
    }));

    {
        auto getter = []()->bool { return !Menu::getInstance()->isOptionChecked(MenuOption::DisableActivityLogger); };
        auto setter = [](bool value) { Menu::getInstance()->setIsOptionChecked(MenuOption::DisableActivityLogger, !value); };
        preferences->addPreference(new CheckPreference("Privacy", "Send data", getter, setter));
    }
    
    static const QString LOD_TUNING("Level of Detail Tuning");
    {
        auto getter = []()->float { return DependencyManager::get<LODManager>()->getDesktopLODDecreaseFPS(); };
        auto setter = [](float value) { DependencyManager::get<LODManager>()->setDesktopLODDecreaseFPS(value); };
        auto preference = new SpinnerPreference(LOD_TUNING, "Minimum desktop FPS", getter, setter);
        preference->setMin(0);
        preference->setMax(120);
        preference->setStep(1);
        preferences->addPreference(preference);
    }

    {
        auto getter = []()->float { return DependencyManager::get<LODManager>()->getHMDLODDecreaseFPS(); };
        auto setter = [](float value) { DependencyManager::get<LODManager>()->setHMDLODDecreaseFPS(value); };
        auto preference = new SpinnerPreference(LOD_TUNING, "Minimum HMD FPS", getter, setter);
        preference->setMin(0);
        preference->setMax(120);
        preference->setStep(1);
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
        auto getter = [=]()->float { return myAvatar->getUniformScale(); };
        auto setter = [=](float value) { myAvatar->setTargetScaleVerbose(value); }; // The hell?
        auto preference = new SpinnerPreference(AVATAR_TUNING, "Avatar scale (default is 1.0)", getter, setter);
        preference->setMin(0.01f);
        preference->setMax(99.9f);
        preference->setDecimals(2);
        preference->setStep(1);
        preferences->addPreference(preference);
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
        auto getter = [=]()->QString { return myAvatar->getAnimGraphOverrideUrl().toString(); };
        auto setter = [=](const QString& value) { myAvatar->setAnimGraphOverrideUrl(QUrl(value)); };
        auto preference = new EditPreference(AVATAR_TUNING, "Avatar animation JSON", getter, setter);
        preference->setPlaceholderText("default");
        preferences->addPreference(preference);
    }

    static const QString AVATAR_CAMERA { "Avatar Camera" };
    {
        auto getter = [=]()->float { return myAvatar->getPitchSpeed(); };
        auto setter = [=](float value) { myAvatar->setPitchSpeed(value); };
        auto preference = new SpinnerPreference(AVATAR_CAMERA, "Camera pitch speed (degrees/second)", getter, setter);
        preference->setMin(1.0f);
        preference->setMax(360.0f);
        preferences->addPreference(preference);
    }
    {
        auto getter = [=]()->float { return myAvatar->getYawSpeed(); };
        auto setter = [=](float value) { myAvatar->setYawSpeed(value); };
        auto preference = new SpinnerPreference(AVATAR_CAMERA, "Camera yaw speed (degrees/second)", getter, setter);
        preference->setMin(1.0f);
        preference->setMax(360.0f);
        preferences->addPreference(preference);
    }

    static const QString AUDIO("Audio");
    {
        auto getter = []()->bool { return DependencyManager::get<AudioClient>()->getReceivedAudioStream().getDynamicJitterBuffers(); };
        auto setter = [](bool value) { DependencyManager::get<AudioClient>()->getReceivedAudioStream().setDynamicJitterBuffers(value); };
        preferences->addPreference(new CheckPreference(AUDIO, "Enable dynamic jitter buffers", getter, setter));
    }
    {
        auto getter = []()->float { return DependencyManager::get<AudioClient>()->getReceivedAudioStream().getDesiredJitterBufferFrames(); };
        auto setter = [](float value) { DependencyManager::get<AudioClient>()->getReceivedAudioStream().setStaticDesiredJitterBufferFrames(value); };
        
        auto preference = new SpinnerPreference(AUDIO, "Static jitter buffer frames", getter, setter);
        preference->setMin(0);
        preference->setMax(10000);
        preference->setStep(1);
        preferences->addPreference(preference);
    }
    {
        auto getter = []()->float { return DependencyManager::get<AudioClient>()->getReceivedAudioStream().getMaxFramesOverDesired(); };
        auto setter = [](float value) { DependencyManager::get<AudioClient>()->getReceivedAudioStream().setMaxFramesOverDesired(value); };
        auto preference = new SpinnerPreference(AUDIO, "Max frames over desired", getter, setter);
        preference->setMax(10000);
        preference->setStep(1);
        preferences->addPreference(preference);
    }
    {
        auto getter = []()->bool { return DependencyManager::get<AudioClient>()->getReceivedAudioStream().getUseStDevForJitterCalc(); };
        auto setter = [](bool value) { DependencyManager::get<AudioClient>()->getReceivedAudioStream().setUseStDevForJitterCalc(value); };
        preferences->addPreference(new CheckPreference(AUDIO, "Use standard deviation for dynamic jitter calc", getter, setter));
    }
    {
        auto getter = []()->float { return DependencyManager::get<AudioClient>()->getReceivedAudioStream().getWindowStarveThreshold(); };
        auto setter = [](float value) { DependencyManager::get<AudioClient>()->getReceivedAudioStream().setWindowStarveThreshold(value); };
        auto preference = new SpinnerPreference(AUDIO, "Window A starve threshold", getter, setter);
        preference->setMax(10000);
        preference->setStep(1);
        preferences->addPreference(preference);
    }
    {
        auto getter = []()->float { return DependencyManager::get<AudioClient>()->getReceivedAudioStream().getWindowSecondsForDesiredCalcOnTooManyStarves(); };
        auto setter = [](float value) { DependencyManager::get<AudioClient>()->getReceivedAudioStream().setWindowSecondsForDesiredCalcOnTooManyStarves(value); };
        auto preference = new SpinnerPreference(AUDIO, "Window A (raise desired on N starves) seconds", getter, setter);
        preference->setMax(10000);
        preference->setStep(1);
        preferences->addPreference(preference);
    }
    {
        auto getter = []()->float { return DependencyManager::get<AudioClient>()->getReceivedAudioStream().getWindowSecondsForDesiredReduction(); };
        auto setter = [](float value) { DependencyManager::get<AudioClient>()->getReceivedAudioStream().setWindowSecondsForDesiredReduction(value); };
        auto preference = new SpinnerPreference(AUDIO, "Window B (desired ceiling) seconds", getter, setter);
        preference->setMax(10000);
        preference->setStep(1);
        preferences->addPreference(preference);
    }
    {
        auto getter = []()->bool { return DependencyManager::get<AudioClient>()->getReceivedAudioStream().getRepetitionWithFade(); };
        auto setter = [](bool value) { DependencyManager::get<AudioClient>()->getReceivedAudioStream().setRepetitionWithFade(value); };
        preferences->addPreference(new CheckPreference(AUDIO, "Repetition with fade", getter, setter));
    }
    {
        auto getter = []()->float { return DependencyManager::get<AudioClient>()->getOutputBufferSize(); };
        auto setter = [](float value) { DependencyManager::get<AudioClient>()->setOutputBufferSize(value); };
        auto preference = new SpinnerPreference(AUDIO, "Output buffer initial size (frames)", getter, setter);
        preference->setMin(1);
        preference->setMax(20);
        preference->setStep(1);
        preferences->addPreference(preference);
    }
    {
        auto getter = []()->bool { return DependencyManager::get<AudioClient>()->getOutputStarveDetectionEnabled(); };
        auto setter = [](bool value) { DependencyManager::get<AudioClient>()->setOutputStarveDetectionEnabled(value); };
        auto preference = new CheckPreference(AUDIO, "Output starve detection (automatic buffer size increase)", getter, setter);
        preferences->addPreference(preference);
    }
    {
        auto getter = []()->float { return DependencyManager::get<AudioClient>()->getOutputStarveDetectionThreshold(); };
        auto setter = [](float value) { DependencyManager::get<AudioClient>()->setOutputStarveDetectionThreshold(value); };
        auto preference = new SpinnerPreference(AUDIO, "Output starve detection threshold", getter, setter);
        preference->setMin(1);
        preference->setMax(500);
        preference->setStep(1);
        preferences->addPreference(preference);
    }
    {
        auto getter = []()->float { return DependencyManager::get<AudioClient>()->getOutputStarveDetectionPeriod(); };
        auto setter = [](float value) { DependencyManager::get<AudioClient>()->setOutputStarveDetectionPeriod(value); };
        auto preference = new SpinnerPreference(AUDIO, "Output starve detection period (ms)", getter, setter);
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
        auto preference = new SpinnerPreference("HMD", "UI horizontal angular size (degrees)", getter, setter);
        preference->setMin(30);
        preference->setMax(160);
        preference->setStep(1);
        preferences->addPreference(preference);
    }


    {
        auto getter = []()->float { return controller::InputDevice::getReticleMoveSpeed(); };
        auto setter = [](float value) { controller::InputDevice::setReticleMoveSpeed(value); };
        auto preference = new SpinnerPreference("Sixense Controllers", "Reticle movement speed", getter, setter);
        preference->setMin(0);
        preference->setMax(100);
        preference->setStep(1);
        preferences->addPreference(preference);
    }

    {
        static const QString RENDER("Graphics");
        auto renderConfig = qApp->getRenderEngine()->getConfiguration();

        auto ambientOcclusionConfig = renderConfig->getConfig<AmbientOcclusionEffect>();
        {
            auto getter = [ambientOcclusionConfig]()->QString { return ambientOcclusionConfig->getPreset(); };
            auto setter = [ambientOcclusionConfig](QString preset) { ambientOcclusionConfig->setPreset(preset); };
            auto preference = new ComboBoxPreference(RENDER, "Ambient occlusion", getter, setter);
            preference->setItems(ambientOcclusionConfig->getPresetList());
            preferences->addPreference(preference);
        }

        auto shadowConfig = renderConfig->getConfig<RenderShadowTask>();
        {
            auto getter = [shadowConfig]()->QString { return shadowConfig->getPreset(); };
            auto setter = [shadowConfig](QString preset) { shadowConfig->setPreset(preset); };
            auto preference = new ComboBoxPreference(RENDER, "Shadows", getter, setter);
            preference->setItems(shadowConfig->getPresetList());
            preferences->addPreference(preference);
        }
    }
}
