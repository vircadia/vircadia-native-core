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
#include "Util.h"

// Proxy object to simplify porting over
HifiAction::HifiAction(const QString& menuOption) : _menuOption(menuOption) {
}

void HifiAction::setCheckable(bool checkable) {
    Menu::getInstance()->setCheckable(_menuOption, checkable);
}

void HifiAction::setChecked(bool checked) {
    Menu::getInstance()->setChecked(_menuOption, checked);
}

void HifiAction::setVisible(bool visible) {
    return Menu::getInstance()->setItemVisible(_menuOption);
}

QString HifiAction::shortcut() const {
    return Menu::getInstance()->getItemShortcut(_menuOption);
}

void HifiAction::setText(const QString& text) {
    Menu::getInstance()->setItemText(_menuOption, text);
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

Menu::Menu(QQuickItem* parent) : HifiMenu(parent) {
}

void Menu::init() {
    auto dialogsManager = DependencyManager::get<DialogsManager>();
    AccountManager& accountManager = AccountManager::getInstance();
    static const QString ROOT_MENU;
    {
        static const QString FILE_MENU{ "File" };
        addMenu(ROOT_MENU, FILE_MENU);
        {
            addItem(FILE_MENU, MenuOption::Login);
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
#if 0
            static const QString SCRIPTS_MENU{ "Scripts" };
            addMenu(FILE_MENU, LOCATION_MENU);
#else 
            static const QString SCRIPTS_MENU{ FILE_MENU };
            addSeparator(FILE_MENU, "Scripts");
#endif
            //Qt::CTRL | Qt::Key_O
            addItem(SCRIPTS_MENU, MenuOption::LoadScript, qApp, SLOT(loadDialog()));
            //Qt::CTRL | Qt::SHIFT | Qt::Key_O
            addItem(SCRIPTS_MENU, MenuOption::LoadScriptURL, [=] {
                qApp->loadScriptURLDialog();
            });
            addItem(SCRIPTS_MENU, MenuOption::StopAllScripts, [=] {
                qApp->stopAllScripts();
            });
            //Qt::CTRL | Qt::Key_R,
            addItem(SCRIPTS_MENU, MenuOption::ReloadAllScripts, [=] {
                qApp->reloadAllScripts();
            });
            // Qt::CTRL | Qt::Key_J,
            addItem(SCRIPTS_MENU, MenuOption::RunningScripts, [=] {
                qApp->toggleRunningScriptsWidget();
            });
        }

        {
#if 0
            static const QString LOCATION_MENU{ "Location" };
            addMenu(FILE_MENU, LOCATION_MENU);
#else 
            addSeparator(FILE_MENU, "Location");
            static const QString LOCATION_MENU{ FILE_MENU };
#endif
            qApp->getBookmarks()->setupMenus(LOCATION_MENU);

            //Qt::CTRL | Qt::Key_L
            addItem(LOCATION_MENU, MenuOption::AddressBar, [=] {
                auto dialogsManager = DependencyManager::get<DialogsManager>();
                dialogsManager->toggleAddressBar();
            });
            addItem(LOCATION_MENU, MenuOption::CopyAddress, [=] {
                auto addressManager = DependencyManager::get<AddressManager>();
                addressManager->copyAddress();
            });
            addItem(LOCATION_MENU, MenuOption::CopyPath, [=] {
                auto addressManager = DependencyManager::get<AddressManager>();
                addressManager->copyPath();
            });
        }

        // Qt::CTRL | Qt::Key_Q
        // QAction::QuitRole
        addItem(FILE_MENU, MenuOption::Quit, [=] {
            qApp->quit();
        });
    }


    {
        static const QString EDIT_MENU{ "Edit" };
        addMenu(ROOT_MENU, EDIT_MENU);

        QUndoStack* undoStack = qApp->getUndoStack();
        QAction* undoAction = undoStack->createUndoAction(this);
        //undoAction->setShortcut(Qt::CTRL | Qt::Key_Z);
        addItem(EDIT_MENU, undoAction->text(), undoAction, SLOT(trigger()));

        QAction* redoAction = undoStack->createRedoAction(this);
        //redoAction->setShortcut(Qt::CTRL | Qt::SHIFT | Qt::Key_Z);
        addItem(EDIT_MENU, redoAction->text(), redoAction, SLOT(trigger()));

        // Qt::CTRL | Qt::Key_Comma
        // QAction::PreferencesRole
        addItem(EDIT_MENU, MenuOption::Preferences, [=] {
            dialogsManager->editPreferences();
        });
        addItem(EDIT_MENU, MenuOption::Animations, [=] {
            dialogsManager->editAnimations();
        });
    }

    {
        static const QString TOOLS_MENU{ "Tools" };
        addMenu(ROOT_MENU, TOOLS_MENU);

#if defined(Q_OS_MAC) || defined(Q_OS_WIN)
        auto speechRecognizer = DependencyManager::get<SpeechRecognizer>();
        //Qt::CTRL | Qt::SHIFT | Qt::Key_C
        addItem(TOOLS_MENU, MenuOption::ControlWithSpeech);
        setChecked(MenuOption::ControlWithSpeech, speechRecognizer->getEnabled());
        connect(speechRecognizer.data(), &SpeechRecognizer::enabledUpdated, [=] {
            setChecked(MenuOption::ControlWithSpeech, speechRecognizer->getEnabled());
        });
#endif
        // Qt::ALT | Qt::Key_S,
        addItem(TOOLS_MENU, MenuOption::ScriptEditor, [=] {
            dialogsManager->showScriptEditor();
        });
        // QML Qt::Key_Backslash,
        addItem(TOOLS_MENU, MenuOption::Chat, [=] {
            dialogsManager->showIRCLink();
        });
        addItem(TOOLS_MENU, MenuOption::AddRemoveFriends, [=] {
            qApp->showFriendsWindow();
        });

        {
            static const QString VIZ_MENU{ "I Am Visible To" };
            addMenu(TOOLS_MENU, VIZ_MENU);
            auto discoverabilityManager = DependencyManager::get<DiscoverabilityManager>();
            // FIXME group
            addCheckableItem(VIZ_MENU, MenuOption::VisibleToEveryone, 
                discoverabilityManager->getDiscoverabilityMode() == Discoverability::All, 
                discoverabilityManager.data(), SLOT(setVisibility()));
            addCheckableItem(VIZ_MENU, MenuOption::VisibleToFriends, 
                discoverabilityManager->getDiscoverabilityMode() == Discoverability::Friends, 
                discoverabilityManager.data(), SLOT(setVisibility()));
            addCheckableItem(VIZ_MENU, MenuOption::VisibleToNoOne,
                discoverabilityManager->getDiscoverabilityMode() == Discoverability::None, 
                discoverabilityManager.data(), SLOT(setVisibility()));
            connect(discoverabilityManager.data(), &DiscoverabilityManager::discoverabilityModeChanged,
                discoverabilityManager.data(), &DiscoverabilityManager::visibilityChanged);
        }

        //Qt::CTRL | Qt::ALT | Qt::Key_T,
        addItem(TOOLS_MENU, MenuOption::ToolWindow, 
            dialogsManager.data(), SLOT(toggleToolWindow()));

        //Qt::CTRL | Qt::ALT | Qt::Key_J,
        addItem(TOOLS_MENU, MenuOption::Console, [=] {
            DependencyManager::get<StandAloneJSConsole>()->toggleConsole();
        });

        // QML Qt::Key_Apostrophe,
        addItem(TOOLS_MENU, MenuOption::ResetSensors, [=] {
            qApp->resetSensors();
        });

        addItem(TOOLS_MENU, MenuOption::PackageModel, [=] {
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
            addItem(SIZE_MENU, MenuOption::IncreaseAvatarSize, [=] {
                avatar->increaseSize();
            });
            // QML Qt::Key_Minus,
            addItem(SIZE_MENU, MenuOption::DecreaseAvatarSize, [=] {
                avatar->decreaseSize();
            });
            // QML Qt::Key_Equal,
            addItem(SIZE_MENU, MenuOption::ResetAvatarSize, [=] {
                avatar->resetSize();
            });

            addItem(SIZE_MENU, MenuOption::ResetAvatarSize, [=] {
                avatar->resetSize();
            });
        }

        //Qt::CTRL | Qt::SHIFT | Qt::Key_K
        addCheckableItem(AVATAR_MENU, MenuOption::KeyboardMotorControl, true, [=](bool) {
            avatar->updateMotionBehavior();
        });
        addCheckableItem(AVATAR_MENU, MenuOption::ScriptedMotorControl, true);
        addCheckableItem(AVATAR_MENU, MenuOption::NamesAboveHeads, true);
        addCheckableItem(AVATAR_MENU, MenuOption::GlowWhenSpeaking, true);
        addCheckableItem(AVATAR_MENU, MenuOption::BlueSpeechSphere, true);
        addCheckableItem(AVATAR_MENU, MenuOption::EnableCharacterController, true,
            avatar, SLOT(updateMotionBehavior()));
        addCheckableItem(AVATAR_MENU, MenuOption::ShiftHipsForIdleAnimations, false, 
            avatar, SLOT(updateMotionBehavior()));
    }

    {
        static const QString VIEW_MENU{ "View" };
        addMenu(ROOT_MENU, VIEW_MENU);

        // Mac Qt::CTRL | Qt::META | Qt::Key_F,
        // Win32/Linux Qt::CTRL | Qt::Key_F,
        addCheckableItem(VIEW_MENU, MenuOption::Fullscreen, false);
        connectCheckable(MenuOption::Fullscreen, qApp, SLOT(setFullscreen(bool)));
        // QML Qt::Key_P, 
        addCheckableItem(VIEW_MENU, MenuOption::FirstPerson, true, 
            qApp, SLOT(cameraMenuChanged()));
        //QML Qt::SHIFT | Qt::Key_H, 
        addCheckableItem(VIEW_MENU, MenuOption::Mirror, true, 
            qApp, SLOT(cameraMenuChanged()));
        // QML Qt::Key_H, 
        addCheckableItem(VIEW_MENU, MenuOption::FullscreenMirror, false, 
            qApp, SLOT(cameraMenuChanged()));


        // Mac Qt::META | Qt::Key_H,
        // Win32/Linux Qt::CTRL | Qt::Key_H,
        addCheckableItem(VIEW_MENU, MenuOption::HMDTools, false, [=](bool checked) {
            dialogsManager->hmdTools(checked);
        });
        addCheckableItem(VIEW_MENU, MenuOption::EnableVRMode, false);
        connectCheckable(MenuOption::EnableVRMode, qApp, SLOT(setEnableVRMode(bool)));
        addCheckableItem(VIEW_MENU, MenuOption::Enable3DTVMode, false);
        connectCheckable(MenuOption::Enable3DTVMode, qApp, SLOT(setEnable3DTVMode(bool)));

        {
            static const QString BORDER_MENU{ "Server Borders" };
            addMenu(VIEW_MENU, BORDER_MENU);
            // Qt::CTRL | Qt::SHIFT | Qt::Key_1
            addCheckableItem(BORDER_MENU, MenuOption::ShowBordersEntityNodes, false, [=](bool checked) {
                qApp->getNodeBoundsDisplay().setShowEntityNodes(checked);
            });
        }
        addCheckableItem(VIEW_MENU, MenuOption::OffAxisProjection, false);
        addCheckableItem(VIEW_MENU, MenuOption::TurnWithHead, false);
        // QML Qt::Key_Slash
        addCheckableItem(VIEW_MENU, MenuOption::Stats, false);

        // Qt::CTRL | Qt::SHIFT | Qt::Key_L
        addItem(VIEW_MENU, MenuOption::Log, [=] {
            qApp->toggleLogDialog();
        });
        addItem(VIEW_MENU, MenuOption::BandwidthDetails, [=] {
            dialogsManager->bandwidthDetails();
        });
        addItem(VIEW_MENU, MenuOption::OctreeStats, [=] {
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
            addCheckableItem(RENDER_MENU, MenuOption::Atmosphere, true);
            addCheckableItem(RENDER_MENU, MenuOption::AmbientOcclusion);
            addCheckableItem(RENDER_MENU, MenuOption::DontFadeOnOctreeServerChanges);
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
                    addCheckableItem(LIGHT_MENU, option);
                    // FIXME
                    // setExclusiveGroup()
                }
                setChecked(MenuOption::RenderAmbientLightGlobal, true);
            }
            {
                static const QString SHADOWS_MENU{ "Shadows" };
                addMenu(RENDER_MENU, SHADOWS_MENU);
                addCheckableItem(SHADOWS_MENU, "No Shadows", true);
                addCheckableItem(SHADOWS_MENU, MenuOption::SimpleShadows);
                addCheckableItem(SHADOWS_MENU, MenuOption::CascadedShadows);
            }
            {
                static const QString FRAMERATE_MENU{ MenuOption::RenderTargetFramerate };
                addMenu(RENDER_MENU, FRAMERATE_MENU);
                //framerateGroup->setExclusive(true);
                addCheckableItem(FRAMERATE_MENU, MenuOption::RenderTargetFramerateUnlimited, true);
                addCheckableItem(FRAMERATE_MENU, MenuOption::RenderTargetFramerate60);
                addCheckableItem(FRAMERATE_MENU, MenuOption::RenderTargetFramerate50);
                addCheckableItem(FRAMERATE_MENU, MenuOption::RenderTargetFramerate40);
                addCheckableItem(FRAMERATE_MENU, MenuOption::RenderTargetFramerate30);
            }
#if !defined(Q_OS_MAC)
            addCheckableItem(RENDER_MENU, MenuOption::RenderTargetFramerateVSyncOn, true, [](bool checked) {
                qApp->setVSyncEnabled();
            });
#endif
            {
                static const QString RES_MENU{ MenuOption::RenderResolution };
                addMenu(RENDER_MENU, RES_MENU);
                // resolutionGroup->setExclusive(true);
                addCheckableItem(RES_MENU, MenuOption::RenderResolutionOne, true);
                addCheckableItem(RES_MENU, MenuOption::RenderResolutionTwoThird);
                addCheckableItem(RES_MENU, MenuOption::RenderResolutionHalf);
                addCheckableItem(RES_MENU, MenuOption::RenderResolutionThird);
                addCheckableItem(RES_MENU, MenuOption::RenderResolutionQuarter);
            }
            // QML Qt::Key_Asterisk,
            addCheckableItem(RENDER_MENU, MenuOption::Stars, true);
            addCheckableItem(RENDER_MENU, MenuOption::EnableGlowEffect, true, [](bool checked){
                DependencyManager::get<GlowEffect>()->toggleGlowEffect(checked);
            });
            //Qt::ALT | Qt::Key_W
            addCheckableItem(RENDER_MENU, MenuOption::Wireframe);
            // QML Qt::SHIFT | Qt::Key_L,
            addItem(RENDER_MENU, MenuOption::LodTools, [=] {
                dialogsManager->lodTools();
            });
        } // Developer -> Render

        {
            static const QString AVATAR_MENU{ "Avatar Dev" };
            addMenu(DEV_MENU, AVATAR_MENU);
            setItemText(AVATAR_MENU, "Avatar");
            {
                static const QString FACE_MENU{ "Face Tracking" };
                addMenu(AVATAR_MENU, FACE_MENU);
                // FIXME GROUP
                addCheckableItem(FACE_MENU, MenuOption::NoFaceTracking, true, [](bool checked) {
                    qApp->setActiveFaceTracker();
                });
#ifdef HAVE_FACESHIFT
                addCheckableItem(FACE_MENU, MenuOption::Faceshift, true, [](bool checked) {
                    qApp->setActiveFaceTracker();
                });
#endif
#ifdef HAVE_DDE
                addCheckableItem(FACE_MENU, MenuOption::UseCamera, true, [](bool checked) {
                    qApp->setActiveFaceTracker();
                });
#endif
                addCheckableItem(FACE_MENU, MenuOption::UseAudioForMouth, true);
                // FIXME integrate the visibility into the main API
                getActionForOption(MenuOption::UseAudioForMouth)->setVisible(false);

                addCheckableItem(FACE_MENU, MenuOption::VelocityFilter, true);
                // FIXME integrate the visibility into the main API
                getActionForOption(MenuOption::VelocityFilter)->setVisible(false);
            }
            addCheckableItem(AVATAR_MENU, MenuOption::RenderSkeletonCollisionShapes);
            addCheckableItem(AVATAR_MENU, MenuOption::RenderHeadCollisionShapes);
            addCheckableItem(AVATAR_MENU, MenuOption::RenderBoundingCollisionShapes);
            addCheckableItem(AVATAR_MENU, MenuOption::RenderLookAtVectors);
            addCheckableItem(AVATAR_MENU, MenuOption::RenderFocusIndicator);
            {
                static const QString HANDS_MENU{ "Hands" };
                addMenu(AVATAR_MENU, HANDS_MENU);
                addCheckableItem(HANDS_MENU, MenuOption::AlignForearmsWithWrists);
                addCheckableItem(HANDS_MENU, MenuOption::AlternateIK);
                addCheckableItem(HANDS_MENU, MenuOption::DisplayHands, true);
                addCheckableItem(HANDS_MENU, MenuOption::DisplayHandTargets);
                addCheckableItem(HANDS_MENU, MenuOption::ShowIKConstraints);
                {
                    static const QString SIXENSE_MENU{ "Sixense" };
                    addMenu(HANDS_MENU, SIXENSE_MENU);
#ifdef __APPLE__
                    addCheckableItem(SIXENSE_MENU, MenuOption::SixenseEnabled, false, [](bool checked) {
                        SixenseManager::getInstance().toggleSixense(checked);
                    });
#endif
                    addCheckableItem(SIXENSE_MENU, MenuOption::FilterSixense, true, [](bool checked) {
                        SixenseManager::getInstance().setFilter(checked);
                    });
                    addCheckableItem(SIXENSE_MENU, MenuOption::LowVelocityFilter, true, [](bool checked) {
                        qApp->setLowVelocityFilter(checked);
                    });
                    addCheckableItem(SIXENSE_MENU, MenuOption::SixenseMouseInput, true);
                    addCheckableItem(SIXENSE_MENU, MenuOption::SixenseLasers);
                }

                {
                    static const QString LEAP_MENU{ "Leap Motion" };
                    addMenu(HANDS_MENU, LEAP_MENU);
                    addCheckableItem(LEAP_MENU, MenuOption::LeapMotionOnHMD);
                    addCheckableItem(LEAP_MENU, MenuOption::LeapMotionOnHMD);
                }

#ifdef HAVE_RSSDK
                {
                    static const QString RSS_MENU{ "RealSense" };
                    addMenu(HANDS_MENU, RSS_MENU);
                    addItem(RSS_MENU, MenuOption::LoadRSSDKFile, [] {
                        RealSense::getInstance()->loadRSSDKFile();
                    });
                    addCheckableItem(RSS_MENU, MenuOption::LeapMotionOnHMD);
                }
#endif
            } // Developer -> Hands 

            {
                static const QString NETWORK_MENU{ "Network" };
                addMenu(DEV_MENU, NETWORK_MENU);
                addCheckableItem(NETWORK_MENU, MenuOption::DisableNackPackets);
                addCheckableItem(NETWORK_MENU, MenuOption::DisableActivityLogger, false, [](bool checked) {
                    UserActivityLogger::getInstance().disable(checked);
                });
                addItem(NETWORK_MENU, MenuOption::CachesSize, [=] {
                    dialogsManager->cachesSizeDialog();
                });
                addItem(NETWORK_MENU, MenuOption::DiskCacheEditor, [=] {
                    dialogsManager->toggleDiskCacheEditor();
                });
            } // Developer -> Network

            {
                static const QString TIMING_MENU{ "Timing and Stats" };
                addMenu(DEV_MENU, TIMING_MENU);
                {
                    static const QString PERF_MENU{ "Performance Timer" };
                    addMenu(TIMING_MENU, PERF_MENU);
                    addCheckableItem(PERF_MENU, MenuOption::DisplayDebugTimingDetails);
                    addCheckableItem(PERF_MENU, MenuOption::OnlyDisplayTopTen, true);
                    addCheckableItem(PERF_MENU, MenuOption::ExpandUpdateTiming);
                    addCheckableItem(PERF_MENU, MenuOption::ExpandMyAvatarTiming);
                    addCheckableItem(PERF_MENU, MenuOption::ExpandMyAvatarSimulateTiming);
                    addCheckableItem(PERF_MENU, MenuOption::ExpandOtherAvatarTiming);
                    addCheckableItem(PERF_MENU, MenuOption::ExpandPaintGLTiming);
                }
                addCheckableItem(TIMING_MENU, MenuOption::TestPing, true);
                addCheckableItem(TIMING_MENU, MenuOption::FrameTimer);
                addItem(TIMING_MENU, MenuOption::RunTimingTests, [] { runTimingTests(); });
                addCheckableItem(TIMING_MENU, MenuOption::PipelineWarnings);
                addCheckableItem(TIMING_MENU, MenuOption::SuppressShortTimings);
            } // Developer -> Timing and Stats

            {
                static const QString AUDIO_MENU{ "Audio" };
                addMenu(DEV_MENU, AUDIO_MENU);
                auto audioIO = DependencyManager::get<AudioClient>();
                addCheckableItem(AUDIO_MENU, MenuOption::AudioNoiseReduction, true, 
                    audioIO.data(), SLOT(toggleAudioNoiseReduction()));
                addCheckableItem(AUDIO_MENU, MenuOption::EchoServerAudio, false,
                    audioIO.data(), SLOT(toggleServerEcho()));
                addCheckableItem(AUDIO_MENU, MenuOption::EchoLocalAudio, false,
                    audioIO.data(), SLOT(toggleLocalEcho()));
                addCheckableItem(AUDIO_MENU, MenuOption::StereoAudio, false, 
                    audioIO.data(), SLOT(toggleStereoInput()));
                // Qt::CTRL | Qt::Key_M,
                addCheckableItem(AUDIO_MENU, MenuOption::MuteAudio, false, 
                    audioIO.data(), SLOT(toggleMute()));
                addCheckableItem(AUDIO_MENU, MenuOption::MuteEnvironment, false, 
                    audioIO.data(), SLOT(sendMuteEnvironmentPacket()));
                {
                    static const QString SCOPE_MENU{ "Audio Scope" };
                    addMenu(AUDIO_MENU, SCOPE_MENU);
                    auto scope = DependencyManager::get<AudioScope>();
                    // Qt::CTRL | Qt::Key_P
                    addCheckableItem(SCOPE_MENU, MenuOption::AudioScope, false, [=](bool checked) {
                        scope->toggle();
                    });
                    // Qt::CTRL | Qt::SHIFT | Qt::Key_P
                    addCheckableItem(SCOPE_MENU, MenuOption::AudioScopePause, false, [=](bool checked) {
                        scope->togglePause();
                    });
                    addSeparator(SCOPE_MENU, "Display Frames");

                    // FIXME GROUP
                    addCheckableItem(SCOPE_MENU, MenuOption::AudioScopeFiveFrames, true, [=](bool checked) {
                        scope->selectAudioScopeFiveFrames();
                    });
                    addCheckableItem(SCOPE_MENU, MenuOption::AudioScopeTwentyFrames, false, [=](bool checked) {
                        scope->selectAudioScopeTwentyFrames();
                    });
                    addCheckableItem(SCOPE_MENU, MenuOption::AudioScopeFiftyFrames, false, [=](bool checked) {
                        scope->selectAudioScopeFiftyFrames();
                    });
                }
                auto statsRenderer = DependencyManager::get<AudioIOStatsRenderer>();

                // Qt::CTRL | Qt::SHIFT | Qt::Key_A,
                addCheckableItem(AUDIO_MENU, MenuOption::AudioStats, false, [=](bool checked) {
                    statsRenderer->toggle();
                });
                addCheckableItem(AUDIO_MENU, MenuOption::AudioStatsShowInjectedStreams, false, [=](bool checked) {
                    statsRenderer->toggleShowInjectedStreams();
                });
            } // Developer -> Audio
        } // Developer

        {
            static const QString HELP_MENU{ "Help" };
            addMenu(ROOT_MENU, HELP_MENU);
            addItem(HELP_MENU, MenuOption::EditEntitiesHelp, [] {
                qApp->showEditEntitiesHelp();
            });
            addItem(HELP_MENU, MenuOption::AboutApp, [] {
                qApp->aboutApp();
            });
        } // Help
    }
}

void Menu::loadSettings() {
//    scanMenuBar(&Menu::loadAction);
}

void Menu::saveSettings() {
//    scanMenuBar(&Menu::saveAction);
}

void Menu::addMenuItem(const MenuItemProperties& properties) {
    if (QThread::currentThread() != Application::getInstance()->getMainThread()) {
        Application::getInstance()->postLambdaEvent([=]{
            addMenuItem(properties);
        });
        return;
    }
     // Shortcut key items: in order of priority
     //QString shortcutKey;
     //KeyEvent shortcutKeyEvent;
     //QKeySequence shortcutKeySequence; // this is what we actually use, it's set from one of the above

     //// location related items: in order of priority
     //int position;
     //QString beforeItem;
     //QString afterItem;

     if (properties.isSeparator) {
         addSeparator(properties.menuName, properties.menuItemName);
         return;
     }
     addItem(properties.menuName, properties.menuItemName);
     if (properties.isCheckable) {
         setCheckable(properties.menuItemName);
         if (properties.isChecked) {
             setChecked(properties.menuItemName);
         }
     }
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
