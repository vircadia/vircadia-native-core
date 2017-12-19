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

#include <thread>

#include <AddressManager.h>
#include <AudioClient.h>
#include <CrashHelpers.h>
#include <DependencyManager.h>
#include <ui/TabletScriptingInterface.h>
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
#include "AvatarBookmarks.h"
#include "devices/DdeFaceTracker.h"
#include "MainWindow.h"
#include "render/DrawStatus.h"
#include "scripting/MenuScriptingInterface.h"
#include "scripting/HMDScriptingInterface.h"
#include "ui/DialogsManager.h"
#include "ui/StandAloneJSConsole.h"
#include "InterfaceLogging.h"
#include "LocationBookmarks.h"

#if defined(Q_OS_MAC) || defined(Q_OS_WIN)
#include "SpeechRecognizer.h"
#endif

#include "Menu.h"

extern bool DEV_DECIMATE_TEXTURES;

Menu* Menu::getInstance() {
    return dynamic_cast<Menu*>(qApp->getWindow()->menuBar());
}

const char* EXCLUSION_GROUP_KEY = "exclusionGroup";

Menu::Menu() {
    auto dialogsManager = DependencyManager::get<DialogsManager>();
    auto accountManager = DependencyManager::get<AccountManager>();

    // File/Application menu ----------------------------------
    MenuWrapper* fileMenu = addMenu("File");

    // File > Login menu items
    {
        addActionToQMenuAndActionHash(fileMenu, MenuOption::Login);

        // connect to the appropriate signal of the AccountManager so that we can change the Login/Logout menu item
        connect(accountManager.data(), &AccountManager::profileChanged,
                dialogsManager.data(), &DialogsManager::toggleLoginDialog);
        connect(accountManager.data(), &AccountManager::logoutComplete,
                dialogsManager.data(), &DialogsManager::toggleLoginDialog);
    }

    // File > Help
    addActionToQMenuAndActionHash(fileMenu, MenuOption::Help, 0, qApp, SLOT(showHelp()));

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

    // Edit > Running Scripts
    auto action = addActionToQMenuAndActionHash(editMenu, MenuOption::RunningScripts, Qt::CTRL | Qt::Key_J);
    connect(action, &QAction::triggered, [] {
        static const QUrl widgetUrl("hifi/dialogs/RunningScripts.qml");
        static const QUrl tabletUrl("../../hifi/dialogs/TabletRunningScripts.qml");
        static const QString name("RunningScripts");
        qApp->showDialog(widgetUrl, tabletUrl, name);
    });

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
    action = addActionToQMenuAndActionHash(editMenu, MenuOption::ReloadAllScripts, Qt::CTRL | Qt::Key_R,
        nullptr, nullptr,
        QAction::NoRole, UNSPECIFIED_POSITION, "Advanced");
    connect(action, &QAction::triggered, [] {
        DependencyManager::get<ScriptEngines>()->reloadAllScripts();
        DependencyManager::get<OffscreenUi>()->clearCache();
    });


    // Edit > Console... [advanced]
    addActionToQMenuAndActionHash(editMenu, MenuOption::Console, Qt::CTRL | Qt::ALT | Qt::Key_J,
        DependencyManager::get<StandAloneJSConsole>().data(),
        SLOT(toggleConsole()),
        QAction::NoRole, UNSPECIFIED_POSITION, "Advanced");

    editMenu->addSeparator();

    // Edit > My Asset Server
    auto assetServerAction = addActionToQMenuAndActionHash(editMenu, MenuOption::AssetServer,
                                                           Qt::CTRL | Qt::SHIFT | Qt::Key_A,
                                                           qApp, SLOT(showAssetServerWidget()));
    auto nodeList = DependencyManager::get<NodeList>();
    QObject::connect(nodeList.data(), &NodeList::canWriteAssetsChanged, assetServerAction, &QAction::setEnabled);
    assetServerAction->setEnabled(nodeList->getThisNodeCanWriteAssets());

    // Edit > Package Model... [advanced]
    addActionToQMenuAndActionHash(editMenu, MenuOption::PackageModel, 0,
        qApp, SLOT(packageModel()),
        QAction::NoRole, UNSPECIFIED_POSITION, "Advanced");

    // Edit > Reload All Content [advanced]
    addActionToQMenuAndActionHash(editMenu, MenuOption::ReloadContent, 0, qApp, SLOT(reloadResourceCaches()),
                                  QAction::NoRole, UNSPECIFIED_POSITION, "Advanced");

    // Avatar menu ----------------------------------
    MenuWrapper* avatarMenu = addMenu("Avatar");
    auto avatarManager = DependencyManager::get<AvatarManager>();
    auto avatar = avatarManager->getMyAvatar();

    // Avatar > Attachments...
    action = addActionToQMenuAndActionHash(avatarMenu, MenuOption::Attachments);
    connect(action, &QAction::triggered, [] {
        qApp->showDialog(QString("hifi/dialogs/AttachmentsDialog.qml"),
            QString("../../hifi/tablet/TabletAttachmentsDialog.qml"), "AttachmentsDialog");
    });

    // Avatar > Size
    MenuWrapper* avatarSizeMenu = avatarMenu->addMenu("Size");

    // Avatar > Size > Increase
    addActionToQMenuAndActionHash(avatarSizeMenu,
        MenuOption::IncreaseAvatarSize,
        0, // QML Qt::Key_Plus,
        avatar.get(), SLOT(increaseSize()));

    // Avatar > Size > Decrease
    addActionToQMenuAndActionHash(avatarSizeMenu,
        MenuOption::DecreaseAvatarSize,
        0, // QML Qt::Key_Minus,
        avatar.get(), SLOT(decreaseSize()));

    // Avatar > Size > Reset
    addActionToQMenuAndActionHash(avatarSizeMenu,
        MenuOption::ResetAvatarSize,
        0, // QML Qt::Key_Equal,
        avatar.get(), SLOT(resetSize()));

    // Avatar > Reset Sensors
    addActionToQMenuAndActionHash(avatarMenu,
        MenuOption::ResetSensors,
        0, // QML Qt::Key_Apostrophe,
        qApp, SLOT(resetSensors()));

    addCheckableActionToQMenuAndActionHash(avatarMenu, MenuOption::EnableAvatarCollisions, 0, true,
        avatar.get(), SLOT(updateMotionBehaviorFromMenu()));

    addCheckableActionToQMenuAndActionHash(avatarMenu, MenuOption::EnableFlying, 0, true,
        avatar.get(), SLOT(setFlyingEnabled(bool)));

    // Avatar > AvatarBookmarks related menus -- Note: the AvatarBookmarks class adds its own submenus here.
    auto avatarBookmarks = DependencyManager::get<AvatarBookmarks>();
    avatarBookmarks->setupMenus(this, avatarMenu);

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
    auto firstPersonAction = cameraModeGroup->addAction(addCheckableActionToQMenuAndActionHash(
                                   viewMenu, MenuOption::FirstPerson, Qt::CTRL | Qt::Key_F,
                                   true, qApp, SLOT(cameraMenuChanged())));

    firstPersonAction->setProperty(EXCLUSION_GROUP_KEY, QVariant::fromValue(cameraModeGroup));

    // View > Third Person
    auto thirdPersonAction = cameraModeGroup->addAction(addCheckableActionToQMenuAndActionHash(
                                   viewMenu, MenuOption::ThirdPerson, Qt::CTRL | Qt::Key_G,
                                   false, qApp, SLOT(cameraMenuChanged())));

    thirdPersonAction->setProperty(EXCLUSION_GROUP_KEY, QVariant::fromValue(cameraModeGroup));

    // View > Mirror
    auto viewMirrorAction = cameraModeGroup->addAction(addCheckableActionToQMenuAndActionHash(
                                   viewMenu, MenuOption::FullscreenMirror, Qt::CTRL | Qt::Key_H,
                                   false, qApp, SLOT(cameraMenuChanged())));

    viewMirrorAction->setProperty(EXCLUSION_GROUP_KEY, QVariant::fromValue(cameraModeGroup));

    // View > Independent [advanced]
    auto viewIndependentAction = cameraModeGroup->addAction(addCheckableActionToQMenuAndActionHash(viewMenu,
        MenuOption::IndependentMode, 0,
        false, qApp, SLOT(cameraMenuChanged()),
        UNSPECIFIED_POSITION, "Advanced"));

    viewIndependentAction->setProperty(EXCLUSION_GROUP_KEY, QVariant::fromValue(cameraModeGroup));

    // View > Entity Camera [advanced]
    auto viewEntityCameraAction = cameraModeGroup->addAction(addCheckableActionToQMenuAndActionHash(viewMenu,
        MenuOption::CameraEntityMode, 0,
        false, qApp, SLOT(cameraMenuChanged()),
        UNSPECIFIED_POSITION, "Advanced"));

    viewEntityCameraAction->setProperty(EXCLUSION_GROUP_KEY, QVariant::fromValue(cameraModeGroup));

    viewMenu->addSeparator();

    // View > Center Player In View
    addCheckableActionToQMenuAndActionHash(viewMenu, MenuOption::CenterPlayerInView,
        0, true, qApp, SLOT(rotationModeChanged()),
        UNSPECIFIED_POSITION, "Advanced");

    // View > Overlays
    addCheckableActionToQMenuAndActionHash(viewMenu, MenuOption::Overlays, 0, true);

    // View > Enter First Person Mode in HMD
    addCheckableActionToQMenuAndActionHash(viewMenu, MenuOption::FirstPersonHMD, 0, true);

    // Navigate menu ----------------------------------
    MenuWrapper* navigateMenu = addMenu("Navigate");

    // Navigate > Show Address Bar
    addActionToQMenuAndActionHash(navigateMenu, MenuOption::AddressBar, Qt::CTRL | Qt::Key_L,
        dialogsManager.data(), SLOT(showAddressBar()));

    // Navigate > LocationBookmarks related menus -- Note: the LocationBookmarks class adds its own submenus here.
    auto locationBookmarks = DependencyManager::get<LocationBookmarks>();
    locationBookmarks->setupMenus(this, navigateMenu);

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
        qApp->showDialog(QString("hifi/dialogs/GeneralPreferencesDialog.qml"),
            QString("../../hifi/tablet/TabletGeneralPreferences.qml"), "GeneralPreferencesDialog");
    });

    action = addActionToQMenuAndActionHash(settingsMenu, "Audio...");
    connect(action, &QAction::triggered, [] {
        static const QUrl widgetUrl("hifi/dialogs/Audio.qml");
        static const QUrl tabletUrl("../../hifi/audio/Audio.qml");
        static const QString name("AudioDialog");
        qApp->showDialog(widgetUrl, tabletUrl, name);
    });

    // Settings > Avatar...
    action = addActionToQMenuAndActionHash(settingsMenu, "Avatar...");
    connect(action, &QAction::triggered, [] {
        qApp->showDialog(QString("hifi/dialogs/AvatarPreferencesDialog.qml"),
            QString("../../hifi/tablet/TabletAvatarPreferences.qml"), "AvatarPreferencesDialog");
    });

    // Settings > LOD...
    action = addActionToQMenuAndActionHash(settingsMenu, "LOD...");
    connect(action, &QAction::triggered, [] {
        qApp->showDialog(QString("hifi/dialogs/LodPreferencesDialog.qml"),
            QString("../../hifi/tablet/TabletLodPreferences.qml"), "LodPreferencesDialog");
    });

    action = addActionToQMenuAndActionHash(settingsMenu, "Controller Settings...");
    connect(action, &QAction::triggered, [] {
            auto tablet = DependencyManager::get<TabletScriptingInterface>()->getTablet("com.highfidelity.interface.tablet.system");
            auto hmd = DependencyManager::get<HMDScriptingInterface>();
            tablet->loadQMLSource("ControllerSettings.qml");

            if (!hmd->getShouldShowTablet()) {
                hmd->toggleShouldShowTablet();
            }
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

    // Developer menu ----------------------------------
    MenuWrapper* developerMenu = addMenu("Developer", "Developer");

    // Developer > Graphics...
    action = addActionToQMenuAndActionHash(developerMenu, "Graphics...");
    connect(action, &QAction::triggered, [] {
        qApp->showDialog(QString("hifi/dialogs/GraphicsPreferencesDialog.qml"),
            QString("../../hifi/tablet/TabletGraphicsPreferences.qml"), "GraphicsPreferencesDialog");
    });

    // Developer > UI >>>
    MenuWrapper* uiOptionsMenu = developerMenu->addMenu("UI");
    action = addCheckableActionToQMenuAndActionHash(uiOptionsMenu, MenuOption::DesktopTabletToToolbar, 0,
                                                    qApp->getDesktopTabletBecomesToolbarSetting());
    connect(action, &QAction::triggered, [action] {
        qApp->setDesktopTabletBecomesToolbarSetting(action->isChecked());
    });

    action = addCheckableActionToQMenuAndActionHash(uiOptionsMenu, MenuOption::HMDTabletToToolbar, 0,
                                                    qApp->getHmdTabletBecomesToolbarSetting());
    connect(action, &QAction::triggered, [action] {
        qApp->setHmdTabletBecomesToolbarSetting(action->isChecked());
    });

    // Developer > Render >>>
    MenuWrapper* renderOptionsMenu = developerMenu->addMenu("Render");
    addCheckableActionToQMenuAndActionHash(renderOptionsMenu, MenuOption::WorldAxes);
    addCheckableActionToQMenuAndActionHash(renderOptionsMenu, MenuOption::DefaultSkybox, 0, true);

    // Developer > Render > Throttle FPS If Not Focus
    addCheckableActionToQMenuAndActionHash(renderOptionsMenu, MenuOption::ThrottleFPSIfNotFocus, 0, true);

    // Developer > Render > OpenVR threaded submit
    addCheckableActionToQMenuAndActionHash(renderOptionsMenu, MenuOption::OpenVrThreadedSubmit, 0, true);

    // Developer > Render > Resolution
    MenuWrapper* resolutionMenu = renderOptionsMenu->addMenu(MenuOption::RenderResolution);
    QActionGroup* resolutionGroup = new QActionGroup(resolutionMenu);
    resolutionGroup->setExclusive(true);
    resolutionGroup->addAction(addCheckableActionToQMenuAndActionHash(resolutionMenu, MenuOption::RenderResolutionOne, 0, true));
    resolutionGroup->addAction(addCheckableActionToQMenuAndActionHash(resolutionMenu, MenuOption::RenderResolutionTwoThird, 0, false));
    resolutionGroup->addAction(addCheckableActionToQMenuAndActionHash(resolutionMenu, MenuOption::RenderResolutionHalf, 0, false));
    resolutionGroup->addAction(addCheckableActionToQMenuAndActionHash(resolutionMenu, MenuOption::RenderResolutionThird, 0, false));
    resolutionGroup->addAction(addCheckableActionToQMenuAndActionHash(resolutionMenu, MenuOption::RenderResolutionQuarter, 0, false));

    //const QString  = "Automatic Texture Memory";
    //const QString  = "64 MB";
    //const QString  = "256 MB";
    //const QString  = "512 MB";
    //const QString  = "1024 MB";
    //const QString  = "2048 MB";

    // Developer > Render > Maximum Texture Memory
    MenuWrapper* textureMenu = renderOptionsMenu->addMenu(MenuOption::RenderMaxTextureMemory);
    QActionGroup* textureGroup = new QActionGroup(textureMenu);
    textureGroup->setExclusive(true);
    textureGroup->addAction(addCheckableActionToQMenuAndActionHash(textureMenu, MenuOption::RenderMaxTextureAutomatic, 0, true));
    textureGroup->addAction(addCheckableActionToQMenuAndActionHash(textureMenu, MenuOption::RenderMaxTexture4MB, 0, false));
    textureGroup->addAction(addCheckableActionToQMenuAndActionHash(textureMenu, MenuOption::RenderMaxTexture64MB, 0, false));
    textureGroup->addAction(addCheckableActionToQMenuAndActionHash(textureMenu, MenuOption::RenderMaxTexture256MB, 0, false));
    textureGroup->addAction(addCheckableActionToQMenuAndActionHash(textureMenu, MenuOption::RenderMaxTexture512MB, 0, false));
    textureGroup->addAction(addCheckableActionToQMenuAndActionHash(textureMenu, MenuOption::RenderMaxTexture1024MB, 0, false));
    textureGroup->addAction(addCheckableActionToQMenuAndActionHash(textureMenu, MenuOption::RenderMaxTexture2048MB, 0, false));
    connect(textureGroup, &QActionGroup::triggered, [textureGroup] {
        auto checked = textureGroup->checkedAction();
        auto text = checked->text();
        gpu::Context::Size newMaxTextureMemory { 0 };
        if (MenuOption::RenderMaxTexture4MB == text) {
            newMaxTextureMemory = MB_TO_BYTES(4);
        } else if (MenuOption::RenderMaxTexture64MB == text) {
            newMaxTextureMemory = MB_TO_BYTES(64);
        } else if (MenuOption::RenderMaxTexture256MB == text) {
            newMaxTextureMemory = MB_TO_BYTES(256);
        } else if (MenuOption::RenderMaxTexture512MB == text) {
            newMaxTextureMemory = MB_TO_BYTES(512);
        } else if (MenuOption::RenderMaxTexture1024MB == text) {
            newMaxTextureMemory = MB_TO_BYTES(1024);
        } else if (MenuOption::RenderMaxTexture2048MB == text) {
            newMaxTextureMemory = MB_TO_BYTES(2048);
        }
        gpu::Texture::setAllowedGPUMemoryUsage(newMaxTextureMemory);
    });

#ifdef Q_OS_WIN
    // Developer > Render > Enable Sparse Textures
    {
        auto action = addCheckableActionToQMenuAndActionHash(renderOptionsMenu, MenuOption::SparseTextureManagement, 0, gpu::Texture::getEnableSparseTextures());
        connect(action, &QAction::triggered, [&](bool checked) {
            qDebug() << "[TEXTURE TRANSFER SUPPORT] --- Enable Dynamic Texture Management menu option:" << checked;
            gpu::Texture::setEnableSparseTextures(checked);
        });
    }

#else
    qDebug() << "[TEXTURE TRANSFER SUPPORT] Incremental Texture Transfer and Dynamic Texture Management not supported on this platform.";
#endif


    {
        auto action = addActionToQMenuAndActionHash(renderOptionsMenu, MenuOption::RenderClearKtxCache);
        connect(action, &QAction::triggered, []{
            Setting::Handle<int>(KTXCache::SETTING_VERSION_NAME, KTXCache::INVALID_VERSION).set(KTXCache::INVALID_VERSION);
        });
    }

    // Developer > Render > LOD Tools
    addActionToQMenuAndActionHash(renderOptionsMenu, MenuOption::LodTools, 0,
                                  qApp, SLOT(loadLODToolsDialog()));

    // HACK enable texture decimation
    {
        auto action = addCheckableActionToQMenuAndActionHash(renderOptionsMenu, "Decimate Textures");
        connect(action, &QAction::triggered, [&](bool checked) {
            DEV_DECIMATE_TEXTURES = checked;
        });
    }

    // Developer > Assets >>>
    // Menu item is not currently needed but code should be kept in case it proves useful again at some stage.
//#define WANT_ASSET_MIGRATION
#ifdef WANT_ASSET_MIGRATION
    MenuWrapper* assetDeveloperMenu = developerMenu->addMenu("Assets");
    auto& atpMigrator = ATPAssetMigrator::getInstance();
    atpMigrator.setDialogParent(this);

    addActionToQMenuAndActionHash(assetDeveloperMenu, MenuOption::AssetMigration,
        0, &atpMigrator,
        SLOT(loadEntityServerFile()));
#endif

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
    faceTrackingMenu->addSeparator();
    addCheckableActionToQMenuAndActionHash(faceTrackingMenu, MenuOption::MuteFaceTracking,
        [](bool mute) { FaceTracker::setIsMuted(mute); },
        Qt::CTRL | Qt::SHIFT | Qt::Key_F, FaceTracker::isMuted());
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

    action = addCheckableActionToQMenuAndActionHash(avatarDebugMenu, MenuOption::AvatarReceiveStats, 0, false);
    connect(action, &QAction::triggered, [this]{ Avatar::setShowReceiveStats(isOptionChecked(MenuOption::AvatarReceiveStats)); });
    action = addCheckableActionToQMenuAndActionHash(avatarDebugMenu, MenuOption::ShowBoundingCollisionShapes, 0, false);
    connect(action, &QAction::triggered, [this]{ Avatar::setShowCollisionShapes(isOptionChecked(MenuOption::ShowBoundingCollisionShapes)); });
    action = addCheckableActionToQMenuAndActionHash(avatarDebugMenu, MenuOption::ShowMyLookAtVectors, 0, false);
    connect(action, &QAction::triggered, [this]{ Avatar::setShowMyLookAtVectors(isOptionChecked(MenuOption::ShowMyLookAtVectors)); });
    action = addCheckableActionToQMenuAndActionHash(avatarDebugMenu, MenuOption::ShowOtherLookAtVectors, 0, false);
    connect(action, &QAction::triggered, [this]{ Avatar::setShowOtherLookAtVectors(isOptionChecked(MenuOption::ShowOtherLookAtVectors)); });

    action = addCheckableActionToQMenuAndActionHash(avatarDebugMenu, MenuOption::EnableLookAtSnapping, 0, true);
    connect(action, &QAction::triggered, [this, avatar]{
            avatar->setProperty("lookAtSnappingEnabled", isOptionChecked(MenuOption::EnableLookAtSnapping));
        });

    addCheckableActionToQMenuAndActionHash(avatarDebugMenu, MenuOption::FixGaze, 0, false);
    addCheckableActionToQMenuAndActionHash(avatarDebugMenu, MenuOption::AnimDebugDrawDefaultPose, 0, false,
        avatar.get(), SLOT(setEnableDebugDrawDefaultPose(bool)));
    addCheckableActionToQMenuAndActionHash(avatarDebugMenu, MenuOption::AnimDebugDrawAnimPose, 0, false,
        avatar.get(), SLOT(setEnableDebugDrawAnimPose(bool)));
    addCheckableActionToQMenuAndActionHash(avatarDebugMenu, MenuOption::AnimDebugDrawPosition, 0, false,
        avatar.get(), SLOT(setEnableDebugDrawPosition(bool)));
    addCheckableActionToQMenuAndActionHash(avatarDebugMenu, MenuOption::MeshVisible, 0, true,
        avatar.get(), SLOT(setEnableMeshVisible(bool)));
    addCheckableActionToQMenuAndActionHash(avatarDebugMenu, MenuOption::DisableEyelidAdjustment, 0, false);
    addCheckableActionToQMenuAndActionHash(avatarDebugMenu, MenuOption::TurnWithHead, 0, false);
    addCheckableActionToQMenuAndActionHash(avatarDebugMenu, MenuOption::UseAnimPreAndPostRotations, 0, true,
        avatar.get(), SLOT(setUseAnimPreAndPostRotations(bool)));
    addCheckableActionToQMenuAndActionHash(avatarDebugMenu, MenuOption::EnableInverseKinematics, 0, true,
        avatar.get(), SLOT(setEnableInverseKinematics(bool)));
    addCheckableActionToQMenuAndActionHash(avatarDebugMenu, MenuOption::RenderSensorToWorldMatrix, 0, false,
        avatar.get(), SLOT(setEnableDebugDrawSensorToWorldMatrix(bool)));
    addCheckableActionToQMenuAndActionHash(avatarDebugMenu, MenuOption::RenderIKTargets, 0, false,
        avatar.get(), SLOT(setEnableDebugDrawIKTargets(bool)));
    addCheckableActionToQMenuAndActionHash(avatarDebugMenu, MenuOption::RenderIKConstraints, 0, false,
        avatar.get(), SLOT(setEnableDebugDrawIKConstraints(bool)));
    addCheckableActionToQMenuAndActionHash(avatarDebugMenu, MenuOption::RenderIKChains, 0, false,
        avatar.get(), SLOT(setEnableDebugDrawIKChains(bool)));
    addCheckableActionToQMenuAndActionHash(avatarDebugMenu, MenuOption::RenderDetailedCollision, 0, false,
        avatar.get(), SLOT(setEnableDebugDrawDetailedCollision(bool)));

    addCheckableActionToQMenuAndActionHash(avatarDebugMenu, MenuOption::ActionMotorControl,
        Qt::CTRL | Qt::SHIFT | Qt::Key_K, true, avatar.get(), SLOT(updateMotionBehaviorFromMenu()),
        UNSPECIFIED_POSITION, "Developer");

    addCheckableActionToQMenuAndActionHash(avatarDebugMenu, MenuOption::ScriptedMotorControl, 0, true,
        avatar.get(), SLOT(updateMotionBehaviorFromMenu()),
        UNSPECIFIED_POSITION, "Developer");

    // Developer > Hands >>>
    MenuWrapper* handOptionsMenu = developerMenu->addMenu("Hands");
    addCheckableActionToQMenuAndActionHash(handOptionsMenu, MenuOption::DisplayHandTargets, 0, false,
        avatar.get(), SLOT(setEnableDebugDrawHandControllers(bool)));

    // Developer > Entities >>>
    MenuWrapper* entitiesOptionsMenu = developerMenu->addMenu("Entities");

    addActionToQMenuAndActionHash(entitiesOptionsMenu, MenuOption::OctreeStats, 0,
        qApp, SLOT(loadEntityStatisticsDialog()));

    addCheckableActionToQMenuAndActionHash(entitiesOptionsMenu, MenuOption::ShowRealtimeEntityStats);

    // Developer > Network >>>
    MenuWrapper* networkMenu = developerMenu->addMenu("Network");
    action = addActionToQMenuAndActionHash(networkMenu, MenuOption::Networking);
    connect(action, &QAction::triggered, [] {
        qApp->showDialog(QString("hifi/dialogs/NetworkingPreferencesDialog.qml"),
            QString("../../hifi/tablet/TabletNetworkingPreferences.qml"), "NetworkingPreferencesDialog");
    });
    addActionToQMenuAndActionHash(networkMenu, MenuOption::ReloadContent, 0, qApp, SLOT(reloadResourceCaches()));
    addActionToQMenuAndActionHash(networkMenu, MenuOption::ClearDiskCache, 0,
        DependencyManager::get<AssetClient>().data(), SLOT(clearCache()));
    addCheckableActionToQMenuAndActionHash(networkMenu,
        MenuOption::DisableActivityLogger,
        0,
        false,
        &UserActivityLogger::getInstance(),
        SLOT(disable(bool)));
    addActionToQMenuAndActionHash(networkMenu, MenuOption::ShowDSConnectTable, 0,
        qApp, SLOT(loadDomainConnectionDialog()));

    #if (PR_BUILD || DEV_BUILD)
    addCheckableActionToQMenuAndActionHash(networkMenu, MenuOption::SendWrongProtocolVersion, 0, false,
                qApp, SLOT(sendWrongProtocolVersionsSignature(bool)));

    addCheckableActionToQMenuAndActionHash(networkMenu, MenuOption::SendWrongDSConnectVersion, 0, false,
                                           nodeList.data(), SLOT(toggleSendNewerDSConnectVersion(bool)));
    #endif


    // Developer >> Tests >>>
    MenuWrapper* testMenu = developerMenu->addMenu("Tests");
    addActionToQMenuAndActionHash(testMenu, MenuOption::RunClientScriptTests, 0, dialogsManager.data(), SLOT(showTestingResults()));

    // Developer > Timing >>>
    MenuWrapper* timingMenu = developerMenu->addMenu("Timing");
    MenuWrapper* perfTimerMenu = timingMenu->addMenu("Performance Timer");
    addCheckableActionToQMenuAndActionHash(perfTimerMenu, MenuOption::DisplayDebugTimingDetails, 0, false,
            qApp, SLOT(enablePerfStats(bool)));
    addCheckableActionToQMenuAndActionHash(perfTimerMenu, MenuOption::OnlyDisplayTopTen, 0, true);
    addCheckableActionToQMenuAndActionHash(perfTimerMenu, MenuOption::ExpandUpdateTiming, 0, false);
    addCheckableActionToQMenuAndActionHash(perfTimerMenu, MenuOption::ExpandMyAvatarTiming, 0, false);
    addCheckableActionToQMenuAndActionHash(perfTimerMenu, MenuOption::ExpandMyAvatarSimulateTiming, 0, false);
    addCheckableActionToQMenuAndActionHash(perfTimerMenu, MenuOption::ExpandOtherAvatarTiming, 0, false);
    addCheckableActionToQMenuAndActionHash(perfTimerMenu, MenuOption::ExpandPaintGLTiming, 0, false);
    addCheckableActionToQMenuAndActionHash(perfTimerMenu, MenuOption::ExpandPhysicsSimulationTiming, 0, false);

    addCheckableActionToQMenuAndActionHash(timingMenu, MenuOption::FrameTimer);
    addActionToQMenuAndActionHash(timingMenu, MenuOption::RunTimingTests, 0, qApp, SLOT(runTests()));
    addCheckableActionToQMenuAndActionHash(timingMenu, MenuOption::PipelineWarnings);
    addCheckableActionToQMenuAndActionHash(timingMenu, MenuOption::LogExtraTimings);
    addCheckableActionToQMenuAndActionHash(timingMenu, MenuOption::SuppressShortTimings);


    // Developer > Audio >>>
    MenuWrapper* audioDebugMenu = developerMenu->addMenu("Audio");

    action = addActionToQMenuAndActionHash(audioDebugMenu, "Stats...");
    connect(action, &QAction::triggered, [] {
        auto scriptEngines = DependencyManager::get<ScriptEngines>();
        QUrl defaultScriptsLoc = PathUtils::defaultScriptsLocation();
        defaultScriptsLoc.setPath(defaultScriptsLoc.path() + "developer/utilities/audio/stats.js");
        scriptEngines->loadScript(defaultScriptsLoc.toString());
    });

    action = addActionToQMenuAndActionHash(audioDebugMenu, "Buffers...");
    connect(action, &QAction::triggered, [] {
        qApp->showDialog(QString("hifi/dialogs/AudioBuffers.qml"),
            QString("../../hifi/tablet/TabletAudioBuffers.qml"), "AudioBuffersDialog");
    });

    auto audioIO = DependencyManager::get<AudioClient>();
    addActionToQMenuAndActionHash(audioDebugMenu, MenuOption::MuteEnvironment, 0,
        audioIO.data(), SLOT(sendMuteEnvironmentPacket()));

    action = addActionToQMenuAndActionHash(audioDebugMenu, MenuOption::AudioScope);
    connect(action, &QAction::triggered, [] {
        auto scriptEngines = DependencyManager::get<ScriptEngines>();
        QUrl defaultScriptsLoc = PathUtils::defaultScriptsLocation();
        defaultScriptsLoc.setPath(defaultScriptsLoc.path() + "developer/utilities/audio/audioScope.js");
        scriptEngines->loadScript(defaultScriptsLoc.toString());
    });

    // Developer > Physics >>>
    MenuWrapper* physicsOptionsMenu = developerMenu->addMenu("Physics");
    {
        auto drawStatusConfig = qApp->getRenderEngine()->getConfiguration()->getConfig<render::DrawStatus>("RenderMainView.DrawStatus");
        addCheckableActionToQMenuAndActionHash(physicsOptionsMenu, MenuOption::PhysicsShowOwned,
            0, false, drawStatusConfig, SLOT(setShowNetwork(bool)));
    }
    addCheckableActionToQMenuAndActionHash(physicsOptionsMenu, MenuOption::PhysicsShowHulls);

    // Developer > Ask to Reset Settings
    addCheckableActionToQMenuAndActionHash(developerMenu, MenuOption::AskToResetSettings, 0, false);

    // Developer > Display Crash Options
    addCheckableActionToQMenuAndActionHash(developerMenu, MenuOption::DisplayCrashOptions, 0, true);

    // Developer > Crash >>>
    MenuWrapper* crashMenu = developerMenu->addMenu("Crash");

    addActionToQMenuAndActionHash(crashMenu, MenuOption::DeadlockInterface, 0, qApp, SLOT(deadlockApplication()));

    action = addActionToQMenuAndActionHash(crashMenu, MenuOption::CrashPureVirtualFunction);
    connect(action, &QAction::triggered, qApp, []() { crash::pureVirtualCall(); });
    action = addActionToQMenuAndActionHash(crashMenu, MenuOption::CrashPureVirtualFunctionThreaded);
    connect(action, &QAction::triggered, qApp, []() { std::thread([]() { crash::pureVirtualCall(); }); });

    action = addActionToQMenuAndActionHash(crashMenu, MenuOption::CrashDoubleFree);
    connect(action, &QAction::triggered, qApp, []() { crash::doubleFree(); });
    action = addActionToQMenuAndActionHash(crashMenu, MenuOption::CrashDoubleFreeThreaded);
    connect(action, &QAction::triggered, qApp, []() { std::thread([]() { crash::doubleFree(); }); });

    action = addActionToQMenuAndActionHash(crashMenu, MenuOption::CrashAbort);
    connect(action, &QAction::triggered, qApp, []() { crash::doAbort(); });
    action = addActionToQMenuAndActionHash(crashMenu, MenuOption::CrashAbortThreaded);
    connect(action, &QAction::triggered, qApp, []() { std::thread([]() { crash::doAbort(); }); });

    action = addActionToQMenuAndActionHash(crashMenu, MenuOption::CrashNullDereference);
    connect(action, &QAction::triggered, qApp, []() { crash::nullDeref(); });
    action = addActionToQMenuAndActionHash(crashMenu, MenuOption::CrashNullDereferenceThreaded);
    connect(action, &QAction::triggered, qApp, []() { std::thread([]() { crash::nullDeref(); }); });

    action = addActionToQMenuAndActionHash(crashMenu, MenuOption::CrashOutOfBoundsVectorAccess);
    connect(action, &QAction::triggered, qApp, []() { crash::outOfBoundsVectorCrash(); });
    action = addActionToQMenuAndActionHash(crashMenu, MenuOption::CrashOutOfBoundsVectorAccessThreaded);
    connect(action, &QAction::triggered, qApp, []() { std::thread([]() { crash::outOfBoundsVectorCrash(); }); });

    action = addActionToQMenuAndActionHash(crashMenu, MenuOption::CrashNewFault);
    connect(action, &QAction::triggered, qApp, []() { crash::newFault(); });
    action = addActionToQMenuAndActionHash(crashMenu, MenuOption::CrashNewFaultThreaded);
    connect(action, &QAction::triggered, qApp, []() { std::thread([]() { crash::newFault(); }); });

    // Developer > Log...
    addActionToQMenuAndActionHash(developerMenu, MenuOption::Log, Qt::CTRL | Qt::SHIFT | Qt::Key_L,
                                  qApp, SLOT(toggleLogDialog()));
    auto essLogAction = addActionToQMenuAndActionHash(developerMenu, MenuOption::EntityScriptServerLog, 0,
                                                      qApp, SLOT(toggleEntityScriptServerLogDialog()));
    QObject::connect(nodeList.data(), &NodeList::canRezChanged, essLogAction, [essLogAction] {
        auto nodeList = DependencyManager::get<NodeList>();
        essLogAction->setEnabled(nodeList->getThisNodeCanRez());
    });
    essLogAction->setEnabled(nodeList->getThisNodeCanRez());

    action = addActionToQMenuAndActionHash(developerMenu, "Script Log (HMD friendly)...", Qt::NoButton,
                                           qApp, SLOT(showScriptLogs()));

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
