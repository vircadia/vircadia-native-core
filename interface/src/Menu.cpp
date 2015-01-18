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

#include <cstdlib>

#include <QBoxLayout>
#include <QColorDialog>
#include <QClipboard>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QInputDialog>
#include <QLineEdit>
#include <QMainWindow>
#include <QMenuBar>
#include <QMessageBox>
#include <QShortcut>
#include <QSlider>
#include <QUuid>
#include <QHBoxLayout>
#include <QDesktopServices>

#include <AccountManager.h>
#include <AddressManager.h>
#include <DependencyManager.h>
#include <MainWindow.h>
#include <GlowEffect.h>
#include <PathUtils.h>
#include <UUID.h>
#include <UserActivityLogger.h>
#include <XmppClient.h>

#include "Application.h"
#include "AccountManager.h"
#include "Audio.h"
#include "audio/AudioIOStatsRenderer.h"
#include "audio/AudioScope.h"
#include "devices/Faceshift.h"
#include "devices/RealSense.h"
#include "devices/SixenseManager.h"
#include "devices/Visage.h"
#include "Menu.h"
#include "scripting/LocationScriptingInterface.h"
#include "scripting/MenuScriptingInterface.h"
#include "Util.h"
#include "ui/AddressBarDialog.h"
#include "ui/AnimationsDialog.h"
#include "ui/AttachmentsDialog.h"
#include "ui/BandwidthDialog.h"
#include "ui/CachesSizeDialog.h"
#include "ui/DataWebDialog.h"
#include "ui/DialogsManager.h"
#include "ui/HMDToolsDialog.h"
#include "ui/LodToolsDialog.h"
#include "ui/LoginDialog.h"
#include "ui/OctreeStatsDialog.h"
#include "ui/PreferencesDialog.h"
#include "ui/InfoView.h"
#include "ui/MetavoxelEditor.h"
#include "ui/MetavoxelNetworkSimulator.h"
#include "ui/ModelsBrowser.h"
#include "ui/NodeBounds.h"
#include "ui/StandAloneJSConsole.h"

Menu* Menu::_instance = NULL;

Menu* Menu::getInstance() {
    static QMutex menuInstanceMutex;

    // lock the menu instance mutex to make sure we don't race and create two menus and crash
    menuInstanceMutex.lock();

    if (!_instance) {
        qDebug("First call to Menu::getInstance() - initing menu.");

        _instance = new Menu();
    }

    menuInstanceMutex.unlock();

    return _instance;
}

Menu::Menu() {
    Application *appInstance = Application::getInstance();

    QMenu* fileMenu = addMenu("File");

#ifdef Q_OS_MAC
    addActionToQMenuAndActionHash(fileMenu,
                                  MenuOption::AboutApp,
                                  0,
                                  this,
                                  SLOT(aboutApp()),
                                  QAction::AboutRole);
#endif
    auto dialogsManager = DependencyManager::get<DialogsManager>();
    AccountManager& accountManager = AccountManager::getInstance();

    {
        addActionToQMenuAndActionHash(fileMenu, MenuOption::Login);
        
        // connect to the appropriate signal of the AccountManager so that we can change the Login/Logout menu item
        connect(&accountManager, &AccountManager::profileChanged,
                dialogsManager.data(), &DialogsManager::toggleLoginDialog);
        connect(&accountManager, &AccountManager::logoutComplete,
                dialogsManager.data(), &DialogsManager::toggleLoginDialog);
    }
    
    addDisabledActionAndSeparator(fileMenu, "Scripts");
    addActionToQMenuAndActionHash(fileMenu, MenuOption::LoadScript, Qt::CTRL | Qt::Key_O,
                                  appInstance, SLOT(loadDialog()));
    addActionToQMenuAndActionHash(fileMenu, MenuOption::LoadScriptURL,
                                    Qt::CTRL | Qt::SHIFT | Qt::Key_O, appInstance, SLOT(loadScriptURLDialog()));
    addActionToQMenuAndActionHash(fileMenu, MenuOption::StopAllScripts, 0, appInstance, SLOT(stopAllScripts()));
    addActionToQMenuAndActionHash(fileMenu, MenuOption::ReloadAllScripts, Qt::CTRL | Qt::Key_R,
                                  appInstance, SLOT(reloadAllScripts()));
    addActionToQMenuAndActionHash(fileMenu, MenuOption::RunningScripts, Qt::CTRL | Qt::Key_J,
                                  appInstance, SLOT(toggleRunningScriptsWidget()));

    addDisabledActionAndSeparator(fileMenu, "Location");
    qApp->getBookmarks()->setupMenus(this, fileMenu);
    
    addActionToQMenuAndActionHash(fileMenu,
                                  MenuOption::AddressBar,
                                  Qt::Key_Enter,
                                  dialogsManager.data(),
                                  SLOT(toggleAddressBar()));
    addActionToQMenuAndActionHash(fileMenu, MenuOption::CopyAddress, 0,
                                  this, SLOT(copyAddress()));
    addActionToQMenuAndActionHash(fileMenu, MenuOption::CopyPath, 0,
                                  this, SLOT(copyPath()));

    addDisabledActionAndSeparator(fileMenu, "Upload Avatar Model");
    addActionToQMenuAndActionHash(fileMenu, MenuOption::UploadHead, 0,
                                  Application::getInstance(), SLOT(uploadHead()));
    addActionToQMenuAndActionHash(fileMenu, MenuOption::UploadSkeleton, 0,
                                  Application::getInstance(), SLOT(uploadSkeleton()));
    addActionToQMenuAndActionHash(fileMenu, MenuOption::UploadAttachment, 0,
                                  Application::getInstance(), SLOT(uploadAttachment()));
    addActionToQMenuAndActionHash(fileMenu, MenuOption::UploadEntity, 0,
                                  Application::getInstance(), SLOT(uploadEntity()));
    addDisabledActionAndSeparator(fileMenu, "Settings");
    addActionToQMenuAndActionHash(fileMenu, MenuOption::SettingsImport, 0, this, SLOT(importSettings()));
    addActionToQMenuAndActionHash(fileMenu, MenuOption::SettingsExport, 0, this, SLOT(exportSettings()));

    addActionToQMenuAndActionHash(fileMenu,
                                  MenuOption::Quit,
                                  Qt::CTRL | Qt::Key_Q,
                                  appInstance,
                                  SLOT(quit()),
                                  QAction::QuitRole);


    QMenu* editMenu = addMenu("Edit");

    QUndoStack* undoStack = Application::getInstance()->getUndoStack();
    QAction* undoAction = undoStack->createUndoAction(editMenu);
    undoAction->setShortcut(Qt::CTRL | Qt::Key_Z);
    addActionToQMenuAndActionHash(editMenu, undoAction);

    QAction* redoAction = undoStack->createRedoAction(editMenu);
    redoAction->setShortcut(Qt::CTRL | Qt::SHIFT | Qt::Key_Z);
    addActionToQMenuAndActionHash(editMenu, redoAction);

    addActionToQMenuAndActionHash(editMenu,
                                  MenuOption::Preferences,
                                  Qt::CTRL | Qt::Key_Comma,
                                  dialogsManager.data(),
                                  SLOT(editPreferences()),
                                  QAction::PreferencesRole);

    addActionToQMenuAndActionHash(editMenu, MenuOption::Attachments, 0,
                                  dialogsManager.data(), SLOT(editAttachments()));
    addActionToQMenuAndActionHash(editMenu, MenuOption::Animations, 0,
                                  dialogsManager.data(), SLOT(editAnimations()));

    QMenu* toolsMenu = addMenu("Tools");
    addActionToQMenuAndActionHash(toolsMenu, MenuOption::MetavoxelEditor, 0,
                                  dialogsManager.data(), SLOT(showMetavoxelEditor()));
    addActionToQMenuAndActionHash(toolsMenu, MenuOption::ScriptEditor,  Qt::ALT | Qt::Key_S,
                                  dialogsManager.data(), SLOT(showScriptEditor()));

#if defined(Q_OS_MAC) || defined(Q_OS_WIN)
    QAction* speechRecognizerAction = addCheckableActionToQMenuAndActionHash(toolsMenu, MenuOption::ControlWithSpeech,
            Qt::CTRL | Qt::SHIFT | Qt::Key_C, _speechRecognizer.getEnabled(), &_speechRecognizer, SLOT(setEnabled(bool)));
    connect(&_speechRecognizer, SIGNAL(enabledUpdated(bool)), speechRecognizerAction, SLOT(setChecked(bool)));
#endif

#ifdef HAVE_QXMPP
    addActionToQMenuAndActionHash(toolsMenu, MenuOption::Chat, Qt::Key_Backslash,
                                  dialogsManager.data(), SLOT(showChat()));
    dialogsManager->setupChat();
#endif

    addActionToQMenuAndActionHash(toolsMenu,
                                  MenuOption::ToolWindow,
                                  Qt::CTRL | Qt::ALT | Qt::Key_T,
                                  dialogsManager.data(),
                                  SLOT(toggleToolWindow()));

    addActionToQMenuAndActionHash(toolsMenu,
                                  MenuOption::Console,
                                  Qt::CTRL | Qt::ALT | Qt::Key_J,
                                  DependencyManager::get<StandAloneJSConsole>().data(),
                                  SLOT(toggleConsole()));

    addActionToQMenuAndActionHash(toolsMenu,
                                  MenuOption::ResetSensors,
                                  Qt::Key_Apostrophe,
                                  appInstance,
                                  SLOT(resetSensors()));

    QMenu* avatarMenu = addMenu("Avatar");

    QMenu* avatarSizeMenu = avatarMenu->addMenu("Size");
    addActionToQMenuAndActionHash(avatarSizeMenu,
                                  MenuOption::IncreaseAvatarSize,
                                  Qt::Key_Plus,
                                  appInstance->getAvatar(),
                                  SLOT(increaseSize()));
    addActionToQMenuAndActionHash(avatarSizeMenu,
                                  MenuOption::DecreaseAvatarSize,
                                  Qt::Key_Minus,
                                  appInstance->getAvatar(),
                                  SLOT(decreaseSize()));
    addActionToQMenuAndActionHash(avatarSizeMenu,
                                  MenuOption::ResetAvatarSize,
                                  Qt::Key_Equal,
                                  appInstance->getAvatar(),
                                  SLOT(resetSize()));

    QObject* avatar = appInstance->getAvatar();
    addCheckableActionToQMenuAndActionHash(avatarMenu, MenuOption::KeyboardMotorControl, 
            Qt::CTRL | Qt::SHIFT | Qt::Key_K, true, avatar, SLOT(updateMotionBehavior()));
    addCheckableActionToQMenuAndActionHash(avatarMenu, MenuOption::ScriptedMotorControl, 0, true,
            avatar, SLOT(updateMotionBehavior()));
    addCheckableActionToQMenuAndActionHash(avatarMenu, MenuOption::ChatCircling, 0, false);
    addCheckableActionToQMenuAndActionHash(avatarMenu, MenuOption::NamesAboveHeads, 0, true);
    addCheckableActionToQMenuAndActionHash(avatarMenu, MenuOption::GlowWhenSpeaking, 0, true);
    addCheckableActionToQMenuAndActionHash(avatarMenu, MenuOption::BlueSpeechSphere, 0, true);
    addCheckableActionToQMenuAndActionHash(avatarMenu, MenuOption::ObeyEnvironmentalGravity, Qt::SHIFT | Qt::Key_G, false,
            avatar, SLOT(updateMotionBehavior()));
    addCheckableActionToQMenuAndActionHash(avatarMenu, MenuOption::StandOnNearbyFloors, 0, true,
            avatar, SLOT(updateMotionBehavior()));
    addCheckableActionToQMenuAndActionHash(avatarMenu, MenuOption::ShiftHipsForIdleAnimations, 0, false,
            avatar, SLOT(updateMotionBehavior()));

    QMenu* collisionsMenu = avatarMenu->addMenu("Collide With");
    addCheckableActionToQMenuAndActionHash(collisionsMenu, MenuOption::CollideAsRagdoll, 0, false, 
            avatar, SLOT(onToggleRagdoll()));
    addCheckableActionToQMenuAndActionHash(collisionsMenu, MenuOption::CollideWithAvatars,
            0, true, avatar, SLOT(updateCollisionGroups()));
    addCheckableActionToQMenuAndActionHash(collisionsMenu, MenuOption::CollideWithEnvironment,
            0, false, avatar, SLOT(updateCollisionGroups()));

    QMenu* viewMenu = addMenu("View");

#ifdef Q_OS_MAC
    addCheckableActionToQMenuAndActionHash(viewMenu,
                                           MenuOption::Fullscreen,
                                           Qt::CTRL | Qt::META | Qt::Key_F,
                                           false,
                                           appInstance,
                                           SLOT(setFullscreen(bool)));
#else
    addCheckableActionToQMenuAndActionHash(viewMenu,
                                           MenuOption::Fullscreen,
                                           Qt::CTRL | Qt::Key_F,
                                           false,
                                           appInstance,
                                           SLOT(setFullscreen(bool)));
#endif
    addCheckableActionToQMenuAndActionHash(viewMenu, MenuOption::FirstPerson, Qt::Key_P, true,
                                            appInstance,SLOT(cameraMenuChanged()));
    addCheckableActionToQMenuAndActionHash(viewMenu, MenuOption::Mirror, Qt::SHIFT | Qt::Key_H, true);
    addCheckableActionToQMenuAndActionHash(viewMenu, MenuOption::FullscreenMirror, Qt::Key_H, false,
                                            appInstance, SLOT(cameraMenuChanged()));
    addCheckableActionToQMenuAndActionHash(viewMenu, MenuOption::UserInterface, Qt::Key_Slash, true);

    addCheckableActionToQMenuAndActionHash(viewMenu, MenuOption::HMDTools, Qt::META | Qt::Key_H,
                                           false,
                                           dialogsManager.data(),
                                           SLOT(hmdTools(bool)));


    addCheckableActionToQMenuAndActionHash(viewMenu, MenuOption::EnableVRMode, 0,
                                           false,
                                           appInstance,
                                           SLOT(setEnableVRMode(bool)));

    addCheckableActionToQMenuAndActionHash(viewMenu, MenuOption::Enable3DTVMode, 0,
                                           false,
                                           appInstance,
                                           SLOT(setEnable3DTVMode(bool)));


    QMenu* nodeBordersMenu = viewMenu->addMenu("Server Borders");
    NodeBounds& nodeBounds = appInstance->getNodeBoundsDisplay();
    addCheckableActionToQMenuAndActionHash(nodeBordersMenu, MenuOption::ShowBordersEntityNodes,
                                           Qt::CTRL | Qt::SHIFT | Qt::Key_1, false,
                                           &nodeBounds, SLOT(setShowEntityNodes(bool)));

    addCheckableActionToQMenuAndActionHash(viewMenu, MenuOption::OffAxisProjection, 0, false);
    addCheckableActionToQMenuAndActionHash(viewMenu, MenuOption::TurnWithHead, 0, false);


    addDisabledActionAndSeparator(viewMenu, "Stats");
    addCheckableActionToQMenuAndActionHash(viewMenu, MenuOption::Stats, Qt::Key_Percent);
    addActionToQMenuAndActionHash(viewMenu, MenuOption::Log, Qt::CTRL | Qt::Key_L, appInstance, SLOT(toggleLogDialog()));
    addCheckableActionToQMenuAndActionHash(viewMenu, MenuOption::Bandwidth, 0, true);
    addActionToQMenuAndActionHash(viewMenu, MenuOption::BandwidthDetails, 0,
                                  dialogsManager.data(), SLOT(bandwidthDetails()));
    addActionToQMenuAndActionHash(viewMenu, MenuOption::OctreeStats, 0,
                                  dialogsManager.data(), SLOT(octreeStatsDetails()));
    addActionToQMenuAndActionHash(viewMenu, MenuOption::EditEntitiesHelp, 0, this, SLOT(showEditEntitiesHelp()));

    QMenu* developerMenu = addMenu("Developer");

    QMenu* renderOptionsMenu = developerMenu->addMenu("Render");
    addCheckableActionToQMenuAndActionHash(renderOptionsMenu, MenuOption::Atmosphere, Qt::SHIFT | Qt::Key_A, true);
    addCheckableActionToQMenuAndActionHash(renderOptionsMenu, MenuOption::Avatars, 0, true);
    addCheckableActionToQMenuAndActionHash(renderOptionsMenu, MenuOption::Metavoxels, 0, true);
    addCheckableActionToQMenuAndActionHash(renderOptionsMenu, MenuOption::Entities, 0, true);
    addCheckableActionToQMenuAndActionHash(renderOptionsMenu, MenuOption::AmbientOcclusion);
    addCheckableActionToQMenuAndActionHash(renderOptionsMenu, MenuOption::DontFadeOnOctreeServerChanges);
    addCheckableActionToQMenuAndActionHash(renderOptionsMenu, MenuOption::DisableAutoAdjustLOD);
    
    QMenu* shadowMenu = renderOptionsMenu->addMenu("Shadows");
    QActionGroup* shadowGroup = new QActionGroup(shadowMenu);
    shadowGroup->addAction(addCheckableActionToQMenuAndActionHash(shadowMenu, "None", 0, true));
    shadowGroup->addAction(addCheckableActionToQMenuAndActionHash(shadowMenu, MenuOption::SimpleShadows, 0, false));
    shadowGroup->addAction(addCheckableActionToQMenuAndActionHash(shadowMenu, MenuOption::CascadedShadows, 0, false));

    {
        QMenu* framerateMenu = renderOptionsMenu->addMenu(MenuOption::RenderTargetFramerate);
        QActionGroup* framerateGroup = new QActionGroup(framerateMenu);
        framerateGroup->setExclusive(true);
        framerateGroup->addAction(addCheckableActionToQMenuAndActionHash(framerateMenu, MenuOption::RenderTargetFramerateUnlimited, 0, true));
        framerateGroup->addAction(addCheckableActionToQMenuAndActionHash(framerateMenu, MenuOption::RenderTargetFramerate60, 0, false));
        framerateGroup->addAction(addCheckableActionToQMenuAndActionHash(framerateMenu, MenuOption::RenderTargetFramerate50, 0, false));
        framerateGroup->addAction(addCheckableActionToQMenuAndActionHash(framerateMenu, MenuOption::RenderTargetFramerate40, 0, false));
        framerateGroup->addAction(addCheckableActionToQMenuAndActionHash(framerateMenu, MenuOption::RenderTargetFramerate30, 0, false));

#if defined(Q_OS_MAC)
#else
        addCheckableActionToQMenuAndActionHash(renderOptionsMenu, MenuOption::RenderTargetFramerateVSyncOn, 0, true, this, SLOT(changeVSync()));
#endif
    }


    QMenu* resolutionMenu = renderOptionsMenu->addMenu(MenuOption::RenderResolution);
    QActionGroup* resolutionGroup = new QActionGroup(resolutionMenu);
    resolutionGroup->setExclusive(true);
    resolutionGroup->addAction(addCheckableActionToQMenuAndActionHash(resolutionMenu, MenuOption::RenderResolutionOne, 0, true));
    resolutionGroup->addAction(addCheckableActionToQMenuAndActionHash(resolutionMenu, MenuOption::RenderResolutionTwoThird, 0, false));
    resolutionGroup->addAction(addCheckableActionToQMenuAndActionHash(resolutionMenu, MenuOption::RenderResolutionHalf, 0, false));
    resolutionGroup->addAction(addCheckableActionToQMenuAndActionHash(resolutionMenu, MenuOption::RenderResolutionThird, 0, false));
    resolutionGroup->addAction(addCheckableActionToQMenuAndActionHash(resolutionMenu, MenuOption::RenderResolutionQuarter, 0, false));

    addCheckableActionToQMenuAndActionHash(renderOptionsMenu, MenuOption::Stars, Qt::Key_Asterisk, true);
    addCheckableActionToQMenuAndActionHash(renderOptionsMenu, MenuOption::EnableGlowEffect, 0, true, 
                                            DependencyManager::get<GlowEffect>().data(), SLOT(toggleGlowEffect(bool)));

    addCheckableActionToQMenuAndActionHash(renderOptionsMenu, MenuOption::Wireframe, Qt::ALT | Qt::Key_W, false);
    addActionToQMenuAndActionHash(renderOptionsMenu, MenuOption::LodTools, Qt::SHIFT | Qt::Key_L,
                                  dialogsManager.data(), SLOT(lodTools()));

    QMenu* avatarDebugMenu = developerMenu->addMenu("Avatar");
#ifdef HAVE_FACESHIFT
    addCheckableActionToQMenuAndActionHash(avatarDebugMenu,
                                           MenuOption::Faceshift,
                                           0,
                                           true,
                                           DependencyManager::get<Faceshift>().data(),
                                           SLOT(setTCPEnabled(bool)));
#endif
#ifdef HAVE_VISAGE
    addCheckableActionToQMenuAndActionHash(avatarDebugMenu, MenuOption::Visage, 0, false,
            DependencyManager::get<Visage>().data(), SLOT(updateEnabled()));
#endif

    addCheckableActionToQMenuAndActionHash(avatarDebugMenu, MenuOption::RenderSkeletonCollisionShapes);
    addCheckableActionToQMenuAndActionHash(avatarDebugMenu, MenuOption::RenderHeadCollisionShapes);
    addCheckableActionToQMenuAndActionHash(avatarDebugMenu, MenuOption::RenderBoundingCollisionShapes);
    addCheckableActionToQMenuAndActionHash(avatarDebugMenu, MenuOption::RenderLookAtVectors, 0, false);
    addCheckableActionToQMenuAndActionHash(avatarDebugMenu, MenuOption::RenderFocusIndicator, 0, false);

    QMenu* metavoxelOptionsMenu = developerMenu->addMenu("Metavoxels");
    addCheckableActionToQMenuAndActionHash(metavoxelOptionsMenu, MenuOption::DisplayHermiteData, 0, false,
        Application::getInstance()->getMetavoxels(), SLOT(refreshVoxelData()));
    addCheckableActionToQMenuAndActionHash(metavoxelOptionsMenu, MenuOption::RenderSpanners, 0, true);
    addCheckableActionToQMenuAndActionHash(metavoxelOptionsMenu, MenuOption::RenderDualContourSurfaces, 0, true);
    addActionToQMenuAndActionHash(metavoxelOptionsMenu, MenuOption::NetworkSimulator, 0,
                                  dialogsManager.data(), SLOT(showMetavoxelNetworkSimulator()));
    
    QMenu* handOptionsMenu = developerMenu->addMenu("Hands");
    addCheckableActionToQMenuAndActionHash(handOptionsMenu, MenuOption::AlignForearmsWithWrists, 0, false);
    addCheckableActionToQMenuAndActionHash(handOptionsMenu, MenuOption::AlternateIK, 0, false);
    addCheckableActionToQMenuAndActionHash(handOptionsMenu, MenuOption::DisplayHands, 0, true);
    addCheckableActionToQMenuAndActionHash(handOptionsMenu, MenuOption::DisplayHandTargets, 0, false);
    addCheckableActionToQMenuAndActionHash(handOptionsMenu, MenuOption::ShowIKConstraints, 0, false);
    
    QMenu* sixenseOptionsMenu = handOptionsMenu->addMenu("Sixense");
#ifdef __APPLE__
    addCheckableActionToQMenuAndActionHash(sixenseOptionsMenu,
                                           MenuOption::SixenseEnabled,
                                           0, false,
                                           this,
                                           SLOT(toggleSixense(bool)));
#endif
    addCheckableActionToQMenuAndActionHash(sixenseOptionsMenu,
                                           MenuOption::FilterSixense,
                                           0,
                                           true,
                                           &SixenseManager::getInstance(),
                                           SLOT(setFilter(bool)));
    addCheckableActionToQMenuAndActionHash(sixenseOptionsMenu,
                                           MenuOption::LowVelocityFilter,
                                           0,
                                           true,
                                           appInstance,
                                           SLOT(setLowVelocityFilter(bool)));
    addCheckableActionToQMenuAndActionHash(sixenseOptionsMenu, MenuOption::SixenseMouseInput, 0, true);
    addCheckableActionToQMenuAndActionHash(sixenseOptionsMenu, MenuOption::SixenseLasers, 0, false);

    QMenu* leapOptionsMenu = handOptionsMenu->addMenu("Leap Motion");
    addCheckableActionToQMenuAndActionHash(leapOptionsMenu, MenuOption::LeapMotionOnHMD, 0, false);

#ifdef HAVE_RSSDK
    QMenu* realSenseOptionsMenu = handOptionsMenu->addMenu("RealSense");
    addActionToQMenuAndActionHash(realSenseOptionsMenu, MenuOption::LoadRSSDKFile, 0, this, SLOT(loadRSSDKFile()));
#endif

    QMenu* networkMenu = developerMenu->addMenu("Network");
    addCheckableActionToQMenuAndActionHash(networkMenu, MenuOption::DisableNackPackets, 0, false);
    addCheckableActionToQMenuAndActionHash(networkMenu,
                                           MenuOption::DisableActivityLogger,
                                           0,
                                           false,
                                           &UserActivityLogger::getInstance(),
                                           SLOT(disable(bool)));
    addActionToQMenuAndActionHash(networkMenu, MenuOption::CachesSize, 0,
                                  dialogsManager.data(), SLOT(cachesSizeDialog()));
    
    addActionToQMenuAndActionHash(developerMenu, MenuOption::WalletPrivateKey, 0, this, SLOT(changePrivateKey()));

    QMenu* timingMenu = developerMenu->addMenu("Timing and Stats");
    QMenu* perfTimerMenu = timingMenu->addMenu("Performance Timer");
    addCheckableActionToQMenuAndActionHash(perfTimerMenu, MenuOption::DisplayTimingDetails, 0, true);
    addCheckableActionToQMenuAndActionHash(perfTimerMenu, MenuOption::ExpandUpdateTiming, 0, false);
    addCheckableActionToQMenuAndActionHash(perfTimerMenu, MenuOption::ExpandMyAvatarTiming, 0, false);
    addCheckableActionToQMenuAndActionHash(perfTimerMenu, MenuOption::ExpandMyAvatarSimulateTiming, 0, false);
    addCheckableActionToQMenuAndActionHash(perfTimerMenu, MenuOption::ExpandOtherAvatarTiming, 0, false);
    addCheckableActionToQMenuAndActionHash(perfTimerMenu, MenuOption::ExpandPaintGLTiming, 0, false);

    addCheckableActionToQMenuAndActionHash(timingMenu, MenuOption::TestPing, 0, true);
    addCheckableActionToQMenuAndActionHash(timingMenu, MenuOption::FrameTimer);
    addActionToQMenuAndActionHash(timingMenu, MenuOption::RunTimingTests, 0, this, SLOT(runTests()));
    addCheckableActionToQMenuAndActionHash(timingMenu, MenuOption::PipelineWarnings);
    addCheckableActionToQMenuAndActionHash(timingMenu, MenuOption::SuppressShortTimings);

    auto audioIO = DependencyManager::get<Audio>();
    QMenu* audioDebugMenu = developerMenu->addMenu("Audio");
    addCheckableActionToQMenuAndActionHash(audioDebugMenu, MenuOption::AudioNoiseReduction,
                                           0,
                                           true,
                                           audioIO.data(),
                                           SLOT(toggleAudioNoiseReduction()));

    addCheckableActionToQMenuAndActionHash(audioDebugMenu, MenuOption::EchoServerAudio, 0, false,
                                           audioIO.data(), SLOT(toggleServerEcho()));
    addCheckableActionToQMenuAndActionHash(audioDebugMenu, MenuOption::EchoLocalAudio, 0, false,
                                           audioIO.data(), SLOT(toggleLocalEcho()));
    addCheckableActionToQMenuAndActionHash(audioDebugMenu, MenuOption::StereoAudio, 0, false,
                                           audioIO.data(), SLOT(toggleStereoInput()));
    addCheckableActionToQMenuAndActionHash(audioDebugMenu, MenuOption::MuteAudio,
                                           Qt::CTRL | Qt::Key_M,
                                           false,
                                           audioIO.data(),
                                           SLOT(toggleMute()));
    addActionToQMenuAndActionHash(audioDebugMenu,
                                  MenuOption::MuteEnvironment,
                                  0,
                                  audioIO.data(),
                                  SLOT(sendMuteEnvironmentPacket()));

    addCheckableActionToQMenuAndActionHash(audioDebugMenu, MenuOption::AudioSourceInject,
                                           0,
                                           false,
                                           audioIO.data(),
                                           SLOT(toggleAudioSourceInject()));
    QMenu* audioSourceMenu = audioDebugMenu->addMenu("Generated Audio Source"); 
    {
        QAction *pinkNoise = addCheckableActionToQMenuAndActionHash(audioSourceMenu, MenuOption::AudioSourcePinkNoise,
                                                               0,
                                                               false,
                                                               audioIO.data(),
                                                               SLOT(selectAudioSourcePinkNoise()));
        
        QAction *sine440 = addCheckableActionToQMenuAndActionHash(audioSourceMenu, MenuOption::AudioSourceSine440,
                                                                    0,
                                                                    true,
                                                                    audioIO.data(),
                                                                    SLOT(selectAudioSourceSine440()));

        QActionGroup* audioSourceGroup = new QActionGroup(audioSourceMenu);
        audioSourceGroup->addAction(pinkNoise);
        audioSourceGroup->addAction(sine440);
    }
    
    auto scope = DependencyManager::get<AudioScope>();

    QMenu* audioScopeMenu = audioDebugMenu->addMenu("Audio Scope");
    addCheckableActionToQMenuAndActionHash(audioScopeMenu, MenuOption::AudioScope,
                                           Qt::CTRL | Qt::Key_P, false,
                                           scope.data(),
                                           SLOT(toggle()));
    addCheckableActionToQMenuAndActionHash(audioScopeMenu, MenuOption::AudioScopePause,
                                           Qt::CTRL | Qt::SHIFT | Qt::Key_P ,
                                           false,
                                           scope.data(),
                                           SLOT(togglePause()));
    addDisabledActionAndSeparator(audioScopeMenu, "Display Frames");
    {
        QAction *fiveFrames = addCheckableActionToQMenuAndActionHash(audioScopeMenu, MenuOption::AudioScopeFiveFrames,
                                               0,
                                               true,
                                               scope.data(),
                                               SLOT(selectAudioScopeFiveFrames()));

        QAction *twentyFrames = addCheckableActionToQMenuAndActionHash(audioScopeMenu, MenuOption::AudioScopeTwentyFrames,
                                               0,
                                               false,
                                               scope.data(),
                                               SLOT(selectAudioScopeTwentyFrames()));

        QAction *fiftyFrames = addCheckableActionToQMenuAndActionHash(audioScopeMenu, MenuOption::AudioScopeFiftyFrames,
                                               0,
                                               false,
                                               scope.data(),
                                               SLOT(selectAudioScopeFiftyFrames()));

        QActionGroup* audioScopeFramesGroup = new QActionGroup(audioScopeMenu);
        audioScopeFramesGroup->addAction(fiveFrames);
        audioScopeFramesGroup->addAction(twentyFrames);
        audioScopeFramesGroup->addAction(fiftyFrames);
    }
    
    auto statsRenderer = DependencyManager::get<AudioIOStatsRenderer>();
    addCheckableActionToQMenuAndActionHash(audioDebugMenu, MenuOption::AudioStats,
                                           Qt::CTRL | Qt::SHIFT | Qt::Key_A,
                                           false,
                                           statsRenderer.data(),
                                           SLOT(toggle()));

    addCheckableActionToQMenuAndActionHash(audioDebugMenu, MenuOption::AudioStatsShowInjectedStreams,
                                            0,
                                            false,
                                            statsRenderer.data(),
                                            SLOT(toggleShowInjectedStreams()));

    connect(audioIO.data(), SIGNAL(muteToggled()), this, SLOT(audioMuteToggled()));

#ifndef Q_OS_MAC
    QMenu* helpMenu = addMenu("Help");
    QAction* helpAction = helpMenu->addAction(MenuOption::AboutApp);
    connect(helpAction, SIGNAL(triggered()), this, SLOT(aboutApp()));
#endif
}

void Menu::loadSettings(QSettings* settings) {
    bool lockedSettings = false;
    if (!settings) {
        settings = Application::getInstance()->lockSettings();
        lockedSettings = true;
    }

    auto audio = DependencyManager::get<Audio>();
    audio->setOutputStarveDetectionEnabled(settings->value("audioOutputStarveDetectionEnabled", DEFAULT_AUDIO_OUTPUT_STARVE_DETECTION_ENABLED).toBool());
    audio->setOutputStarveDetectionThreshold(settings->value("audioOutputStarveDetectionThreshold", DEFAULT_AUDIO_OUTPUT_STARVE_DETECTION_THRESHOLD).toInt());
    audio->setOutputStarveDetectionPeriod(settings->value("audioOutputStarveDetectionPeriod", DEFAULT_AUDIO_OUTPUT_STARVE_DETECTION_PERIOD).toInt());
    int bufferSize = settings->value("audioOutputBufferSize", DEFAULT_AUDIO_OUTPUT_BUFFER_SIZE_FRAMES).toInt();
    QMetaObject::invokeMethod(audio.data(), "setOutputBufferSize", Q_ARG(int, bufferSize));
    
    _snapshotsLocation = settings->value("snapshotsLocation",
                                         QStandardPaths::writableLocation(QStandardPaths::DesktopLocation)).toString();
    setScriptsLocation(settings->value("scriptsLocation", QString()).toString());

#if defined(Q_OS_MAC) || defined(Q_OS_WIN)
    _speechRecognizer.setEnabled(settings->value("speechRecognitionEnabled", false).toBool());
#endif

    _walletPrivateKey = settings->value("privateKey").toByteArray();

    scanMenuBar(&loadAction, settings);
    Application::getInstance()->getAvatar()->loadData(settings);
    Application::getInstance()->updateWindowTitle();

    // notify that a settings has changed
    connect(&DependencyManager::get<NodeList>()->getDomainHandler(), &DomainHandler::hostnameChanged, this, &Menu::bumpSettings);

    // MyAvatar caches some menu options, so we have to update them whenever we load settings.
    // TODO: cache more settings in MyAvatar that are checked with very high frequency.
    setIsOptionChecked(MenuOption::KeyboardMotorControl , true);
    MyAvatar* myAvatar = Application::getInstance()->getAvatar();
    myAvatar->updateCollisionGroups();
    myAvatar->onToggleRagdoll();
    myAvatar->updateMotionBehavior();

    if (lockedSettings) {
        Application::getInstance()->unlockSettings();
    }
}

void Menu::saveSettings(QSettings* settings) {
    bool lockedSettings = false;
    if (!settings) {
        settings = Application::getInstance()->lockSettings();
        lockedSettings = true;
    }

    auto audio = DependencyManager::get<Audio>();
    settings->setValue("audioOutputStarveDetectionEnabled", audio->getOutputStarveDetectionEnabled());
    settings->setValue("audioOutputStarveDetectionThreshold", audio->getOutputStarveDetectionThreshold());
    settings->setValue("audioOutputStarveDetectionPeriod", audio->getOutputStarveDetectionPeriod());
    settings->setValue("audioOutputBufferSize", audio->getOutputBufferSize());

    settings->setValue("snapshotsLocation", _snapshotsLocation);
    settings->setValue("scriptsLocation", _scriptsLocation);
#if defined(Q_OS_MAC) || defined(Q_OS_WIN)
    settings->setValue("speechRecognitionEnabled", _speechRecognizer.getEnabled());
#endif
    settings->setValue("privateKey", _walletPrivateKey);

    scanMenuBar(&saveAction, settings);
    Application::getInstance()->getAvatar()->saveData(settings);
    
    DependencyManager::get<AddressManager>()->storeCurrentAddress();

    if (lockedSettings) {
        Application::getInstance()->unlockSettings();
    }
}

void Menu::importSettings() {
    QString locationDir(QStandardPaths::displayName(QStandardPaths::DesktopLocation));
    QString fileName = QFileDialog::getOpenFileName(Application::getInstance()->getWindow(),
                                                    tr("Open .ini config file"),
                                                    locationDir,
                                                    tr("Text files (*.ini)"));
    if (fileName != "") {
        QSettings tmp(fileName, QSettings::IniFormat);
        loadSettings(&tmp);
    }
}

void Menu::exportSettings() {
    QString locationDir(QStandardPaths::displayName(QStandardPaths::DesktopLocation));
    QString fileName = QFileDialog::getSaveFileName(Application::getInstance()->getWindow(),
                                                    tr("Save .ini config file"),
                                                    locationDir,
                                                    tr("Text files (*.ini)"));
    if (fileName != "") {
        QSettings tmp(fileName, QSettings::IniFormat);
        saveSettings(&tmp);
        tmp.sync();
    }
}

void Menu::loadAction(QSettings* set, QAction* action) {
    if (action->isChecked() != set->value(action->text(), action->isChecked()).toBool()) {
        action->trigger();
    }
}

void Menu::saveAction(QSettings* set, QAction* action) {
    set->setValue(action->text(),  action->isChecked());
}

void Menu::scanMenuBar(settingsAction modifySetting, QSettings* set) {
    QList<QMenu*> menus = this->findChildren<QMenu *>();

    for (QList<QMenu *>::const_iterator it = menus.begin(); menus.end() != it; ++it) {
        scanMenu(*it, modifySetting, set);
    }
}

void Menu::scanMenu(QMenu* menu, settingsAction modifySetting, QSettings* set) {
    QList<QAction*> actions = menu->actions();

    set->beginGroup(menu->title());
    for (QList<QAction *>::const_iterator it = actions.begin(); actions.end() != it; ++it) {
        if ((*it)->menu()) {
            scanMenu((*it)->menu(), modifySetting, set);
        }
        if ((*it)->isCheckable()) {
            modifySetting(set, *it);
        }
    }
    set->endGroup();
}

void Menu::addDisabledActionAndSeparator(QMenu* destinationMenu, const QString& actionName, int menuItemLocation) {
    QAction* actionBefore = NULL;
    if (menuItemLocation >= 0 && destinationMenu->actions().size() > menuItemLocation) {
        actionBefore = destinationMenu->actions()[menuItemLocation];
    }
    if (actionBefore) {
        QAction* separator = new QAction("",destinationMenu);
        destinationMenu->insertAction(actionBefore, separator);
        separator->setSeparator(true);

        QAction* separatorText = new QAction(actionName,destinationMenu);
        separatorText->setEnabled(false);
        destinationMenu->insertAction(actionBefore, separatorText);

    } else {
        destinationMenu->addSeparator();
        (destinationMenu->addAction(actionName))->setEnabled(false);
    }
}

QAction* Menu::addActionToQMenuAndActionHash(QMenu* destinationMenu,
                                             const QString& actionName,
                                             const QKeySequence& shortcut,
                                             const QObject* receiver,
                                             const char* member,
                                             QAction::MenuRole role,
                                             int menuItemLocation) {
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

    return action;
}

QAction* Menu::addActionToQMenuAndActionHash(QMenu* destinationMenu,
                                             QAction* action,
                                             const QString& actionName,
                                             const QKeySequence& shortcut,
                                             QAction::MenuRole role,
                                             int menuItemLocation) {
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

    return action;
}

QAction* Menu::addCheckableActionToQMenuAndActionHash(QMenu* destinationMenu,
                                                      const QString& actionName,
                                                      const QKeySequence& shortcut,
                                                      const bool checked,
                                                      const QObject* receiver,
                                                      const char* member,
                                                      int menuItemLocation) {

    QAction* action = addActionToQMenuAndActionHash(destinationMenu, actionName, shortcut, receiver, member,
                                                        QAction::NoRole, menuItemLocation);
    action->setCheckable(true);
    action->setChecked(checked);
    connect(action, SIGNAL(changed()), this, SLOT(bumpSettings()));

    return action;
}

void Menu::removeAction(QMenu* menu, const QString& actionName) {
    menu->removeAction(_actionHash.value(actionName));
    _actionHash.remove(actionName);
}

void Menu::setIsOptionChecked(const QString& menuOption, bool isChecked) {
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
        qDebug() << "NULL Action for menuOption '" << menuOption << "'";
    }
}

QAction* Menu::getActionForOption(const QString& menuOption) {
    return _actionHash.value(menuOption);
}

QAction* Menu::getActionFromName(const QString& menuName, QMenu* menu) {
    QList<QAction*> menuActions;
    if (menu) {
        menuActions = menu->actions();
    } else {
        menuActions = actions();
    }
    
    foreach (QAction* menuAction, menuActions) {
        if (menuName == menuAction->text()) {
            return menuAction;
        }
    }
    return NULL;
}

QMenu* Menu::getSubMenuFromName(const QString& menuName, QMenu* menu) {
    QAction* action = getActionFromName(menuName, menu);
    if (action) {
        return action->menu();
    }
    return NULL;
}

QMenu* Menu::getMenuParent(const QString& menuName, QString& finalMenuPart) {
    QStringList menuTree = menuName.split(">");
    QMenu* parent = NULL;
    QMenu* menu = NULL;
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

QMenu* Menu::getMenu(const QString& menuName) {
    QStringList menuTree = menuName.split(">");
    QMenu* parent = NULL;
    QMenu* menu = NULL;
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
    QMenu* parent = NULL;
    QAction* action = NULL;
    foreach (QString menuTreePart, menuTree) {
        action = getActionFromName(menuTreePart.trimmed(), parent);
        if (!action) {
            break;
        }
        parent = action->menu();
    }
    return action;
}

int Menu::findPositionOfMenuItem(QMenu* menu, const QString& searchMenuItem) {
    int position = 0;
    foreach(QAction* action, menu->actions()) {
        if (action->text() == searchMenuItem) {
            return position;
        }
        position++;
    }
    return UNSPECIFIED_POSITION; // not found
}

int Menu::positionBeforeSeparatorIfNeeded(QMenu* menu, int requestedPosition) {
    QList<QAction*> menuActions = menu->actions();
    if (requestedPosition > 1 && requestedPosition < menuActions.size()) {
        QAction* beforeRequested = menuActions[requestedPosition - 1];
        if (beforeRequested->isSeparator()) {
            requestedPosition--;
        }
    }
    return requestedPosition;
}


QMenu* Menu::addMenu(const QString& menuName) {
    QStringList menuTree = menuName.split(">");
    QMenu* addTo = NULL;
    QMenu* menu = NULL;
    foreach (QString menuTreePart, menuTree) {
        menu = getSubMenuFromName(menuTreePart.trimmed(), addTo);
        if (!menu) {
            if (!addTo) {
                menu = QMenuBar::addMenu(menuTreePart.trimmed());
            } else {
                menu = addTo->addMenu(menuTreePart.trimmed());
            }
        }
        addTo = menu;
    }
    
    QMenuBar::repaint();
    return menu;
}

void Menu::removeMenu(const QString& menuName) {
    QAction* action = getMenuAction(menuName);
    
    // only proceed if the menu actually exists
    if (action) {
        QString finalMenuPart;
        QMenu* parent = getMenuParent(menuName, finalMenuPart);
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

void Menu::addSeparator(const QString& menuName, const QString& separatorName) {
    QMenu* menuObj = getMenu(menuName);
    if (menuObj) {
        addDisabledActionAndSeparator(menuObj, separatorName);
    }
}

void Menu::removeSeparator(const QString& menuName, const QString& separatorName) {
    QMenu* menu = getMenu(menuName);
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
    QMenu* menuObj = getMenu(properties.menuName);
    if (menuObj) {
        QShortcut* shortcut = NULL;
        if (!properties.shortcutKeySequence.isEmpty()) {
            shortcut = new QShortcut(properties.shortcutKeySequence, this);
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
            addDisabledActionAndSeparator(menuObj, properties.menuItemName, requestedPosition);
        } else if (properties.isCheckable) {
            menuItemAction = addCheckableActionToQMenuAndActionHash(menuObj, properties.menuItemName,
                                                                    properties.shortcutKeySequence, properties.isChecked,
                                                                    MenuScriptingInterface::getInstance(), SLOT(menuItemTriggered()), requestedPosition);
        } else {
            menuItemAction = addActionToQMenuAndActionHash(menuObj, properties.menuItemName, properties.shortcutKeySequence,
                                                           MenuScriptingInterface::getInstance(), SLOT(menuItemTriggered()),
                                                           QAction::NoRole, requestedPosition);
        }
        if (shortcut && menuItemAction) {
            connect(shortcut, SIGNAL(activated()), menuItemAction, SLOT(trigger()));
        }
        QMenuBar::repaint();
    }
}

void Menu::removeMenuItem(const QString& menu, const QString& menuitem) {
    QMenu* menuObj = getMenu(menu);
    if (menuObj) {
        removeAction(menuObj, menuitem);
        QMenuBar::repaint();
    }
};

bool Menu::menuItemExists(const QString& menu, const QString& menuitem) {
    QAction* menuItemAction = _actionHash.value(menuitem);
    if (menuItemAction) {
        return (getMenu(menu) != NULL);
    }
    return false;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////// TODO: Move to appropriate files ////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Menu::aboutApp() {
    InfoView::forcedShow(INFO_HELP_PATH);
}

void Menu::showEditEntitiesHelp() {
    InfoView::forcedShow(INFO_EDIT_ENTITIES_PATH);
}

bool Menu::getShadowsEnabled() const {
    return isOptionChecked(MenuOption::SimpleShadows) || isOptionChecked(MenuOption::CascadedShadows);
}

void Menu::bumpSettings() {
    Application::getInstance()->bumpSettings();
}

void sendFakeEnterEvent() {
    QPoint lastCursorPosition = QCursor::pos();
    auto glCanvas = DependencyManager::get<GLCanvas>();

    QPoint windowPosition = glCanvas->mapFromGlobal(lastCursorPosition);
    QEnterEvent enterEvent = QEnterEvent(windowPosition, windowPosition, lastCursorPosition);
    QCoreApplication::sendEvent(glCanvas.data(), &enterEvent);
}

const float DIALOG_RATIO_OF_WINDOW = 0.30f;

void Menu::toggleSixense(bool shouldEnable) {
    SixenseManager& sixenseManager = SixenseManager::getInstance();
    
    if (shouldEnable && !sixenseManager.isInitialized()) {
        sixenseManager.initialize();
        sixenseManager.setFilter(isOptionChecked(MenuOption::FilterSixense));
        sixenseManager.setLowVelocityFilter(isOptionChecked(MenuOption::LowVelocityFilter));
    }
    
    sixenseManager.setIsEnabled(shouldEnable);
}

void Menu::changePrivateKey() {
    // setup the dialog
    QInputDialog privateKeyDialog(Application::getInstance()->getWindow());
    privateKeyDialog.setWindowTitle("Change Private Key");
    privateKeyDialog.setLabelText("RSA 2048-bit Private Key:");
    privateKeyDialog.setWindowFlags(Qt::Sheet);
    privateKeyDialog.setTextValue(QString(_walletPrivateKey));
    privateKeyDialog.resize(privateKeyDialog.parentWidget()->size().width() * DIALOG_RATIO_OF_WINDOW,
                            privateKeyDialog.size().height());
    
    int dialogReturn = privateKeyDialog.exec();
    if (dialogReturn == QDialog::Accepted) {
        // pull the private key from the dialog
        _walletPrivateKey = privateKeyDialog.textValue().toUtf8();
    }

    bumpSettings();
    
    sendFakeEnterEvent();
}

void Menu::copyAddress() {
    auto addressManager = DependencyManager::get<AddressManager>();
    QString address = addressManager->currentAddress().toString();
    QClipboard* clipboard = QApplication::clipboard();
    clipboard->setText(address);
}

void Menu::copyPath() {
    auto addressManager = DependencyManager::get<AddressManager>();
    QString path = addressManager->currentPath();
    QClipboard* clipboard = QApplication::clipboard();
    clipboard->setText(path);
}

void Menu::changeVSync() {
    Application::getInstance()->setVSyncEnabled(isOptionChecked(MenuOption::RenderTargetFramerateVSyncOn));
}

void Menu::displayNameLocationResponse(const QString& errorString) {

    if (!errorString.isEmpty()) {
        QMessageBox msgBox;
        msgBox.setText(errorString);
        msgBox.exec();
    }    
}

void Menu::loadRSSDKFile() {
    RealSense::getInstance()->loadRSSDKFile();
}

void Menu::audioMuteToggled() {
    QAction *muteAction = _actionHash.value(MenuOption::MuteAudio);
    if (muteAction) {
        muteAction->setChecked(DependencyManager::get<Audio>()->isMuted());
    }
}

void Menu::runTests() {
    runTimingTests();
}

QString Menu::getSnapshotsLocation() const {
    if (_snapshotsLocation.isNull() || _snapshotsLocation.isEmpty() || QDir(_snapshotsLocation).exists() == false) {
        return QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    }
    return _snapshotsLocation;
}

void Menu::setScriptsLocation(const QString& scriptsLocation) {
    _scriptsLocation = scriptsLocation;
    bumpSettings();
    emit scriptLocationChanged(scriptsLocation);
}

