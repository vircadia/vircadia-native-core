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

#include "Application.h"
#include "AccountManager.h"
#include "assets/ATPAssetMigrator.h"
#include "audio/AudioScope.h"
#include "avatar/AvatarManager.h"
#include "devices/DdeFaceTracker.h"
#include "devices/Faceshift.h"
#include "input-plugins/SpacemouseManager.h"
#include "MainWindow.h"
#include "scripting/MenuScriptingInterface.h"
#include "ui/AssetUploadDialogFactory.h"
#include "ui/DialogsManager.h"
#include "ui/StandAloneJSConsole.h"
#include "InterfaceLogging.h"

#if defined(Q_OS_MAC) || defined(Q_OS_WIN)
#include "SpeechRecognizer.h"
#endif

#include "Menu.h"

static const char* const MENU_PROPERTY_NAME = "com.highfidelity.Menu";

Menu* Menu::getInstance() {
    static Menu* instance = globalInstance<Menu>(MENU_PROPERTY_NAME);
    return instance;
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

    // File > Update -- FIXME: needs implementation
    auto updateAction = addActionToQMenuAndActionHash(fileMenu, "Update");
    updateAction->setDisabled(true);

    // File > Help
    addActionToQMenuAndActionHash(fileMenu, MenuOption::Help, 0, qApp, SLOT(showHelp()));

    // File > Crash Reporter...-- FIXME: needs implementation
    auto crashReporterAction = addActionToQMenuAndActionHash(fileMenu, "Crash Reporter...");
    crashReporterAction->setDisabled(true);

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

    // Edit > Stop All Scripts... [advanced]
    addActionToQMenuAndActionHash(editMenu, MenuOption::StopAllScripts, 0, qApp, SLOT(stopAllScripts()),
        QAction::NoRole, UNSPECIFIED_POSITION, "Advanced");

    // Edit > Reload All Scripts... [advanced]
    addActionToQMenuAndActionHash(editMenu, MenuOption::ReloadAllScripts, Qt::CTRL | Qt::Key_R,
        qApp, SLOT(reloadAllScripts()),
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

    // Audio > Level Meter  [advanced] -- FIXME: needs implementation
    auto levelMeterAction = addCheckableActionToQMenuAndActionHash(audioMenu, "Level Meter", 0, false, NULL, NULL, UNSPECIFIED_POSITION, "Advanced");
    levelMeterAction->setDisabled(true);


    // Avatar menu ----------------------------------
    MenuWrapper* avatarMenu = addMenu("Avatar");
    auto avatarManager = DependencyManager::get<AvatarManager>();
    QObject* avatar = avatarManager->getMyAvatar();

    // Avatar > Attachments...
    addActionToQMenuAndActionHash(avatarMenu, MenuOption::Attachments, 0,
        dialogsManager.data(), SLOT(editAttachments()));

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


    // Navigate menu ----------------------------------
    MenuWrapper* navigateMenu = addMenu("Navigate");

    // Navigate > Home -- FIXME: needs implementation
    auto homeAction = addActionToQMenuAndActionHash(navigateMenu, "Home");
    homeAction->setDisabled(true);

    addActionToQMenuAndActionHash(navigateMenu, MenuOption::AddressBar, Qt::CTRL | Qt::Key_L,
        dialogsManager.data(), SLOT(toggleAddressBar()));

    // Navigate > Directory -- FIXME: needs implementation
    addActionToQMenuAndActionHash(navigateMenu, "Directory");

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
    addActionToQMenuAndActionHash(settingsMenu, MenuOption::Preferences, Qt::CTRL | Qt::Key_Comma,
        dialogsManager.data(), SLOT(editPreferences()), QAction::PreferencesRole);

    // Settings > Avatar...-- FIXME: needs implementation
    auto avatarAction = addActionToQMenuAndActionHash(settingsMenu, "Avatar...");
    avatarAction->setDisabled(true);

    // Settings > Audio...-- FIXME: needs implementation
    auto audioAction = addActionToQMenuAndActionHash(settingsMenu, "Audio...");
    audioAction->setDisabled(true);

    // Settings > LOD...-- FIXME: needs implementation
    auto lodAction = addActionToQMenuAndActionHash(settingsMenu, "LOD...");
    lodAction->setDisabled(true);

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

    // Developer > Render >>>
    MenuWrapper* renderOptionsMenu = developerMenu->addMenu("Render");
    addCheckableActionToQMenuAndActionHash(renderOptionsMenu, MenuOption::Atmosphere, 0, true);
    addCheckableActionToQMenuAndActionHash(renderOptionsMenu, MenuOption::DebugAmbientOcclusion);
    addCheckableActionToQMenuAndActionHash(renderOptionsMenu, MenuOption::Antialiasing);

    // Developer > Render > Ambient Light
    MenuWrapper* ambientLightMenu = renderOptionsMenu->addMenu(MenuOption::RenderAmbientLight);
    QActionGroup* ambientLightGroup = new QActionGroup(ambientLightMenu);
    ambientLightGroup->setExclusive(true);
    ambientLightGroup->addAction(addCheckableActionToQMenuAndActionHash(ambientLightMenu, MenuOption::RenderAmbientLightGlobal, 0, true));
    ambientLightGroup->addAction(addCheckableActionToQMenuAndActionHash(ambientLightMenu, MenuOption::RenderAmbientLight0, 0, false));
    ambientLightGroup->addAction(addCheckableActionToQMenuAndActionHash(ambientLightMenu, MenuOption::RenderAmbientLight1, 0, false));
    ambientLightGroup->addAction(addCheckableActionToQMenuAndActionHash(ambientLightMenu, MenuOption::RenderAmbientLight2, 0, false));
    ambientLightGroup->addAction(addCheckableActionToQMenuAndActionHash(ambientLightMenu, MenuOption::RenderAmbientLight3, 0, false));
    ambientLightGroup->addAction(addCheckableActionToQMenuAndActionHash(ambientLightMenu, MenuOption::RenderAmbientLight4, 0, false));
    ambientLightGroup->addAction(addCheckableActionToQMenuAndActionHash(ambientLightMenu, MenuOption::RenderAmbientLight5, 0, false));
    ambientLightGroup->addAction(addCheckableActionToQMenuAndActionHash(ambientLightMenu, MenuOption::RenderAmbientLight6, 0, false));
    ambientLightGroup->addAction(addCheckableActionToQMenuAndActionHash(ambientLightMenu, MenuOption::RenderAmbientLight7, 0, false));
    ambientLightGroup->addAction(addCheckableActionToQMenuAndActionHash(ambientLightMenu, MenuOption::RenderAmbientLight8, 0, false));
    ambientLightGroup->addAction(addCheckableActionToQMenuAndActionHash(ambientLightMenu, MenuOption::RenderAmbientLight9, 0, false));

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

    // Developer > Render > Stars
    addCheckableActionToQMenuAndActionHash(renderOptionsMenu, MenuOption::Stars,
        0, // QML Qt::Key_Asterisk,
        true);

    // Developer > Render > LOD Tools
    addActionToQMenuAndActionHash(renderOptionsMenu, MenuOption::LodTools,
        0, // QML Qt::SHIFT | Qt::Key_L,
        dialogsManager.data(), SLOT(lodTools()));

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
    addCheckableActionToQMenuAndActionHash(avatarDebugMenu, MenuOption::ComfortMode, 0, true);

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
    addCheckableActionToQMenuAndActionHash(handOptionsMenu, MenuOption::DisplayHandTargets, 0, false);
    addCheckableActionToQMenuAndActionHash(handOptionsMenu, MenuOption::LowVelocityFilter, 0, true,
        qApp, SLOT(setLowVelocityFilter(bool)));

    MenuWrapper* leapOptionsMenu = handOptionsMenu->addMenu("Leap Motion");
    addCheckableActionToQMenuAndActionHash(leapOptionsMenu, MenuOption::LeapMotionOnHMD, 0, false);

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

    // Developer > Timing and Stats >>>
    MenuWrapper* timingMenu = developerMenu->addMenu("Timing and Stats");
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
    addCheckableActionToQMenuAndActionHash(timingMenu, MenuOption::ShowRealtimeEntityStats);

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

    // Developer > Physics >>>
    MenuWrapper* physicsOptionsMenu = developerMenu->addMenu("Physics");
    addCheckableActionToQMenuAndActionHash(physicsOptionsMenu, MenuOption::PhysicsShowOwned);
    addCheckableActionToQMenuAndActionHash(physicsOptionsMenu, MenuOption::PhysicsShowHulls);

    // Developer > Display Crash Options
    addCheckableActionToQMenuAndActionHash(developerMenu, MenuOption::DisplayCrashOptions, 0, true);
    // Developer > Crash Application
    addActionToQMenuAndActionHash(developerMenu, MenuOption::CrashInterface, 0, qApp, SLOT(crashApplication()));

    // Developer > Log...
    addActionToQMenuAndActionHash(developerMenu, MenuOption::Log, Qt::CTRL | Qt::SHIFT | Qt::Key_L,
         qApp, SLOT(toggleLogDialog()));

    // Developer > Stats
    addCheckableActionToQMenuAndActionHash(developerMenu, MenuOption::Stats);

    // Developer > Audio Stats...
    addActionToQMenuAndActionHash(developerMenu, MenuOption::AudioNetworkStats, 0,
        dialogsManager.data(), SLOT(audioStatsDetails()));

    // Developer > Bandwidth Stats...
    addActionToQMenuAndActionHash(developerMenu, MenuOption::BandwidthDetails, 0,
        dialogsManager.data(), SLOT(bandwidthDetails()));

    // Developer > Entity Stats...
    addActionToQMenuAndActionHash(developerMenu, MenuOption::OctreeStats, 0,
        dialogsManager.data(), SLOT(octreeStatsDetails()));

    // Developer > World Axes
    addCheckableActionToQMenuAndActionHash(developerMenu, MenuOption::WorldAxes);



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
    
    addCheckableActionToQMenuAndActionHash(viewMenu, MenuOption::CenterPlayerInView,
                                           0, false, qApp, SLOT(rotationModeChanged()),
                                           UNSPECIFIED_POSITION, "Advanced");

#endif
}

void Menu::toggleAdvancedMenus() {
    setGroupingIsVisible("Advanced", !getGroupingIsVisible("Advanced"));
}

void Menu::toggleDeveloperMenus() {
    setGroupingIsVisible("Developer", !getGroupingIsVisible("Developer"));
}

void Menu::loadSettings() {
    scanMenuBar(&Menu::loadAction);
}

void Menu::saveSettings() {
    scanMenuBar(&Menu::saveAction);
}

void Menu::loadAction(Settings& settings, QAction& action) {
    if (action.isChecked() != settings.value(action.text(), action.isChecked()).toBool()) {
        action.trigger();
    }
}

void Menu::saveAction(Settings& settings, QAction& action) {
    settings.setValue(action.text(),  action.isChecked());
}

void Menu::scanMenuBar(settingsAction modifySetting) {
    Settings settings;
    foreach (QMenu* menu, findChildren<QMenu*>()) {
        scanMenu(*menu, modifySetting, settings);
    }
}

void Menu::scanMenu(QMenu& menu, settingsAction modifySetting, Settings& settings) {
    settings.beginGroup(menu.title());
    foreach (QAction* action, menu.actions()) {
        if (action->menu()) {
            scanMenu(*action->menu(), modifySetting, settings);
        } else if (action->isCheckable()) {
            modifySetting(settings, *action);
        }
    }
    settings.endGroup();
}

void Menu::addDisabledActionAndSeparator(MenuWrapper* destinationMenu, const QString& actionName, 
                                            int menuItemLocation, const QString& grouping) {
    QAction* actionBefore = NULL;
    QAction* separator;
    QAction* separatorText;

    if (menuItemLocation >= 0 && destinationMenu->actions().size() > menuItemLocation) {
        actionBefore = destinationMenu->actions()[menuItemLocation];
    }
    if (actionBefore) {
        separator = new QAction("",destinationMenu);
        destinationMenu->insertAction(actionBefore, separator);
        separator->setSeparator(true);

        separatorText = new QAction(actionName,destinationMenu);
        separatorText->setEnabled(false);
        destinationMenu->insertAction(actionBefore, separatorText);

    } else {
        separator = destinationMenu->addSeparator();
        separatorText = destinationMenu->addAction(actionName);
        separatorText->setEnabled(false);
    }

    if (isValidGrouping(grouping)) {
        _groupingActions[grouping] << separator;
        _groupingActions[grouping] << separatorText;
        bool isVisible = getGroupingIsVisible(grouping);
        separator->setVisible(isVisible);
        separatorText->setVisible(isVisible);
    }
}

QAction* Menu::addActionToQMenuAndActionHash(MenuWrapper* destinationMenu,
                                             const QString& actionName,
                                             const QKeySequence& shortcut,
                                             const QObject* receiver,
                                             const char* member,
                                             QAction::MenuRole role,
                                             int menuItemLocation, 
                                             const QString& grouping) {
    QAction* action = NULL;
    QAction* actionBefore = NULL;

    if (menuItemLocation >= 0 && destinationMenu->actions().size() > menuItemLocation) {
        actionBefore = destinationMenu->actions()[menuItemLocation];
    }

    if (!actionBefore) {
        if (receiver && member) {
            action = destinationMenu->addAction(actionName, receiver, member, shortcut);
        } else {
            action = destinationMenu->addAction(actionName);
            action->setShortcut(shortcut);
        }
    } else {
        action = new QAction(actionName, destinationMenu);
        action->setShortcut(shortcut);
        destinationMenu->insertAction(actionBefore, action);

        if (receiver && member) {
            connect(action, SIGNAL(triggered()), receiver, member);
        }
    }
    action->setMenuRole(role);

    _actionHash.insert(actionName, action);

    if (isValidGrouping(grouping)) {
        _groupingActions[grouping] << action;
        action->setVisible(getGroupingIsVisible(grouping));
    }

    return action;
}

QAction* Menu::addActionToQMenuAndActionHash(MenuWrapper* destinationMenu,
                                             QAction* action,
                                             const QString& actionName,
                                             const QKeySequence& shortcut,
                                             QAction::MenuRole role,
                                             int menuItemLocation, 
                                             const QString& grouping) {
    QAction* actionBefore = NULL;

    if (menuItemLocation >= 0 && destinationMenu->actions().size() > menuItemLocation) {
        actionBefore = destinationMenu->actions()[menuItemLocation];
    }

    if (!actionName.isEmpty()) {
        action->setText(actionName);
    }

    if (shortcut != 0) {
        action->setShortcut(shortcut);
    }

    if (role != QAction::NoRole) {
        action->setMenuRole(role);
    }

    if (!actionBefore) {
        destinationMenu->addAction(action);
    } else {
        destinationMenu->insertAction(actionBefore, action);
    }

    _actionHash.insert(action->text(), action);

    if (isValidGrouping(grouping)) {
        _groupingActions[grouping] << action;
        action->setVisible(getGroupingIsVisible(grouping));
    }

    return action;
}

QAction* Menu::addCheckableActionToQMenuAndActionHash(MenuWrapper* destinationMenu,
                                                      const QString& actionName,
                                                      const QKeySequence& shortcut,
                                                      const bool checked,
                                                      const QObject* receiver,
                                                      const char* member,
                                                      int menuItemLocation, 
                                                      const QString& grouping) {

    QAction* action = addActionToQMenuAndActionHash(destinationMenu, actionName, shortcut, receiver, member,
                                                        QAction::NoRole, menuItemLocation);
    action->setCheckable(true);
    action->setChecked(checked);

    if (isValidGrouping(grouping)) {
        _groupingActions[grouping] << action;
        action->setVisible(getGroupingIsVisible(grouping));
    }

    return action;
}

void Menu::removeAction(MenuWrapper* menu, const QString& actionName) {
    auto action = _actionHash.value(actionName);
    menu->removeAction(action);
    _actionHash.remove(actionName);
    for (auto& grouping : _groupingActions) {
        grouping.remove(action);
    }
}

void Menu::setIsOptionChecked(const QString& menuOption, bool isChecked) {
    if (thread() != QThread::currentThread()) {
        QMetaObject::invokeMethod(Menu::getInstance(), "setIsOptionChecked", Qt::BlockingQueuedConnection,
                    Q_ARG(const QString&, menuOption),
                    Q_ARG(bool, isChecked));
        return;
    }
    QAction* menu = _actionHash.value(menuOption);
    if (menu) {
        menu->setChecked(isChecked);
    }
}

bool Menu::isOptionChecked(const QString& menuOption) const {
    const QAction* menu = _actionHash.value(menuOption);
    if (menu) {
        return menu->isChecked();
    }
    return false;
}

void Menu::triggerOption(const QString& menuOption) {
    QAction* action = _actionHash.value(menuOption);
    if (action) {
        action->trigger();
    } else {
        qCDebug(interfaceapp) << "NULL Action for menuOption '" << menuOption << "'";
    }
}

QAction* Menu::getActionForOption(const QString& menuOption) {
    return _actionHash.value(menuOption);
}

QAction* Menu::getActionFromName(const QString& menuName, MenuWrapper* menu) {
    QList<QAction*> menuActions;
    if (menu) {
        menuActions = menu->actions();
    } else {
        menuActions = actions();
    }

    foreach (QAction* menuAction, menuActions) {
        QString actionText = menuAction->text();
        if (menuName == menuAction->text()) {
            return menuAction;
        }
    }
    return NULL;
}

MenuWrapper* Menu::getSubMenuFromName(const QString& menuName, MenuWrapper* menu) {
    QAction* action = getActionFromName(menuName, menu);
    if (action) {
        return MenuWrapper::fromMenu(action->menu());
    }
    return NULL;
}

MenuWrapper* Menu::getMenuParent(const QString& menuName, QString& finalMenuPart) {
    QStringList menuTree = menuName.split(">");
    MenuWrapper* parent = NULL;
    MenuWrapper* menu = NULL;
    foreach (QString menuTreePart, menuTree) {
        parent = menu;
        finalMenuPart = menuTreePart.trimmed();
        menu = getSubMenuFromName(finalMenuPart, parent);
        if (!menu) {
            break;
        }
    }
    return parent;
}

MenuWrapper* Menu::getMenu(const QString& menuName) {
    QStringList menuTree = menuName.split(">");
    MenuWrapper* parent = NULL;
    MenuWrapper* menu = NULL;
    int item = 0;
    foreach (QString menuTreePart, menuTree) {
        menu = getSubMenuFromName(menuTreePart.trimmed(), parent);
        if (!menu) {
            break;
        }
        parent = menu;
        item++;
    }
    return menu;
}

QAction* Menu::getMenuAction(const QString& menuName) {
    QStringList menuTree = menuName.split(">");
    MenuWrapper* parent = NULL;
    QAction* action = NULL;
    foreach (QString menuTreePart, menuTree) {
        action = getActionFromName(menuTreePart.trimmed(), parent);
        if (!action) {
            break;
        }
        parent = MenuWrapper::fromMenu(action->menu());
    }
    return action;
}

int Menu::findPositionOfMenuItem(MenuWrapper* menu, const QString& searchMenuItem) {
    int position = 0;
    foreach(QAction* action, menu->actions()) {
        if (action->text() == searchMenuItem) {
            return position;
        }
        position++;
    }
    return UNSPECIFIED_POSITION; // not found
}

int Menu::positionBeforeSeparatorIfNeeded(MenuWrapper* menu, int requestedPosition) {
    QList<QAction*> menuActions = menu->actions();
    if (requestedPosition > 1 && requestedPosition < menuActions.size()) {
        QAction* beforeRequested = menuActions[requestedPosition - 1];
        if (beforeRequested->isSeparator()) {
            requestedPosition--;
        }
    }
    return requestedPosition;
}


MenuWrapper* Menu::addMenu(const QString& menuName, const QString& grouping) {
    QStringList menuTree = menuName.split(">");
    MenuWrapper* addTo = NULL;
    MenuWrapper* menu = NULL;
    foreach (QString menuTreePart, menuTree) {
        menu = getSubMenuFromName(menuTreePart.trimmed(), addTo);
        if (!menu) {
            if (!addTo) {
                menu = new MenuWrapper(QMenuBar::addMenu(menuTreePart.trimmed()));
            } else {
                menu = addTo->addMenu(menuTreePart.trimmed());
            }
        }
        addTo = menu;
    }

    if (isValidGrouping(grouping)) {
        auto action = getMenuAction(menuName);
        if (action) {
            _groupingActions[grouping] << action;
            action->setVisible(getGroupingIsVisible(grouping));
        }
    }

    QMenuBar::repaint();
    return menu;
}

void Menu::removeMenu(const QString& menuName) {
    QAction* action = getMenuAction(menuName);

    // only proceed if the menu actually exists
    if (action) {
        QString finalMenuPart;
        MenuWrapper* parent = getMenuParent(menuName, finalMenuPart);
        if (parent) {
            parent->removeAction(action);
        } else {
            QMenuBar::removeAction(action);
        }

        QMenuBar::repaint();
    }
}

bool Menu::menuExists(const QString& menuName) {
    QAction* action = getMenuAction(menuName);

    // only proceed if the menu actually exists
    if (action) {
        return true;
    }
    return false;
}

void Menu::addSeparator(const QString& menuName, const QString& separatorName, const QString& grouping) {
    MenuWrapper* menuObj = getMenu(menuName);
    if (menuObj) {
        addDisabledActionAndSeparator(menuObj, separatorName);
    }
}

void Menu::removeSeparator(const QString& menuName, const QString& separatorName) {
    MenuWrapper* menu = getMenu(menuName);
    bool separatorRemoved = false;
    if (menu) {
        int textAt = findPositionOfMenuItem(menu, separatorName);
        QList<QAction*> menuActions = menu->actions();
        QAction* separatorText = menuActions[textAt];
        if (textAt > 0 && textAt < menuActions.size()) {
            QAction* separatorLine = menuActions[textAt - 1];
            if (separatorLine) {
                if (separatorLine->isSeparator()) {
                    menu->removeAction(separatorText);
                    menu->removeAction(separatorLine);
                    separatorRemoved = true;
                }
            }
        }
    }
    if (separatorRemoved) {
        QMenuBar::repaint();
    }
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

void Menu::removeMenuItem(const QString& menu, const QString& menuitem) {
    MenuWrapper* menuObj = getMenu(menu);
    if (menuObj) {
        removeAction(menuObj, menuitem);
        QMenuBar::repaint();
    }
}

bool Menu::menuItemExists(const QString& menu, const QString& menuitem) {
    QAction* menuItemAction = _actionHash.value(menuitem);
    if (menuItemAction) {
        return (getMenu(menu) != NULL);
    }
    return false;
}

bool Menu::getGroupingIsVisible(const QString& grouping) {
    if (grouping.isEmpty() || grouping.isNull()) {
        return true;
    }
    if (_groupingVisible.contains(grouping)) {
        return _groupingVisible[grouping];
    }
    return false;
}

void Menu::setGroupingIsVisible(const QString& grouping, bool isVisible) {
    // NOTE: Default grouping always visible
    if (grouping.isEmpty() || grouping.isNull()) {
        return;
    }
    _groupingVisible[grouping] = isVisible;

    for (auto action: _groupingActions[grouping]) {
        action->setVisible(isVisible);
    }

    QMenuBar::repaint();
}

void Menu::addActionGroup(const QString& groupName, const QStringList& actionList, const QString& selected) {
    auto menu = addMenu(groupName);
    
    QActionGroup* actionGroup = new QActionGroup(menu);
    actionGroup->setExclusive(true);
    
    auto menuScriptingInterface = MenuScriptingInterface::getInstance();
    for (auto action : actionList) {
        auto item = addCheckableActionToQMenuAndActionHash(menu, action, 0, action == selected,
                                                           menuScriptingInterface,
                                                           SLOT(menuItemTriggered()));
        actionGroup->addAction(item);
    }
    
    QMenuBar::repaint();
}

void Menu::removeActionGroup(const QString& groupName) {
    removeMenu(groupName);
}

MenuWrapper::MenuWrapper(QMenu* menu) : _realMenu(menu) {
    VrMenu::executeOrQueue([=](VrMenu* vrMenu) {
        vrMenu->addMenu(menu);
    });
    _backMap[menu] = this;
}

QList<QAction*> MenuWrapper::actions() {
    return _realMenu->actions();
}

MenuWrapper* MenuWrapper::addMenu(const QString& menuName) {
    return new MenuWrapper(_realMenu->addMenu(menuName));
}

void MenuWrapper::setEnabled(bool enabled) {
    _realMenu->setEnabled(enabled);
}

QAction* MenuWrapper::addSeparator() {
    return _realMenu->addSeparator();
}

void MenuWrapper::addAction(QAction* action) {
    _realMenu->addAction(action);
    VrMenu::executeOrQueue([=](VrMenu* vrMenu) {
        vrMenu->addAction(_realMenu, action);
    });
}

QAction* MenuWrapper::addAction(const QString& menuName) {
    QAction* action = _realMenu->addAction(menuName);
    VrMenu::executeOrQueue([=](VrMenu* vrMenu) {
        vrMenu->addAction(_realMenu, action);
    });
    return action;
}

QAction* MenuWrapper::addAction(const QString& menuName, const QObject* receiver, const char* member, const QKeySequence& shortcut) {
    QAction* action = _realMenu->addAction(menuName, receiver, member, shortcut);
    VrMenu::executeOrQueue([=](VrMenu* vrMenu) {
        vrMenu->addAction(_realMenu, action);
    });
    return action;
}

void MenuWrapper::removeAction(QAction* action) {
    _realMenu->removeAction(action);
    VrMenu::executeOrQueue([=](VrMenu* vrMenu) {
        vrMenu->removeAction(action);
    });
}

void MenuWrapper::insertAction(QAction* before, QAction* action) {
    _realMenu->insertAction(before, action);
    VrMenu::executeOrQueue([=](VrMenu* vrMenu) {
        vrMenu->insertAction(before, action);
    });
}

QHash<QMenu*, MenuWrapper*> MenuWrapper::_backMap;
