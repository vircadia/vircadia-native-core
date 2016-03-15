//
//  Menu.cpp
//  interface/src
//
//  Created by Stephen Birarda on 8/12/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QFileDialog>
#include <QMenuBar>
#include <QShortcut>

#include <AddressManager.h>
#include <AudioClient.h>
#include <DependencyManager.h>
#include <display-plugins/DisplayPlugin.h>
#include <PathUtils.h>
#include <SettingHandle.h>
#include <UserActivityLogger.h>
#include <VrMenu.h>
#include <ScriptEngines.h>
#include <MenuItemProperties.h>

#include "Application.h"
#include "AccountManager.h"
#include "assets/ATPAssetMigrator.h"
#include "audio/AudioScope.h"
#include "avatar/AvatarManager.h"
#include "devices/DdeFaceTracker.h"
#include "devices/Faceshift.h"
#include "input-plugins/SpacemouseManager.h"
#include "MainWindow.h"
#include "render/DrawStatus.h"
#include "scripting/MenuScriptingInterface.h"
#include "ui/AssetUploadDialogFactory.h"
#include "ui/DialogsManager.h"
#include "ui/StandAloneJSConsole.h"
#include "InterfaceLogging.h"

#if defined(Q_OS_MAC) || defined(Q_OS_WIN)
#include "SpeechRecognizer.h"
#endif

#include "Menu.h"

Menu* Menu::getInstance() {
    return static_cast<Menu*>(qApp->getWindow()->menuBar());
}

Menu::Menu() {
    auto dialogsManager = DependencyManager::get<DialogsManager>();
    AccountManager& accountManager = AccountManager::getInstance();

    // File/Application menu ----------------------------------
    MenuWrapper* fileMenu = addMenu("File");

    // File > Login menu items
    {
        addActionToQMenuAndActionHash(fileMenu, MenuOption::Login);

        // connect to the appropriate signal of the AccountManager so that we can change the Login/Logout menu item
        connect(&accountManager, &AccountManager::profileChanged,
                dialogsManager.data(), &DialogsManager::toggleLoginDialog);
        connect(&accountManager, &AccountManager::logoutComplete,
                dialogsManager.data(), &DialogsManager::toggleLoginDialog);
    }

    // File > Help
    addActionToQMenuAndActionHash(fileMenu, MenuOption::Help, 0, qApp, SLOT(showHelp()));

    // File > About
    addActionToQMenuAndActionHash(fileMenu, MenuOption::AboutApp, 0, qApp, SLOT(aboutApp()), QAction::AboutRole);

    // File > Quit
    addActionToQMenuAndActionHash(fileMenu, MenuOption::Quit, Qt::CTRL | Qt::Key_Q, qApp, SLOT(quit()), QAction::QuitRole);


    // Edit menu ----------------------------------
    MenuWrapper* editMenu = addMenu("Edit");

    // Edit > Undo
    QUndoStack* undoStack = qApp->getUndoStack();
    QAction* undoAction = undoStack->createUndoAction(editMenu);
    undoAction->setShortcut(Qt::CTRL | Qt::Key_Z);
    addActionToQMenuAndActionHash(editMenu, undoAction);

    // Edit > Redo
    QAction* redoAction = undoStack->createRedoAction(editMenu);
    redoAction->setShortcut(Qt::CTRL | Qt::SHIFT | Qt::Key_Z);
    addActionToQMenuAndActionHash(editMenu, redoAction);

    // Edit > Running Sccripts
    addActionToQMenuAndActionHash(editMenu, MenuOption::RunningScripts, Qt::CTRL | Qt::Key_J,
        qApp, SLOT(toggleRunningScriptsWidget()));

    // Edit > Open and Run Script from File... [advanced]
    addActionToQMenuAndActionHash(editMenu, MenuOption::LoadScript, Qt::CTRL | Qt::Key_O,
        qApp, SLOT(loadDialog()),
        QAction::NoRole, UNSPECIFIED_POSITION, "Advanced");

    // Edit > Open and Run Script from Url... [advanced]
    addActionToQMenuAndActionHash(editMenu, MenuOption::LoadScriptURL,
        Qt::CTRL | Qt::SHIFT | Qt::Key_O, qApp, SLOT(loadScriptURLDialog()),
        QAction::NoRole, UNSPECIFIED_POSITION, "Advanced");

    auto scriptEngines = DependencyManager::get<ScriptEngines>();
    // Edit > Stop All Scripts... [advanced]
    addActionToQMenuAndActionHash(editMenu, MenuOption::StopAllScripts, 0, 
        scriptEngines.data(), SLOT(stopAllScripts()),
        QAction::NoRole, UNSPECIFIED_POSITION, "Advanced");

    // Edit > Reload All Scripts... [advanced]
    addActionToQMenuAndActionHash(editMenu, MenuOption::ReloadAllScripts, Qt::CTRL | Qt::Key_R,
        scriptEngines.data(), SLOT(reloadAllScripts()),
        QAction::NoRole, UNSPECIFIED_POSITION, "Advanced");

    // Edit > Scripts Editor... [advanced]
    addActionToQMenuAndActionHash(editMenu, MenuOption::ScriptEditor, Qt::ALT | Qt::Key_S,
        dialogsManager.data(), SLOT(showScriptEditor()),
        QAction::NoRole, UNSPECIFIED_POSITION, "Advanced");

    // Edit > Console... [advanced]
    addActionToQMenuAndActionHash(editMenu, MenuOption::Console, Qt::CTRL | Qt::ALT | Qt::Key_J,
        DependencyManager::get<StandAloneJSConsole>().data(),
        SLOT(toggleConsole()),
        QAction::NoRole, UNSPECIFIED_POSITION, "Advanced");

    // Edit > Reload All Content [advanced]
    addActionToQMenuAndActionHash(editMenu, MenuOption::ReloadContent, 0, qApp, SLOT(reloadResourceCaches()),
        QAction::NoRole, UNSPECIFIED_POSITION, "Advanced");


    // Edit > Package Model... [advanced]
    addActionToQMenuAndActionHash(editMenu, MenuOption::PackageModel, 0,
        qApp, SLOT(packageModel()),
        QAction::NoRole, UNSPECIFIED_POSITION, "Advanced");


    // Audio menu ----------------------------------
    MenuWrapper* audioMenu = addMenu("Audio");
    auto audioIO = DependencyManager::get<AudioClient>();

    // Audio > Mute
    addCheckableActionToQMenuAndActionHash(audioMenu, MenuOption::MuteAudio, Qt::CTRL | Qt::Key_M, false, 
        audioIO.data(), SLOT(toggleMute()));

    // Audio > Show Level Meter
    addCheckableActionToQMenuAndActionHash(audioMenu, MenuOption::AudioTools, 0, true);


    // Avatar menu ----------------------------------
    MenuWrapper* avatarMenu = addMenu("Avatar");
    auto avatarManager = DependencyManager::get<AvatarManager>();
    QObject* avatar = avatarManager->getMyAvatar();

    // Avatar > Attachments...
    auto action = addActionToQMenuAndActionHash(avatarMenu, MenuOption::Attachments);
    connect(action, &QAction::triggered, [] {
        DependencyManager::get<OffscreenUi>()->show(QString("hifi/dialogs/AttachmentsDialog.qml"), "AttachmentsDialog");
    });


    // Avatar > Size
    MenuWrapper* avatarSizeMenu = avatarMenu->addMenu("Size");

    // Avatar > Size > Increase
    addActionToQMenuAndActionHash(avatarSizeMenu,
        MenuOption::IncreaseAvatarSize,
        0, // QML Qt::Key_Plus,
        avatar, SLOT(increaseSize()));

    // Avatar > Size > Decrease
    addActionToQMenuAndActionHash(avatarSizeMenu,
        MenuOption::DecreaseAvatarSize,
        0, // QML Qt::Key_Minus,
        avatar, SLOT(decreaseSize()));

    // Avatar > Size > Reset
    addActionToQMenuAndActionHash(avatarSizeMenu,
        MenuOption::ResetAvatarSize,
        0, // QML Qt::Key_Equal,
        avatar, SLOT(resetSize()));

    // Avatar > Reset Sensors
    addActionToQMenuAndActionHash(avatarMenu,
        MenuOption::ResetSensors,
        0, // QML Qt::Key_Apostrophe,
        qApp, SLOT(resetSensors()));


    // Display menu ----------------------------------
    // FIXME - this is not yet matching Alan's spec because it doesn't have
    // menus for "2D"/"3D" - we need to add support for detecting the appropriate
    // default 3D display mode
    addMenu(DisplayPlugin::MENU_PATH());
    MenuWrapper* displayModeMenu = addMenu(MenuOption::OutputMenu);
    QActionGroup* displayModeGroup = new QActionGroup(displayModeMenu);
    displayModeGroup->setExclusive(true);


    // View menu ----------------------------------
    MenuWrapper* viewMenu = addMenu("View");
    QActionGroup* cameraModeGroup = new QActionGroup(viewMenu);

    // View > [camera group]
    cameraModeGroup->setExclusive(true);

    // View > First Person
    cameraModeGroup->addAction(addCheckableActionToQMenuAndActionHash(viewMenu,
        MenuOption::FirstPerson, 0, // QML Qt:: Key_P
        false, qApp, SLOT(cameraMenuChanged())));

    // View > Third Person
    cameraModeGroup->addAction(addCheckableActionToQMenuAndActionHash(viewMenu,
        MenuOption::ThirdPerson, 0,
        true, qApp, SLOT(cameraMenuChanged())));

    // View > Mirror
    cameraModeGroup->addAction(addCheckableActionToQMenuAndActionHash(viewMenu,
        MenuOption::FullscreenMirror, 0, // QML Qt::Key_H,
        false, qApp, SLOT(cameraMenuChanged())));

    // View > Independent [advanced]
    cameraModeGroup->addAction(addCheckableActionToQMenuAndActionHash(viewMenu,
        MenuOption::IndependentMode, 0,
        false, qApp, SLOT(cameraMenuChanged()),
        UNSPECIFIED_POSITION, "Advanced"));

    // View > Entity Camera [advanced]
    cameraModeGroup->addAction(addCheckableActionToQMenuAndActionHash(viewMenu,
        MenuOption::CameraEntityMode, 0,
        false, qApp, SLOT(cameraMenuChanged()),
        UNSPECIFIED_POSITION, "Advanced"));

    viewMenu->addSeparator();

    // View > Mini Mirror
    addCheckableActionToQMenuAndActionHash(viewMenu, MenuOption::MiniMirror, 0, false);

    // View > Center Player In View
    addCheckableActionToQMenuAndActionHash(viewMenu, MenuOption::CenterPlayerInView,
        0, true, qApp, SLOT(rotationModeChanged()),
        UNSPECIFIED_POSITION, "Advanced");

    // View > Overlays
    addCheckableActionToQMenuAndActionHash(viewMenu, MenuOption::Overlays, 0, true,
        qApp, SLOT(setOverlaysVisible(bool)));

    // Navigate menu ----------------------------------
    MenuWrapper* navigateMenu = addMenu("Navigate");

    // Navigate > Show Address Bar
    addActionToQMenuAndActionHash(navigateMenu, MenuOption::AddressBar, Qt::CTRL | Qt::Key_L,
        dialogsManager.data(), SLOT(toggleAddressBar()));

    // Navigate > Bookmark related menus -- Note: the Bookmark class adds its own submenus here.
    qApp->getBookmarks()->setupMenus(this, navigateMenu);

    // Navigate > Copy Address [advanced]
    auto addressManager = DependencyManager::get<AddressManager>();
    addActionToQMenuAndActionHash(navigateMenu, MenuOption::CopyAddress, 0,
        addressManager.data(), SLOT(copyAddress()),
        QAction::NoRole, UNSPECIFIED_POSITION, "Advanced");

    // Navigate > Copy Path [advanced]
    addActionToQMenuAndActionHash(navigateMenu, MenuOption::CopyPath, 0,
        addressManager.data(), SLOT(copyPath()),
        QAction::NoRole, UNSPECIFIED_POSITION, "Advanced");


    // Settings menu ----------------------------------
    MenuWrapper* settingsMenu = addMenu("Settings");

    // Settings > Advance Menus
    addCheckableActionToQMenuAndActionHash(settingsMenu, "Advanced Menus", 0, false, this, SLOT(toggleAdvancedMenus()));

    // Settings > Developer Menus
    addCheckableActionToQMenuAndActionHash(settingsMenu, "Developer Menus", 0, false, this, SLOT(toggleDeveloperMenus()));

    // Settings > General...
    action = addActionToQMenuAndActionHash(settingsMenu, MenuOption::Preferences, Qt::CTRL | Qt::Key_Comma, nullptr, nullptr, QAction::PreferencesRole);
    connect(action, &QAction::triggered, [] {
        DependencyManager::get<OffscreenUi>()->toggle(QString("hifi/dialogs/GeneralPreferencesDialog.qml"), "GeneralPreferencesDialog");
    });

    // Settings > Avatar...
    action = addActionToQMenuAndActionHash(settingsMenu, "Avatar...");
    connect(action, &QAction::triggered, [] {
        DependencyManager::get<OffscreenUi>()->toggle(QString("hifi/dialogs/AvatarPreferencesDialog.qml"), "AvatarPreferencesDialog");
    });

    // Settings > Audio...
    action = addActionToQMenuAndActionHash(settingsMenu, "Audio...");
    connect(action, &QAction::triggered, [] {
        DependencyManager::get<OffscreenUi>()->toggle(QString("hifi/dialogs/AudioPreferencesDialog.qml"), "AudioPreferencesDialog");
    });

    // Settings > LOD...
    action = addActionToQMenuAndActionHash(settingsMenu, "LOD...");
    connect(action, &QAction::triggered, [] {
        DependencyManager::get<OffscreenUi>()->toggle(QString("hifi/dialogs/LodPreferencesDialog.qml"), "LodPreferencesDialog");
    });

    // Settings > Control with Speech [advanced]
#if defined(Q_OS_MAC) || defined(Q_OS_WIN)
    auto speechRecognizer = DependencyManager::get<SpeechRecognizer>();
    QAction* speechRecognizerAction = addCheckableActionToQMenuAndActionHash(settingsMenu, MenuOption::ControlWithSpeech,
        Qt::CTRL | Qt::SHIFT | Qt::Key_C,
        speechRecognizer->getEnabled(),
        speechRecognizer.data(),
        SLOT(setEnabled(bool)),
        UNSPECIFIED_POSITION, "Advanced");
    connect(speechRecognizer.data(), SIGNAL(enabledUpdated(bool)), speechRecognizerAction, SLOT(setChecked(bool)));
#endif

    // Settings > Input Devices
    MenuWrapper* inputModeMenu = addMenu(MenuOption::InputMenu, "Advanced");
    QActionGroup* inputModeGroup = new QActionGroup(inputModeMenu);
    inputModeGroup->setExclusive(false);


    // Developer menu ----------------------------------
    MenuWrapper* developerMenu = addMenu("Developer", "Developer");

    // Developer > Graphics...
    action = addActionToQMenuAndActionHash(developerMenu, "Graphics...");
    connect(action, &QAction::triggered, [] {
        DependencyManager::get<OffscreenUi>()->toggle(QString("hifi/dialogs/GraphicsPreferencesDialog.qml"), "GraphicsPreferencesDialog");
    });

    // Developer > Render >>>
    MenuWrapper* renderOptionsMenu = developerMenu->addMenu("Render");
    addCheckableActionToQMenuAndActionHash(renderOptionsMenu, MenuOption::WorldAxes);
    addCheckableActionToQMenuAndActionHash(renderOptionsMenu, MenuOption::Stars, 0, true);

    // Developer > Render > Throttle FPS If Not Focus
    addCheckableActionToQMenuAndActionHash(renderOptionsMenu, MenuOption::ThrottleFPSIfNotFocus, 0, true);

    // Developer > Render > Resolution
    MenuWrapper* resolutionMenu = renderOptionsMenu->addMenu(MenuOption::RenderResolution);
    QActionGroup* resolutionGroup = new QActionGroup(resolutionMenu);
    resolutionGroup->setExclusive(true);
    resolutionGroup->addAction(addCheckableActionToQMenuAndActionHash(resolutionMenu, MenuOption::RenderResolutionOne, 0, true));
    resolutionGroup->addAction(addCheckableActionToQMenuAndActionHash(resolutionMenu, MenuOption::RenderResolutionTwoThird, 0, false));
    resolutionGroup->addAction(addCheckableActionToQMenuAndActionHash(resolutionMenu, MenuOption::RenderResolutionHalf, 0, false));
    resolutionGroup->addAction(addCheckableActionToQMenuAndActionHash(resolutionMenu, MenuOption::RenderResolutionThird, 0, false));
    resolutionGroup->addAction(addCheckableActionToQMenuAndActionHash(resolutionMenu, MenuOption::RenderResolutionQuarter, 0, false));

    // Developer > Render > LOD Tools
    addActionToQMenuAndActionHash(renderOptionsMenu, MenuOption::LodTools, 0, dialogsManager.data(), SLOT(lodTools()));

    // Developer > Assets >>>
    MenuWrapper* assetDeveloperMenu = developerMenu->addMenu("Assets");
    auto& assetDialogFactory = AssetUploadDialogFactory::getInstance();
    assetDialogFactory.setDialogParent(this);
    QAction* assetUpload = addActionToQMenuAndActionHash(assetDeveloperMenu,
        MenuOption::UploadAsset,
        0,
        &assetDialogFactory,
        SLOT(showDialog()));

    // disable the asset upload action by default - it gets enabled only if asset server becomes present
    assetUpload->setEnabled(false);

    auto& atpMigrator = ATPAssetMigrator::getInstance();
    atpMigrator.setDialogParent(this);

    addActionToQMenuAndActionHash(assetDeveloperMenu, MenuOption::AssetMigration,
        0, &atpMigrator,
        SLOT(loadEntityServerFile()));

    // Developer > Avatar >>>
    MenuWrapper* avatarDebugMenu = developerMenu->addMenu("Avatar");

    // Developer > Avatar > Face Tracking
    MenuWrapper* faceTrackingMenu = avatarDebugMenu->addMenu("Face Tracking");
    {
        QActionGroup* faceTrackerGroup = new QActionGroup(avatarDebugMenu);

        bool defaultNoFaceTracking = true;
#ifdef HAVE_DDE
        defaultNoFaceTracking = false;
#endif
        QAction* noFaceTracker = addCheckableActionToQMenuAndActionHash(faceTrackingMenu, MenuOption::NoFaceTracking,
            0, defaultNoFaceTracking,
            qApp, SLOT(setActiveFaceTracker()));
        faceTrackerGroup->addAction(noFaceTracker);

#ifdef HAVE_FACESHIFT
        QAction* faceshiftFaceTracker = addCheckableActionToQMenuAndActionHash(faceTrackingMenu, MenuOption::Faceshift,
            0, false,
            qApp, SLOT(setActiveFaceTracker()));
        faceTrackerGroup->addAction(faceshiftFaceTracker);
#endif
#ifdef HAVE_DDE
        QAction* ddeFaceTracker = addCheckableActionToQMenuAndActionHash(faceTrackingMenu, MenuOption::UseCamera,
            0, true,
            qApp, SLOT(setActiveFaceTracker()));
        faceTrackerGroup->addAction(ddeFaceTracker);
#endif
    }
#ifdef HAVE_DDE
    faceTrackingMenu->addSeparator();
    QAction* binaryEyelidControl = addCheckableActionToQMenuAndActionHash(faceTrackingMenu, MenuOption::BinaryEyelidControl, 0, true);
    binaryEyelidControl->setVisible(true);  // DDE face tracking is on by default
    QAction* coupleEyelids = addCheckableActionToQMenuAndActionHash(faceTrackingMenu, MenuOption::CoupleEyelids, 0, true);
    coupleEyelids->setVisible(true);  // DDE face tracking is on by default
    QAction* useAudioForMouth = addCheckableActionToQMenuAndActionHash(faceTrackingMenu, MenuOption::UseAudioForMouth, 0, true);
    useAudioForMouth->setVisible(true);  // DDE face tracking is on by default
    QAction* ddeFiltering = addCheckableActionToQMenuAndActionHash(faceTrackingMenu, MenuOption::VelocityFilter, 0, true);
    ddeFiltering->setVisible(true);  // DDE face tracking is on by default
    QAction* ddeCalibrate = addActionToQMenuAndActionHash(faceTrackingMenu, MenuOption::CalibrateCamera, 0,
        DependencyManager::get<DdeFaceTracker>().data(), SLOT(calibrate()));
    ddeCalibrate->setVisible(true);  // DDE face tracking is on by default
#endif
#if defined(HAVE_FACESHIFT) || defined(HAVE_DDE)
    faceTrackingMenu->addSeparator();
    addCheckableActionToQMenuAndActionHash(faceTrackingMenu, MenuOption::MuteFaceTracking,
        Qt::CTRL | Qt::SHIFT | Qt::Key_F, true);  // DDE face tracking is on by default
    addCheckableActionToQMenuAndActionHash(faceTrackingMenu, MenuOption::AutoMuteAudio, 0, false);
#endif

#ifdef HAVE_IVIEWHMD
    // Developer > Avatar > Eye Tracking
    MenuWrapper* eyeTrackingMenu = avatarDebugMenu->addMenu("Eye Tracking");
    addCheckableActionToQMenuAndActionHash(eyeTrackingMenu, MenuOption::SMIEyeTracking, 0, false,
        qApp, SLOT(setActiveEyeTracker()));
    {
        MenuWrapper* calibrateEyeTrackingMenu = eyeTrackingMenu->addMenu("Calibrate");
        addActionToQMenuAndActionHash(calibrateEyeTrackingMenu, MenuOption::OnePointCalibration, 0,
            qApp, SLOT(calibrateEyeTracker1Point()));
        addActionToQMenuAndActionHash(calibrateEyeTrackingMenu, MenuOption::ThreePointCalibration, 0,
            qApp, SLOT(calibrateEyeTracker3Points()));
        addActionToQMenuAndActionHash(calibrateEyeTrackingMenu, MenuOption::FivePointCalibration, 0,
            qApp, SLOT(calibrateEyeTracker5Points()));
    }
    addCheckableActionToQMenuAndActionHash(eyeTrackingMenu, MenuOption::SimulateEyeTracking, 0, false,
        qApp, SLOT(setActiveEyeTracker()));
#endif

    addCheckableActionToQMenuAndActionHash(avatarDebugMenu, MenuOption::AvatarReceiveStats, 0, false,
        avatarManager.data(), SLOT(setShouldShowReceiveStats(bool)));

    addCheckableActionToQMenuAndActionHash(avatarDebugMenu, MenuOption::RenderBoundingCollisionShapes);
    addCheckableActionToQMenuAndActionHash(avatarDebugMenu, MenuOption::RenderLookAtVectors, 0, false);
    addCheckableActionToQMenuAndActionHash(avatarDebugMenu, MenuOption::RenderLookAtTargets, 0, false);
    addCheckableActionToQMenuAndActionHash(avatarDebugMenu, MenuOption::RenderFocusIndicator, 0, false);
    addCheckableActionToQMenuAndActionHash(avatarDebugMenu, MenuOption::ShowWhosLookingAtMe, 0, false);
    addCheckableActionToQMenuAndActionHash(avatarDebugMenu, MenuOption::FixGaze, 0, false);
    addCheckableActionToQMenuAndActionHash(avatarDebugMenu, MenuOption::AnimDebugDrawDefaultPose, 0, false,
        avatar, SLOT(setEnableDebugDrawDefaultPose(bool)));
    addCheckableActionToQMenuAndActionHash(avatarDebugMenu, MenuOption::AnimDebugDrawAnimPose, 0, false,
        avatar, SLOT(setEnableDebugDrawAnimPose(bool)));
    addCheckableActionToQMenuAndActionHash(avatarDebugMenu, MenuOption::AnimDebugDrawPosition, 0, false,
        avatar, SLOT(setEnableDebugDrawPosition(bool)));
    addCheckableActionToQMenuAndActionHash(avatarDebugMenu, MenuOption::MeshVisible, 0, true,
        avatar, SLOT(setEnableMeshVisible(bool)));
    addCheckableActionToQMenuAndActionHash(avatarDebugMenu, MenuOption::DisableEyelidAdjustment, 0, false);
    addCheckableActionToQMenuAndActionHash(avatarDebugMenu, MenuOption::TurnWithHead, 0, false);
    addCheckableActionToQMenuAndActionHash(avatarDebugMenu, MenuOption::UseAnimPreAndPostRotations, 0, false,
        avatar, SLOT(setUseAnimPreAndPostRotations(bool)));
    addCheckableActionToQMenuAndActionHash(avatarDebugMenu, MenuOption::EnableInverseKinematics, 0, true,
        avatar, SLOT(setEnableInverseKinematics(bool)));
    addCheckableActionToQMenuAndActionHash(avatarDebugMenu, MenuOption::RenderSensorToWorldMatrix, 0, false,
        avatar, SLOT(setEnableDebugDrawSensorToWorldMatrix(bool)));

    addCheckableActionToQMenuAndActionHash(avatarDebugMenu, MenuOption::KeyboardMotorControl,
        Qt::CTRL | Qt::SHIFT | Qt::Key_K, true, avatar, SLOT(updateMotionBehaviorFromMenu()),
        UNSPECIFIED_POSITION, "Developer");

    addCheckableActionToQMenuAndActionHash(avatarDebugMenu, MenuOption::ScriptedMotorControl, 0, true,
        avatar, SLOT(updateMotionBehaviorFromMenu()),
        UNSPECIFIED_POSITION, "Developer");

    addCheckableActionToQMenuAndActionHash(avatarDebugMenu, MenuOption::EnableCharacterController, 0, true,
        avatar, SLOT(updateMotionBehaviorFromMenu()),
        UNSPECIFIED_POSITION, "Developer");

    // Developer > Hands >>>
    MenuWrapper* handOptionsMenu = developerMenu->addMenu("Hands");
    addCheckableActionToQMenuAndActionHash(handOptionsMenu, MenuOption::DisplayHandTargets, 0, false,
        avatar, SLOT(setEnableDebugDrawHandControllers(bool)));
    addCheckableActionToQMenuAndActionHash(handOptionsMenu, MenuOption::LowVelocityFilter, 0, true,
        qApp, SLOT(setLowVelocityFilter(bool)));

    MenuWrapper* leapOptionsMenu = handOptionsMenu->addMenu("Leap Motion");
    addCheckableActionToQMenuAndActionHash(leapOptionsMenu, MenuOption::LeapMotionOnHMD, 0, false);

    // Developer > Entities >>>
    MenuWrapper* entitiesOptionsMenu = developerMenu->addMenu("Entities");
    addActionToQMenuAndActionHash(entitiesOptionsMenu, MenuOption::OctreeStats, 0,
        dialogsManager.data(), SLOT(octreeStatsDetails()));
    addCheckableActionToQMenuAndActionHash(entitiesOptionsMenu, MenuOption::ShowRealtimeEntityStats);

    // Developer > Network >>>
    MenuWrapper* networkMenu = developerMenu->addMenu("Network");
    addActionToQMenuAndActionHash(networkMenu, MenuOption::ReloadContent, 0, qApp, SLOT(reloadResourceCaches()));
    addCheckableActionToQMenuAndActionHash(networkMenu, MenuOption::DisableNackPackets, 0, false,
        qApp->getEntityEditPacketSender(),
        SLOT(toggleNackPackets()));
    addCheckableActionToQMenuAndActionHash(networkMenu,
        MenuOption::DisableActivityLogger,
        0,
        false,
        &UserActivityLogger::getInstance(),
        SLOT(disable(bool)));
    addActionToQMenuAndActionHash(networkMenu, MenuOption::CachesSize, 0,
        dialogsManager.data(), SLOT(cachesSizeDialog()));
    addActionToQMenuAndActionHash(networkMenu, MenuOption::DiskCacheEditor, 0,
        dialogsManager.data(), SLOT(toggleDiskCacheEditor()));
    addActionToQMenuAndActionHash(networkMenu, MenuOption::ShowDSConnectTable, 0,
        dialogsManager.data(), SLOT(showDomainConnectionDialog()));
    addActionToQMenuAndActionHash(networkMenu, MenuOption::BandwidthDetails, 0,
        dialogsManager.data(), SLOT(bandwidthDetails()));


    // Developer > Timing >>>
    MenuWrapper* timingMenu = developerMenu->addMenu("Timing");
    MenuWrapper* perfTimerMenu = timingMenu->addMenu("Performance Timer");
    addCheckableActionToQMenuAndActionHash(perfTimerMenu, MenuOption::DisplayDebugTimingDetails, 0, false);
    addCheckableActionToQMenuAndActionHash(perfTimerMenu, MenuOption::OnlyDisplayTopTen, 0, true);
    addCheckableActionToQMenuAndActionHash(perfTimerMenu, MenuOption::ExpandUpdateTiming, 0, false);
    addCheckableActionToQMenuAndActionHash(perfTimerMenu, MenuOption::ExpandMyAvatarTiming, 0, false);
    addCheckableActionToQMenuAndActionHash(perfTimerMenu, MenuOption::ExpandMyAvatarSimulateTiming, 0, false);
    addCheckableActionToQMenuAndActionHash(perfTimerMenu, MenuOption::ExpandOtherAvatarTiming, 0, false);
    addCheckableActionToQMenuAndActionHash(perfTimerMenu, MenuOption::ExpandPaintGLTiming, 0, false);

    addCheckableActionToQMenuAndActionHash(timingMenu, MenuOption::FrameTimer);
    addActionToQMenuAndActionHash(timingMenu, MenuOption::RunTimingTests, 0, qApp, SLOT(runTests()));
    addCheckableActionToQMenuAndActionHash(timingMenu, MenuOption::PipelineWarnings);
    addCheckableActionToQMenuAndActionHash(timingMenu, MenuOption::LogExtraTimings);
    addCheckableActionToQMenuAndActionHash(timingMenu, MenuOption::SuppressShortTimings);

    // Developer > Audio >>>
    MenuWrapper* audioDebugMenu = developerMenu->addMenu("Audio");
    addCheckableActionToQMenuAndActionHash(audioDebugMenu, MenuOption::AudioNoiseReduction, 0, true, 
        audioIO.data(), SLOT(toggleAudioNoiseReduction()));
    addCheckableActionToQMenuAndActionHash(audioDebugMenu, MenuOption::EchoServerAudio, 0, false,
        audioIO.data(), SLOT(toggleServerEcho()));
    addCheckableActionToQMenuAndActionHash(audioDebugMenu, MenuOption::EchoLocalAudio, 0, false,
        audioIO.data(), SLOT(toggleLocalEcho()));
    addActionToQMenuAndActionHash(audioDebugMenu, MenuOption::MuteEnvironment, 0,
        audioIO.data(), SLOT(sendMuteEnvironmentPacket()));

    auto scope = DependencyManager::get<AudioScope>();
    MenuWrapper* audioScopeMenu = audioDebugMenu->addMenu("Audio Scope");
    addCheckableActionToQMenuAndActionHash(audioScopeMenu, MenuOption::AudioScope, Qt::CTRL | Qt::Key_P, false,
        scope.data(), SLOT(toggle()));
    addCheckableActionToQMenuAndActionHash(audioScopeMenu, MenuOption::AudioScopePause, Qt::CTRL | Qt::SHIFT | Qt::Key_P, false,
        scope.data(), SLOT(togglePause()));

    addDisabledActionAndSeparator(audioScopeMenu, "Display Frames");
    {
        QAction* fiveFrames = addCheckableActionToQMenuAndActionHash(audioScopeMenu, MenuOption::AudioScopeFiveFrames,
            0, true, scope.data(), SLOT(selectAudioScopeFiveFrames()));

        QAction* twentyFrames = addCheckableActionToQMenuAndActionHash(audioScopeMenu, MenuOption::AudioScopeTwentyFrames,
            0, false, scope.data(), SLOT(selectAudioScopeTwentyFrames()));

        QAction* fiftyFrames = addCheckableActionToQMenuAndActionHash(audioScopeMenu, MenuOption::AudioScopeFiftyFrames,
            0, false, scope.data(), SLOT(selectAudioScopeFiftyFrames()));

        QActionGroup* audioScopeFramesGroup = new QActionGroup(audioScopeMenu);
        audioScopeFramesGroup->addAction(fiveFrames);
        audioScopeFramesGroup->addAction(twentyFrames);
        audioScopeFramesGroup->addAction(fiftyFrames);
    }

    // Developer > Audio > Audio Network Stats...
    addActionToQMenuAndActionHash(audioDebugMenu, MenuOption::AudioNetworkStats, 0,
        dialogsManager.data(), SLOT(audioStatsDetails()));

    // Developer > Physics >>>
    MenuWrapper* physicsOptionsMenu = developerMenu->addMenu("Physics");
    {
        auto drawStatusConfig = qApp->getRenderEngine()->getConfiguration()->getConfig<render::DrawStatus>();
        addCheckableActionToQMenuAndActionHash(physicsOptionsMenu, MenuOption::PhysicsShowOwned,
            0, false, drawStatusConfig, SLOT(setShowNetwork(bool)));
    }
    addCheckableActionToQMenuAndActionHash(physicsOptionsMenu, MenuOption::PhysicsShowHulls);

    // Developer > Display Crash Options
    addCheckableActionToQMenuAndActionHash(developerMenu, MenuOption::DisplayCrashOptions, 0, true);
    // Developer > Crash Application
    addActionToQMenuAndActionHash(developerMenu, MenuOption::CrashInterface, 0, qApp, SLOT(crashApplication()));
    // Developer > Deadlock Application
    addActionToQMenuAndActionHash(developerMenu, MenuOption::DeadlockInterface, 0, qApp, SLOT(deadlockApplication()));

    // Developer > Log...
    addActionToQMenuAndActionHash(developerMenu, MenuOption::Log, Qt::CTRL | Qt::SHIFT | Qt::Key_L,
         qApp, SLOT(toggleLogDialog()));

    // Developer > Stats
    addCheckableActionToQMenuAndActionHash(developerMenu, MenuOption::Stats);


#if 0 ///  -------------- REMOVED FOR NOW --------------
    addDisabledActionAndSeparator(navigateMenu, "History");
    QAction* backAction = addActionToQMenuAndActionHash(navigateMenu, MenuOption::Back, 0, addressManager.data(), SLOT(goBack()));
    QAction* forwardAction = addActionToQMenuAndActionHash(navigateMenu, MenuOption::Forward, 0, addressManager.data(), SLOT(goForward()));

    // connect to the AddressManager signal to enable and disable the back and forward menu items
    connect(addressManager.data(), &AddressManager::goBackPossible, backAction, &QAction::setEnabled);
    connect(addressManager.data(), &AddressManager::goForwardPossible, forwardAction, &QAction::setEnabled);

    // set the two actions to start disabled since the stacks are clear on startup
    backAction->setDisabled(true);
    forwardAction->setDisabled(true);

    MenuWrapper* toolsMenu = addMenu("Tools");
    addActionToQMenuAndActionHash(toolsMenu,
                                  MenuOption::ToolWindow,
                                  Qt::CTRL | Qt::ALT | Qt::Key_T,
                                  dialogsManager.data(),
                                  SLOT(toggleToolWindow()),
                                  QAction::NoRole, UNSPECIFIED_POSITION, "Advanced");


    addCheckableActionToQMenuAndActionHash(avatarMenu, MenuOption::NamesAboveHeads, 0, true, 
                NULL, NULL, UNSPECIFIED_POSITION, "Advanced");
#endif
}

void Menu::addMenuItem(const MenuItemProperties& properties) {
    MenuWrapper* menuObj = getMenu(properties.menuName);
    if (menuObj) {
        QShortcut* shortcut = NULL;
        if (!properties.shortcutKeySequence.isEmpty()) {
            shortcut = new QShortcut(properties.shortcutKeySequence, this);
            shortcut->setContext(Qt::WidgetWithChildrenShortcut);
        }

        // check for positioning requests
        int requestedPosition = properties.position;
        if (requestedPosition == UNSPECIFIED_POSITION && !properties.beforeItem.isEmpty()) {
            requestedPosition = findPositionOfMenuItem(menuObj, properties.beforeItem);
            // double check that the requested location wasn't a separator label
            requestedPosition = positionBeforeSeparatorIfNeeded(menuObj, requestedPosition);
        }
        if (requestedPosition == UNSPECIFIED_POSITION && !properties.afterItem.isEmpty()) {
            int afterPosition = findPositionOfMenuItem(menuObj, properties.afterItem);
            if (afterPosition != UNSPECIFIED_POSITION) {
                requestedPosition = afterPosition + 1;
            }
        }

        QAction* menuItemAction = NULL;
        if (properties.isSeparator) {
            addDisabledActionAndSeparator(menuObj, properties.menuItemName, requestedPosition, properties.grouping);
        } else if (properties.isCheckable) {
            menuItemAction = addCheckableActionToQMenuAndActionHash(menuObj, properties.menuItemName,
                                                                    properties.shortcutKeySequence, properties.isChecked,
                                                                    MenuScriptingInterface::getInstance(), SLOT(menuItemTriggered()), 
                                                                    requestedPosition, properties.grouping);
        } else {
            menuItemAction = addActionToQMenuAndActionHash(menuObj, properties.menuItemName, properties.shortcutKeySequence,
                                                           MenuScriptingInterface::getInstance(), SLOT(menuItemTriggered()),
                                                           QAction::NoRole, requestedPosition, properties.grouping);
        }
        if (shortcut && menuItemAction) {
            connect(shortcut, SIGNAL(activated()), menuItemAction, SLOT(trigger()));
        }
        QMenuBar::repaint();
    }
}
