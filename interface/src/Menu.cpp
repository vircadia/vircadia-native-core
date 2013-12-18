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
#include <QSlider>
#include <QStandardPaths>
#include <QUuid>
#include <QWindow>

#include <UUID.h>

#include "Application.h"
#include "DataServerClient.h"
#include "PairingHandler.h"
#include "Menu.h"
#include "Util.h"
#include "InfoView.h"

Menu* Menu::_instance = NULL;

Menu* Menu::getInstance() {
    static QMutex menuInstanceMutex;
    
    // lock the menu instance mutex to make sure we don't race and create two menus and crash
    menuInstanceMutex.lock();
    
    if (!_instance) {
        qDebug("First call to Menu::getInstance() - initing menu.\n");
        
        _instance = new Menu();
    }
    
    menuInstanceMutex.unlock();
        
    return _instance;
}

const ViewFrustumOffset DEFAULT_FRUSTUM_OFFSET = {-135.0f, 0.0f, 0.0f, 25.0f, 0.0f};
const float DEFAULT_FACESHIFT_EYE_DEFLECTION = 0.25f;

Menu::Menu() :
    _actionHash(),
    _audioJitterBufferSamples(0),
    _bandwidthDialog(NULL),
    _fieldOfView(DEFAULT_FIELD_OF_VIEW_DEGREES),
    _faceshiftEyeDeflection(DEFAULT_FACESHIFT_EYE_DEFLECTION),
    _frustumDrawMode(FRUSTUM_DRAW_MODE_ALL),
    _viewFrustumOffset(DEFAULT_FRUSTUM_OFFSET),
    _voxelModeActionsGroup(NULL),
    _voxelStatsDialog(NULL),
    _lodToolsDialog(NULL),
    _maxVoxels(DEFAULT_MAX_VOXELS_PER_SYSTEM),
    _voxelSizeScale(DEFAULT_OCTREE_SIZE_SCALE),
    _boundaryLevelAdjust(0),
    _maxVoxelPacketsPerSecond(DEFAULT_MAX_VOXEL_PPS)
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
    
    (addActionToQMenuAndActionHash(fileMenu,
                                   MenuOption::Login,
                                   0,
                                   this,
                                   SLOT(login())));

    addDisabledActionAndSeparator(fileMenu, "Scripts");
    addActionToQMenuAndActionHash(fileMenu, MenuOption::LoadScript, Qt::CTRL | Qt::Key_O, appInstance, SLOT(loadScript()));
    _activeScriptsMenu = fileMenu->addMenu("Running Scripts");

    addDisabledActionAndSeparator(fileMenu, "Voxels");
    addActionToQMenuAndActionHash(fileMenu, MenuOption::ExportVoxels, Qt::CTRL | Qt::Key_E, appInstance, SLOT(exportVoxels()));
    addActionToQMenuAndActionHash(fileMenu, MenuOption::ImportVoxels, Qt::CTRL | Qt::Key_I, appInstance, SLOT(importVoxels()));
    
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
                                   SLOT(goToDomain()));
    addActionToQMenuAndActionHash(fileMenu,
                                  MenuOption::GoToLocation,
                                  Qt::CTRL | Qt::SHIFT | Qt::Key_L,
                                   this,
                                   SLOT(goToLocation()));
    addActionToQMenuAndActionHash(fileMenu,
                                  MenuOption::GoToUser,
                                  Qt::Key_At,
                                  this,
                                  SLOT(goToUser()));

    
    addDisabledActionAndSeparator(fileMenu, "Settings");
    addActionToQMenuAndActionHash(fileMenu, MenuOption::SettingsImport, 0, this, SLOT(importSettings()));
    addActionToQMenuAndActionHash(fileMenu, MenuOption::SettingsExport, 0, this, SLOT(exportSettings()));
    
    addDisabledActionAndSeparator(fileMenu, "Devices");
    addActionToQMenuAndActionHash(fileMenu, MenuOption::Pair, 0, PairingHandler::getInstance(), SLOT(sendPairRequest()));
    addCheckableActionToQMenuAndActionHash(fileMenu, MenuOption::TransmitterDrive, 0, true);
    
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
    
    addDisabledActionAndSeparator(editMenu, "Voxels");
    
    addActionToQMenuAndActionHash(editMenu, MenuOption::CutVoxels, Qt::CTRL | Qt::Key_X, appInstance, SLOT(cutVoxels()));
    addActionToQMenuAndActionHash(editMenu, MenuOption::CopyVoxels, Qt::CTRL | Qt::Key_C, appInstance, SLOT(copyVoxels()));
    addActionToQMenuAndActionHash(editMenu, MenuOption::PasteVoxels, Qt::CTRL | Qt::Key_V, appInstance, SLOT(pasteVoxels()));
    addActionToQMenuAndActionHash(editMenu, MenuOption::NudgeVoxels, Qt::CTRL | Qt::Key_N, appInstance, SLOT(nudgeVoxels()));
    
    #ifdef __APPLE__
        addActionToQMenuAndActionHash(editMenu, MenuOption::DeleteVoxels, Qt::Key_Backspace, appInstance, SLOT(deleteVoxels()));
    #else
        addActionToQMenuAndActionHash(editMenu, MenuOption::DeleteVoxels, Qt::Key_Delete, appInstance, SLOT(deleteVoxels()));
    #endif
    
    addDisabledActionAndSeparator(editMenu, "Physics");
    addCheckableActionToQMenuAndActionHash(editMenu, MenuOption::Gravity, Qt::SHIFT | Qt::Key_G, true);
    addCheckableActionToQMenuAndActionHash(editMenu,
                                           MenuOption::Collisions,
                                           0,
                                           true,
                                           appInstance->getAvatar(),
                                           SLOT(setWantCollisionsOn(bool)));
    
    QMenu* toolsMenu = addMenu("Tools");
    
    _voxelModeActionsGroup = new QActionGroup(this);
    _voxelModeActionsGroup->setExclusive(false);
    
    QAction* addVoxelMode = addCheckableActionToQMenuAndActionHash(toolsMenu, MenuOption::VoxelAddMode, Qt::Key_V);
    _voxelModeActionsGroup->addAction(addVoxelMode);
    
    QAction* deleteVoxelMode = addCheckableActionToQMenuAndActionHash(toolsMenu, MenuOption::VoxelDeleteMode, Qt::Key_R);
    _voxelModeActionsGroup->addAction(deleteVoxelMode);
    
    QAction* colorVoxelMode = addCheckableActionToQMenuAndActionHash(toolsMenu, MenuOption::VoxelColorMode, Qt::Key_B);
    _voxelModeActionsGroup->addAction(colorVoxelMode);
    
    QAction* selectVoxelMode = addCheckableActionToQMenuAndActionHash(toolsMenu, MenuOption::VoxelSelectMode, Qt::Key_O);
    _voxelModeActionsGroup->addAction(selectVoxelMode);
    
    QAction* getColorMode = addCheckableActionToQMenuAndActionHash(toolsMenu, MenuOption::VoxelGetColorMode, Qt::Key_G);
    _voxelModeActionsGroup->addAction(getColorMode);
    
    addCheckableActionToQMenuAndActionHash(toolsMenu, MenuOption::ClickToFly);
    
    
    // connect each of the voxel mode actions to the updateVoxelModeActionsSlot
    foreach (QAction* action, _voxelModeActionsGroup->actions()) {
        connect(action, SIGNAL(triggered()), this, SLOT(updateVoxelModeActions()));
    }
    
    QAction* voxelPaintColor = addActionToQMenuAndActionHash(toolsMenu,
                                                             MenuOption::VoxelPaintColor,
                                                             Qt::META | Qt::Key_C,
                                                             this,
                                                             SLOT(chooseVoxelPaintColor()));
    
    Application::getInstance()->getSwatch()->setAction(voxelPaintColor);
    
    QColor paintColor(128, 128, 128);
    voxelPaintColor->setData(paintColor);
    voxelPaintColor->setIcon(Swatch::createIcon(paintColor));
    
    addActionToQMenuAndActionHash(toolsMenu,
                                  MenuOption::DecreaseVoxelSize,
                                  QKeySequence::ZoomOut,
                                  appInstance,
                                  SLOT(decreaseVoxelSize()));
    addActionToQMenuAndActionHash(toolsMenu,
                                  MenuOption::IncreaseVoxelSize,
                                  QKeySequence::ZoomIn,
                                  appInstance,
                                  SLOT(increaseVoxelSize()));
    addActionToQMenuAndActionHash(toolsMenu, MenuOption::ResetSwatchColors, 0, this, SLOT(resetSwatchColors()));

    
    QMenu* viewMenu = addMenu("View");
    
    addCheckableActionToQMenuAndActionHash(viewMenu,
                                           MenuOption::Fullscreen,
                                           Qt::CTRL | Qt::META | Qt::Key_F,
                                           false,
                                           appInstance,
                                           SLOT(setFullscreen(bool)));
    addCheckableActionToQMenuAndActionHash(viewMenu, MenuOption::FirstPerson, Qt::Key_P, true);
    addCheckableActionToQMenuAndActionHash(viewMenu, MenuOption::Mirror, Qt::SHIFT | Qt::Key_H);
    addCheckableActionToQMenuAndActionHash(viewMenu, MenuOption::FullscreenMirror, Qt::Key_H);
    
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
                                  0,
                                  appInstance->getAvatar(),
                                  SLOT(resetSize()));

    addCheckableActionToQMenuAndActionHash(viewMenu,
                                           MenuOption::OffAxisProjection,
                                           0,
                                           true);
    addCheckableActionToQMenuAndActionHash(viewMenu,
                                           MenuOption::TurnWithHead,
                                           0,
                                           true);
    addCheckableActionToQMenuAndActionHash(viewMenu, MenuOption::MoveWithLean, 0, false);
    addCheckableActionToQMenuAndActionHash(viewMenu, MenuOption::HeadMouse, 0, false);
    
    
    addDisabledActionAndSeparator(viewMenu, "Stats");
    addCheckableActionToQMenuAndActionHash(viewMenu, MenuOption::Stats, Qt::Key_Slash);
    addActionToQMenuAndActionHash(viewMenu, MenuOption::Log, Qt::CTRL | Qt::Key_L, appInstance, SLOT(toggleLogDialog()));
    addCheckableActionToQMenuAndActionHash(viewMenu, MenuOption::Oscilloscope, 0, true);
    addCheckableActionToQMenuAndActionHash(viewMenu, MenuOption::Bandwidth, 0, true);
    addActionToQMenuAndActionHash(viewMenu, MenuOption::BandwidthDetails, 0, this, SLOT(bandwidthDetails()));
    addActionToQMenuAndActionHash(viewMenu, MenuOption::VoxelStats, 0, this, SLOT(voxelStatsDetails()));
     
    QMenu* developerMenu = addMenu("Developer");

    QMenu* renderOptionsMenu = developerMenu->addMenu("Rendering Options");

    addCheckableActionToQMenuAndActionHash(renderOptionsMenu, MenuOption::Stars, Qt::Key_Asterisk, true);
    addCheckableActionToQMenuAndActionHash(renderOptionsMenu, MenuOption::Atmosphere, Qt::SHIFT | Qt::Key_A, true);
    addActionToQMenuAndActionHash(renderOptionsMenu,
                                  MenuOption::GlowMode,
                                  0,
                                  appInstance->getGlowEffect(),
                                  SLOT(cycleRenderMode()));
    
    addCheckableActionToQMenuAndActionHash(renderOptionsMenu, MenuOption::ParticleCloud, 0, false);
    addCheckableActionToQMenuAndActionHash(renderOptionsMenu, MenuOption::Shadows, 0, false);
    addCheckableActionToQMenuAndActionHash(renderOptionsMenu, MenuOption::Metavoxels, 0, false);
    

    QMenu* voxelOptionsMenu = developerMenu->addMenu("Voxel Options");

    addCheckableActionToQMenuAndActionHash(voxelOptionsMenu,
                                           MenuOption::Voxels,
                                           Qt::SHIFT | Qt::Key_V,
                                           true,
                                           appInstance,
                                           SLOT(setRenderVoxels(bool)));
    addCheckableActionToQMenuAndActionHash(voxelOptionsMenu, MenuOption::DontRenderVoxels);
    addCheckableActionToQMenuAndActionHash(voxelOptionsMenu, MenuOption::DontCallOpenGLForVoxels);

    _useVoxelShader = addCheckableActionToQMenuAndActionHash(voxelOptionsMenu, MenuOption::UseVoxelShader, 0, 
                                           false, appInstance->getVoxels(), SLOT(setUseVoxelShader(bool)));

    addCheckableActionToQMenuAndActionHash(voxelOptionsMenu, MenuOption::VoxelsAsPoints, 0, 
                                           false, appInstance->getVoxels(), SLOT(setVoxelsAsPoints(bool)));

    addCheckableActionToQMenuAndActionHash(voxelOptionsMenu, MenuOption::VoxelTextures);
    addCheckableActionToQMenuAndActionHash(voxelOptionsMenu, MenuOption::AmbientOcclusion);
    addCheckableActionToQMenuAndActionHash(voxelOptionsMenu, MenuOption::DontFadeOnVoxelServerChanges);
    addActionToQMenuAndActionHash(voxelOptionsMenu, MenuOption::LodTools, Qt::SHIFT | Qt::Key_L, this, SLOT(lodTools()));

    QMenu* voxelProtoOptionsMenu = voxelOptionsMenu->addMenu("Voxel Server Protocol Options");
    
    addCheckableActionToQMenuAndActionHash(voxelProtoOptionsMenu, MenuOption::DisableColorVoxels);
    addCheckableActionToQMenuAndActionHash(voxelProtoOptionsMenu, MenuOption::DisableLowRes);
    addCheckableActionToQMenuAndActionHash(voxelProtoOptionsMenu, MenuOption::DisableDeltaSending);
    addCheckableActionToQMenuAndActionHash(voxelProtoOptionsMenu, MenuOption::EnableVoxelPacketCompression);
    addCheckableActionToQMenuAndActionHash(voxelProtoOptionsMenu, MenuOption::EnableOcclusionCulling);
    addCheckableActionToQMenuAndActionHash(voxelProtoOptionsMenu, MenuOption::DestructiveAddVoxel);

    QMenu* avatarOptionsMenu = developerMenu->addMenu("Avatar Options");
    
    addCheckableActionToQMenuAndActionHash(avatarOptionsMenu, MenuOption::Avatars, 0, true);
    addCheckableActionToQMenuAndActionHash(avatarOptionsMenu, MenuOption::CollisionProxies);
    
    addActionToQMenuAndActionHash(avatarOptionsMenu,
                                  MenuOption::FaceMode,
                                  0,
                                  &appInstance->getAvatar()->getHead().getVideoFace(),
                                  SLOT(cycleRenderMode()));
    
    addCheckableActionToQMenuAndActionHash(avatarOptionsMenu, MenuOption::LookAtVectors, 0, true);
    addCheckableActionToQMenuAndActionHash(avatarOptionsMenu, MenuOption::LookAtIndicator, 0, true);
    addCheckableActionToQMenuAndActionHash(avatarOptionsMenu,
                                           MenuOption::FaceshiftTCP,
                                           0,
                                           false,
                                           appInstance->getFaceshift(),
                                           SLOT(setTCPEnabled(bool)));
    addCheckableActionToQMenuAndActionHash(avatarOptionsMenu, MenuOption::ChatCircling, 0, true);

    QMenu* webcamOptionsMenu = developerMenu->addMenu("Webcam Options");

    addCheckableActionToQMenuAndActionHash(webcamOptionsMenu,
                                           MenuOption::Webcam,
                                           0,
                                           false,
                                           appInstance->getWebcam(),
                                           SLOT(setEnabled(bool)));

    addActionToQMenuAndActionHash(webcamOptionsMenu,
                                  MenuOption::WebcamMode,
                                  0,
                                  appInstance->getWebcam()->getGrabber(),
                                  SLOT(cycleVideoSendMode()));

    addCheckableActionToQMenuAndActionHash(webcamOptionsMenu,
                                           MenuOption::WebcamTexture,
                                           0,
                                           false,
                                           appInstance->getWebcam()->getGrabber(),
                                           SLOT(setDepthOnly(bool)));

    QMenu* handOptionsMenu = developerMenu->addMenu("Hand Options");

    addCheckableActionToQMenuAndActionHash(handOptionsMenu,
                                           MenuOption::FilterSixense,
                                           0,
                                           true,
                                           appInstance->getSixenseManager(),
                                           SLOT(setFilter(bool)));
    addCheckableActionToQMenuAndActionHash(handOptionsMenu, MenuOption::SimulateLeapHand);
    addCheckableActionToQMenuAndActionHash(handOptionsMenu, MenuOption::DisplayLeapHands, 0, true);
    addCheckableActionToQMenuAndActionHash(handOptionsMenu, MenuOption::LeapDrive, 0, false);
    addCheckableActionToQMenuAndActionHash(handOptionsMenu, MenuOption::DisplayHandTargets, 0, false);    
    addCheckableActionToQMenuAndActionHash(handOptionsMenu, MenuOption::BallFromHand, 0, false);
    addCheckableActionToQMenuAndActionHash(handOptionsMenu, MenuOption::VoxelDrumming, 0, false);
    addCheckableActionToQMenuAndActionHash(handOptionsMenu, MenuOption::PlaySlaps, 0, false);
    
 

    QMenu* trackingOptionsMenu = developerMenu->addMenu("Tracking Options");
    addCheckableActionToQMenuAndActionHash(trackingOptionsMenu,
                                           MenuOption::SkeletonTracking,
                                           0,
                                           false,
                                           appInstance->getWebcam(),
                                           SLOT(setSkeletonTrackingOn(bool)));
    
    addCheckableActionToQMenuAndActionHash(trackingOptionsMenu,
                                           MenuOption::LEDTracking,
                                           0,
                                           false,
                                           appInstance->getWebcam()->getGrabber(),
                                           SLOT(setLEDTrackingOn(bool)));
    
    addDisabledActionAndSeparator(developerMenu, "Testing");

    QMenu* timingMenu = developerMenu->addMenu("Timing and Statistics Tools");
    addCheckableActionToQMenuAndActionHash(timingMenu, MenuOption::TestPing, 0, true);
    addCheckableActionToQMenuAndActionHash(timingMenu, MenuOption::FrameTimer);
    addActionToQMenuAndActionHash(timingMenu, MenuOption::RunTimingTests, 0, this, SLOT(runTests()));
    addActionToQMenuAndActionHash(timingMenu,
                                  MenuOption::TreeStats,
                                  Qt::SHIFT | Qt::Key_S,
                                  appInstance->getVoxels(),
                                  SLOT(collectStatsForTreesAndVBOs()));
    
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

    addCheckableActionToQMenuAndActionHash(renderDebugMenu, MenuOption::AutomaticallyAuditTree);


    addActionToQMenuAndActionHash(renderDebugMenu,
                                  MenuOption::ShowAllLocalVoxels,
                                  Qt::CTRL | Qt::Key_A,
                                  appInstance->getVoxels(),
                                  SLOT(showAllLocalVoxels()));
    
    addActionToQMenuAndActionHash(renderDebugMenu,
                                  MenuOption::KillLocalVoxels,
                                  Qt::CTRL | Qt::Key_K,
                                  appInstance, SLOT(doKillLocalVoxels()));
    
    addActionToQMenuAndActionHash(renderDebugMenu,
                                  MenuOption::RandomizeVoxelColors,
                                  Qt::CTRL | Qt::Key_R,
                                  appInstance->getVoxels(),
                                  SLOT(randomizeVoxelColors()));
    
    addActionToQMenuAndActionHash(renderDebugMenu,
                                  MenuOption::FalseColorRandomly,
                                  0,
                                  appInstance->getVoxels(),
                                  SLOT(falseColorizeRandom()));
    
    addActionToQMenuAndActionHash(renderDebugMenu,
                                  MenuOption::FalseColorEveryOtherVoxel,
                                  0,
                                  appInstance->getVoxels(),
                                  SLOT(falseColorizeRandomEveryOther()));
    
    addActionToQMenuAndActionHash(renderDebugMenu,
                                  MenuOption::FalseColorByDistance,
                                  0,
                                  appInstance->getVoxels(),
                                  SLOT(falseColorizeDistanceFromView()));
    
    addActionToQMenuAndActionHash(renderDebugMenu,
                                  MenuOption::FalseColorOutOfView,
                                  0,
                                  appInstance->getVoxels(),
                                  SLOT(falseColorizeInView()));
    
    addActionToQMenuAndActionHash(renderDebugMenu,
                                  MenuOption::FalseColorBySource,
                                  0,
                                  appInstance->getVoxels(),
                                  SLOT(falseColorizeBySource()));
    
    addActionToQMenuAndActionHash(renderDebugMenu,
                                  MenuOption::ShowTrueColors,
                                  Qt::CTRL | Qt::Key_T,
                                  appInstance->getVoxels(),
                                  SLOT(trueColorize()));

    addDisabledActionAndSeparator(renderDebugMenu, "Coverage Maps");
    addActionToQMenuAndActionHash(renderDebugMenu,
                                  MenuOption::FalseColorOccluded,
                                  0,
                                  appInstance->getVoxels(),
                                  SLOT(falseColorizeOccluded()));
    
    addActionToQMenuAndActionHash(renderDebugMenu,
                                  MenuOption::FalseColorOccludedV2,
                                  0,
                                  appInstance->getVoxels(),
                                  SLOT(falseColorizeOccludedV2()));
    
    addCheckableActionToQMenuAndActionHash(renderDebugMenu, MenuOption::CoverageMap, Qt::SHIFT | Qt::CTRL | Qt::Key_O);
    addCheckableActionToQMenuAndActionHash(renderDebugMenu, MenuOption::CoverageMapV2, Qt::SHIFT | Qt::CTRL | Qt::Key_P);
                                           
    QMenu* audioDebugMenu = developerMenu->addMenu("Audio Debugging Tools");
    addCheckableActionToQMenuAndActionHash(audioDebugMenu, MenuOption::EchoServerAudio);
    addCheckableActionToQMenuAndActionHash(audioDebugMenu, MenuOption::EchoLocalAudio);


    addCheckableActionToQMenuAndActionHash(developerMenu, MenuOption::ExtraDebugging);
    addActionToQMenuAndActionHash(developerMenu, MenuOption::PasteToVoxel, 
                Qt::CTRL | Qt::SHIFT | Qt::Key_V, 
                this,
                SLOT(pasteToVoxel()));
                
    
#ifndef Q_OS_MAC
    QMenu* helpMenu = addMenu("Help");
    QAction* helpAction = helpMenu->addAction(MenuOption::AboutApp);
    connect(helpAction, SIGNAL(triggered()), this, SLOT(aboutApp()));
#endif
    
}

Menu::~Menu() {
    bandwidthDetailsClosed();
    voxelStatsDetailsClosed();
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
    Application::getInstance()->getSwatch()->loadData(settings);
    Application::getInstance()->getProfile()->loadData(settings);
    Application::getInstance()->updateWindowTitle();
    NodeList::getInstance()->loadData(settings);
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
    Application::getInstance()->getSwatch()->saveData(settings);
    Application::getInstance()->getProfile()->saveData(settings);
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

void Menu::addDisabledActionAndSeparator(QMenu* destinationMenu, const QString& actionName) {
    destinationMenu->addSeparator();
    (destinationMenu->addAction(actionName))->setEnabled(false);
}

QAction* Menu::addActionToQMenuAndActionHash(QMenu* destinationMenu,
                                             const QString actionName,
                                             const QKEYSEQUENCE& shortcut,
                                             const QObject* receiver,
                                             const char* member,
                                             QACTION_MENUROLE role) {
    QAction* action;
    
    if (receiver && member) {
        action = destinationMenu->addAction(actionName, receiver, member, shortcut);
    } else {
        action = destinationMenu->addAction(actionName);
        action->setShortcut(shortcut);
    }
    action->setMenuRole((QAction::MenuRole)role);
    
    _actionHash.insert(actionName, action);
    
    return action;
}

QAction* Menu::addCheckableActionToQMenuAndActionHash(QMenu* destinationMenu,
                                                      const QString actionName,
                                                      const QKeySequence& shortcut,
                                                      const bool checked,
                                                      const QObject* receiver,
                                                      const char* member) {
    QAction* action = addActionToQMenuAndActionHash(destinationMenu, actionName, (QKEYSEQUENCE&)shortcut, receiver, member);
    action->setCheckable(true);
    action->setChecked(checked);
    
    return action;
}

void Menu::removeAction(QMenu* menu, const QString& actionName) {
    menu->removeAction(_actionHash.value(actionName));
}


bool Menu::isOptionChecked(const QString& menuOption) {
    return _actionHash.value(menuOption)->isChecked();
}

void Menu::triggerOption(const QString& menuOption) {
    _actionHash.value(menuOption)->trigger();
}

QAction* Menu::getActionForOption(const QString& menuOption) {
    return _actionHash.value(menuOption);
}

bool Menu::isVoxelModeActionChecked() {
    foreach (QAction* action, _voxelModeActionsGroup->actions()) {
        if (action->isChecked()) {
            return true;
        }
    }
    return false;
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
const float DIALOG_RATIO_OF_WINDOW = 0.30;

void Menu::login() {
    QInputDialog loginDialog(Application::getInstance()->getWindow());
    loginDialog.setWindowTitle("Login");
    loginDialog.setLabelText("Username:");
    QString username = Application::getInstance()->getProfile()->getUsername();
    loginDialog.setTextValue(username);
    loginDialog.setWindowFlags(Qt::Sheet);
    loginDialog.resize(loginDialog.parentWidget()->size().width() * DIALOG_RATIO_OF_WINDOW, loginDialog.size().height());
    
    int dialogReturn = loginDialog.exec();
    
    if (dialogReturn == QDialog::Accepted && !loginDialog.textValue().isEmpty() && loginDialog.textValue() != username) {
        // there has been a username change
        // ask for a profile reset with the new username
        Application::getInstance()->resetProfile(loginDialog.textValue());
  
    }
    
    sendFakeEnterEvent();
}

void Menu::editPreferences() {
    Application* applicationInstance = Application::getInstance();
    
    QDialog dialog(applicationInstance->getWindow());
    dialog.setWindowTitle("Interface Preferences");
    QBoxLayout* layout = new QBoxLayout(QBoxLayout::TopToBottom);
    dialog.setLayout(layout);
    
    QFormLayout* form = new QFormLayout();
    layout->addLayout(form, 1);
        
    QString faceURLString = applicationInstance->getProfile()->getFaceModelURL().toString();
    QLineEdit* faceURLEdit = new QLineEdit(faceURLString);
    faceURLEdit->setMinimumWidth(QLINE_MINIMUM_WIDTH);
    form->addRow("Face URL:", faceURLEdit);
    
    QString skeletonURLString = applicationInstance->getProfile()->getSkeletonModelURL().toString();
    QLineEdit* skeletonURLEdit = new QLineEdit(skeletonURLString);
    skeletonURLEdit->setMinimumWidth(QLINE_MINIMUM_WIDTH);
    form->addRow("Skeleton URL:", skeletonURLEdit);
    
    QSlider* pupilDilation = new QSlider(Qt::Horizontal);
    pupilDilation->setValue(applicationInstance->getAvatar()->getHead().getPupilDilation() * pupilDilation->maximum());
    form->addRow("Pupil Dilation:", pupilDilation);
    
    QSlider* faceshiftEyeDeflection = new QSlider(Qt::Horizontal);
    faceshiftEyeDeflection->setValue(_faceshiftEyeDeflection * faceshiftEyeDeflection->maximum());
    form->addRow("Faceshift Eye Deflection:", faceshiftEyeDeflection);
    
    QSpinBox* fieldOfView = new QSpinBox();
    fieldOfView->setMaximum(180);
    fieldOfView->setMinimum(1);
    fieldOfView->setValue(_fieldOfView);
    form->addRow("Vertical Field of View (Degrees):", fieldOfView);
    
    QDoubleSpinBox* leanScale = new QDoubleSpinBox();
    leanScale->setValue(applicationInstance->getAvatar()->getLeanScale());
    form->addRow("Lean Scale:", leanScale);
    
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
        
        if (faceModelURL.toString() != faceURLString) {
            // change the faceModelURL in the profile, it will also update this user's BlendFace
            applicationInstance->getProfile()->setFaceModelURL(faceModelURL);
            
            // send the new face mesh URL to the data-server (if we have a client UUID)
            DataServerClient::putValueForKey(DataServerKey::FaceMeshURL,
                                             faceModelURL.toString().toLocal8Bit().constData());
        }
        
        QUrl skeletonModelURL(skeletonURLEdit->text());
        
        if (skeletonModelURL.toString() != skeletonURLString) {
            // change the skeletonModelURL in the profile, it will also update this user's Body
            applicationInstance->getProfile()->setSkeletonModelURL(skeletonModelURL);
            
            // send the new skeleton model URL to the data-server (if we have a client UUID)
            DataServerClient::putValueForKey(DataServerKey::SkeletonURL,
                                             skeletonModelURL.toString().toLocal8Bit().constData());
        }
                        
        applicationInstance->getAvatar()->getHead().setPupilDilation(pupilDilation->value() / (float)pupilDilation->maximum());
        
        _maxVoxels = maxVoxels->value();
        applicationInstance->getVoxels()->setMaxVoxels(_maxVoxels);

        _maxVoxelPacketsPerSecond = maxVoxelsPPS->value();
        
        applicationInstance->getAvatar()->setLeanScale(leanScale->value());
        
        _audioJitterBufferSamples = audioJitterBufferSamples->value();
        
        if (_audioJitterBufferSamples != 0) {
            applicationInstance->getAudio()->setJitterBufferSamples(_audioJitterBufferSamples);
        }
        
        _fieldOfView = fieldOfView->value();
        applicationInstance->resizeGL(applicationInstance->getGLWidget()->width(), applicationInstance->getGLWidget()->height());
        
        _faceshiftEyeDeflection = faceshiftEyeDeflection->value() / (float)faceshiftEyeDeflection->maximum();
    }
    
    sendFakeEnterEvent();
}

void Menu::goToDomain() {
    
    QString currentDomainHostname = NodeList::getInstance()->getDomainHostname();
    
    if (NodeList::getInstance()->getDomainPort() != DEFAULT_DOMAIN_SERVER_PORT) {
        // add the port to the currentDomainHostname string if it is custom
        currentDomainHostname.append(QString(":%1").arg(NodeList::getInstance()->getDomainPort()));
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
        
        // send a node kill request, indicating to other clients that they should play the "disappeared" effect
        NodeList::getInstance()->sendKillNode(&NODE_TYPE_AVATAR_MIXER, 1);
        
        // give our nodeList the new domain-server hostname
        NodeList::getInstance()->setDomainHostname(domainDialog.textValue());
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
        QByteArray newCoordinates;
        
        QString delimiterPattern(",");
        QStringList coordinateItems = coordinateDialog.textValue().split(delimiterPattern);
        
        const int NUMBER_OF_COORDINATE_ITEMS = 3;
        const int X_ITEM = 0;
        const int Y_ITEM = 1;
        const int Z_ITEM = 2;
        if (coordinateItems.size() == NUMBER_OF_COORDINATE_ITEMS) {
            double x = coordinateItems[X_ITEM].toDouble();
            double y = coordinateItems[Y_ITEM].toDouble();
            double z = coordinateItems[Z_ITEM].toDouble();
            glm::vec3 newAvatarPos(x, y, z);
            
            if (newAvatarPos != avatarPos) {
                // send a node kill request, indicating to other clients that they should play the "disappeared" effect
                NodeList::getInstance()->sendKillNode(&NODE_TYPE_AVATAR_MIXER, 1);
                
                qDebug("Going To Location: %f, %f, %f...\n", x, y, z);
                myAvatar->setPosition(newAvatarPos); 
            }
        }
    }
    
    sendFakeEnterEvent();
}

void Menu::goToUser() {
    QInputDialog userDialog(Application::getInstance()->getWindow());
    userDialog.setWindowTitle("Go to User");
    userDialog.setLabelText("Destination user:");
    QString username = Application::getInstance()->getProfile()->getUsername();
    userDialog.setTextValue(username);
    userDialog.setWindowFlags(Qt::Sheet);
    userDialog.resize(userDialog.parentWidget()->size().width() * DIALOG_RATIO_OF_WINDOW, userDialog.size().height());
    
    int dialogReturn = userDialog.exec();
    if (dialogReturn == QDialog::Accepted && !userDialog.textValue().isEmpty()) {
        // there's a username entered by the user, make a request to the data-server
        DataServerClient::getValuesForKeysAndUserString((QStringList() << DataServerKey::Domain << DataServerKey::Position),
                                                        userDialog.textValue());
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
            qDebug() << "problem with octcode...\n";
        }
    }
    
    sendFakeEnterEvent();
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

void Menu::bandwidthDetailsClosed() {
    if (_bandwidthDialog) {
        delete _bandwidthDialog;
        _bandwidthDialog = NULL;
    }
}

void Menu::voxelStatsDetails() {
    if (!_voxelStatsDialog) {
        _voxelStatsDialog = new VoxelStatsDialog(Application::getInstance()->getGLWidget(),
                                                 Application::getInstance()->getOcteeSceneStats());
        connect(_voxelStatsDialog, SIGNAL(closed()), SLOT(voxelStatsDetailsClosed()));
        _voxelStatsDialog->show();
    }
    _voxelStatsDialog->raise();
}

void Menu::voxelStatsDetailsClosed() {
    if (_voxelStatsDialog) {
        delete _voxelStatsDialog;
        _voxelStatsDialog = NULL;
    }
}

void Menu::setVoxelSizeScale(float sizeScale) {
    _voxelSizeScale = sizeScale;
    Application::getInstance()->getVoxels()->redrawInViewVoxels();
}

void Menu::setBoundaryLevelAdjust(int boundaryLevelAdjust) {
    _boundaryLevelAdjust = boundaryLevelAdjust;
    Application::getInstance()->getVoxels()->redrawInViewVoxels();
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

void Menu::updateVoxelModeActions() {
    // only the sender can be checked
    foreach (QAction* action, _voxelModeActionsGroup->actions()) {
        if (action->isChecked() && action != sender()) {
            action->setChecked(false);
        }
    }
}

void Menu::chooseVoxelPaintColor() {
    Application* appInstance = Application::getInstance();
    QAction* paintColor = _actionHash.value(MenuOption::VoxelPaintColor);
    
    QColor selected = QColorDialog::getColor(paintColor->data().value<QColor>(),
                                             appInstance->getGLWidget(),
                                             "Voxel Paint Color");
    if (selected.isValid()) {
        paintColor->setData(selected);
        paintColor->setIcon(Swatch::createIcon(selected));
    }
    
    // restore the main window's active state
    appInstance->getWindow()->activateWindow();
}

void Menu::runTests() {
    runTimingTests();
}

void Menu::resetSwatchColors() {
    Application::getInstance()->getSwatch()->reset();
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

