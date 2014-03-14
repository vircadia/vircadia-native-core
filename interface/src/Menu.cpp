//
//  Menu.cpp
//  hifi
//
//  Created by Stephen Birarda on 8/12/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include <cstdlib>


#include <QBoxLayout>
#include <QColorDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QInputDialog>
#include <QLineEdit>
#include <QMainWindow>
#include <QMenuBar>
#include <QShortcut>
#include <QSlider>
#include <QStandardPaths>
#include <QUuid>
#include <QWindow>

#include <AccountManager.h>
#include <XmppClient.h>
#include <UUID.h>

#include "Application.h"
#include "Menu.h"
#include "MenuScriptingInterface.h"
#include "Util.h"
#include "InfoView.h"
#include "ui/MetavoxelEditor.h"

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

const ViewFrustumOffset DEFAULT_FRUSTUM_OFFSET = {-135.0f, 0.0f, 0.0f, 25.0f, 0.0f};
const float DEFAULT_FACESHIFT_EYE_DEFLECTION = 0.25f;
const int FIVE_SECONDS_OF_FRAMES = 5 * 60;

Menu::Menu() :
    _actionHash(),
    _audioJitterBufferSamples(0),
    _bandwidthDialog(NULL),
    _fieldOfView(DEFAULT_FIELD_OF_VIEW_DEGREES),
    _faceshiftEyeDeflection(DEFAULT_FACESHIFT_EYE_DEFLECTION),
    _frustumDrawMode(FRUSTUM_DRAW_MODE_ALL),
    _viewFrustumOffset(DEFAULT_FRUSTUM_OFFSET),
    _octreeStatsDialog(NULL),
    _lodToolsDialog(NULL),
    _maxVoxels(DEFAULT_MAX_VOXELS_PER_SYSTEM),
    _voxelSizeScale(DEFAULT_OCTREE_SIZE_SCALE),
    _boundaryLevelAdjust(0),
    _maxVoxelPacketsPerSecond(DEFAULT_MAX_VOXEL_PPS),
    _lastAdjust(usecTimestampNow()),
    _fpsAverage(FIVE_SECONDS_OF_FRAMES),
    _loginAction(NULL),
    _preferencesDialog(NULL)

{
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

    AccountManager& accountManager = AccountManager::getInstance();
    
    _loginAction = addActionToQMenuAndActionHash(fileMenu, MenuOption::Logout);
    
    // call our toggle login function now so the menu option is setup properly
    toggleLoginMenuItem();
    
    // connect to the appropriate slots of the AccountManager so that we can change the Login/Logout menu item
    connect(&accountManager, &AccountManager::accessTokenChanged, this, &Menu::toggleLoginMenuItem);
    connect(&accountManager, &AccountManager::logoutComplete, this, &Menu::toggleLoginMenuItem);

    addDisabledActionAndSeparator(fileMenu, "Scripts");
    addActionToQMenuAndActionHash(fileMenu, MenuOption::LoadScript, Qt::CTRL | Qt::Key_O, appInstance, SLOT(loadDialog()));
    addActionToQMenuAndActionHash(fileMenu, MenuOption::StopAllScripts, 0, appInstance, SLOT(stopAllScripts()));
    addActionToQMenuAndActionHash(fileMenu, MenuOption::ReloadAllScripts, 0, appInstance, SLOT(reloadAllScripts()));
    _activeScriptsMenu = fileMenu->addMenu("Running Scripts");

    addDisabledActionAndSeparator(fileMenu, "Go");
    addActionToQMenuAndActionHash(fileMenu,
                                  MenuOption::GoHome,
                                  Qt::CTRL | Qt::Key_G,
                                  appInstance->getAvatar(),
                                  SLOT(goHome()));
    addActionToQMenuAndActionHash(fileMenu,
                                  MenuOption::GoToDomain,
                                  Qt::CTRL | Qt::Key_D,
                                   this,
                                   SLOT(goToDomainDialog()));
    addActionToQMenuAndActionHash(fileMenu,
                                  MenuOption::GoToLocation,
                                  Qt::CTRL | Qt::SHIFT | Qt::Key_L,
                                   this,
                                   SLOT(goToLocation()));
    addActionToQMenuAndActionHash(fileMenu,
                                  MenuOption::GoTo,
                                  Qt::Key_At,
                                  this,
                                  SLOT(goTo()));

    addDisabledActionAndSeparator(fileMenu, "Upload/Browse");
    addActionToQMenuAndActionHash(fileMenu, MenuOption::UploaderAvatarHead, 0, Application::getInstance(), SLOT(uploadFST()));
    addActionToQMenuAndActionHash(fileMenu, MenuOption::UploaderAvatarSkeleton, 0, Application::getInstance(), SLOT(uploadFST()));
    
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

    addActionToQMenuAndActionHash(editMenu,
                                  MenuOption::Preferences,
                                  Qt::CTRL | Qt::Key_Comma,
                                  this,
                                  SLOT(editPreferences()),
                                  QAction::PreferencesRole);

    addDisabledActionAndSeparator(editMenu, "Physics");
    addCheckableActionToQMenuAndActionHash(editMenu, MenuOption::Gravity, Qt::SHIFT | Qt::Key_G, false);

    
    addCheckableActionToQMenuAndActionHash(editMenu, MenuOption::ClickToFly);

    addAvatarCollisionSubMenu(editMenu);

    QMenu* toolsMenu = addMenu("Tools");
    addActionToQMenuAndActionHash(toolsMenu, MenuOption::MetavoxelEditor, 0, this, SLOT(showMetavoxelEditor()));

    _chatAction = addActionToQMenuAndActionHash(toolsMenu,
                                                MenuOption::Chat,
                                                Qt::Key_Return,
                                                this,
                                                SLOT(showChat()));
#ifdef HAVE_QXMPP
    const QXmppClient& xmppClient = XmppClient::getInstance().getXMPPClient();
    toggleChat();
    connect(&xmppClient, SIGNAL(connected()), this, SLOT(toggleChat()));
    connect(&xmppClient, SIGNAL(disconnected()), this, SLOT(toggleChat()));
#else
    _chatAction->setEnabled(false);
#endif

    QMenu* viewMenu = addMenu("View");

    addCheckableActionToQMenuAndActionHash(viewMenu,
                                           MenuOption::Fullscreen,
                                           Qt::CTRL | Qt::META | Qt::Key_F,
                                           false,
                                           appInstance,
                                           SLOT(setFullscreen(bool)));
    addCheckableActionToQMenuAndActionHash(viewMenu, MenuOption::FirstPerson, Qt::Key_P, true,
                                            appInstance,SLOT(cameraMenuChanged()));
    addCheckableActionToQMenuAndActionHash(viewMenu, MenuOption::Mirror, Qt::SHIFT | Qt::Key_H, true);
    addCheckableActionToQMenuAndActionHash(viewMenu, MenuOption::FullscreenMirror, Qt::Key_H, false,
                                            appInstance, SLOT(cameraMenuChanged()));
                                            
    addCheckableActionToQMenuAndActionHash(viewMenu, MenuOption::Enable3DTVMode, 0,
                                           false,
                                           appInstance,
                                           SLOT(setEnable3DTVMode(bool)));


    QMenu* avatarSizeMenu = viewMenu->addMenu("Avatar Size");

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

    addCheckableActionToQMenuAndActionHash(viewMenu, MenuOption::OffAxisProjection, 0, false);
    addCheckableActionToQMenuAndActionHash(viewMenu, MenuOption::TurnWithHead, 0, false);
    addCheckableActionToQMenuAndActionHash(viewMenu, MenuOption::MoveWithLean, 0, false);
    addCheckableActionToQMenuAndActionHash(viewMenu, MenuOption::HeadMouse, 0, false);


    addDisabledActionAndSeparator(viewMenu, "Stats");
    addCheckableActionToQMenuAndActionHash(viewMenu, MenuOption::Stats, Qt::Key_Slash);
    addActionToQMenuAndActionHash(viewMenu, MenuOption::Log, Qt::CTRL | Qt::Key_L, appInstance, SLOT(toggleLogDialog()));
    addCheckableActionToQMenuAndActionHash(viewMenu, MenuOption::Oscilloscope, 0, true);
    addCheckableActionToQMenuAndActionHash(viewMenu, MenuOption::Bandwidth, 0, true);
    addActionToQMenuAndActionHash(viewMenu, MenuOption::BandwidthDetails, 0, this, SLOT(bandwidthDetails()));
    addActionToQMenuAndActionHash(viewMenu, MenuOption::OctreeStats, 0, this, SLOT(octreeStatsDetails()));

    QMenu* developerMenu = addMenu("Developer");

    QMenu* renderOptionsMenu = developerMenu->addMenu("Rendering Options");

    addCheckableActionToQMenuAndActionHash(renderOptionsMenu, MenuOption::Stars, Qt::Key_Asterisk, true);
    addCheckableActionToQMenuAndActionHash(renderOptionsMenu, MenuOption::Atmosphere, Qt::SHIFT | Qt::Key_A, true);
    addActionToQMenuAndActionHash(renderOptionsMenu,
                                  MenuOption::GlowMode,
                                  0,
                                  appInstance->getGlowEffect(),
                                  SLOT(cycleRenderMode()));

    addCheckableActionToQMenuAndActionHash(renderOptionsMenu, MenuOption::Shadows, 0, false);
    addCheckableActionToQMenuAndActionHash(renderOptionsMenu, MenuOption::Metavoxels, 0, true);
    addCheckableActionToQMenuAndActionHash(renderOptionsMenu, MenuOption::BuckyBalls, 0, true);
    addCheckableActionToQMenuAndActionHash(renderOptionsMenu, MenuOption::Particles, 0, true);


    QMenu* voxelOptionsMenu = developerMenu->addMenu("Voxel Options");

    addCheckableActionToQMenuAndActionHash(voxelOptionsMenu,
                                           MenuOption::Voxels,
                                           Qt::SHIFT | Qt::Key_V,
                                           true,
                                           appInstance,
                                           SLOT(setRenderVoxels(bool)));

    addCheckableActionToQMenuAndActionHash(voxelOptionsMenu, MenuOption::VoxelTextures);
    addCheckableActionToQMenuAndActionHash(voxelOptionsMenu, MenuOption::AmbientOcclusion);
    addActionToQMenuAndActionHash(voxelOptionsMenu, MenuOption::LodTools, Qt::SHIFT | Qt::Key_L, this, SLOT(lodTools()));
    addCheckableActionToQMenuAndActionHash(voxelOptionsMenu, MenuOption::DontFadeOnVoxelServerChanges);
    addCheckableActionToQMenuAndActionHash(voxelOptionsMenu, MenuOption::DisableAutoAdjustLOD);

    QMenu* avatarOptionsMenu = developerMenu->addMenu("Avatar Options");

    addCheckableActionToQMenuAndActionHash(avatarOptionsMenu, MenuOption::Avatars, 0, true);
    addCheckableActionToQMenuAndActionHash(avatarOptionsMenu, MenuOption::RenderSkeletonCollisionProxies);
    addCheckableActionToQMenuAndActionHash(avatarOptionsMenu, MenuOption::RenderHeadCollisionProxies);

    addCheckableActionToQMenuAndActionHash(avatarOptionsMenu, MenuOption::LookAtVectors, 0, false);
    addCheckableActionToQMenuAndActionHash(avatarOptionsMenu,
                                           MenuOption::Faceshift,
                                           0,
                                           true,
                                           appInstance->getFaceshift(),
                                           SLOT(setTCPEnabled(bool)));
#ifdef HAVE_VISAGE
    addCheckableActionToQMenuAndActionHash(avatarOptionsMenu, MenuOption::Visage, 0, true,
        appInstance->getVisage(), SLOT(updateEnabled()));
#endif

    addCheckableActionToQMenuAndActionHash(avatarOptionsMenu, MenuOption::ChatCircling, 0, false);

    QMenu* handOptionsMenu = developerMenu->addMenu("Hand Options");

    addCheckableActionToQMenuAndActionHash(handOptionsMenu,
                                           MenuOption::FilterSixense,
                                           0,
                                           true,
                                           appInstance->getSixenseManager(),
                                           SLOT(setFilter(bool)));
    addCheckableActionToQMenuAndActionHash(handOptionsMenu, MenuOption::DisplayHands, 0, true);
    addCheckableActionToQMenuAndActionHash(handOptionsMenu, MenuOption::DisplayHandTargets, 0, false);
    addCheckableActionToQMenuAndActionHash(handOptionsMenu, MenuOption::PlaySlaps, 0, false);
    addCheckableActionToQMenuAndActionHash(handOptionsMenu, MenuOption::HandsCollideWithSelf, 0, false);

    addDisabledActionAndSeparator(developerMenu, "Testing");

    QMenu* timingMenu = developerMenu->addMenu("Timing and Statistics Tools");
    addCheckableActionToQMenuAndActionHash(timingMenu, MenuOption::TestPing, 0, true);
    addCheckableActionToQMenuAndActionHash(timingMenu, MenuOption::FrameTimer);
    addActionToQMenuAndActionHash(timingMenu, MenuOption::RunTimingTests, 0, this, SLOT(runTests()));

    QMenu* frustumMenu = developerMenu->addMenu("View Frustum Debugging Tools");
    addCheckableActionToQMenuAndActionHash(frustumMenu, MenuOption::DisplayFrustum, Qt::SHIFT | Qt::Key_F);
    addActionToQMenuAndActionHash(frustumMenu,
                                  MenuOption::FrustumRenderMode,
                                  Qt::SHIFT | Qt::Key_R,
                                  this,
                                  SLOT(cycleFrustumRenderMode()));
    updateFrustumRenderModeAction();


    QMenu* renderDebugMenu = developerMenu->addMenu("Render Debugging Tools");
    addCheckableActionToQMenuAndActionHash(renderDebugMenu, MenuOption::PipelineWarnings, Qt::CTRL | Qt::SHIFT | Qt::Key_P);
    addCheckableActionToQMenuAndActionHash(renderDebugMenu, MenuOption::SuppressShortTimings, Qt::CTRL | Qt::SHIFT | Qt::Key_S);

    addCheckableActionToQMenuAndActionHash(renderDebugMenu, 
                                  MenuOption::CullSharedFaces, 
                                  Qt::CTRL | Qt::SHIFT | Qt::Key_C, 
                                  false,
                                  appInstance->getVoxels(),
                                  SLOT(cullSharedFaces()));

    addCheckableActionToQMenuAndActionHash(renderDebugMenu, 
                                  MenuOption::ShowCulledSharedFaces, 
                                  Qt::CTRL | Qt::SHIFT | Qt::Key_X, 
                                  false,
                                  appInstance->getVoxels(),
                                  SLOT(showCulledSharedFaces()));

    QMenu* audioDebugMenu = developerMenu->addMenu("Audio Debugging Tools");
    addCheckableActionToQMenuAndActionHash(audioDebugMenu, MenuOption::AudioNoiseReduction,
                                           0,
                                           true,
                                           appInstance->getAudio(),
                                           SLOT(toggleAudioNoiseReduction()));
    addCheckableActionToQMenuAndActionHash(audioDebugMenu, MenuOption::EchoServerAudio);
    addCheckableActionToQMenuAndActionHash(audioDebugMenu, MenuOption::EchoLocalAudio);
    addCheckableActionToQMenuAndActionHash(audioDebugMenu, MenuOption::MuteAudio,
                                           Qt::CTRL | Qt::Key_M,
                                           false,
                                           appInstance->getAudio(),
                                           SLOT(toggleMute()));
    
    addActionToQMenuAndActionHash(developerMenu, MenuOption::PasteToVoxel,
                Qt::CTRL | Qt::SHIFT | Qt::Key_V,
                this,
                SLOT(pasteToVoxel()));

    connect(appInstance->getAudio(), SIGNAL(muteToggled()), this, SLOT(audioMuteToggled()));
    
#ifndef Q_OS_MAC
    QMenu* helpMenu = addMenu("Help");
    QAction* helpAction = helpMenu->addAction(MenuOption::AboutApp);
    connect(helpAction, SIGNAL(triggered()), this, SLOT(aboutApp()));
#endif
}

Menu::~Menu() {
    bandwidthDetailsClosed();
    octreeStatsDetailsClosed();
}

void Menu::loadSettings(QSettings* settings) {
    if (!settings) {
        settings = Application::getInstance()->getSettings();
    }

    _audioJitterBufferSamples = loadSetting(settings, "audioJitterBufferSamples", 0);
    _fieldOfView = loadSetting(settings, "fieldOfView", DEFAULT_FIELD_OF_VIEW_DEGREES);
    _faceshiftEyeDeflection = loadSetting(settings, "faceshiftEyeDeflection", DEFAULT_FACESHIFT_EYE_DEFLECTION);
    _maxVoxels = loadSetting(settings, "maxVoxels", DEFAULT_MAX_VOXELS_PER_SYSTEM);
    _maxVoxelPacketsPerSecond = loadSetting(settings, "maxVoxelsPPS", DEFAULT_MAX_VOXEL_PPS);
    _voxelSizeScale = loadSetting(settings, "voxelSizeScale", DEFAULT_OCTREE_SIZE_SCALE);
    _boundaryLevelAdjust = loadSetting(settings, "boundaryLevelAdjust", 0);

    settings->beginGroup("View Frustum Offset Camera");
    // in case settings is corrupt or missing loadSetting() will check for NaN
    _viewFrustumOffset.yaw = loadSetting(settings, "viewFrustumOffsetYaw", 0.0f);
    _viewFrustumOffset.pitch = loadSetting(settings, "viewFrustumOffsetPitch", 0.0f);
    _viewFrustumOffset.roll = loadSetting(settings, "viewFrustumOffsetRoll", 0.0f);
    _viewFrustumOffset.distance = loadSetting(settings, "viewFrustumOffsetDistance", 0.0f);
    _viewFrustumOffset.up = loadSetting(settings, "viewFrustumOffsetUp", 0.0f);
    settings->endGroup();

    scanMenuBar(&loadAction, settings);
    Application::getInstance()->getAvatar()->loadData(settings);
    Application::getInstance()->updateWindowTitle();
    NodeList::getInstance()->loadData(settings);

    // MyAvatar caches some menu options, so we have to update them whenever we load settings.
    // TODO: cache more settings in MyAvatar that are checked with very high frequency.
    MyAvatar* myAvatar = Application::getInstance()->getAvatar();
    myAvatar->updateCollisionFlags();
}

void Menu::saveSettings(QSettings* settings) {
    if (!settings) {
        settings = Application::getInstance()->getSettings();
    }

    settings->setValue("audioJitterBufferSamples", _audioJitterBufferSamples);
    settings->setValue("fieldOfView", _fieldOfView);
    settings->setValue("faceshiftEyeDeflection", _faceshiftEyeDeflection);
    settings->setValue("maxVoxels", _maxVoxels);
    settings->setValue("maxVoxelsPPS", _maxVoxelPacketsPerSecond);
    settings->setValue("voxelSizeScale", _voxelSizeScale);
    settings->setValue("boundaryLevelAdjust", _boundaryLevelAdjust);
    settings->beginGroup("View Frustum Offset Camera");
    settings->setValue("viewFrustumOffsetYaw", _viewFrustumOffset.yaw);
    settings->setValue("viewFrustumOffsetPitch", _viewFrustumOffset.pitch);
    settings->setValue("viewFrustumOffsetRoll", _viewFrustumOffset.roll);
    settings->setValue("viewFrustumOffsetDistance", _viewFrustumOffset.distance);
    settings->setValue("viewFrustumOffsetUp", _viewFrustumOffset.up);
    settings->endGroup();

    scanMenuBar(&saveAction, settings);
    Application::getInstance()->getAvatar()->saveData(settings);
    NodeList::getInstance()->saveData(settings);
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

void Menu::handleViewFrustumOffsetKeyModifier(int key) {
    const float VIEW_FRUSTUM_OFFSET_DELTA = 0.5f;
    const float VIEW_FRUSTUM_OFFSET_UP_DELTA = 0.05f;

    switch (key) {
        case Qt::Key_BracketLeft:
            _viewFrustumOffset.yaw -= VIEW_FRUSTUM_OFFSET_DELTA;
            break;

        case Qt::Key_BracketRight:
            _viewFrustumOffset.yaw += VIEW_FRUSTUM_OFFSET_DELTA;
            break;

        case Qt::Key_BraceLeft:
            _viewFrustumOffset.pitch -= VIEW_FRUSTUM_OFFSET_DELTA;
            break;

        case Qt::Key_BraceRight:
            _viewFrustumOffset.pitch += VIEW_FRUSTUM_OFFSET_DELTA;
            break;

        case Qt::Key_ParenLeft:
            _viewFrustumOffset.roll -= VIEW_FRUSTUM_OFFSET_DELTA;
            break;

        case Qt::Key_ParenRight:
            _viewFrustumOffset.roll += VIEW_FRUSTUM_OFFSET_DELTA;
            break;

        case Qt::Key_Less:
            _viewFrustumOffset.distance -= VIEW_FRUSTUM_OFFSET_DELTA;
            break;

        case Qt::Key_Greater:
            _viewFrustumOffset.distance += VIEW_FRUSTUM_OFFSET_DELTA;
            break;

        case Qt::Key_Comma:
            _viewFrustumOffset.up -= VIEW_FRUSTUM_OFFSET_UP_DELTA;
            break;

        case Qt::Key_Period:
            _viewFrustumOffset.up += VIEW_FRUSTUM_OFFSET_UP_DELTA;
            break;

        default:
            break;
    }
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

    return action;
}

void Menu::removeAction(QMenu* menu, const QString& actionName) {
    menu->removeAction(_actionHash.value(actionName));
}

void Menu::setIsOptionChecked(const QString& menuOption, bool isChecked) {
    QAction* menu = _actionHash.value(menuOption);
    if (menu) {
        menu->setChecked(isChecked);
    }
}

bool Menu::isOptionChecked(const QString& menuOption) {
    QAction* menu = _actionHash.value(menuOption);
    if (menu) {
        return menu->isChecked();
    }
    return false;
}

void Menu::triggerOption(const QString& menuOption) {
    _actionHash.value(menuOption)->trigger();
}

QAction* Menu::getActionForOption(const QString& menuOption) {
    return _actionHash.value(menuOption);
}

void Menu::aboutApp() {
    InfoView::forcedShow();
}

void sendFakeEnterEvent() {
    QPoint lastCursorPosition = QCursor::pos();
    QGLWidget* glWidget = Application::getInstance()->getGLWidget();

    QPoint windowPosition = glWidget->mapFromGlobal(lastCursorPosition);
    QEnterEvent enterEvent = QEnterEvent(windowPosition, windowPosition, lastCursorPosition);
    QCoreApplication::sendEvent(glWidget, &enterEvent);
}

const int QLINE_MINIMUM_WIDTH = 400;
const float DIALOG_RATIO_OF_WINDOW = 0.30f;

void Menu::loginForCurrentDomain() {
    QDialog loginDialog(Application::getInstance()->getWindow());
    loginDialog.setWindowTitle("Login");
    
    QBoxLayout* layout = new QBoxLayout(QBoxLayout::TopToBottom);
    loginDialog.setLayout(layout);
    loginDialog.setWindowFlags(Qt::Sheet);
    
    QFormLayout* form = new QFormLayout();
    layout->addLayout(form, 1);
    
    QLineEdit* loginLineEdit = new QLineEdit();
    loginLineEdit->setMinimumWidth(QLINE_MINIMUM_WIDTH);
    form->addRow("Login:", loginLineEdit);
    
    QLineEdit* passwordLineEdit = new QLineEdit();
    passwordLineEdit->setMinimumWidth(QLINE_MINIMUM_WIDTH);
    passwordLineEdit->setEchoMode(QLineEdit::Password);
    form->addRow("Password:", passwordLineEdit);
    
    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    loginDialog.connect(buttons, SIGNAL(accepted()), SLOT(accept()));
    loginDialog.connect(buttons, SIGNAL(rejected()), SLOT(reject()));
    layout->addWidget(buttons);
    
    int dialogReturn = loginDialog.exec();
    
    if (dialogReturn == QDialog::Accepted && !loginLineEdit->text().isEmpty() && !passwordLineEdit->text().isEmpty()) {
        // attempt to get an access token given this username and password
        AccountManager::getInstance().requestAccessToken(loginLineEdit->text(), passwordLineEdit->text());
    }
    
    sendFakeEnterEvent();
}

void Menu::editPreferences() {
    if (! _preferencesDialog) {
        _preferencesDialog = new PreferencesDialog(Application::getInstance()->getGLWidget());
        _preferencesDialog->show();
    }
    _preferencesDialog->raise();

    /*

    Application* applicationInstance = Application::getInstance();

    QDialog dialog(applicationInstance->getWindow());
    dialog.setWindowTitle("Interface Preferences");
    
    QBoxLayout* layout = new QBoxLayout(QBoxLayout::TopToBottom);
    dialog.setLayout(layout);
    
    QFormLayout* form = new QFormLayout();
    layout->addLayout(form, 1);

    QString faceURLString = applicationInstance->getAvatar()->getHead()->getFaceModel().getURL().toString();
    QLineEdit* faceURLEdit = new QLineEdit(faceURLString);
    faceURLEdit->setMinimumWidth(QLINE_MINIMUM_WIDTH);
    faceURLEdit->setPlaceholderText(DEFAULT_HEAD_MODEL_URL.toString());
    form->addRow("Face URL:", faceURLEdit);

    QString skeletonURLString = applicationInstance->getAvatar()->getSkeletonModel().getURL().toString();
    QLineEdit* skeletonURLEdit = new QLineEdit(skeletonURLString);
    skeletonURLEdit->setMinimumWidth(QLINE_MINIMUM_WIDTH);
    skeletonURLEdit->setPlaceholderText(DEFAULT_BODY_MODEL_URL.toString());
    form->addRow("Skeleton URL:", skeletonURLEdit);

    QString displayNameString = applicationInstance->getAvatar()->getDisplayName();
    QLineEdit* displayNameEdit = new QLineEdit(displayNameString);
    displayNameEdit->setMinimumWidth(QLINE_MINIMUM_WIDTH);
    form->addRow("Display name:", displayNameEdit);

    QSlider* pupilDilation = new QSlider(Qt::Horizontal);
    pupilDilation->setValue(applicationInstance->getAvatar()->getHead()->getPupilDilation() * pupilDilation->maximum());
    form->addRow("Pupil Dilation:", pupilDilation);

    QSlider* faceshiftEyeDeflection = new QSlider(Qt::Horizontal);
    faceshiftEyeDeflection->setValue(_faceshiftEyeDeflection * faceshiftEyeDeflection->maximum());
    form->addRow("Faceshift Eye Deflection:", faceshiftEyeDeflection);

    QSpinBox* fieldOfView = new QSpinBox();
    fieldOfView->setMaximum(180.f);
    fieldOfView->setMinimum(1.f);
    fieldOfView->setValue(_fieldOfView);
    form->addRow("Vertical Field of View (Degrees):", fieldOfView);

    QDoubleSpinBox* leanScale = new QDoubleSpinBox();
    leanScale->setValue(applicationInstance->getAvatar()->getLeanScale());
    form->addRow("Lean Scale:", leanScale);

    QDoubleSpinBox* avatarScale = new QDoubleSpinBox();
    avatarScale->setValue(applicationInstance->getAvatar()->getScale());
    form->addRow("Avatar Scale:", avatarScale);

    QSpinBox* audioJitterBufferSamples = new QSpinBox();
    audioJitterBufferSamples->setMaximum(10000);
    audioJitterBufferSamples->setMinimum(-10000);
    audioJitterBufferSamples->setValue(_audioJitterBufferSamples);
    form->addRow("Audio Jitter Buffer Samples (0 for automatic):", audioJitterBufferSamples);

    QSpinBox* maxVoxels = new QSpinBox();
    const int MAX_MAX_VOXELS = 5000000;
    const int MIN_MAX_VOXELS = 0;
    const int STEP_MAX_VOXELS = 50000;
    maxVoxels->setMaximum(MAX_MAX_VOXELS);
    maxVoxels->setMinimum(MIN_MAX_VOXELS);
    maxVoxels->setSingleStep(STEP_MAX_VOXELS);
    maxVoxels->setValue(_maxVoxels);
    form->addRow("Maximum Voxels:", maxVoxels);

    QSpinBox* maxVoxelsPPS = new QSpinBox();
    const int MAX_MAX_VOXELS_PPS = 6000;
    const int MIN_MAX_VOXELS_PPS = 60;
    const int STEP_MAX_VOXELS_PPS = 10;
    maxVoxelsPPS->setMaximum(MAX_MAX_VOXELS_PPS);
    maxVoxelsPPS->setMinimum(MIN_MAX_VOXELS_PPS);
    maxVoxelsPPS->setSingleStep(STEP_MAX_VOXELS_PPS);
    maxVoxelsPPS->setValue(_maxVoxelPacketsPerSecond);
    form->addRow("Maximum Voxels Packets Per Second:", maxVoxelsPPS);

    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    dialog.connect(buttons, SIGNAL(accepted()), SLOT(accept()));
    dialog.connect(buttons, SIGNAL(rejected()), SLOT(reject()));
    layout->addWidget(buttons);

    int ret = dialog.exec();
    if (ret == QDialog::Accepted) {
        QUrl faceModelURL(faceURLEdit->text());
        
        bool shouldDispatchIdentityPacket = false;

        if (faceModelURL.toString() != faceURLString) {
            // change the faceModelURL in the profile, it will also update this user's BlendFace
            applicationInstance->getAvatar()->setFaceModelURL(faceModelURL);
            shouldDispatchIdentityPacket = true;
        }

        QUrl skeletonModelURL(skeletonURLEdit->text());

        if (skeletonModelURL.toString() != skeletonURLString) {
            // change the skeletonModelURL in the profile, it will also update this user's Body
            applicationInstance->getAvatar()->setSkeletonModelURL(skeletonModelURL);
            shouldDispatchIdentityPacket = true;
        }

        QString displayNameStr(displayNameEdit->text());
        
        if (displayNameStr != displayNameString) {
            applicationInstance->getAvatar()->setDisplayName(displayNameStr);
            shouldDispatchIdentityPacket = true;
        }
                
        if (shouldDispatchIdentityPacket) {
            applicationInstance->getAvatar()->sendIdentityPacket();
        }

        applicationInstance->getAvatar()->getHead()->setPupilDilation(pupilDilation->value() / (float)pupilDilation->maximum());

        _maxVoxels = maxVoxels->value();
        applicationInstance->getVoxels()->setMaxVoxels(_maxVoxels);

        _maxVoxelPacketsPerSecond = maxVoxelsPPS->value();

        applicationInstance->getAvatar()->setLeanScale(leanScale->value());
        applicationInstance->getAvatar()->setClampedTargetScale(avatarScale->value());

        _audioJitterBufferSamples = audioJitterBufferSamples->value();

        if (_audioJitterBufferSamples != 0) {
            applicationInstance->getAudio()->setJitterBufferSamples(_audioJitterBufferSamples);
        }

        _fieldOfView = fieldOfView->value();
        applicationInstance->resizeGL(applicationInstance->getGLWidget()->width(), applicationInstance->getGLWidget()->height());

        _faceshiftEyeDeflection = faceshiftEyeDeflection->value() / (float)faceshiftEyeDeflection->maximum();
    }
    QMetaObject::invokeMethod(applicationInstance->getAudio(), "reset", Qt::QueuedConnection);
*/
    sendFakeEnterEvent();
}

void Menu::goToDomain(const QString newDomain) {
    if (NodeList::getInstance()->getDomainInfo().getHostname() != newDomain) {
        
        // send a node kill request, indicating to other clients that they should play the "disappeared" effect
        Application::getInstance()->getAvatar()->sendKillAvatar();
        
        // give our nodeList the new domain-server hostname
        NodeList::getInstance()->getDomainInfo().setHostname(newDomain);
    }
}

void Menu::goToDomainDialog() {

    QString currentDomainHostname = NodeList::getInstance()->getDomainInfo().getHostname();

    if (NodeList::getInstance()->getDomainInfo().getPort() != DEFAULT_DOMAIN_SERVER_PORT) {
        // add the port to the currentDomainHostname string if it is custom
        currentDomainHostname.append(QString(":%1").arg(NodeList::getInstance()->getDomainInfo().getPort()));
    }

    QInputDialog domainDialog(Application::getInstance()->getWindow());
    domainDialog.setWindowTitle("Go to Domain");
    domainDialog.setLabelText("Domain server:");
    domainDialog.setTextValue(currentDomainHostname);
    domainDialog.setWindowFlags(Qt::Sheet);
    domainDialog.resize(domainDialog.parentWidget()->size().width() * DIALOG_RATIO_OF_WINDOW, domainDialog.size().height());

    int dialogReturn = domainDialog.exec();
    if (dialogReturn == QDialog::Accepted) {
        QString newHostname(DEFAULT_DOMAIN_HOSTNAME);

        if (domainDialog.textValue().size() > 0) {
            // the user input a new hostname, use that
            newHostname = domainDialog.textValue();
        }
        
        goToDomain(newHostname);
    }

    sendFakeEnterEvent();
}

void Menu::goToOrientation(QString orientation) {
    
    if (orientation.isEmpty()) {
        return;
    }
    
    QStringList orientationItems = orientation.split(QRegExp("_|,"), QString::SkipEmptyParts);
    
    const int NUMBER_OF_ORIENTATION_ITEMS = 4;
    const int W_ITEM = 0;
    const int X_ITEM = 1;
    const int Y_ITEM = 2;
    const int Z_ITEM = 3;
    
    if (orientationItems.size() == NUMBER_OF_ORIENTATION_ITEMS) {
        
        double w = replaceLastOccurrence('-', '.', orientationItems[W_ITEM].trimmed()).toDouble();
        double x = replaceLastOccurrence('-', '.', orientationItems[X_ITEM].trimmed()).toDouble();
        double y = replaceLastOccurrence('-', '.', orientationItems[Y_ITEM].trimmed()).toDouble();
        double z = replaceLastOccurrence('-', '.', orientationItems[Z_ITEM].trimmed()).toDouble();
        
        glm::quat newAvatarOrientation(w, x, y, z);
        
        MyAvatar* myAvatar = Application::getInstance()->getAvatar();
        glm::quat avatarOrientation = myAvatar->getOrientation();
        if (newAvatarOrientation != avatarOrientation) {
            myAvatar->setOrientation(newAvatarOrientation);
        }
    }
}

bool Menu::goToDestination(QString destination) {
    
    QStringList coordinateItems = destination.split(QRegExp("_|,"), QString::SkipEmptyParts);
    
    const int NUMBER_OF_COORDINATE_ITEMS = 3;
    const int X_ITEM = 0;
    const int Y_ITEM = 1;
    const int Z_ITEM = 2;
    if (coordinateItems.size() == NUMBER_OF_COORDINATE_ITEMS) {
        
        double x = replaceLastOccurrence('-', '.', coordinateItems[X_ITEM].trimmed()).toDouble();
        double y = replaceLastOccurrence('-', '.', coordinateItems[Y_ITEM].trimmed()).toDouble();
        double z = replaceLastOccurrence('-', '.', coordinateItems[Z_ITEM].trimmed()).toDouble();
        
        glm::vec3 newAvatarPos(x, y, z);
        
        MyAvatar* myAvatar = Application::getInstance()->getAvatar();
        glm::vec3 avatarPos = myAvatar->getPosition();
        if (newAvatarPos != avatarPos) {
            // send a node kill request, indicating to other clients that they should play the "disappeared" effect
            MyAvatar::sendKillAvatar();
            
            qDebug("Going To Location: %f, %f, %f...", x, y, z);
            myAvatar->setPosition(newAvatarPos);
        }
        
        return true;
    }
    
    // no coordinates were parsed
    return false;
}

void Menu::goTo() {
    
    QInputDialog gotoDialog(Application::getInstance()->getWindow());
    gotoDialog.setWindowTitle("Go to");
    gotoDialog.setLabelText("Destination:");
    QString destination = QString();
    gotoDialog.setTextValue(destination);
    gotoDialog.setWindowFlags(Qt::Sheet);
    gotoDialog.resize(gotoDialog.parentWidget()->size().width() * DIALOG_RATIO_OF_WINDOW, gotoDialog.size().height());
    
    int dialogReturn = gotoDialog.exec();
    if (dialogReturn == QDialog::Accepted && !gotoDialog.textValue().isEmpty()) {
        
        destination = gotoDialog.textValue();
        
        // go to coordinate destination or to Username
        if (!goToDestination(destination)) {
            JSONCallbackParameters callbackParams;
            callbackParams.jsonCallbackReceiver = Application::getInstance()->getAvatar();
            callbackParams.jsonCallbackMethod = "goToLocationFromResponse";
            
            // there's a username entered by the user, make a request to the data-server for the associated location
            AccountManager::getInstance().authenticatedRequest("/api/v1/users/" + gotoDialog.textValue() + "/address",
                                                               QNetworkAccessManager::GetOperation,
                                                               callbackParams);
        }
    }
    
    sendFakeEnterEvent();
}

void Menu::goToLocation() {
    MyAvatar* myAvatar = Application::getInstance()->getAvatar();
    glm::vec3 avatarPos = myAvatar->getPosition();
    QString currentLocation = QString("%1, %2, %3").arg(QString::number(avatarPos.x),
                                                        QString::number(avatarPos.y), QString::number(avatarPos.z));


    QInputDialog coordinateDialog(Application::getInstance()->getWindow());
    coordinateDialog.setWindowTitle("Go to Location");
    coordinateDialog.setLabelText("Coordinate as x,y,z:");
    coordinateDialog.setTextValue(currentLocation);
    coordinateDialog.setWindowFlags(Qt::Sheet);
    coordinateDialog.resize(coordinateDialog.parentWidget()->size().width() * 0.30, coordinateDialog.size().height());

    int dialogReturn = coordinateDialog.exec();
    if (dialogReturn == QDialog::Accepted && !coordinateDialog.textValue().isEmpty()) {
        goToDestination(coordinateDialog.textValue());
    }

    sendFakeEnterEvent();
}

void Menu::pasteToVoxel() {
    QInputDialog pasteToOctalCodeDialog(Application::getInstance()->getWindow());
    pasteToOctalCodeDialog.setWindowTitle("Paste to Voxel");
    pasteToOctalCodeDialog.setLabelText("Octal Code:");
    QString octalCode = "";
    pasteToOctalCodeDialog.setTextValue(octalCode);
    pasteToOctalCodeDialog.setWindowFlags(Qt::Sheet);
    pasteToOctalCodeDialog.resize(pasteToOctalCodeDialog.parentWidget()->size().width() * DIALOG_RATIO_OF_WINDOW,
        pasteToOctalCodeDialog.size().height());

    int dialogReturn = pasteToOctalCodeDialog.exec();
    if (dialogReturn == QDialog::Accepted && !pasteToOctalCodeDialog.textValue().isEmpty()) {
        // we got an octalCode to paste to...
        QString locationToPaste = pasteToOctalCodeDialog.textValue();
        unsigned char* octalCodeDestination = hexStringToOctalCode(locationToPaste);

        // check to see if it was a legit octcode...
        if (locationToPaste == octalCodeToHexString(octalCodeDestination)) {
            Application::getInstance()->pasteVoxelsToOctalCode(octalCodeDestination);
        } else {
            qDebug() << "Problem with octcode...";
        }
    }

    sendFakeEnterEvent();
}

void Menu::toggleLoginMenuItem() {
    AccountManager& accountManager = AccountManager::getInstance();

    disconnect(_loginAction, 0, 0, 0);
    
    if (accountManager.isLoggedIn()) {
        // change the menu item to logout
        _loginAction->setText("Logout " + accountManager.getUsername());
        connect(_loginAction, &QAction::triggered, &accountManager, &AccountManager::logout);
    } else {
        // change the menu item to login
        _loginAction->setText("Login");
        
        connect(_loginAction, &QAction::triggered, this, &Menu::loginForCurrentDomain);
    }
}

void Menu::bandwidthDetails() {
    if (! _bandwidthDialog) {
        _bandwidthDialog = new BandwidthDialog(Application::getInstance()->getGLWidget(),
                                               Application::getInstance()->getBandwidthMeter());
        connect(_bandwidthDialog, SIGNAL(closed()), SLOT(bandwidthDetailsClosed()));

        _bandwidthDialog->show();
    }
    _bandwidthDialog->raise();
}

void Menu::showMetavoxelEditor() {
    if (!_MetavoxelEditor) {
        _MetavoxelEditor = new MetavoxelEditor();
    }
    _MetavoxelEditor->raise();
}

void Menu::showChat() {
    if (!_chatWindow) {
        _chatWindow = new ChatWindow();
        QMainWindow* mainWindow = Application::getInstance()->getWindow();
        QBoxLayout* boxLayout = static_cast<QBoxLayout*>(mainWindow->centralWidget()->layout());
        boxLayout->addWidget(_chatWindow, 0, Qt::AlignRight);
    } else {
        if (!_chatWindow->isVisible()) {
            _chatWindow->show();
        }
    }
}

void Menu::toggleChat() {
#ifdef HAVE_QXMPP
    _chatAction->setEnabled(XmppClient::getInstance().getXMPPClient().isConnected());
    if (!_chatAction->isEnabled() && _chatWindow) {
        _chatWindow->hide();
    }
#endif
}

void Menu::audioMuteToggled() {
    QAction *muteAction = _actionHash.value(MenuOption::MuteAudio);
    muteAction->setChecked(Application::getInstance()->getAudio()->getMuted());
}

void Menu::bandwidthDetailsClosed() {
    if (_bandwidthDialog) {
        delete _bandwidthDialog;
        _bandwidthDialog = NULL;
    }
}

void Menu::octreeStatsDetails() {
    if (!_octreeStatsDialog) {
        _octreeStatsDialog = new OctreeStatsDialog(Application::getInstance()->getGLWidget(),
                                                 Application::getInstance()->getOcteeSceneStats());
        connect(_octreeStatsDialog, SIGNAL(closed()), SLOT(octreeStatsDetailsClosed()));
        _octreeStatsDialog->show();
    }
    _octreeStatsDialog->raise();
}

void Menu::octreeStatsDetailsClosed() {
    if (_octreeStatsDialog) {
        delete _octreeStatsDialog;
        _octreeStatsDialog = NULL;
    }
}

QString Menu::getLODFeedbackText() {
    // determine granularity feedback
    int boundaryLevelAdjust = getBoundaryLevelAdjust();
    QString granularityFeedback;

    switch (boundaryLevelAdjust) {
        case 0: {
            granularityFeedback = QString("at standard granularity.");
        } break;
        case 1: {
            granularityFeedback = QString("at half of standard granularity.");
        } break;
        case 2: {
            granularityFeedback = QString("at a third of standard granularity.");
        } break;
        default: {
            granularityFeedback = QString("at 1/%1th of standard granularity.").arg(boundaryLevelAdjust + 1);
        } break;
    }

    // distance feedback    
    float voxelSizeScale = getVoxelSizeScale();
    float relativeToDefault = voxelSizeScale / DEFAULT_OCTREE_SIZE_SCALE;
    QString result;
    if (relativeToDefault > 1.01) {
        result = QString("%1 further %2").arg(relativeToDefault,8,'f',2).arg(granularityFeedback);
    } else if (relativeToDefault > 0.99) {
            result = QString("the default distance %1").arg(granularityFeedback);
    } else {
        result = QString("%1 of default %2").arg(relativeToDefault,8,'f',3).arg(granularityFeedback);
    }
    return result;
}

void Menu::autoAdjustLOD(float currentFPS) {
    // NOTE: our first ~100 samples at app startup are completely all over the place, and we don't 
    // really want to count them in our average, so we will ignore the real frame rates and stuff
    // our moving average with simulated good data
    const int IGNORE_THESE_SAMPLES = 100;
    const float ASSUMED_FPS = 60.0f;
    if (_fpsAverage.getSampleCount() < IGNORE_THESE_SAMPLES) {
        currentFPS = ASSUMED_FPS;
    }
    _fpsAverage.updateAverage(currentFPS);

    bool changed = false;
    quint64 now = usecTimestampNow();
    quint64 elapsed = now - _lastAdjust;

    if (elapsed > ADJUST_LOD_DOWN_DELAY && _fpsAverage.getAverage() < ADJUST_LOD_DOWN_FPS 
            && _voxelSizeScale > ADJUST_LOD_MIN_SIZE_SCALE) {

        _voxelSizeScale *= ADJUST_LOD_DOWN_BY;
        if (_voxelSizeScale < ADJUST_LOD_MIN_SIZE_SCALE) {
            _voxelSizeScale = ADJUST_LOD_MIN_SIZE_SCALE;
        }
        changed = true;
        _lastAdjust = now;
        qDebug() << "adjusting LOD down... average fps for last approximately 5 seconds=" << _fpsAverage.getAverage()
                     << "_voxelSizeScale=" << _voxelSizeScale;
    }

    if (elapsed > ADJUST_LOD_UP_DELAY && _fpsAverage.getAverage() > ADJUST_LOD_UP_FPS 
            && _voxelSizeScale < ADJUST_LOD_MAX_SIZE_SCALE) {
        _voxelSizeScale *= ADJUST_LOD_UP_BY;
        if (_voxelSizeScale > ADJUST_LOD_MAX_SIZE_SCALE) {
            _voxelSizeScale = ADJUST_LOD_MAX_SIZE_SCALE;
        }
        changed = true;
        _lastAdjust = now;
        qDebug() << "adjusting LOD up... average fps for last approximately 5 seconds=" << _fpsAverage.getAverage()
                     << "_voxelSizeScale=" << _voxelSizeScale;
    }
    
    if (changed) {
        if (_lodToolsDialog) {
            _lodToolsDialog->reloadSliders();
        }
    }
}

void Menu::setVoxelSizeScale(float sizeScale) {
    _voxelSizeScale = sizeScale;
}

void Menu::setBoundaryLevelAdjust(int boundaryLevelAdjust) {
    _boundaryLevelAdjust = boundaryLevelAdjust;
}

void Menu::lodTools() {
    if (!_lodToolsDialog) {
        _lodToolsDialog = new LodToolsDialog(Application::getInstance()->getGLWidget());
        connect(_lodToolsDialog, SIGNAL(closed()), SLOT(lodToolsClosed()));
        _lodToolsDialog->show();
    }
    _lodToolsDialog->raise();
}

void Menu::lodToolsClosed() {
    if (_lodToolsDialog) {
        delete _lodToolsDialog;
        _lodToolsDialog = NULL;
    }
}

void Menu::cycleFrustumRenderMode() {
    _frustumDrawMode = (FrustumDrawMode)((_frustumDrawMode + 1) % FRUSTUM_DRAW_MODE_COUNT);
    updateFrustumRenderModeAction();
}

void Menu::runTests() {
    runTimingTests();
}

void Menu::updateFrustumRenderModeAction() {
    QAction* frustumRenderModeAction = _actionHash.value(MenuOption::FrustumRenderMode);
    switch (_frustumDrawMode) {
        default:
        case FRUSTUM_DRAW_MODE_ALL:
            frustumRenderModeAction->setText("Render Mode - All");
            break;
        case FRUSTUM_DRAW_MODE_VECTORS:
            frustumRenderModeAction->setText("Render Mode - Vectors");
            break;
        case FRUSTUM_DRAW_MODE_PLANES:
            frustumRenderModeAction->setText("Render Mode - Planes");
            break;
        case FRUSTUM_DRAW_MODE_NEAR_PLANE:
            frustumRenderModeAction->setText("Render Mode - Near");
            break;
        case FRUSTUM_DRAW_MODE_FAR_PLANE:
            frustumRenderModeAction->setText("Render Mode - Far");
            break;
        case FRUSTUM_DRAW_MODE_KEYHOLE:
            frustumRenderModeAction->setText("Render Mode - Keyhole");
            break;
    }
}

void Menu::addAvatarCollisionSubMenu(QMenu* overMenu) {
    // add avatar collisions subMenu to overMenu
    QMenu* subMenu = overMenu->addMenu("Collision Options");

    Application* appInstance = Application::getInstance();
    QObject* avatar = appInstance->getAvatar();
    addCheckableActionToQMenuAndActionHash(subMenu, MenuOption::CollideWithEnvironment, 
            0, false, avatar, SLOT(updateCollisionFlags()));
    addCheckableActionToQMenuAndActionHash(subMenu, MenuOption::CollideWithAvatars, 
            0, true, avatar, SLOT(updateCollisionFlags()));
    addCheckableActionToQMenuAndActionHash(subMenu, MenuOption::CollideWithVoxels, 
            0, false, avatar, SLOT(updateCollisionFlags()));
    addCheckableActionToQMenuAndActionHash(subMenu, MenuOption::CollideWithParticles, 
            0, true, avatar, SLOT(updateCollisionFlags()));
}

QString Menu::replaceLastOccurrence(QChar search, QChar replace, QString string) {
    int lastIndex;
    lastIndex = string.lastIndexOf(search);
    if (lastIndex > 0) {
        lastIndex = string.lastIndexOf(search);
        string.replace(lastIndex, 1, replace);
    }
    
    return string;
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
            removeAction(parent, finalMenuPart);
        } else {
            QMenuBar::removeAction(action);
        }

        QMenuBar::repaint();
    }
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
    }
    QMenuBar::repaint();
};

