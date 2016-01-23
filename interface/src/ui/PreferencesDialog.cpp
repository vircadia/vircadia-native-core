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

#include "Application.h"
#include "DialogsManager.h"
#include "LODManager.h"
#include "Menu.h"
#include "Snapshot.h"
#include "UserActivityLogger.h"

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
