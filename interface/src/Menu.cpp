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

#include "Menu.h"
#include <QDesktopServices>
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
#include "avatar/AvatarPackager.h"
#include "AvatarBookmarks.h"
#include "MainWindow.h"
#include "render/DrawStatus.h"
#include "scripting/MenuScriptingInterface.h"
#include "scripting/HMDScriptingInterface.h"
#include "ui/DialogsManager.h"
#include "ui/StandAloneJSConsole.h"
#include "InterfaceLogging.h"
#include "LocationBookmarks.h"
#include "DeferredLightingEffect.h"
#include "PickManager.h"

#include "scripting/SettingsScriptingInterface.h"
#if defined(Q_OS_MAC) || defined(Q_OS_WIN)
#include "SpeechRecognizer.h"
#endif

#include "MeshPartPayload.h"
#include "scripting/RenderScriptingInterface.h"

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
        addActionToQMenuAndActionHash(fileMenu, QCoreApplication::translate("MenuOption", MenuOption::Login.toUtf8().constData()));

        // connect to the appropriate signal of the AccountManager so that we can change the Login/Logout menu item
        connect(accountManager.data(), &AccountManager::profileChanged,
                dialogsManager.data(), &DialogsManager::toggleLoginDialog);
        connect(accountManager.data(), &AccountManager::logoutComplete,
                dialogsManager.data(), &DialogsManager::toggleLoginDialog);
    }

    // File > Quit
    addActionToQMenuAndActionHash(fileMenu, QCoreApplication::translate("MenuOption", MenuOption::Quit.toUtf8().constData()), Qt::CTRL | Qt::Key_Q, qApp, SLOT(quit()), QAction::QuitRole);


    // Edit menu ----------------------------------
    MenuWrapper* editMenu = addMenu("Edit");

    // Edit > Cut
    auto cutAction = addActionToQMenuAndActionHash(editMenu, "Cut", QKeySequence::Cut);
    connect(cutAction, &QAction::triggered, [] {
            QKeyEvent* keyEvent = new QKeyEvent(QEvent::KeyPress, Qt::Key_X, Qt::ControlModifier);
            QCoreApplication::postEvent(QCoreApplication::instance(), keyEvent);
    });

    // Edit > Copy
    auto copyAction = addActionToQMenuAndActionHash(editMenu, "Copy", QKeySequence::Copy);
    connect(copyAction, &QAction::triggered, [] {
            QKeyEvent* keyEvent = new QKeyEvent(QEvent::KeyPress, Qt::Key_C, Qt::ControlModifier);
            QCoreApplication::postEvent(QCoreApplication::instance(), keyEvent);
    });

    // Edit > Paste
    auto pasteAction = addActionToQMenuAndActionHash(editMenu, "Paste", QKeySequence::Paste);
    connect(pasteAction, &QAction::triggered, [] {
            QKeyEvent* keyEvent = new QKeyEvent(QEvent::KeyPress, Qt::Key_V, Qt::ControlModifier);
            QCoreApplication::postEvent(QCoreApplication::instance(), keyEvent);
    });

    // Edit > Delete
    auto deleteAction = addActionToQMenuAndActionHash(editMenu, "Delete", QKeySequence::Delete);
    connect(deleteAction, &QAction::triggered, [] {
            QKeyEvent* keyEvent = new QKeyEvent(QEvent::KeyPress, Qt::Key_Delete, Qt::NoModifier);
            QCoreApplication::postEvent(QCoreApplication::instance(), keyEvent);
    });

    editMenu->addSeparator();

    // Edit > Running Scripts
    auto action = addActionToQMenuAndActionHash(editMenu, QCoreApplication::translate("MenuOption", MenuOption::RunningScripts.toUtf8().constData()), Qt::CTRL | Qt::Key_J);
    connect(action, &QAction::triggered, [] {
        if (!qApp->getLoginDialogPoppedUp()) {
            static const QUrl widgetUrl("hifi/dialogs/RunningScripts.qml");
            static const QUrl tabletUrl("hifi/dialogs/TabletRunningScripts.qml");
            static const QString name("RunningScripts");
            qApp->showDialog(widgetUrl, tabletUrl, name);
        }
    });

    editMenu->addSeparator();

    // Edit > Asset Browser
    auto assetServerAction = addActionToQMenuAndActionHash(editMenu, QCoreApplication::translate("MenuOption", MenuOption::AssetServer.toUtf8().constData()),
                                                           Qt::CTRL | Qt::SHIFT | Qt::Key_A,
                                                           qApp, SLOT(showAssetServerWidget()));
    {
        auto nodeList = DependencyManager::get<NodeList>();
        QObject::connect(nodeList.data(), &NodeList::canWriteAssetsChanged, assetServerAction, &QAction::setEnabled);
        assetServerAction->setEnabled(nodeList->getThisNodeCanWriteAssets());
    }

    // Edit > Avatar Packager
#ifndef Q_OS_ANDROID
    action = addActionToQMenuAndActionHash(editMenu, QCoreApplication::translate("MenuOption", MenuOption::AvatarPackager.toUtf8().constData()));
    connect(action, &QAction::triggered, [] {
        DependencyManager::get<AvatarPackager>()->open();
    });
#endif

    // Edit > Reload All Content
    addActionToQMenuAndActionHash(editMenu, QCoreApplication::translate("MenuOption", MenuOption::ReloadContent.toUtf8().constData()), 0, qApp, SLOT(reloadResourceCaches()));

    // Display menu ----------------------------------
    // FIXME - this is not yet matching Alan's spec because it doesn't have
    // menus for "2D"/"3D" - we need to add support for detecting the appropriate
    // default 3D display mode
    addMenu(DisplayPlugin::MENU_PATH());
    MenuWrapper* displayModeMenu = addMenu(QCoreApplication::translate("MenuOption", MenuOption::OutputMenu.toUtf8().constData()));
    QActionGroup* displayModeGroup = new QActionGroup(displayModeMenu);
    displayModeGroup->setExclusive(true);


    // View menu ----------------------------------
    MenuWrapper* viewMenu = addMenu("View");
    QActionGroup* cameraModeGroup = new QActionGroup(viewMenu);

    // View > [camera group]
    cameraModeGroup->setExclusive(true);

    // View > First Person
    auto firstPersonAction = cameraModeGroup->addAction(addCheckableActionToQMenuAndActionHash(
                                   viewMenu, QCoreApplication::translate("MenuOption", MenuOption::FirstPersonLookAt.toUtf8().constData()), 0,
                                   true, qApp, SLOT(cameraMenuChanged())));

    firstPersonAction->setProperty(EXCLUSION_GROUP_KEY, QVariant::fromValue(cameraModeGroup));

    // View > Look At
    auto lookAtAction = cameraModeGroup->addAction(addCheckableActionToQMenuAndActionHash(
                                   viewMenu, QCoreApplication::translate("MenuOption", MenuOption::LookAtCamera.toUtf8().constData()), 0,
                                   false, qApp, SLOT(cameraMenuChanged())));

    lookAtAction->setProperty(EXCLUSION_GROUP_KEY, QVariant::fromValue(cameraModeGroup));

    // View > Selfie
    auto selfieAction = cameraModeGroup->addAction(addCheckableActionToQMenuAndActionHash(
        viewMenu, QCoreApplication::translate("MenuOption", MenuOption::SelfieCamera.toUtf8().constData()), 0,
        false, qApp, SLOT(cameraMenuChanged())));

    selfieAction->setProperty(EXCLUSION_GROUP_KEY, QVariant::fromValue(cameraModeGroup));

    viewMenu->addSeparator();

    // View > Center Player In View
    addCheckableActionToQMenuAndActionHash(viewMenu, QCoreApplication::translate("MenuOption", MenuOption::CenterPlayerInView.toUtf8().constData()),
        0, true, qApp, SLOT(rotationModeChanged()));

    // View > Enter First Person Mode in HMD
    addCheckableActionToQMenuAndActionHash(viewMenu, QCoreApplication::translate("MenuOption", MenuOption::FirstPersonHMD.toUtf8().constData()), 0, true);

    //TODO: Remove Navigation menu when these functions are included in GoTo menu
    // Navigate menu ----------------------------------
    MenuWrapper* navigateMenu = addMenu("Navigate");

    // Navigate > LocationBookmarks related menus -- Note: the LocationBookmarks class adds its own submenus here.
    auto locationBookmarks = DependencyManager::get<LocationBookmarks>();
    locationBookmarks->setupMenus(this, navigateMenu);

    // Navigate > Copy Address
    auto addressManager = DependencyManager::get<AddressManager>();
    addActionToQMenuAndActionHash(navigateMenu, QCoreApplication::translate("MenuOption", MenuOption::CopyAddress.toUtf8().constData()), 0,
        addressManager.data(), SLOT(copyAddress()));

    // Navigate > Copy Path
    addActionToQMenuAndActionHash(navigateMenu, QCoreApplication::translate("MenuOption", MenuOption::CopyPath.toUtf8().constData()), 0,
        addressManager.data(), SLOT(copyPath()));

    // Navigate > Start-up Location
    MenuWrapper* startupLocationMenu = navigateMenu->addMenu(QCoreApplication::translate("MenuOption", MenuOption::StartUpLocation.toUtf8().constData()));
    QActionGroup* startupLocatiopnGroup = new QActionGroup(startupLocationMenu);
    startupLocatiopnGroup->setExclusive(true);
    startupLocatiopnGroup->addAction(addCheckableActionToQMenuAndActionHash(startupLocationMenu, QCoreApplication::translate("MenuOption", MenuOption::HomeLocation.toUtf8().constData()), 0, 
        false));
    startupLocatiopnGroup->addAction(addCheckableActionToQMenuAndActionHash(startupLocationMenu, QCoreApplication::translate("MenuOption", MenuOption::LastLocation.toUtf8().constData()), 0, 
        true));

    // Settings menu ----------------------------------
    MenuWrapper* settingsMenu = addMenu("Settings");

    // Settings > General...
    action = addActionToQMenuAndActionHash(settingsMenu, QCoreApplication::translate("MenuOption", MenuOption::Preferences.toUtf8().constData()), Qt::CTRL | Qt::Key_G, nullptr, nullptr);
    connect(action, &QAction::triggered, [] {
        if (!qApp->getLoginDialogPoppedUp()) {
            qApp->showDialog(QString("hifi/dialogs/GeneralPreferencesDialog.qml"),
                QString("hifi/tablet/TabletGeneralPreferences.qml"), "GeneralPreferencesDialog");
        }
    });

    // Settings > Controls...
    action = addActionToQMenuAndActionHash(settingsMenu, "Controls...");
    connect(action, &QAction::triggered, [] {
            auto tablet = DependencyManager::get<TabletScriptingInterface>()->getTablet("com.highfidelity.interface.tablet.system");
            auto hmd = DependencyManager::get<HMDScriptingInterface>();
            tablet->pushOntoStack("hifi/tablet/ControllerSettings.qml");

            if (!hmd->getShouldShowTablet()) {
                hmd->toggleShouldShowTablet();
            }
    });

    // Settings > Audio...
    action = addActionToQMenuAndActionHash(settingsMenu, "Audio...");
    connect(action, &QAction::triggered, [] {
        static const QUrl tabletUrl("hifi/audio/Audio.qml");
        auto tablet = DependencyManager::get<TabletScriptingInterface>()->getTablet("com.highfidelity.interface.tablet.system");
        auto hmd = DependencyManager::get<HMDScriptingInterface>();
        tablet->pushOntoStack(tabletUrl);

        if (!hmd->getShouldShowTablet()) {
            hmd->toggleShouldShowTablet();
        }
    });

    // Settings > Graphics...
    action = addActionToQMenuAndActionHash(settingsMenu, "Graphics...");
    connect(action, &QAction::triggered, [] {
        auto tablet = DependencyManager::get<TabletScriptingInterface>()->getTablet("com.highfidelity.interface.tablet.system");
        auto hmd = DependencyManager::get<HMDScriptingInterface>();
        tablet->pushOntoStack("hifi/dialogs/graphics/GraphicsSettings.qml");

        if (!hmd->getShouldShowTablet()) {
            hmd->toggleShouldShowTablet();
        }
    });

    // Settings > Security...
    action = addActionToQMenuAndActionHash(settingsMenu, "Security...");
    connect(action, &QAction::triggered, [] {
		auto tablet = DependencyManager::get<TabletScriptingInterface>()->getTablet("com.highfidelity.interface.tablet.system");
		auto hmd = DependencyManager::get<HMDScriptingInterface>();
		tablet->pushOntoStack("hifi/dialogs/security/Security.qml");

		if (!hmd->getShouldShowTablet()) {
			hmd->toggleShouldShowTablet();
		}
    });
    
    // Settings > Entity Script / QML Whitelist
    action = addActionToQMenuAndActionHash(settingsMenu, "Entity Script / QML Whitelist");
    connect(action, &QAction::triggered, [] {
        auto tablet = DependencyManager::get<TabletScriptingInterface>()->getTablet("com.highfidelity.interface.tablet.system");
        auto hmd = DependencyManager::get<HMDScriptingInterface>();
        
        tablet->pushOntoStack("hifi/dialogs/security/EntityScriptQMLWhitelist.qml");

        if (!hmd->getShouldShowTablet()) {
            hmd->toggleShouldShowTablet();
        }
    });

    // Settings > Developer Menu
    addCheckableActionToQMenuAndActionHash(settingsMenu, "Developer Menu", 0, false, this, SLOT(toggleDeveloperMenus()));

    // Settings > Ask to Reset Settings
    addCheckableActionToQMenuAndActionHash(settingsMenu, QCoreApplication::translate("MenuOption", MenuOption::AskToResetSettings.toUtf8().constData()), 0, false);

    // Developer menu ----------------------------------
    MenuWrapper* developerMenu = addMenu("Developer", "Developer");
    
    // Developer > Scripting >>>
    MenuWrapper* scriptingOptionsMenu = developerMenu->addMenu("Scripting");
    
    // Developer > Scripting > Console...
    addActionToQMenuAndActionHash(scriptingOptionsMenu, QCoreApplication::translate("MenuOption", MenuOption::Console.toUtf8().constData()), Qt::CTRL | Qt::ALT | Qt::Key_J,
                                  DependencyManager::get<StandAloneJSConsole>().data(),
                                  SLOT(toggleConsole()),
                                  QAction::NoRole,
                                  UNSPECIFIED_POSITION);

     // Developer > Scripting > API Debugger
    action = addActionToQMenuAndActionHash(scriptingOptionsMenu, "API Debugger");
    connect(action, &QAction::triggered, [] {
        QUrl defaultScriptsLoc = PathUtils::defaultScriptsLocation();
        defaultScriptsLoc.setPath(defaultScriptsLoc.path() + "developer/utilities/tools/currentAPI.js");
        DependencyManager::get<ScriptEngines>()->loadScript(defaultScriptsLoc.toString());
    });
    
    // Developer > Scripting > Entity Script Server Log
    auto essLogAction = addActionToQMenuAndActionHash(scriptingOptionsMenu, QCoreApplication::translate("MenuOption", MenuOption::EntityScriptServerLog.toUtf8().constData()), 0,
                                                      qApp, SLOT(toggleEntityScriptServerLogDialog()));
    {
        auto nodeList = DependencyManager::get<NodeList>();
        QObject::connect(nodeList.data(), &NodeList::canRezChanged, essLogAction, [essLogAction] {
            auto nodeList = DependencyManager::get<NodeList>();
            essLogAction->setEnabled(nodeList->getThisNodeCanRez());
        });
        essLogAction->setEnabled(nodeList->getThisNodeCanRez());
    }

    // Developer > Scripting > Script Log (HMD friendly)...
    addActionToQMenuAndActionHash(scriptingOptionsMenu, "Script Log (HMD friendly)...", Qt::NoButton,
                                           qApp, SLOT(showScriptLogs()));

    // Developer > Scripting > Verbose Logging
    addCheckableActionToQMenuAndActionHash(scriptingOptionsMenu, QCoreApplication::translate("MenuOption", MenuOption::VerboseLogging.toUtf8().constData()), 0, false,
                                           qApp, SLOT(updateVerboseLogging()));
    
    // Developer > Scripting > Enable Speech Control API
#if defined(Q_OS_MAC) || defined(Q_OS_WIN)
    auto speechRecognizer = DependencyManager::get<SpeechRecognizer>();
    QAction* speechRecognizerAction = addCheckableActionToQMenuAndActionHash(scriptingOptionsMenu, QCoreApplication::translate("MenuOption", MenuOption::ControlWithSpeech.toUtf8().constData()),
        Qt::CTRL | Qt::SHIFT | Qt::Key_C,
        speechRecognizer->getEnabled(),
        speechRecognizer.data(),
        SLOT(setEnabled(bool)),
        UNSPECIFIED_POSITION);
    connect(speechRecognizer.data(), SIGNAL(enabledUpdated(bool)), speechRecognizerAction, SLOT(setChecked(bool)));
#endif
    
    // Developer > UI >>>
    MenuWrapper* uiOptionsMenu = developerMenu->addMenu("UI");
    action = addCheckableActionToQMenuAndActionHash(uiOptionsMenu, QCoreApplication::translate("MenuOption", MenuOption::DesktopTabletToToolbar.toUtf8().constData()), 0,
                                                    qApp->getDesktopTabletBecomesToolbarSetting());
    
    // Developer > UI > Show Overlays
    addCheckableActionToQMenuAndActionHash(uiOptionsMenu, QCoreApplication::translate("MenuOption", MenuOption::Overlays.toUtf8().constData()), 0, true);
    
    // Developer > UI > Desktop Tablet Becomes Toolbar
    connect(action, &QAction::triggered, [action] {
        qApp->setDesktopTabletBecomesToolbarSetting(action->isChecked());
    });
    
     // Developer > UI > HMD Tablet Becomes Toolbar
    action = addCheckableActionToQMenuAndActionHash(uiOptionsMenu, QCoreApplication::translate("MenuOption", MenuOption::HMDTabletToToolbar.toUtf8().constData()), 0,
                                                    qApp->getHmdTabletBecomesToolbarSetting());
    connect(action, &QAction::triggered, [action] {
        qApp->setHmdTabletBecomesToolbarSetting(action->isChecked());
    });

    // Developer > Render >>>
    MenuWrapper* renderOptionsMenu = developerMenu->addMenu("Render");

    addCheckableActionToQMenuAndActionHash(renderOptionsMenu, QCoreApplication::translate("MenuOption", MenuOption::AntiAliasing.toUtf8().constData()), 0, RenderScriptingInterface::getInstance()->getAntialiasingEnabled(),
        RenderScriptingInterface::getInstance(), SLOT(setAntialiasingEnabled(bool)));

    addCheckableActionToQMenuAndActionHash(renderOptionsMenu, QCoreApplication::translate("MenuOption", MenuOption::Shadows.toUtf8().constData()), 0, RenderScriptingInterface::getInstance()->getShadowsEnabled(),
        RenderScriptingInterface::getInstance(), SLOT(setShadowsEnabled(bool)));

    addCheckableActionToQMenuAndActionHash(renderOptionsMenu, QCoreApplication::translate("MenuOption", MenuOption::AmbientOcclusion.toUtf8().constData()), 0, RenderScriptingInterface::getInstance()->getAmbientOcclusionEnabled(),
        RenderScriptingInterface::getInstance(), SLOT(setAmbientOcclusionEnabled(bool)));

    addCheckableActionToQMenuAndActionHash(renderOptionsMenu, QCoreApplication::translate("MenuOption", MenuOption::WorldAxes.toUtf8().constData()));
    addCheckableActionToQMenuAndActionHash(renderOptionsMenu, QCoreApplication::translate("MenuOption", MenuOption::DefaultSkybox.toUtf8().constData()), 0, true);

    // Developer > Render > Throttle FPS If Not Focus
    addCheckableActionToQMenuAndActionHash(renderOptionsMenu, QCoreApplication::translate("MenuOption", MenuOption::ThrottleFPSIfNotFocus.toUtf8().constData()), 0, true);

    // Developer > Render > OpenVR threaded submit
    addCheckableActionToQMenuAndActionHash(renderOptionsMenu, QCoreApplication::translate("MenuOption", MenuOption::OpenVrThreadedSubmit.toUtf8().constData()), 0, true);

    //const QString  = "Automatic Texture Memory";
    //const QString  = "64 MB";
    //const QString  = "256 MB";
    //const QString  = "512 MB";
    //const QString  = "1024 MB";
    //const QString  = "2048 MB";

    // Developer > Render > Maximum Texture Memory
    MenuWrapper* textureMenu = renderOptionsMenu->addMenu(QCoreApplication::translate("MenuOption", MenuOption::RenderMaxTextureMemory.toUtf8().constData()));
    QActionGroup* textureGroup = new QActionGroup(textureMenu);
    textureGroup->setExclusive(true);
    textureGroup->addAction(addCheckableActionToQMenuAndActionHash(textureMenu, QCoreApplication::translate("MenuOption", MenuOption::RenderMaxTextureAutomatic.toUtf8().constData()), 0, true));
    textureGroup->addAction(addCheckableActionToQMenuAndActionHash(textureMenu, QCoreApplication::translate("MenuOption", MenuOption::RenderMaxTexture4MB.toUtf8().constData()), 0, false));
    textureGroup->addAction(addCheckableActionToQMenuAndActionHash(textureMenu, QCoreApplication::translate("MenuOption", MenuOption::RenderMaxTexture64MB.toUtf8().constData()), 0, false));
    textureGroup->addAction(addCheckableActionToQMenuAndActionHash(textureMenu, QCoreApplication::translate("MenuOption", MenuOption::RenderMaxTexture256MB.toUtf8().constData()), 0, false));
    textureGroup->addAction(addCheckableActionToQMenuAndActionHash(textureMenu, QCoreApplication::translate("MenuOption", MenuOption::RenderMaxTexture512MB.toUtf8().constData()), 0, false));
    textureGroup->addAction(addCheckableActionToQMenuAndActionHash(textureMenu, QCoreApplication::translate("MenuOption", MenuOption::RenderMaxTexture1024MB.toUtf8().constData()), 0, false));
    textureGroup->addAction(addCheckableActionToQMenuAndActionHash(textureMenu, QCoreApplication::translate("MenuOption", MenuOption::RenderMaxTexture2048MB.toUtf8().constData()), 0, false));
    textureGroup->addAction(addCheckableActionToQMenuAndActionHash(textureMenu, QCoreApplication::translate("MenuOption", MenuOption::RenderMaxTexture4096MB.toUtf8().constData()), 0, false));
    textureGroup->addAction(addCheckableActionToQMenuAndActionHash(textureMenu, QCoreApplication::translate("MenuOption", MenuOption::RenderMaxTexture6144MB.toUtf8().constData()), 0, false));
    textureGroup->addAction(addCheckableActionToQMenuAndActionHash(textureMenu, QCoreApplication::translate("MenuOption", MenuOption::RenderMaxTexture8192MB.toUtf8().constData()), 0, false));
    connect(textureGroup, &QActionGroup::triggered, [textureGroup] {
        auto checked = textureGroup->checkedAction();
        auto text = checked->text();
        gpu::Context::Size newMaxTextureMemory { 0 };
        if (QCoreApplication::translate("MenuOption", MenuOption::RenderMaxTexture4MB.toUtf8().constData()) == text) {
            newMaxTextureMemory = MB_TO_BYTES(4);
        } else if (QCoreApplication::translate("MenuOption", MenuOption::RenderMaxTexture64MB.toUtf8().constData()) == text) {
            newMaxTextureMemory = MB_TO_BYTES(64);
        } else if (QCoreApplication::translate("MenuOption", MenuOption::RenderMaxTexture256MB.toUtf8().constData()) == text) {
            newMaxTextureMemory = MB_TO_BYTES(256);
        } else if (QCoreApplication::translate("MenuOption", MenuOption::RenderMaxTexture512MB.toUtf8().constData()) == text) {
            newMaxTextureMemory = MB_TO_BYTES(512);
        } else if (QCoreApplication::translate("MenuOption", MenuOption::RenderMaxTexture1024MB.toUtf8().constData()) == text) {
            newMaxTextureMemory = MB_TO_BYTES(1024);
        } else if (QCoreApplication::translate("MenuOption", MenuOption::RenderMaxTexture2048MB.toUtf8().constData()) == text) {
            newMaxTextureMemory = MB_TO_BYTES(2048);
        } else if (QCoreApplication::translate("MenuOption", MenuOption::RenderMaxTexture4096MB.toUtf8().constData()) == text) {
            newMaxTextureMemory = MB_TO_BYTES(4096);
        } else if (QCoreApplication::translate("MenuOption", MenuOption::RenderMaxTexture6144MB.toUtf8().constData()) == text) {
            newMaxTextureMemory = MB_TO_BYTES(6144);
        } else if (QCoreApplication::translate("MenuOption", MenuOption::RenderMaxTexture8192MB.toUtf8().constData()) == text) {
            newMaxTextureMemory = MB_TO_BYTES(8192);
        }
        gpu::Texture::setAllowedGPUMemoryUsage(newMaxTextureMemory);
    });

#ifdef Q_OS_WIN
    // Developer > Render > Enable Sparse Textures
    {
        auto action = addCheckableActionToQMenuAndActionHash(renderOptionsMenu, QCoreApplication::translate("MenuOption", MenuOption::SparseTextureManagement.toUtf8().constData()), 0, gpu::Texture::getEnableSparseTextures());
        connect(action, &QAction::triggered, [&](bool checked) {
            qDebug() << "[TEXTURE TRANSFER SUPPORT] --- Enable Dynamic Texture Management menu option:" << checked;
            gpu::Texture::setEnableSparseTextures(checked);
        });
    }

#else
    qDebug() << "[TEXTURE TRANSFER SUPPORT] Incremental Texture Transfer and Dynamic Texture Management not supported on this platform.";
#endif


    {
        auto action = addActionToQMenuAndActionHash(renderOptionsMenu, QCoreApplication::translate("MenuOption", MenuOption::RenderClearKtxCache.toUtf8().constData()));
        connect(action, &QAction::triggered, []{
            Setting::Handle<int>(KTXCache::SETTING_VERSION_NAME, KTXCache::INVALID_VERSION).set(KTXCache::INVALID_VERSION);
        });
    }

    // Developer > Render > LOD Tools
    addActionToQMenuAndActionHash(renderOptionsMenu, QCoreApplication::translate("MenuOption", MenuOption::LodTools.toUtf8().constData()), 0,
                                  qApp, SLOT(loadLODToolsDialog()));

    // HACK enable texture decimation
    {
        auto action = addCheckableActionToQMenuAndActionHash(renderOptionsMenu, "Decimate Textures");
        connect(action, &QAction::triggered, [&](bool checked) {
            DEV_DECIMATE_TEXTURES = checked;
        });
    }

    addCheckableActionToQMenuAndActionHash(renderOptionsMenu, QCoreApplication::translate("MenuOption", MenuOption::ComputeBlendshapes.toUtf8().constData()), 0, true,
        DependencyManager::get<ModelBlender>().data(), SLOT(setComputeBlendshapes(bool)));

    action = addCheckableActionToQMenuAndActionHash(renderOptionsMenu, QCoreApplication::translate("MenuOption", MenuOption::MaterialProceduralShaders.toUtf8().constData()), 0, false);
    connect(action, &QAction::triggered, [action] {
        MeshPartPayload::enableMaterialProceduralShaders = action->isChecked();
    });

    {
        auto drawStatusConfig = qApp->getRenderEngine()->getConfiguration()->getConfig<render::DrawStatus>("RenderMainView.DrawStatus");
        addCheckableActionToQMenuAndActionHash(renderOptionsMenu, QCoreApplication::translate("MenuOption", MenuOption::HighlightTransitions.toUtf8().constData()), 0, false,
            drawStatusConfig, SLOT(setShowFade(bool)));
    }

    // Developer > Assets >>>
    // Menu item is not currently needed but code should be kept in case it proves useful again at some stage.
//#define WANT_ASSET_MIGRATION
#ifdef WANT_ASSET_MIGRATION
    MenuWrapper* assetDeveloperMenu = developerMenu->addMenu("Assets");
    auto& atpMigrator = ATPAssetMigrator::getInstance();
    atpMigrator.setDialogParent(this);

    addActionToQMenuAndActionHash(assetDeveloperMenu, QCoreApplication::translate("MenuOption", MenuOption::AssetMigration.toUtf8().constData()),
        0, &atpMigrator,
        SLOT(loadEntityServerFile()));
#endif

    // Developer > Avatar >>>
    MenuWrapper* avatarDebugMenu = developerMenu->addMenu("Avatar");

    action = addCheckableActionToQMenuAndActionHash(avatarDebugMenu, QCoreApplication::translate("MenuOption", MenuOption::AvatarReceiveStats.toUtf8().constData()), 0, false);
    connect(action, &QAction::triggered, [this]{ Avatar::setShowReceiveStats(isOptionChecked(QCoreApplication::translate("MenuOption", MenuOption::AvatarReceiveStats.toUtf8().constData()))); });
    action = addCheckableActionToQMenuAndActionHash(avatarDebugMenu, QCoreApplication::translate("MenuOption", MenuOption::ShowBoundingCollisionShapes.toUtf8().constData()), 0, false);
    connect(action, &QAction::triggered, [this]{ Avatar::setShowCollisionShapes(isOptionChecked(QCoreApplication::translate("MenuOption", MenuOption::ShowBoundingCollisionShapes.toUtf8().constData()))); });
    action = addCheckableActionToQMenuAndActionHash(avatarDebugMenu, QCoreApplication::translate("MenuOption", MenuOption::ShowMyLookAtVectors.toUtf8().constData()), 0, false);
    connect(action, &QAction::triggered, [this]{ Avatar::setShowMyLookAtVectors(isOptionChecked(QCoreApplication::translate("MenuOption", MenuOption::ShowMyLookAtVectors.toUtf8().constData()))); });
    action = addCheckableActionToQMenuAndActionHash(avatarDebugMenu, QCoreApplication::translate("MenuOption", MenuOption::ShowMyLookAtTarget.toUtf8().constData()), 0, false);
    connect(action, &QAction::triggered, [this]{ Avatar::setShowMyLookAtTarget(isOptionChecked(QCoreApplication::translate("MenuOption", MenuOption::ShowMyLookAtTarget.toUtf8().constData()))); });
    action = addCheckableActionToQMenuAndActionHash(avatarDebugMenu, QCoreApplication::translate("MenuOption", MenuOption::ShowOtherLookAtVectors.toUtf8().constData()), 0, false);
    connect(action, &QAction::triggered, [this]{ Avatar::setShowOtherLookAtVectors(isOptionChecked(QCoreApplication::translate("MenuOption", MenuOption::ShowOtherLookAtVectors.toUtf8().constData()))); });
    action = addCheckableActionToQMenuAndActionHash(avatarDebugMenu, QCoreApplication::translate("MenuOption", MenuOption::ShowOtherLookAtTarget.toUtf8().constData()), 0, false);
    connect(action, &QAction::triggered, [this]{ Avatar::setShowOtherLookAtTarget(isOptionChecked(QCoreApplication::translate("MenuOption", MenuOption::ShowOtherLookAtTarget.toUtf8().constData()))); });

    auto avatarManager = DependencyManager::get<AvatarManager>();
    auto avatar = avatarManager->getMyAvatar();

    action = addCheckableActionToQMenuAndActionHash(avatarDebugMenu, QCoreApplication::translate("MenuOption", MenuOption::EnableLookAtSnapping.toUtf8().constData()), 0, true);
    connect(action, &QAction::triggered, [this, avatar]{
            avatar->setProperty("lookAtSnappingEnabled", isOptionChecked(QCoreApplication::translate("MenuOption", MenuOption::EnableLookAtSnapping.toUtf8().constData())));
        });

    addCheckableActionToQMenuAndActionHash(avatarDebugMenu, QCoreApplication::translate("MenuOption", MenuOption::FixGaze.toUtf8().constData()), 0, false);
    addCheckableActionToQMenuAndActionHash(avatarDebugMenu, QCoreApplication::translate("MenuOption", MenuOption::ToggleHipsFollowing.toUtf8().constData()), 0, false,
        avatar.get(), SLOT(setToggleHips(bool)));
    addCheckableActionToQMenuAndActionHash(avatarDebugMenu, QCoreApplication::translate("MenuOption", MenuOption::AnimDebugDrawBaseOfSupport.toUtf8().constData()), 0, false,
        avatar.get(), SLOT(setEnableDebugDrawBaseOfSupport(bool)));
    addCheckableActionToQMenuAndActionHash(avatarDebugMenu, QCoreApplication::translate("MenuOption", MenuOption::AnimDebugDrawDefaultPose.toUtf8().constData()), 0, false,
        avatar.get(), SLOT(setEnableDebugDrawDefaultPose(bool)));
    addCheckableActionToQMenuAndActionHash(avatarDebugMenu, QCoreApplication::translate("MenuOption", MenuOption::AnimDebugDrawAnimPose.toUtf8().constData()), 0, false,
        avatar.get(), SLOT(setEnableDebugDrawAnimPose(bool)));
    addCheckableActionToQMenuAndActionHash(avatarDebugMenu, QCoreApplication::translate("MenuOption", MenuOption::AnimDebugDrawPosition.toUtf8().constData()), 0, false,
        avatar.get(), SLOT(setEnableDebugDrawPosition(bool)));
    addCheckableActionToQMenuAndActionHash(avatarDebugMenu, QCoreApplication::translate("MenuOption", MenuOption::AnimDebugDrawOtherSkeletons.toUtf8().constData()), 0, false,
        avatarManager.data(), SLOT(setEnableDebugDrawOtherSkeletons(bool)));
    addCheckableActionToQMenuAndActionHash(avatarDebugMenu, QCoreApplication::translate("MenuOption", MenuOption::MeshVisible.toUtf8().constData()), 0, true,
        avatar.get(), SLOT(setEnableMeshVisible(bool)));
    addCheckableActionToQMenuAndActionHash(avatarDebugMenu, QCoreApplication::translate("MenuOption", MenuOption::DisableEyelidAdjustment.toUtf8().constData()), 0, false);
    addCheckableActionToQMenuAndActionHash(avatarDebugMenu, QCoreApplication::translate("MenuOption", MenuOption::TurnWithHead.toUtf8().constData()), 0, false);
    addCheckableActionToQMenuAndActionHash(avatarDebugMenu, QCoreApplication::translate("MenuOption", MenuOption::EnableInverseKinematics.toUtf8().constData()), 0, true,
        avatar.get(), SLOT(setEnableInverseKinematics(bool)));
    addCheckableActionToQMenuAndActionHash(avatarDebugMenu, QCoreApplication::translate("MenuOption", MenuOption::RenderSensorToWorldMatrix.toUtf8().constData()), 0, false,
        avatar.get(), SLOT(setEnableDebugDrawSensorToWorldMatrix(bool)));
    addCheckableActionToQMenuAndActionHash(avatarDebugMenu, QCoreApplication::translate("MenuOption", MenuOption::RenderIKTargets.toUtf8().constData()), 0, false,
        avatar.get(), SLOT(setEnableDebugDrawIKTargets(bool)));
    addCheckableActionToQMenuAndActionHash(avatarDebugMenu, QCoreApplication::translate("MenuOption", MenuOption::RenderIKConstraints.toUtf8().constData()), 0, false,
        avatar.get(), SLOT(setEnableDebugDrawIKConstraints(bool)));
    addCheckableActionToQMenuAndActionHash(avatarDebugMenu, QCoreApplication::translate("MenuOption", MenuOption::RenderIKChains.toUtf8().constData()), 0, false,
        avatar.get(), SLOT(setEnableDebugDrawIKChains(bool)));
    addCheckableActionToQMenuAndActionHash(avatarDebugMenu, QCoreApplication::translate("MenuOption", MenuOption::RenderDetailedCollision.toUtf8().constData()), 0, false,
        avatar.get(), SLOT(setEnableDebugDrawDetailedCollision(bool)));

    addCheckableActionToQMenuAndActionHash(avatarDebugMenu, QCoreApplication::translate("MenuOption", MenuOption::ActionMotorControl.toUtf8().constData()), 0, true,
        avatar.get(), SLOT(updateMotionBehaviorFromMenu()),
        UNSPECIFIED_POSITION, "Developer");

    addCheckableActionToQMenuAndActionHash(avatarDebugMenu, QCoreApplication::translate("MenuOption", MenuOption::ScriptedMotorControl.toUtf8().constData()), 0, true,
        avatar.get(), SLOT(updateMotionBehaviorFromMenu()),
        UNSPECIFIED_POSITION, "Developer");

    addCheckableActionToQMenuAndActionHash(avatarDebugMenu, QCoreApplication::translate("MenuOption", MenuOption::ShowTrackedObjects.toUtf8().constData()), 0, false, qApp, SLOT(setShowTrackedObjects(bool)));

    addActionToQMenuAndActionHash(avatarDebugMenu, QCoreApplication::translate("MenuOption", MenuOption::PackageModel.toUtf8().constData()), 0, qApp, SLOT(packageModel()));

    // Developer > Hands >>>
    MenuWrapper* handOptionsMenu = developerMenu->addMenu("Hands");
    addCheckableActionToQMenuAndActionHash(handOptionsMenu, QCoreApplication::translate("MenuOption", MenuOption::DisplayHandTargets.toUtf8().constData()), 0, false,
        avatar.get(), SLOT(setEnableDebugDrawHandControllers(bool)));

    // Developer > Entities >>>
    MenuWrapper* entitiesOptionsMenu = developerMenu->addMenu("Entities");

    addActionToQMenuAndActionHash(entitiesOptionsMenu, QCoreApplication::translate("MenuOption", MenuOption::OctreeStats.toUtf8().constData()), 0,
        qApp, SLOT(loadEntityStatisticsDialog()));

    addCheckableActionToQMenuAndActionHash(entitiesOptionsMenu, QCoreApplication::translate("MenuOption", MenuOption::ShowRealtimeEntityStats.toUtf8().constData()));

    // Developer > Network >>>
    MenuWrapper* networkMenu = developerMenu->addMenu("Network");
    action = addActionToQMenuAndActionHash(networkMenu, QCoreApplication::translate("MenuOption", MenuOption::Networking.toUtf8().constData()));
    connect(action, &QAction::triggered, [] {
        qApp->showDialog(QString("hifi/dialogs/NetworkingPreferencesDialog.qml"),
            QString("hifi/tablet/TabletNetworkingPreferences.qml"), "NetworkingPreferencesDialog");
    });
    addActionToQMenuAndActionHash(networkMenu, QCoreApplication::translate("MenuOption", MenuOption::ReloadContent.toUtf8().constData()), 0, qApp, SLOT(reloadResourceCaches()));
    addActionToQMenuAndActionHash(networkMenu, QCoreApplication::translate("MenuOption", MenuOption::ClearDiskCache.toUtf8().constData()), 0,
        DependencyManager::get<AssetClient>().data(), SLOT(clearCache()));
    addCheckableActionToQMenuAndActionHash(networkMenu,
        QCoreApplication::translate("MenuOption", MenuOption::DisableActivityLogger.toUtf8().constData()),
        0,
        false,
        &UserActivityLogger::getInstance(),
        SLOT(disable(bool)));
    addActionToQMenuAndActionHash(networkMenu, QCoreApplication::translate("MenuOption", MenuOption::ShowDSConnectTable.toUtf8().constData()), 0,
        qApp, SLOT(loadDomainConnectionDialog()));

    #if (PR_BUILD || DEV_BUILD)
    addCheckableActionToQMenuAndActionHash(networkMenu, QCoreApplication::translate("MenuOption", MenuOption::SendWrongProtocolVersion.toUtf8().constData()), 0, false,
                qApp, SLOT(sendWrongProtocolVersionsSignature(bool)));

    {
        auto nodeList = DependencyManager::get<NodeList>();
        addCheckableActionToQMenuAndActionHash(networkMenu, QCoreApplication::translate("MenuOption", MenuOption::SendWrongDSConnectVersion.toUtf8().constData()), 0, false,
            nodeList.data(), SLOT(toggleSendNewerDSConnectVersion(bool)));
    }
    #endif

    // Developer > Timing >>>
    MenuWrapper* timingMenu = developerMenu->addMenu("Timing");
    MenuWrapper* perfTimerMenu = timingMenu->addMenu("Performance Timer");
    addCheckableActionToQMenuAndActionHash(perfTimerMenu, QCoreApplication::translate("MenuOption", MenuOption::DisplayDebugTimingDetails.toUtf8().constData()));
    addCheckableActionToQMenuAndActionHash(perfTimerMenu, QCoreApplication::translate("MenuOption", MenuOption::OnlyDisplayTopTen.toUtf8().constData()), 0, true);
    addCheckableActionToQMenuAndActionHash(perfTimerMenu, QCoreApplication::translate("MenuOption", MenuOption::ExpandUpdateTiming.toUtf8().constData()), 0, false);
    addCheckableActionToQMenuAndActionHash(perfTimerMenu, QCoreApplication::translate("MenuOption", MenuOption::ExpandSimulationTiming.toUtf8().constData()), 0, false);
    addCheckableActionToQMenuAndActionHash(perfTimerMenu, QCoreApplication::translate("MenuOption", MenuOption::ExpandPhysicsTiming.toUtf8().constData()), 0, false);
    addCheckableActionToQMenuAndActionHash(perfTimerMenu, QCoreApplication::translate("MenuOption", MenuOption::ExpandMyAvatarTiming.toUtf8().constData()), 0, false);
    addCheckableActionToQMenuAndActionHash(perfTimerMenu, QCoreApplication::translate("MenuOption", MenuOption::ExpandMyAvatarSimulateTiming.toUtf8().constData()), 0, false);
    addCheckableActionToQMenuAndActionHash(perfTimerMenu, QCoreApplication::translate("MenuOption", MenuOption::ExpandOtherAvatarTiming.toUtf8().constData()), 0, false);
    addCheckableActionToQMenuAndActionHash(perfTimerMenu, QCoreApplication::translate("MenuOption", MenuOption::ExpandPaintGLTiming.toUtf8().constData()), 0, false);

    addCheckableActionToQMenuAndActionHash(timingMenu, QCoreApplication::translate("MenuOption", MenuOption::FrameTimer.toUtf8().constData()));
    addActionToQMenuAndActionHash(timingMenu, QCoreApplication::translate("MenuOption", MenuOption::RunTimingTests.toUtf8().constData()), 0, qApp, SLOT(runTests()));
    addCheckableActionToQMenuAndActionHash(timingMenu, QCoreApplication::translate("MenuOption", MenuOption::PipelineWarnings.toUtf8().constData()));
    addCheckableActionToQMenuAndActionHash(timingMenu, QCoreApplication::translate("MenuOption", MenuOption::LogExtraTimings.toUtf8().constData()));
    addCheckableActionToQMenuAndActionHash(timingMenu, QCoreApplication::translate("MenuOption", MenuOption::SuppressShortTimings.toUtf8().constData()));


    // Developer > Audio >>>
    MenuWrapper* audioDebugMenu = developerMenu->addMenu("Audio");

    action = addActionToQMenuAndActionHash(audioDebugMenu, "Stats...");
    connect(action, &QAction::triggered, [] {
        QUrl defaultScriptsLoc = PathUtils::defaultScriptsLocation();
        defaultScriptsLoc.setPath(defaultScriptsLoc.path() + "developer/utilities/audio/stats.js");
        DependencyManager::get<ScriptEngines>()->loadScript(defaultScriptsLoc.toString());
    });

    action = addActionToQMenuAndActionHash(audioDebugMenu, "Buffers...");
    connect(action, &QAction::triggered, [] {
        qApp->showDialog(QString("hifi/dialogs/AudioBuffers.qml"),
            QString("hifi/tablet/TabletAudioBuffers.qml"), "AudioBuffersDialog");
    });

    addActionToQMenuAndActionHash(audioDebugMenu, QCoreApplication::translate("MenuOption", MenuOption::MuteEnvironment.toUtf8().constData()), 0,
        DependencyManager::get<AudioClient>().data(), SLOT(sendMuteEnvironmentPacket()));

    action = addActionToQMenuAndActionHash(audioDebugMenu, QCoreApplication::translate("MenuOption", MenuOption::AudioScope.toUtf8().constData()));
    connect(action, &QAction::triggered, [] {
        QUrl defaultScriptsLoc = PathUtils::defaultScriptsLocation();
        defaultScriptsLoc.setPath(defaultScriptsLoc.path() + "developer/utilities/audio/audioScope.js");
        DependencyManager::get<ScriptEngines>()->loadScript(defaultScriptsLoc.toString());
    });

    // Developer > Physics >>>
    MenuWrapper* physicsOptionsMenu = developerMenu->addMenu("Physics");
    {
        auto drawStatusConfig = qApp->getRenderEngine()->getConfiguration()->getConfig<render::DrawStatus>("RenderMainView.DrawStatus");
        addCheckableActionToQMenuAndActionHash(physicsOptionsMenu, QCoreApplication::translate("MenuOption", MenuOption::PhysicsShowOwned.toUtf8().constData()),
            0, false, drawStatusConfig, SLOT(setShowNetwork(bool)));
    }

    addCheckableActionToQMenuAndActionHash(physicsOptionsMenu, QCoreApplication::translate("MenuOption", MenuOption::PhysicsShowBulletWireframe.toUtf8().constData()), 0, false, qApp, SLOT(setShowBulletWireframe(bool)));
    addCheckableActionToQMenuAndActionHash(physicsOptionsMenu, QCoreApplication::translate("MenuOption", MenuOption::PhysicsShowBulletAABBs.toUtf8().constData()), 0, false, qApp, SLOT(setShowBulletAABBs(bool)));
    addCheckableActionToQMenuAndActionHash(physicsOptionsMenu, QCoreApplication::translate("MenuOption", MenuOption::PhysicsShowBulletContactPoints.toUtf8().constData()), 0, false, qApp, SLOT(setShowBulletContactPoints(bool)));
    addCheckableActionToQMenuAndActionHash(physicsOptionsMenu, QCoreApplication::translate("MenuOption", MenuOption::PhysicsShowBulletConstraints.toUtf8().constData()), 0, false, qApp, SLOT(setShowBulletConstraints(bool)));
    addCheckableActionToQMenuAndActionHash(physicsOptionsMenu, QCoreApplication::translate("MenuOption", MenuOption::PhysicsShowBulletConstraintLimits.toUtf8().constData()), 0, false, qApp, SLOT(setShowBulletConstraintLimits(bool)));

    // Developer > Picking >>>
    MenuWrapper* pickingOptionsMenu = developerMenu->addMenu("Picking");
    addCheckableActionToQMenuAndActionHash(pickingOptionsMenu, QCoreApplication::translate("MenuOption", MenuOption::ForceCoarsePicking.toUtf8().constData()), 0, false,
        DependencyManager::get<PickManager>().data(), SLOT(setForceCoarsePicking(bool)));

    // Developer > Crash >>>
    bool result = false;
    const QString HIFI_SHOW_DEVELOPER_CRASH_MENU("HIFI_SHOW_DEVELOPER_CRASH_MENU");
    result = QProcessEnvironment::systemEnvironment().contains(HIFI_SHOW_DEVELOPER_CRASH_MENU);
    if (result) {
        MenuWrapper* crashMenu = developerMenu->addMenu("Crash");
    
        // Developer > Crash > Display Crash Options
        addCheckableActionToQMenuAndActionHash(crashMenu, QCoreApplication::translate("MenuOption", MenuOption::DisplayCrashOptions.toUtf8().constData()), 0, true);

        addActionToQMenuAndActionHash(crashMenu, QCoreApplication::translate("MenuOption", MenuOption::DeadlockInterface.toUtf8().constData()), 0, qApp, SLOT(deadlockApplication()));
        addActionToQMenuAndActionHash(crashMenu, QCoreApplication::translate("MenuOption", MenuOption::UnresponsiveInterface.toUtf8().constData()), 0, qApp, SLOT(unresponsiveApplication()));

        action = addActionToQMenuAndActionHash(crashMenu, QCoreApplication::translate("MenuOption", MenuOption::CrashPureVirtualFunction.toUtf8().constData()));
        connect(action, &QAction::triggered, qApp, []() { crash::pureVirtualCall(); });
        action = addActionToQMenuAndActionHash(crashMenu, QCoreApplication::translate("MenuOption", MenuOption::CrashPureVirtualFunctionThreaded.toUtf8().constData()));
        connect(action, &QAction::triggered, qApp, []() { std::thread(crash::pureVirtualCall).join(); });

        action = addActionToQMenuAndActionHash(crashMenu, QCoreApplication::translate("MenuOption", MenuOption::CrashDoubleFree.toUtf8().constData()));
        connect(action, &QAction::triggered, qApp, []() { crash::doubleFree(); });
        action = addActionToQMenuAndActionHash(crashMenu, QCoreApplication::translate("MenuOption", MenuOption::CrashDoubleFreeThreaded.toUtf8().constData()));
        connect(action, &QAction::triggered, qApp, []() { std::thread(crash::doubleFree).join(); });

        action = addActionToQMenuAndActionHash(crashMenu, QCoreApplication::translate("MenuOption", MenuOption::CrashAbort.toUtf8().constData()));
        connect(action, &QAction::triggered, qApp, []() { crash::doAbort(); });
        action = addActionToQMenuAndActionHash(crashMenu, QCoreApplication::translate("MenuOption", MenuOption::CrashAbortThreaded.toUtf8().constData()));
        connect(action, &QAction::triggered, qApp, []() { std::thread(crash::doAbort).join(); });

        action = addActionToQMenuAndActionHash(crashMenu, QCoreApplication::translate("MenuOption", MenuOption::CrashNullDereference.toUtf8().constData()));
        connect(action, &QAction::triggered, qApp, []() { crash::nullDeref(); });
        action = addActionToQMenuAndActionHash(crashMenu, QCoreApplication::translate("MenuOption", MenuOption::CrashNullDereferenceThreaded.toUtf8().constData()));
        connect(action, &QAction::triggered, qApp, []() { std::thread(crash::nullDeref).join(); });

        action = addActionToQMenuAndActionHash(crashMenu, QCoreApplication::translate("MenuOption", MenuOption::CrashOutOfBoundsVectorAccess.toUtf8().constData()));
        connect(action, &QAction::triggered, qApp, []() { crash::outOfBoundsVectorCrash(); });
        action = addActionToQMenuAndActionHash(crashMenu, QCoreApplication::translate("MenuOption", MenuOption::CrashOutOfBoundsVectorAccessThreaded.toUtf8().constData()));
        connect(action, &QAction::triggered, qApp, []() { std::thread(crash::outOfBoundsVectorCrash).join(); });

        action = addActionToQMenuAndActionHash(crashMenu, QCoreApplication::translate("MenuOption", MenuOption::CrashNewFault.toUtf8().constData()));
        connect(action, &QAction::triggered, qApp, []() { crash::newFault(); });
        action = addActionToQMenuAndActionHash(crashMenu, QCoreApplication::translate("MenuOption", MenuOption::CrashNewFaultThreaded.toUtf8().constData()));
        connect(action, &QAction::triggered, qApp, []() { std::thread(crash::newFault).join(); });

        addActionToQMenuAndActionHash(crashMenu, QCoreApplication::translate("MenuOption", MenuOption::CrashOnShutdown.toUtf8().constData()), 0, qApp, SLOT(crashOnShutdown()));
    }
    

    // Developer > Show Statistics
    addCheckableActionToQMenuAndActionHash(developerMenu, QCoreApplication::translate("MenuOption", MenuOption::Stats.toUtf8().constData()), 0, true);

    // Developer > Show Animation Statistics
    addCheckableActionToQMenuAndActionHash(developerMenu, QCoreApplication::translate("MenuOption", MenuOption::AnimStats.toUtf8().constData()));

    // Developer > Log
    addActionToQMenuAndActionHash(developerMenu, QCoreApplication::translate("MenuOption", MenuOption::Log.toUtf8().constData()), Qt::CTRL | Qt::SHIFT | Qt::Key_L,
                                  qApp, SLOT(toggleLogDialog()));

#if 0 ///  -------------- REMOVED FOR NOW --------------
    addDisabledActionAndSeparator(navigateMenu, "History");
    QAction* backAction = addActionToQMenuAndActionHash(navigateMenu, QCoreApplication::translate("MenuOption", MenuOption::Back.toUtf8().constData()), 0, addressManager.data(), SLOT(goBack()));
    QAction* forwardAction = addActionToQMenuAndActionHash(navigateMenu, QCoreApplication::translate("MenuOption", MenuOption::Forward.toUtf8().constData()), 0, addressManager.data(), SLOT(goForward()));

    // connect to the AddressManager signal to enable and disable the back and forward menu items
    connect(addressManager.data(), &AddressManager::goBackPossible, backAction, &QAction::setEnabled);
    connect(addressManager.data(), &AddressManager::goForwardPossible, forwardAction, &QAction::setEnabled);

    // set the two actions to start disabled since the stacks are clear on startup
    backAction->setDisabled(true);
    forwardAction->setDisabled(true);

    MenuWrapper* toolsMenu = addMenu("Tools");
    addActionToQMenuAndActionHash(toolsMenu,
                                  QCoreApplication::translate("MenuOption", MenuOption::ToolWindow.toUtf8().constData()),
                                  Qt::CTRL | Qt::ALT | Qt::Key_T,
                                  dialogsManager.data(),
                                  SLOT(toggleToolWindow()),
                                  QAction::NoRole, UNSPECIFIED_POSITION, "Advanced");


    addCheckableActionToQMenuAndActionHash(avatarMenu, QCoreApplication::translate("MenuOption", MenuOption::NamesAboveHeads.toUtf8().constData()), 0, true,
                NULL, NULL, UNSPECIFIED_POSITION, "Advanced");
#endif

    // Help/Application menu ----------------------------------
    MenuWrapper * helpMenu = addMenu("Help");

    // Help > About Project Athena
    action = addActionToQMenuAndActionHash(helpMenu, "About Project Athena");
    connect(action, &QAction::triggered, [] {
        qApp->showDialog(QString("hifi/dialogs/AboutDialog.qml"),
            QString("hifi/dialogs/TabletAboutDialog.qml"), "AboutDialog");
    });
    helpMenu->addSeparator();

    // Help > Athena Docs
    action = addActionToQMenuAndActionHash(helpMenu, "Online Documentation");
    connect(action, &QAction::triggered, qApp, [] {
        QDesktopServices::openUrl(QUrl("https://docs.projectathena.dev/"));
    });

    // Help > Athena Forum
    /* action = addActionToQMenuAndActionHash(helpMenu, "Online Forums");
    connect(action, &QAction::triggered, qApp, [] {
        QDesktopServices::openUrl(QUrl("https://forums.highfidelity.com/"));
    }); */

    // Help > Scripting Reference
    action = addActionToQMenuAndActionHash(helpMenu, "Online Script Reference");
    connect(action, &QAction::triggered, qApp, [] {
        QDesktopServices::openUrl(QUrl("https://apidocs.projectathena.dev/"));
    });

    addActionToQMenuAndActionHash(helpMenu, "Controls Reference", 0, qApp, SLOT(showHelp()));

    helpMenu->addSeparator();

    // Help > Release Notes
    action = addActionToQMenuAndActionHash(helpMenu, "Release Notes");
    connect(action, &QAction::triggered, qApp, [] {
        QDesktopServices::openUrl(QUrl("https://docs.projectathena.dev/release-notes.html"));
    });

    // Help > Report a Bug!
    action = addActionToQMenuAndActionHash(helpMenu, "Report a Bug!");
    connect(action, &QAction::triggered, qApp, [] {
        QDesktopServices::openUrl(QUrl("https://github.com/kasenvr/project-athena/issues"));
    });
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
