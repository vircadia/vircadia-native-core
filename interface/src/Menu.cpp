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
#include <QQmlContext>

#include <AddressManager.h>
#include <AudioClient.h>
#include <DependencyManager.h>
#include <GlowEffect.h>
#include <PathUtils.h>
#include <SettingHandle.h>
#include <UserActivityLogger.h>
#include <OffscreenUi.h>
#include "Application.h"
#include "AccountManager.h"
#include "audio/AudioIOStatsRenderer.h"
#include "audio/AudioScope.h"
#include "avatar/AvatarManager.h"
#include "devices/DdeFaceTracker.h"
#include "devices/Faceshift.h"
#include "devices/RealSense.h"
#include "devices/SixenseManager.h"
#include "MainWindow.h"
#include "scripting/MenuScriptingInterface.h"
#if defined(Q_OS_MAC) || defined(Q_OS_WIN)
#include "SpeechRecognizer.h"
#endif
#include "ui/DialogsManager.h"
#include "ui/NodeBounds.h"
#include "ui/StandAloneJSConsole.h"
#include "InterfaceLogging.h"

#include "Menu.h"

// Proxy object to simplify porting over
HifiAction::HifiAction(const QString & menuOption) : _menuOption(menuOption) {
}

//void HifiAction::setCheckable(bool) {
//    Menu::getInstance()->set
//    qFatal("Not implemented");
//}

void HifiAction::setChecked(bool checked) {
    Menu::getInstance()->setChecked(_menuOption, checked);
}

void HifiAction::setVisible(bool visible) {
    QObject* result = Menu::getInstance()->findMenuObject(_menuOption);
    if (result) {
        result->setProperty("visible", visible);
    }
}

const QString & HifiAction::shortcut() const {
    static const QString NONE;
    QObject* result = Menu::getInstance()->findMenuObject(_menuOption);
    if (!result) {
        return NONE;
    }
    QObject* shortcut = result->property("shortcut").value<QObject*>();
    if (!shortcut) {
        return NONE;
    }
    shortcut->dumpObjectInfo();
    return NONE;
}

void HifiAction::setText(const QString & text) {
    Menu::getInstance()->setText(_menuOption, text);
}

void HifiAction::setTriggerAction(std::function<void()> f) {
    Menu::getInstance()->setTriggerAction(_menuOption, f);
}

void HifiAction::setToggleAction(std::function<void(bool)> f) {
    Menu::getInstance()->setToggleAction(_menuOption, f);
}

Menu* Menu::_instance = nullptr;

Menu* Menu::getInstance() {
    // Optimistic check for menu existence
    if (!_instance) {
        static QMutex menuInstanceMutex;
        withLock(menuInstanceMutex, [] {
            if (!_instance) {
                qmlRegisterType<Menu>("Hifi", 1, 0, NAME.toLocal8Bit().constData());
                qCDebug(interfaceapp, "First call to Menu::getInstance() - initing menu.");
                Menu::load();
                auto uiRoot = DependencyManager::get<OffscreenUi>()->getRootItem();
                _instance = uiRoot->findChild<Menu*>(NAME);
                if (!_instance) {
                    qFatal("Could not load menu QML");
                } else {
                    _instance->init();
                }
            }
        });
    }
    return _instance;
}

Menu::Menu(QQuickItem * parent) : HifiMenu(parent) {
}


void Menu::init() {
    auto dialogsManager = DependencyManager::get<DialogsManager>();
    AccountManager& accountManager = AccountManager::getInstance();
    static const QString ROOT_MENU;
    {
        static const QString FILE_MENU{ "File" };
        addMenu(ROOT_MENU, FILE_MENU);
        {
            addMenuItem(FILE_MENU, MenuOption::Login);
            // connect to the appropriate signal of the AccountManager so that we can change the Login/Logout menu item
            connect(&accountManager, &AccountManager::profileChanged,
                dialogsManager.data(), &DialogsManager::toggleLoginDialog);
            connect(&accountManager, &AccountManager::logoutComplete,
                dialogsManager.data(), &DialogsManager::toggleLoginDialog);
        }

#ifdef Q_OS_MAC
        addActionToQMenuAndActionHash(fileMenu, MenuOption::AboutApp, 0, qApp, SLOT(aboutApp()), QAction::AboutRole);
#endif

        {
            static const QString SCRIPTS_MENU{ "Scripts" };
            addMenu(FILE_MENU, SCRIPTS_MENU);
            //Qt::CTRL | Qt::Key_O
            addMenuItem(SCRIPTS_MENU, MenuOption::LoadScript, [=] {
                qApp->loadDialog();
            });
            //Qt::CTRL | Qt::SHIFT | Qt::Key_O
            addMenuItem(SCRIPTS_MENU, MenuOption::LoadScriptURL, [=] {
                qApp->loadScriptURLDialog();
            });
            addMenuItem(SCRIPTS_MENU, MenuOption::StopAllScripts, [=] {
                qApp->stopAllScripts();
            });
            //Qt::CTRL | Qt::Key_R,
            addMenuItem(SCRIPTS_MENU, MenuOption::ReloadAllScripts, [=] {
                qApp->reloadAllScripts();
            });
            // Qt::CTRL | Qt::Key_J,
            addMenuItem(SCRIPTS_MENU, MenuOption::RunningScripts, [=] {
                qApp->toggleRunningScriptsWidget();
            });
        }

        {
            static const QString LOCATION_MENU{ "Location" };
            addMenu(FILE_MENU, LOCATION_MENU);
            qApp->getBookmarks()->setupMenus(LOCATION_MENU);
            //Qt::CTRL | Qt::Key_L
            addMenuItem(LOCATION_MENU, MenuOption::AddressBar, [=] {
                auto dialogsManager = DependencyManager::get<DialogsManager>();
                dialogsManager->toggleAddressBar();
            });
            addMenuItem(LOCATION_MENU, MenuOption::CopyAddress, [=] {
                auto addressManager = DependencyManager::get<AddressManager>();
                addressManager->copyAddress();
            });
            addMenuItem(LOCATION_MENU, MenuOption::CopyPath, [=] {
                auto addressManager = DependencyManager::get<AddressManager>();
                addressManager->copyPath();
            });
        }

        // Qt::CTRL | Qt::Key_Q
        // QAction::QuitRole
        addMenuItem(FILE_MENU, MenuOption::Quit, [=] {
            qApp->quit();
        });
    }


    {
        static const QString EDIT_MENU{ "Edit" };
        addMenu(ROOT_MENU, EDIT_MENU);
#if 0
        QUndoStack* undoStack = qApp->getUndoStack();
        QAction* undoAction = undoStack->createUndoAction(editMenu);
        undoAction->setShortcut(Qt::CTRL | Qt::Key_Z);
        addActionToQMenuAndActionHash(editMenu, undoAction);

        QAction* redoAction = undoStack->createRedoAction(editMenu);
        redoAction->setShortcut(Qt::CTRL | Qt::SHIFT | Qt::Key_Z);
        addActionToQMenuAndActionHash(editMenu, redoAction);
#endif

        // Qt::CTRL | Qt::Key_Comma
        // QAction::PreferencesRole
        addMenuItem(EDIT_MENU, MenuOption::Preferences, [=] {
            dialogsManager->editPreferences();
        });
        addMenuItem(EDIT_MENU, MenuOption::Animations, [=] {
            dialogsManager->editAnimations();
        });
    }

    {
        static const QString TOOLS_MENU{ "Tools" };
        addMenu(ROOT_MENU, TOOLS_MENU);

#if defined(Q_OS_MAC) || defined(Q_OS_WIN)
        auto speechRecognizer = DependencyManager::get<SpeechRecognizer>();
        //Qt::CTRL | Qt::SHIFT | Qt::Key_C
        addMenuItem(TOOLS_MENU, MenuOption::ControlWithSpeech);
        setChecked(MenuOption::ControlWithSpeech, speechRecognizer->getEnabled());
        connect(speechRecognizer.data(), &SpeechRecognizer::enabledUpdated, [=] {
            setChecked(MenuOption::ControlWithSpeech, speechRecognizer->getEnabled());
        });
#endif
        // Qt::ALT | Qt::Key_S,
        addMenuItem(TOOLS_MENU, MenuOption::ScriptEditor, [=] {
            dialogsManager->showScriptEditor();
        });
        // QML Qt::Key_Backslash,
        addMenuItem(TOOLS_MENU, MenuOption::Chat, [=] {
            dialogsManager->showIRCLink();
        });
        addMenuItem(TOOLS_MENU, MenuOption::AddRemoveFriends, [=] {
            qApp->showFriendsWindow();
        });

#if 0
        QMenu* visibilityMenu = toolsMenu->addMenu("I Am Visible To");
        {
            QActionGroup* visibilityGroup = new QActionGroup(toolsMenu);
            auto discoverabilityManager = DependencyManager::get<DiscoverabilityManager>();

            QAction* visibleToEveryone = addCheckableActionToQMenuAndActionHash(visibilityMenu, MenuOption::VisibleToEveryone,
                0, discoverabilityManager->getDiscoverabilityMode() == Discoverability::All,
                discoverabilityManager.data(), SLOT(setVisibility()));
            visibilityGroup->addAction(visibleToEveryone);

            QAction* visibleToFriends = addCheckableActionToQMenuAndActionHash(visibilityMenu, MenuOption::VisibleToFriends,
                0, discoverabilityManager->getDiscoverabilityMode() == Discoverability::Friends,
                discoverabilityManager.data(), SLOT(setVisibility()));
            visibilityGroup->addAction(visibleToFriends);

            QAction* visibleToNoOne = addCheckableActionToQMenuAndActionHash(visibilityMenu, MenuOption::VisibleToNoOne,
                0, discoverabilityManager->getDiscoverabilityMode() == Discoverability::None,
                discoverabilityManager.data(), SLOT(setVisibility()));
            visibilityGroup->addAction(visibleToNoOne);

            connect(discoverabilityManager.data(), &DiscoverabilityManager::discoverabilityModeChanged,
                discoverabilityManager.data(), &DiscoverabilityManager::visibilityChanged);
        }
#endif
        //Qt::CTRL | Qt::ALT | Qt::Key_T,
        addMenuItem(TOOLS_MENU, MenuOption::ToolWindow, [=] {
//            dialogsManager->toggleToolWindow();
        });

        //Qt::CTRL | Qt::ALT | Qt::Key_J,
        addMenuItem(TOOLS_MENU, MenuOption::Console, [=] {
            DependencyManager::get<StandAloneJSConsole>()->toggleConsole();
        });

        // QML Qt::Key_Apostrophe,
        addMenuItem(TOOLS_MENU, MenuOption::ResetSensors, [=] {
            qApp->resetSensors();
        });

        addMenuItem(TOOLS_MENU, MenuOption::PackageModel, [=] {
            qApp->packageModel();
        });
    }

    {
        static const QString AVATAR_MENU{ "Avatar" };
        addMenu(ROOT_MENU, AVATAR_MENU);
        auto avatar = DependencyManager::get<AvatarManager>()->getMyAvatar();
        {
            static const QString SIZE_MENU{ "Size" };
            addMenu(AVATAR_MENU, SIZE_MENU);
            // QML Qt::Key_Plus,
            addMenuItem(SIZE_MENU, MenuOption::IncreaseAvatarSize, [=] {
                avatar->increaseSize();
            });
            // QML Qt::Key_Minus,
            addMenuItem(SIZE_MENU, MenuOption::DecreaseAvatarSize, [=] {
                avatar->decreaseSize();
            });
            // QML Qt::Key_Equal,
            addMenuItem(SIZE_MENU, MenuOption::ResetAvatarSize, [=] {
                avatar->resetSize();
            });

            addMenuItem(SIZE_MENU, MenuOption::ResetAvatarSize, [=] {
                avatar->resetSize();
            });
        }

        //Qt::CTRL | Qt::SHIFT | Qt::Key_K
        addCheckableMenuItem(AVATAR_MENU, MenuOption::KeyboardMotorControl, true, [=](bool) {
            avatar->updateMotionBehavior();
        });
        addCheckableMenuItem(AVATAR_MENU, MenuOption::ScriptedMotorControl, true);
        addCheckableMenuItem(AVATAR_MENU, MenuOption::NamesAboveHeads, true);
        addCheckableMenuItem(AVATAR_MENU, MenuOption::GlowWhenSpeaking, true);
        addCheckableMenuItem(AVATAR_MENU, MenuOption::BlueSpeechSphere, true);
        addCheckableMenuItem(AVATAR_MENU, MenuOption::EnableCharacterController, true, [=](bool) {
            avatar->updateMotionBehavior();
        });
        addCheckableMenuItem(AVATAR_MENU, MenuOption::ShiftHipsForIdleAnimations, false, [=](bool) {
            avatar->updateMotionBehavior();
        });
    }

    {
        static const QString VIEW_MENU{ "View" };
        addMenu(ROOT_MENU, VIEW_MENU);

        // Mac Qt::CTRL | Qt::META | Qt::Key_F,
        // Win32/Linux Qt::CTRL | Qt::Key_F,
        addCheckableMenuItem(VIEW_MENU, MenuOption::Fullscreen, false, [=](bool checked) {
//            qApp->setFullscreen(checked);
        });
        // QML Qt::Key_P, 
        addCheckableMenuItem(VIEW_MENU, MenuOption::FirstPerson, true, [=](bool checked) {
//            qApp->cameraMenuChanged();
        });
        //QML Qt::SHIFT | Qt::Key_H, 
        addCheckableMenuItem(VIEW_MENU, MenuOption::Mirror, true);

        // QML Qt::Key_H, 
        addCheckableMenuItem(VIEW_MENU, MenuOption::FullscreenMirror, true, [=](bool checked) {
//            qApp->cameraMenuChanged();
        });

        // Mac Qt::META | Qt::Key_H,
        // Win32/Linux Qt::CTRL | Qt::Key_H,
        addCheckableMenuItem(VIEW_MENU, MenuOption::HMDTools, false, [=](bool checked) {
            dialogsManager->hmdTools(checked);
        });
        addCheckableMenuItem(VIEW_MENU, MenuOption::EnableVRMode, false, [=](bool checked) {
//            qApp->setEnableVRMode(checked);
        });
        addCheckableMenuItem(VIEW_MENU, MenuOption::Enable3DTVMode, false, [=](bool checked) {
//            qApp->setEnable3DTVMode(checked);
        });

        {
            static const QString BORDER_MENU{ "View" };
            addMenu(VIEW_MENU, BORDER_MENU);
            // Qt::CTRL | Qt::SHIFT | Qt::Key_1
            addCheckableMenuItem(BORDER_MENU, MenuOption::ShowBordersEntityNodes, false, [=](bool checked) {
                qApp->getNodeBoundsDisplay().setShowEntityNodes(checked);
            });
        }
        addCheckableMenuItem(VIEW_MENU, MenuOption::OffAxisProjection, false);
        addCheckableMenuItem(VIEW_MENU, MenuOption::TurnWithHead, false);
        // QML Qt::Key_Slash
        addCheckableMenuItem(VIEW_MENU, MenuOption::Stats, false);

        // Qt::CTRL | Qt::SHIFT | Qt::Key_L
        addMenuItem(VIEW_MENU, MenuOption::Log, [=] {
            qApp->toggleLogDialog();
        });
        addMenuItem(VIEW_MENU, MenuOption::BandwidthDetails, [=] {
            dialogsManager->bandwidthDetails();
        });
        addMenuItem(VIEW_MENU, MenuOption::OctreeStats, [=] {
            dialogsManager->octreeStatsDetails();
        });
    }

    {
        static const QString DEV_MENU{ "Developer" };
        addMenu(ROOT_MENU, DEV_MENU);
        {
            static const QString RENDER_MENU{ "Render" };
            addMenu(DEV_MENU, RENDER_MENU);
            // QML Qt::SHIFT | Qt::Key_A, 
            addCheckableMenuItem(RENDER_MENU, MenuOption::Atmosphere, true);
            addCheckableMenuItem(RENDER_MENU, MenuOption::AmbientOcclusion);
            addCheckableMenuItem(RENDER_MENU, MenuOption::DontFadeOnOctreeServerChanges);
            {
                static const QString LIGHT_MENU{ MenuOption::RenderAmbientLight };
                addMenu(RENDER_MENU, LIGHT_MENU);
                static QStringList LIGHTS{
                    MenuOption::RenderAmbientLightGlobal,
                    MenuOption::RenderAmbientLight0,
                    MenuOption::RenderAmbientLight1,
                    MenuOption::RenderAmbientLight2,
                    MenuOption::RenderAmbientLight3,
                    MenuOption::RenderAmbientLight4,
                    MenuOption::RenderAmbientLight5,
                    MenuOption::RenderAmbientLight6,
                    MenuOption::RenderAmbientLight7,
                    MenuOption::RenderAmbientLight8,
                    MenuOption::RenderAmbientLight9,
                };
                foreach(QString option, LIGHTS) {
                    addCheckableMenuItem(LIGHT_MENU, option);
                    // FIXME
                    // setExclusiveGroup()
                }
                setChecked(MenuOption::RenderAmbientLightGlobal, true);
            }
            {
                static const QString SHADOWS_MENU{ "Shadows" };
                addMenu(RENDER_MENU, SHADOWS_MENU);
                addCheckableMenuItem(SHADOWS_MENU, "No Shadows", true);
                addCheckableMenuItem(SHADOWS_MENU, MenuOption::SimpleShadows);
                addCheckableMenuItem(SHADOWS_MENU, MenuOption::CascadedShadows);
            }
            {
                static const QString FRAMERATE_MENU{ MenuOption::RenderTargetFramerate };
                addMenu(RENDER_MENU, FRAMERATE_MENU);
                //framerateGroup->setExclusive(true);
                addCheckableMenuItem(FRAMERATE_MENU, MenuOption::RenderTargetFramerateUnlimited, true);
                addCheckableMenuItem(FRAMERATE_MENU, MenuOption::RenderTargetFramerate60);
                addCheckableMenuItem(FRAMERATE_MENU, MenuOption::RenderTargetFramerate50);
                addCheckableMenuItem(FRAMERATE_MENU, MenuOption::RenderTargetFramerate40);
                addCheckableMenuItem(FRAMERATE_MENU, MenuOption::RenderTargetFramerate30);
            }
#if !defined(Q_OS_MAC)
            addCheckableMenuItem(RENDER_MENU, MenuOption::RenderTargetFramerateVSyncOn, true, [](bool checked) {
                qApp->setVSyncEnabled();
            });
#endif
            {
                static const QString RES_MENU{ MenuOption::RenderResolution };
                addMenu(RENDER_MENU, RES_MENU);
                // resolutionGroup->setExclusive(true);
                addCheckableMenuItem(RES_MENU, MenuOption::RenderResolutionOne, true);
                addCheckableMenuItem(RES_MENU, MenuOption::RenderResolutionTwoThird);
                addCheckableMenuItem(RES_MENU, MenuOption::RenderResolutionHalf);
                addCheckableMenuItem(RES_MENU, MenuOption::RenderResolutionThird);
                addCheckableMenuItem(RES_MENU, MenuOption::RenderResolutionQuarter);
            }
            // QML Qt::Key_Asterisk,
            addCheckableMenuItem(RENDER_MENU, MenuOption::Stars, true);
            addCheckableMenuItem(RENDER_MENU, MenuOption::EnableGlowEffect, true, [](bool checked){
                DependencyManager::get<GlowEffect>()->toggleGlowEffect(checked);
            });
            //Qt::ALT | Qt::Key_W
            addCheckableMenuItem(RENDER_MENU, MenuOption::Wireframe);
            // QML Qt::SHIFT | Qt::Key_L,
            addMenuItem(RENDER_MENU, MenuOption::LodTools, [=] {
                dialogsManager->lodTools();
            });
        }

        {
            static const QString AVATAR_MENU{ "Avatar Dev" };
            addMenu(DEV_MENU, AVATAR_MENU);
            setText(AVATAR_MENU, "Avatar");
            {
            }
        }
    }

#if 0
    QMenu* faceTrackingMenu = avatarDebugMenu->addMenu("Face Tracking");
    {
        QActionGroup* faceTrackerGroup = new QActionGroup(avatarDebugMenu);

        QAction* noFaceTracker = addCheckableActionToQMenuAndActionHash(faceTrackingMenu, MenuOption::NoFaceTracking,
            0, true,
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
            0, false,
            qApp, SLOT(setActiveFaceTracker()));
        faceTrackerGroup->addAction(ddeFaceTracker);
#endif
    }
#ifdef HAVE_DDE
    faceTrackingMenu->addSeparator();
    QAction* useAudioForMouth = addCheckableActionToQMenuAndActionHash(faceTrackingMenu, MenuOption::UseAudioForMouth, 0, true);
    useAudioForMouth->setVisible(false);
    QAction* ddeFiltering = addCheckableActionToQMenuAndActionHash(faceTrackingMenu, MenuOption::VelocityFilter, 0, true);
    ddeFiltering->setVisible(false);
#endif

    addCheckableActionToQMenuAndActionHash(avatarDebugMenu, MenuOption::RenderSkeletonCollisionShapes);
    addCheckableActionToQMenuAndActionHash(avatarDebugMenu, MenuOption::RenderHeadCollisionShapes);
    addCheckableActionToQMenuAndActionHash(avatarDebugMenu, MenuOption::RenderBoundingCollisionShapes);
    addCheckableActionToQMenuAndActionHash(avatarDebugMenu, MenuOption::RenderLookAtVectors, 0, false);
    addCheckableActionToQMenuAndActionHash(avatarDebugMenu, MenuOption::RenderFocusIndicator, 0, false);

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
                                           &SixenseManager::getInstance(),
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
                                           qApp,
                                           SLOT(setLowVelocityFilter(bool)));
    addCheckableActionToQMenuAndActionHash(sixenseOptionsMenu, MenuOption::SixenseMouseInput, 0, true);
    addCheckableActionToQMenuAndActionHash(sixenseOptionsMenu, MenuOption::SixenseLasers, 0, false);

    QMenu* leapOptionsMenu = handOptionsMenu->addMenu("Leap Motion");
    addCheckableActionToQMenuAndActionHash(leapOptionsMenu, MenuOption::LeapMotionOnHMD, 0, false);

#ifdef HAVE_RSSDK
    QMenu* realSenseOptionsMenu = handOptionsMenu->addMenu("RealSense");
    addActionToQMenuAndActionHash(realSenseOptionsMenu, MenuOption::LoadRSSDKFile, 0,
                                  RealSense::getInstance(), SLOT(loadRSSDKFile()));
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
    addActionToQMenuAndActionHash(networkMenu, MenuOption::DiskCacheEditor, 0,
                                  dialogsManager.data(), SLOT(toggleDiskCacheEditor()));

    QMenu* timingMenu = developerMenu->addMenu("Timing and Stats");
    QMenu* perfTimerMenu = timingMenu->addMenu("Performance Timer");
    addCheckableActionToQMenuAndActionHash(perfTimerMenu, MenuOption::DisplayDebugTimingDetails, 0, false);
    addCheckableActionToQMenuAndActionHash(perfTimerMenu, MenuOption::OnlyDisplayTopTen, 0, true);
    addCheckableActionToQMenuAndActionHash(perfTimerMenu, MenuOption::ExpandUpdateTiming, 0, false);
    addCheckableActionToQMenuAndActionHash(perfTimerMenu, MenuOption::ExpandMyAvatarTiming, 0, false);
    addCheckableActionToQMenuAndActionHash(perfTimerMenu, MenuOption::ExpandMyAvatarSimulateTiming, 0, false);
    addCheckableActionToQMenuAndActionHash(perfTimerMenu, MenuOption::ExpandOtherAvatarTiming, 0, false);
    addCheckableActionToQMenuAndActionHash(perfTimerMenu, MenuOption::ExpandPaintGLTiming, 0, false);

    addCheckableActionToQMenuAndActionHash(timingMenu, MenuOption::TestPing, 0, true);
    addCheckableActionToQMenuAndActionHash(timingMenu, MenuOption::FrameTimer);
    addActionToQMenuAndActionHash(timingMenu, MenuOption::RunTimingTests, 0, qApp, SLOT(runTests()));
    addCheckableActionToQMenuAndActionHash(timingMenu, MenuOption::PipelineWarnings);
    addCheckableActionToQMenuAndActionHash(timingMenu, MenuOption::SuppressShortTimings);

    auto audioIO = DependencyManager::get<AudioClient>();
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

    QMenu* helpMenu = addMenu("Help");
    addActionToQMenuAndActionHash(helpMenu, MenuOption::EditEntitiesHelp, 0, qApp, SLOT(showEditEntitiesHelp()));

#ifndef Q_OS_MAC
    QAction* aboutAction = helpMenu->addAction(MenuOption::AboutApp);
    connect(aboutAction, SIGNAL(triggered()), qApp, SLOT(aboutApp()));
#endif
#endif
}

void Menu::loadSettings() {
//    scanMenuBar(&Menu::loadAction);
}

void Menu::saveSettings() {
//    scanMenuBar(&Menu::saveAction);
}
#if 0

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

#endif
