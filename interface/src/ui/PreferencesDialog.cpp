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
#include <ScriptEngines.h>
#include <OffscreenUi.h>
#include <Preferences.h>
#include <RenderShadowTask.h>
#include <display-plugins/CompositorHelper.h>

#include "Application.h"
#include "DialogsManager.h"
#include "LODManager.h"
#include "Menu.h"
#include "Snapshot.h"
#include "SnapshotAnimated.h"
#include "UserActivityLogger.h"

void setupPreferences() {
    auto preferences = DependencyManager::get<Preferences>();
    auto myAvatar = DependencyManager::get<AvatarManager>()->getMyAvatar();
    static const QString AVATAR_BASICS { "Avatar Basics" };
    {
        auto getter = [myAvatar]()->QString { return myAvatar->getDisplayName(); };
        auto setter = [myAvatar](const QString& value) { myAvatar->setDisplayName(value); };
        auto preference = new EditPreference(AVATAR_BASICS, "Avatar display name (optional)", getter, setter);
        preference->setPlaceholderText("Not showing a name");
        preferences->addPreference(preference);
    }

    {
        auto getter = [myAvatar]()->QString { return myAvatar->getCollisionSoundURL(); };
        auto setter = [myAvatar](const QString& value) { myAvatar->setCollisionSoundURL(value); };
        auto preference = new EditPreference(AVATAR_BASICS, "Avatar collision sound URL (optional)", getter, setter);
        preference->setPlaceholderText("Enter the URL of a sound to play when you bump into something");
        preferences->addPreference(preference);
    }

    {
        auto getter = [myAvatar]()->QString { return myAvatar->getFullAvatarURLFromPreferences().toString(); };
        auto setter = [myAvatar](const QString& value) { myAvatar->useFullAvatarURL(value, ""); qApp->clearAvatarOverrideUrl(); };
        auto preference = new AvatarPreference(AVATAR_BASICS, "Appearance", getter, setter);
        preferences->addPreference(preference);
    }

    // Graphics quality
    static const QString GRAPHICS_QUALITY { "Graphics Quality" };
    {
        auto getter = []()->float {
            return DependencyManager::get<LODManager>()->getWorldDetailQuality();
        };

        auto setter = [](float value) {
            DependencyManager::get<LODManager>()->setWorldDetailQuality(value);
        };

        auto wodSlider = new SliderPreference(GRAPHICS_QUALITY, "World Detail", getter, setter);
        wodSlider->setMin(0.25f);
        wodSlider->setMax(0.75f);
        wodSlider->setStep(0.25f);
        preferences->addPreference(wodSlider);

        auto getterShadow = []()->bool {
            auto menu = Menu::getInstance();
            return menu->isOptionChecked(MenuOption::Shadows);
        };
        auto setterShadow = [](bool value) {
            auto menu = Menu::getInstance();
            menu->setIsOptionChecked(MenuOption::Shadows, value);
        };
        preferences->addPreference(new CheckPreference(GRAPHICS_QUALITY, "Show Shadows", getterShadow, setterShadow));
    }

    // UI
    static const QString UI_CATEGORY { "User Interface" };
    {
        auto getter = []()->bool { return qApp->getSettingConstrainToolbarPosition(); };
        auto setter = [](bool value) { qApp->setSettingConstrainToolbarPosition(value); };
        preferences->addPreference(new CheckPreference(UI_CATEGORY, "Constrain Toolbar Position to Horizontal Center", getter, setter));
    }

    {
        auto getter = []()->float { return qApp->getDesktopTabletScale(); };
        auto setter = [](float value) { qApp->setDesktopTabletScale(value); };
        auto preference = new SpinnerPreference(UI_CATEGORY, "Desktop Tablet Scale %", getter, setter);
        preference->setMin(20);
        preference->setMax(500);
        preferences->addPreference(preference);
    }

    {
        auto getter = []()->float { return qApp->getHMDTabletScale(); };
        auto setter = [](float value) { qApp->setHMDTabletScale(value); };
        auto preference = new SpinnerPreference(UI_CATEGORY, "VR Tablet Scale %", getter, setter);
        preference->setMin(20);
        preference->setMax(500);
        preferences->addPreference(preference);
    }

    {
        auto getter = []()->bool { return qApp->getPreferStylusOverLaser(); };
        auto setter = [](bool value) { qApp->setPreferStylusOverLaser(value); };
        preferences->addPreference(new CheckPreference(UI_CATEGORY, "Prefer Stylus Over Laser", getter, setter));
    }

    {
        static const QString RETICLE_ICON_NAME = { Cursor::Manager::getIconName(Cursor::Icon::RETICLE) };
        auto getter = []()->bool { return qApp->getPreferredCursor() == RETICLE_ICON_NAME; };
        auto setter = [](bool value) { qApp->setPreferredCursor(value ? RETICLE_ICON_NAME : QString()); };
        preferences->addPreference(new CheckPreference(UI_CATEGORY, "Use reticle cursor instead of arrow", getter, setter));
    }

    static const QString VIEW_CATEGORY{ "View" };
    {
        auto getter = [myAvatar]()->float { return myAvatar->getRealWorldFieldOfView(); };
        auto setter = [myAvatar](float value) { myAvatar->setRealWorldFieldOfView(value); };
        auto preference = new SpinnerPreference(VIEW_CATEGORY, "Real world vertical field of view (angular size of monitor)", getter, setter);
        preference->setMin(1);
        preference->setMax(180);
        preferences->addPreference(preference);
    }
    {
        auto getter = []()->float { return qApp->getFieldOfView(); };
        auto setter = [](float value) { qApp->setFieldOfView(value); };
        auto preference = new SpinnerPreference(VIEW_CATEGORY, "Vertical field of view", getter, setter);
        preference->setMin(1);
        preference->setMax(180);
        preference->setStep(1);
        preferences->addPreference(preference);
    }


    // FIXME: Remove setting completely or make available through JavaScript API?
    /*
    {
        auto getter = []()->bool { return qApp->getPreferAvatarFingerOverStylus(); };
        auto setter = [](bool value) { qApp->setPreferAvatarFingerOverStylus(value); };
        preferences->addPreference(new CheckPreference(UI_CATEGORY, "Prefer Avatar Finger Over Stylus", getter, setter));
    }
    */

    // Snapshots
    static const QString SNAPSHOTS { "Snapshots" };
    {
        auto getter = []()->QString { return DependencyManager::get<Snapshot>()->_snapshotsLocation.get(); };
        auto setter = [](const QString& value) { DependencyManager::get<Snapshot>()->_snapshotsLocation.set(value); emit DependencyManager::get<Snapshot>()->snapshotLocationSet(value); };
        auto preference = new BrowsePreference(SNAPSHOTS, "Put my snapshots here", getter, setter);
        preferences->addPreference(preference);
    }
    {
        auto getter = []()->float { return SnapshotAnimated::snapshotAnimatedDuration.get(); };
        auto setter = [](float value) { SnapshotAnimated::snapshotAnimatedDuration.set(value); };
        auto preference = new SpinnerPreference(SNAPSHOTS, "Animated Snapshot Duration", getter, setter);
        preference->setMin(1);
        preference->setMax(5);
        preference->setStep(1);
        preferences->addPreference(preference);
    }

    {
        auto getter = []()->bool { return !Menu::getInstance()->isOptionChecked(MenuOption::DisableActivityLogger); };
        auto setter = [](bool value) { Menu::getInstance()->setIsOptionChecked(MenuOption::DisableActivityLogger, !value); };
        preferences->addPreference(new CheckPreference("Privacy", "Send data - High Fidelity uses information provided by your "
                                "client to improve the product through the logging of errors, tracking of usage patterns, "
                                "installation and system details, and crash events. By allowing High Fidelity to collect "
                                "this information you are helping to improve the product. ", getter, setter));
    }
    
    static const QString AVATAR_TUNING { "Avatar Tuning" };
    {
        auto getter = [myAvatar]()->QString { return myAvatar->getDominantHand(); };
        auto setter = [myAvatar](const QString& value) { myAvatar->setDominantHand(value); };
        preferences->addPreference(new PrimaryHandPreference(AVATAR_TUNING, "Dominant Hand", getter, setter));
    }
    {
        auto getter = [myAvatar]()->float { return myAvatar->getTargetScale(); };
        auto setter = [myAvatar](float value) { myAvatar->setTargetScale(value); };
        auto preference = new SpinnerSliderPreference(AVATAR_TUNING, "Avatar Scale", getter, setter);
        preference->setMin(0.25);
        preference->setMax(4);
        preference->setStep(0.05f);
        preference->setDecimals(2);
        preferences->addPreference(preference);
        
        // When the Interface is first loaded, this section setupPreferences(); is loaded - 
        // causing the myAvatar->getDomainMinScale() and myAvatar->getDomainMaxScale() to get set to incorrect values
        // which can't be changed across domain switches. Having these values loaded up when you load the Dialog each time
        // is a way around this, therefore they're not specified here but in the QML.
    }

    {
        auto getter = [myAvatar]()->QString { return myAvatar->getAnimGraphOverrideUrl().toString(); };
        auto setter = [myAvatar](const QString& value) { myAvatar->setAnimGraphOverrideUrl(QUrl(value)); };
        auto preference = new EditPreference(AVATAR_TUNING, "Avatar animation JSON", getter, setter);
        preference->setPlaceholderText("default");
        preferences->addPreference(preference);
    }

    {
        auto getter = [myAvatar]()->bool { return myAvatar->getCollisionsEnabled(); };
        auto setter = [myAvatar](bool value) { myAvatar->setCollisionsEnabled(value); };
        auto preference = new CheckPreference(AVATAR_TUNING, "Enable Avatar collisions", getter, setter);
        preferences->addPreference(preference);
    }

    static const QString FACE_TRACKING{ "Face Tracking" };
    {
        auto getter = []()->float { return DependencyManager::get<DdeFaceTracker>()->getEyeClosingThreshold(); };
        auto setter = [](float value) { DependencyManager::get<DdeFaceTracker>()->setEyeClosingThreshold(value); };
        preferences->addPreference(new SliderPreference(FACE_TRACKING, "Eye Closing Threshold", getter, setter));
    }
    {
        auto getter = []()->float { return FaceTracker::getEyeDeflection(); };
        auto setter = [](float value) { FaceTracker::setEyeDeflection(value); };
        preferences->addPreference(new SliderPreference(FACE_TRACKING, "Eye Deflection", getter, setter));
    }

    static const QString VR_MOVEMENT{ "VR Movement" };
    {

        static const QString movementsControlChannel = QStringLiteral("Hifi-Advanced-Movement-Disabler");
        auto getter = [myAvatar]()->bool { return myAvatar->useAdvancedMovementControls(); };
        auto setter = [myAvatar](bool value) { myAvatar->setUseAdvancedMovementControls(value); };
        preferences->addPreference(new CheckPreference(VR_MOVEMENT,
                                                       QStringLiteral("Advanced movement in VR (Teleport movement when unchecked)"),
                                                       getter, setter));
    }
    {
        auto getter = [myAvatar]()->bool { return myAvatar->getFlyingHMDPref(); };
        auto setter = [myAvatar](bool value) { myAvatar->setFlyingHMDPref(value); };
        preferences->addPreference(new CheckPreference(VR_MOVEMENT, "Flying & jumping (HMD)", getter, setter));
    }
    {
        auto getter = [myAvatar]()->int { return myAvatar->getSnapTurn() ? 0 : 1; };
        auto setter = [myAvatar](int value) { myAvatar->setSnapTurn(value == 0); };
        auto preference = new RadioButtonsPreference(VR_MOVEMENT, "Snap turn / Smooth turn", getter, setter);
        QStringList items;
        items << "Snap turn" << "Smooth turn";
        preference->setItems(items);
        preferences->addPreference(preference);
    }
    {
        auto getter = [=]()->float { return myAvatar->getUserHeight(); };
        auto setter = [=](float value) { myAvatar->setUserHeight(value); };
        auto preference = new SpinnerPreference(VR_MOVEMENT, "User real-world height (meters)", getter, setter);
        preference->setMin(1.0f);
        preference->setMax(2.2f);
        preference->setDecimals(3);
        preference->setStep(0.001f);
        preferences->addPreference(preference);
    }
    {
        auto preference = new ButtonPreference(VR_MOVEMENT, "RESET SENSORS", [] {
            qApp->resetSensors();
        });
        preferences->addPreference(preference);
    }

    static const QString AVATAR_CAMERA{ "Mouse Sensitivity" };
    {
        auto getter = [myAvatar]()->float { return myAvatar->getPitchSpeed(); };
        auto setter = [myAvatar](float value) { myAvatar->setPitchSpeed(value); };
        auto preference = new SpinnerSliderPreference(AVATAR_CAMERA, "Y input:", getter, setter);
        preference->setMin(1.0f);
        preference->setMax(360.0f);
        preference->setStep(1);
        preference->setDecimals(1);
        preferences->addPreference(preference);
    }
    {
        auto getter = [myAvatar]()->float { return myAvatar->getYawSpeed(); };
        auto setter = [myAvatar](float value) { myAvatar->setYawSpeed(value); };
        auto preference = new SpinnerSliderPreference(AVATAR_CAMERA, "X input:", getter, setter);
        preference->setMin(1.0f);
        preference->setMax(360.0f);
        preference->setStep(1);
        preference->setDecimals(1);
        preferences->addPreference(preference);
    }

    static const QString AUDIO_BUFFERS("Audio Buffers");
    {
        auto getter = []()->bool { return !DependencyManager::get<AudioClient>()->getReceivedAudioStream().dynamicJitterBufferEnabled(); };
        auto setter = [](bool value) { DependencyManager::get<AudioClient>()->getReceivedAudioStream().setDynamicJitterBufferEnabled(!value); };
        auto preference = new CheckPreference(AUDIO_BUFFERS, "Disable dynamic jitter buffer", getter, setter);
        preferences->addPreference(preference);
    }
    {
        auto getter = []()->float { return DependencyManager::get<AudioClient>()->getReceivedAudioStream().getStaticJitterBufferFrames(); };
        auto setter = [](float value) { DependencyManager::get<AudioClient>()->getReceivedAudioStream().setStaticJitterBufferFrames(value); };
        auto preference = new SpinnerPreference(AUDIO_BUFFERS, "Static jitter buffer frames", getter, setter);
        preference->setMin(0);
        preference->setMax(2000);
        preference->setStep(1);
        preferences->addPreference(preference);
    }
    {
        auto getter = []()->bool { return !DependencyManager::get<AudioClient>()->getOutputStarveDetectionEnabled(); };
        auto setter = [](bool value) { DependencyManager::get<AudioClient>()->setOutputStarveDetectionEnabled(!value); };
        auto preference = new CheckPreference(AUDIO_BUFFERS, "Disable output starve detection", getter, setter);
        preferences->addPreference(preference);
    }
    {
        auto getter = []()->float { return DependencyManager::get<AudioClient>()->getOutputBufferSize(); };
        auto setter = [](float value) { DependencyManager::get<AudioClient>()->setOutputBufferSize(value); };
        auto preference = new SpinnerPreference(AUDIO_BUFFERS, "Output buffer initial frames", getter, setter);
        preference->setMin(AudioClient::MIN_BUFFER_FRAMES);
        preference->setMax(AudioClient::MAX_BUFFER_FRAMES);
        preference->setStep(1);
        preferences->addPreference(preference);
    }
#if DEV_BUILD || PR_BUILD
    {
        auto getter = []()->bool { return DependencyManager::get<AudioClient>()->isSimulatingJitter(); };
        auto setter = [](bool value) { return DependencyManager::get<AudioClient>()->setIsSimulatingJitter(value); };
        auto preference = new CheckPreference(AUDIO_BUFFERS, "Packet jitter simulator", getter, setter);
        preferences->addPreference(preference);
    }
    {
        auto getter = []()->float { return DependencyManager::get<AudioClient>()->getGateThreshold(); };
        auto setter = [](float value) { return DependencyManager::get<AudioClient>()->setGateThreshold(value); };
        auto preference = new SpinnerPreference(AUDIO_BUFFERS, "Packet throttle threshold", getter, setter);
        preference->setMin(1);
        preference->setMax(200);
        preference->setStep(1);
        preferences->addPreference(preference);
    }
#endif

    {
        static const QString NETWORKING("Networking");

        QWeakPointer<NodeList> nodeListWeak = DependencyManager::get<NodeList>();
        {
            static const int MIN_PORT_NUMBER { 0 };
            static const int MAX_PORT_NUMBER { 65535 };
            auto getter = [nodeListWeak] {
                auto nodeList = nodeListWeak.lock();
                if (nodeList) {
                    return static_cast<int>(nodeList->getSocketLocalPort());
                } else {
                    return -1;
                }
            };
            auto setter = [nodeListWeak](int preset) {
                auto nodeList = nodeListWeak.lock();
                if (nodeList) {
                    nodeList->setSocketLocalPort(static_cast<quint16>(preset));
                }
            };
            auto preference = new IntSpinnerPreference(NETWORKING, "Listening Port", getter, setter);
            preference->setMin(MIN_PORT_NUMBER);
            preference->setMax(MAX_PORT_NUMBER);
            preferences->addPreference(preference);
        }

        {
            auto getter = []()->float { return qApp->getMaxOctreePacketsPerSecond(); };
            auto setter = [](float value) { qApp->setMaxOctreePacketsPerSecond(value); };
            auto preference = new SpinnerPreference(NETWORKING, "Max entities packets sent each second", getter, setter);
            preference->setMin(60);
            preference->setMax(6000);
            preference->setStep(10);
            preferences->addPreference(preference);
        }

    }
}
