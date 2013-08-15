//
//  Menu.cpp
//  hifi
//
//  Created by Stephen Birarda on 8/12/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include <cstdlib>

#include <QMenuBar>
#include <QBoxLayout>
#include <QColorDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QLineEdit>
#include <QMainWindow>
#include <QStandardPaths>

#include "Application.h"
#include "PairingHandler.h"
#include "Menu.h"

Menu* Menu::_instance = NULL;

Menu* Menu::getInstance() {
    if (!_instance) {
        qDebug("First call to Menu::getInstance() - initing menu.\n");
        
        _instance = new Menu();
    }
        
    return _instance;
}

const ViewFrustumOffset DEFAULT_FRUSTUM_OFFSET = {-135.0f, 0.0f, 0.0f, 25.0f, 0.0f};

Menu::Menu() :
    _bandwidthDialog(NULL),
    _frustumDrawMode(FRUSTUM_DRAW_MODE_ALL),
    _viewFrustumOffset(DEFAULT_FRUSTUM_OFFSET)
{
    QApplication *appInstance = Application::getInstance();
    
    QMenu* fileMenu = addMenu("File");
    (addActionToQMenuAndActionHash(fileMenu,
                                  MenuOption::Quit,
                                   appInstance,
                                   SLOT(quit()),
                                   Qt::CTRL | Qt::Key_Q))->setMenuRole(QAction::QuitRole);
    
    (addActionToQMenuAndActionHash(fileMenu,
                                   MenuOption::Preferences,
                                   this,
                                   SLOT(editPreferences()),
                                   Qt::CTRL | Qt::Key_Comma))->setMenuRole(QAction::PreferencesRole);
    
    QMenu* pairMenu = addMenu("Pair");
    addActionToQMenuAndActionHash(pairMenu, MenuOption::Pair, PairingHandler::getInstance(), SLOT(sendPairRequest()));
    
    
    QMenu* optionsMenu = addMenu("Options");
    
    addCheckableActionToQMenuAndActionHash(optionsMenu, MenuOption::Mirror, NULL, NULL, Qt::Key_H, false);
    addCheckableActionToQMenuAndActionHash(optionsMenu, MenuOption::Noise, NULL, NULL, Qt::Key_N, false);
    addCheckableActionToQMenuAndActionHash(optionsMenu, MenuOption::GyroLook, NULL, NULL, 0, true);
    addCheckableActionToQMenuAndActionHash(optionsMenu, MenuOption::HeadMouse, NULL, NULL, 0);
    addCheckableActionToQMenuAndActionHash(optionsMenu, MenuOption::TransmitterDrive, NULL, NULL, 0, true);
    addCheckableActionToQMenuAndActionHash(optionsMenu, MenuOption::Gravity, NULL, NULL, Qt::SHIFT | Qt::Key_G, true);
    addCheckableActionToQMenuAndActionHash(optionsMenu, MenuOption::TestPing, NULL, NULL, 0, true);
    addCheckableActionToQMenuAndActionHash(optionsMenu,
                                           MenuOption::Fullscreen,
                                           appInstance,
                                           SLOT(setFullscreen(bool)),
                                           Qt::Key_F);
    addCheckableActionToQMenuAndActionHash(optionsMenu, MenuOption::Webcam);
    addCheckableActionToQMenuAndActionHash(optionsMenu, MenuOption::SkeletonTracking);
    //        optionsMenu->addAction("Webcam", &_webcam, SLOT(setEnabled(bool)))->setCheckable(true);
    //        optionsMenu->addAction("Skeleton Tracking", &_webcam, SLOT(setSkeletonTrackingOn(bool)))->setCheckable(true);
    addCheckableActionToQMenuAndActionHash(optionsMenu, MenuOption::Collisions, NULL, NULL, 0, true);
    addActionToQMenuAndActionHash(optionsMenu, MenuOption::WebcamMode);
    addCheckableActionToQMenuAndActionHash(optionsMenu, MenuOption::WebcamTexture);
    
    //        optionsMenu->addAction("Cycle Webcam Send Mode", _webcam.getGrabber(), SLOT(cycleVideoSendMode()));
    //        optionsMenu->addAction("Webcam Texture", _webcam.getGrabber(), SLOT(setDepthOnly(bool)))->setCheckable(true);
    
    addActionToQMenuAndActionHash(optionsMenu, MenuOption::GoHome, appInstance, SLOT(goHome()), Qt::CTRL | Qt::Key_G);
    
    QMenu* audioMenu = addMenu("Audio");
    addCheckableActionToQMenuAndActionHash(audioMenu, MenuOption::EchoAudio);
    
    QMenu* renderMenu = addMenu("Render");
    addCheckableActionToQMenuAndActionHash(renderMenu,
                                           MenuOption::Voxels,
                                           appInstance,
                                           SLOT(setRenderVoxels(bool)),
                                           Qt::SHIFT | Qt::Key_V,
                                           true);
    addCheckableActionToQMenuAndActionHash(renderMenu, MenuOption::VoxelTextures);
    addCheckableActionToQMenuAndActionHash(renderMenu, MenuOption::Stars, NULL, NULL, 0, true);
    addCheckableActionToQMenuAndActionHash(renderMenu, MenuOption::Atmosphere, NULL, NULL, Qt::SHIFT | Qt::Key_A, true);
    addCheckableActionToQMenuAndActionHash(renderMenu, MenuOption::GroundPlane, NULL, NULL, 0, true);
    addCheckableActionToQMenuAndActionHash(renderMenu, MenuOption::Avatars, NULL, NULL, 0, true);
    addCheckableActionToQMenuAndActionHash(renderMenu, MenuOption::AvatarAsBalls);
    
    addActionToQMenuAndActionHash(renderMenu, MenuOption::VoxelMode);
    
    //        renderMenu->addAction("Cycle Voxel Mode", _myAvatar.getVoxels(), SLOT(cycleMode()));
    
    addActionToQMenuAndActionHash(renderMenu, MenuOption::FaceMode);
    
    //        renderMenu->addAction("Cycle Face Mode", &_myAvatar.getHead().getFace(), SLOT(cycleRenderMode()));
    
    addCheckableActionToQMenuAndActionHash(renderMenu, MenuOption::FrameTimer);
    addCheckableActionToQMenuAndActionHash(renderMenu, MenuOption::LookAtVectors);
    addCheckableActionToQMenuAndActionHash(renderMenu, MenuOption::LookAtIndicator, NULL, NULL, 0, true);
    addCheckableActionToQMenuAndActionHash(renderMenu, MenuOption::ParticleSystem);
    addCheckableActionToQMenuAndActionHash(renderMenu, MenuOption::FirstPerson, NULL, NULL, Qt::Key_P, true);
    
    addActionToQMenuAndActionHash(renderMenu, MenuOption::IncreaseAvatarSize);
    addActionToQMenuAndActionHash(renderMenu, MenuOption::DecreaseAvatarSize);
    addActionToQMenuAndActionHash(renderMenu, MenuOption::ResetAvatarSize);
    
    //    renderMenu->addAction("Increase Avatar Size", this, SLOT(increaseAvatarSize()), Qt::Key_Plus);
    //        renderMenu->addAction("Decrease Avatar Size", this, SLOT(decreaseAvatarSize()), Qt::Key_Minus);
    //        renderMenu->addAction("Reset Avatar Size", this, SLOT(resetAvatarSize()));
    
    QMenu* toolsMenu = addMenu("Tools");
    addCheckableActionToQMenuAndActionHash(toolsMenu, MenuOption::Stats, NULL, NULL, Qt::Key_Slash);
    addCheckableActionToQMenuAndActionHash(toolsMenu, MenuOption::Log, NULL, NULL, Qt::CTRL | Qt::Key_L);
    addCheckableActionToQMenuAndActionHash(toolsMenu, MenuOption::Oscilloscope, NULL, NULL, 0, true);
    addCheckableActionToQMenuAndActionHash(toolsMenu, MenuOption::Bandwidth, NULL, NULL, 0, true);
    addActionToQMenuAndActionHash(toolsMenu, MenuOption::BandwidthDetails);
    
    //        toolsMenu->addAction("Bandwidth Details", this, SLOT(bandwidthDetails()));
    
    addActionToQMenuAndActionHash(toolsMenu, MenuOption::VoxelStats);
    
    //        toolsMenu->addAction("Voxel Stats Details", this, SLOT(voxelStatsDetails()));
    
    QMenu* voxelMenu = addMenu("Voxels");
    _voxelModeActionsGroup = new QActionGroup(this);
    
    QAction* addVoxelMode = addCheckableActionToQMenuAndActionHash(voxelMenu, MenuOption::VoxelAddMode, NULL, NULL, Qt::Key_V);
    _voxelModeActionsGroup->addAction(addVoxelMode);
    
    QAction* deleteVoxelMode = addCheckableActionToQMenuAndActionHash(voxelMenu,
                                                                      MenuOption::VoxelDeleteMode,
                                                                      NULL,
                                                                      NULL,
                                                                      Qt::Key_R);
    _voxelModeActionsGroup->addAction(deleteVoxelMode);
    
    QAction* colorVoxelMode = addCheckableActionToQMenuAndActionHash(voxelMenu,
                                                                     MenuOption::VoxelColorMode,
                                                                     NULL,
                                                                     NULL,
                                                                     Qt::Key_B);
    _voxelModeActionsGroup->addAction(colorVoxelMode);
    
    QAction* selectVoxelMode = addCheckableActionToQMenuAndActionHash(voxelMenu,
                                                                      MenuOption::VoxelSelectMode,
                                                                      NULL,
                                                                      NULL,
                                                                      Qt::Key_O);
    _voxelModeActionsGroup->addAction(selectVoxelMode);
    
    QAction* getColorMode = addCheckableActionToQMenuAndActionHash(voxelMenu,
                                                                   MenuOption::VoxelGetColorMode,
                                                                   NULL,
                                                                   NULL,
                                                                   Qt::Key_G);
    _voxelModeActionsGroup->addAction(getColorMode);
    
    QAction* voxelPaintColor = addActionToQMenuAndActionHash(voxelMenu,
                                                             MenuOption::VoxelPaintColor,
                                                             this,
                                                             SLOT(chooseVoxelPaintColor()),
                                                             Qt::META | Qt::Key_C);
    
    Application::getInstance()->getSwatch()->setAction(voxelPaintColor);
    
    QColor paintColor(128, 128, 128);
    voxelPaintColor->setData(paintColor);
    voxelPaintColor->setIcon(Swatch::createIcon(paintColor));
    
    addActionToQMenuAndActionHash(voxelMenu,
                                  MenuOption::DecreaseVoxelSize,
                                  appInstance,
                                  SLOT(decreaseVoxelSize()),
                                  QKeySequence::ZoomOut);
    addActionToQMenuAndActionHash(voxelMenu,
                                  MenuOption::IncreaseVoxelSize,
                                  appInstance,
                                  SLOT(increaseVoxelSize()),
                                  QKeySequence::ZoomIn);
    addActionToQMenuAndActionHash(voxelMenu, MenuOption::ResetSwatchColors, appInstance, SLOT(resetSwatchColors()));
    
    addCheckableActionToQMenuAndActionHash(voxelMenu, MenuOption::DestructiveAddVoxel);
    
    addActionToQMenuAndActionHash(voxelMenu, MenuOption::ExportVoxels, NULL, NULL, Qt::CTRL | Qt::Key_E);
    addActionToQMenuAndActionHash(voxelMenu, MenuOption::ImportVoxels, NULL, NULL, Qt::CTRL | Qt::Key_I);
    addActionToQMenuAndActionHash(voxelMenu, MenuOption::ImportVoxelsClipboard, NULL, NULL, Qt::SHIFT | Qt::CTRL | Qt::Key_I);
    addActionToQMenuAndActionHash(voxelMenu, MenuOption::CutVoxels, NULL, NULL, Qt::CTRL | Qt::Key_X);
    addActionToQMenuAndActionHash(voxelMenu, MenuOption::CopyVoxels, NULL, NULL, Qt::CTRL | Qt::Key_C);
    addActionToQMenuAndActionHash(voxelMenu, MenuOption::PasteVoxels, NULL, NULL, Qt::CTRL | Qt::Key_V);
    
    QMenu* debugMenu = addMenu("Debug");

    QMenu* frustumMenu = debugMenu->addMenu("View Frustum Debugging Tools");
    addCheckableActionToQMenuAndActionHash(frustumMenu, MenuOption::DisplayFrustum, NULL, NULL, Qt::SHIFT | Qt::Key_F);
    
    addActionToQMenuAndActionHash(frustumMenu,
                                  MenuOption::FrustumRenderMode,
                                  this,
                                  SLOT(cycleFrustumRenderMode()),
                                  Qt::SHIFT | Qt::Key_R);
    updateFrustumRenderModeAction();
    
    addActionToQMenuAndActionHash(debugMenu, MenuOption::RunTimingTests, NULL, NULL);
    
    //        debugMenu->addAction("Run Timing Tests", this, SLOT(runTests()));
    
    addActionToQMenuAndActionHash(debugMenu, MenuOption::TreeStats, NULL, NULL, Qt::SHIFT | Qt::Key_S);
    
    //        debugMenu->addAction("Calculate Tree Stats", this, SLOT(doTreeStats()), Qt::SHIFT | Qt::Key_S);
    
    QMenu* renderDebugMenu = debugMenu->addMenu("Render Debugging Tools");
    addCheckableActionToQMenuAndActionHash(renderDebugMenu, MenuOption::PipelineWarnings, NULL, NULL);
    
    //        (_renderPipelineWarnings = renderDebugMenu->addAction("Show Render Pipeline Warnings",
    //                                                              this, SLOT(setRenderWarnings(bool))))->setCheckable(true);
    
    addActionToQMenuAndActionHash(renderDebugMenu, MenuOption::KillLocalVoxels, NULL, NULL, Qt::CTRL | Qt::Key_K);
    
    //        renderDebugMenu->addAction("Kill Local Voxels", this, SLOT(doKillLocalVoxels()), Qt::CTRL | Qt::Key_K);
    
    addActionToQMenuAndActionHash(renderDebugMenu, MenuOption::RandomizeVoxelColors, NULL, NULL, Qt::CTRL | Qt::Key_R);
    
    //        renderDebugMenu->addAction("Randomize Voxel TRUE Colors", this, SLOT(doRandomizeVoxelColors()), Qt::CTRL | Qt::Key_R);
    
    addActionToQMenuAndActionHash(renderDebugMenu, MenuOption::FalseColorRandomly, NULL, NULL);
    
    //        renderDebugMenu->addAction("FALSE Color Voxels Randomly", this, SLOT(doFalseRandomizeVoxelColors()));
    
    addActionToQMenuAndActionHash(renderDebugMenu, MenuOption::FalseColorEveryOtherVoxel, NULL, NULL);
    
    //        renderDebugMenu->addAction("FALSE Color Voxel Every Other Randomly", this, SLOT(doFalseRandomizeEveryOtherVoxelColors()));
    
    addActionToQMenuAndActionHash(renderDebugMenu, MenuOption::FalseColorByDistance, NULL, NULL);
    
    //        renderDebugMenu->addAction("FALSE Color Voxels by Distance", this, SLOT(doFalseColorizeByDistance()));
    
    addActionToQMenuAndActionHash(renderDebugMenu, MenuOption::FalseColorOutOfView, NULL, NULL);
    
    //        renderDebugMenu->addAction("FALSE Color Voxel Out of View", this, SLOT(doFalseColorizeInView()));
    
    addActionToQMenuAndActionHash(renderDebugMenu, MenuOption::FalseColorOccluded, NULL, NULL, Qt::CTRL | Qt::Key_O);
    
    //        renderDebugMenu->addAction("FALSE Color Occluded Voxels", this, SLOT(doFalseColorizeOccluded()), Qt::CTRL | Qt::Key_O);
    
    addActionToQMenuAndActionHash(renderDebugMenu, MenuOption::FalseColorOccludedV2, NULL, NULL, Qt::CTRL | Qt::Key_P);
    
    //        renderDebugMenu->addAction("FALSE Color Occluded V2 Voxels", this, SLOT(doFalseColorizeOccludedV2()), Qt::CTRL | Qt::Key_P);
    
    addActionToQMenuAndActionHash(renderDebugMenu, MenuOption::FalseColorBySource, NULL, NULL, Qt::CTRL | Qt::SHIFT | Qt::Key_S);
    
    //        renderDebugMenu->addAction("FALSE Color By Source", this, SLOT(doFalseColorizeBySource()), Qt::CTRL | Qt::SHIFT | Qt::Key_S);
    
    addActionToQMenuAndActionHash(renderDebugMenu, MenuOption::ShowTrueColors, NULL, NULL, Qt::CTRL | Qt::Key_T);
    
    //        renderDebugMenu->addAction("Show TRUE Colors", this, SLOT(doTrueVoxelColors()), Qt::CTRL | Qt::Key_T);
    
    addCheckableActionToQMenuAndActionHash(debugMenu, MenuOption::LowPassFilter);
    
    addCheckableActionToQMenuAndActionHash(debugMenu, MenuOption::Monochrome, NULL, NULL);
    
    //        debugMenu->addAction("Wants Monochrome", this, SLOT(setWantsMonochrome(bool)))->setCheckable(true);
    
    addCheckableActionToQMenuAndActionHash(debugMenu, MenuOption::DisableLowRes, NULL, NULL);
    
    //        debugMenu->addAction("Disable Lower Resolution While Moving", this, SLOT(disableLowResMoving(bool)))->setCheckable(true);
    
    addCheckableActionToQMenuAndActionHash(debugMenu, MenuOption::DisableDeltaSending, NULL, NULL);
    
    //        debugMenu->addAction("Disable Delta Sending", this, SLOT(disableDeltaSending(bool)))->setCheckable(true);
    
    addCheckableActionToQMenuAndActionHash(debugMenu, MenuOption::DisableOcclusionCulling, NULL, NULL, Qt::SHIFT | Qt::Key_C);
    //        (_occlusionCulling = debugMenu->addAction("Disable Occlusion Culling", this, SLOT(disableOcclusionCulling(bool)),
    //                                                  Qt::SHIFT | Qt::Key_C))->setCheckable(true);
    
    addCheckableActionToQMenuAndActionHash(debugMenu, MenuOption::CoverageMap, NULL, NULL, Qt::SHIFT | Qt::CTRL | Qt::Key_O);
    addCheckableActionToQMenuAndActionHash(debugMenu, MenuOption::CoverageMapV2, NULL, NULL, Qt::SHIFT | Qt::CTRL | Qt::Key_P);
    addCheckableActionToQMenuAndActionHash(debugMenu, MenuOption::SimulateLeapHand);
    addCheckableActionToQMenuAndActionHash(debugMenu, MenuOption::TestRaveGlove);
    
    QMenu* audioDebugMenu = debugMenu->addMenu("Audio Debugging Tools");
    addActionToQMenuAndActionHash(audioDebugMenu, MenuOption::ListenModeNormal, NULL, NULL, Qt::CTRL | Qt::Key_1);
    addActionToQMenuAndActionHash(audioDebugMenu, MenuOption::ListenModePoint, NULL, NULL, Qt::CTRL | Qt::Key_2);
    addActionToQMenuAndActionHash(audioDebugMenu, MenuOption::ListenModeSingleSource, NULL, NULL, Qt::CTRL | Qt::Key_3);
    
    // audioDebugMenu->addAction("Listen Mode Single Source", this, SLOT(setListenModeSingleSource()), Qt::CTRL | Qt::Key_3);
    
    QMenu* settingsMenu = addMenu("Settings");
    addCheckableActionToQMenuAndActionHash(settingsMenu, MenuOption::SettingsAutosave, NULL, NULL, 0, true);
    addActionToQMenuAndActionHash(settingsMenu, MenuOption::SettingsLoad, NULL, NULL);
    addActionToQMenuAndActionHash(settingsMenu, MenuOption::SettingsSave, NULL, NULL);
    addActionToQMenuAndActionHash(settingsMenu, MenuOption::SettingsImport, NULL, NULL);
    addActionToQMenuAndActionHash(settingsMenu, MenuOption::SettingsExport, NULL, NULL);
    
    //        _networkAccessManager = new QNetworkAccessManager(this);
}

void Menu::loadSettings(QSettings* settings) {
    if (!settings) {
        settings = Application::getInstance()->getSettings();
    }
    
    _gyroCameraSensitivity = loadSetting(settings, "gyroCameraSensitivity", 0.5f);
    _audioJitterBufferSamples = loadSetting(settings, "audioJitterBufferSamples", 0);
    _fieldOfView = loadSetting(settings, "fieldOfView", DEFAULT_FIELD_OF_VIEW_DEGREES);
    
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
}

void Menu::saveSettings(QSettings* settings) {
    if (!settings) {
        settings = Application::getInstance()->getSettings();
    }
    
    settings->setValue("gyroCameraSensitivity", _gyroCameraSensitivity);
    settings->setValue("audioJitterBufferSamples", _audioJitterBufferSamples);
    settings->setValue("fieldOfView", _fieldOfView);
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
    
    // ask the NodeList to save its data
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

QAction* Menu::addActionToQMenuAndActionHash(QMenu* destinationMenu,
                                             const QString actionName,
                                             const QObject* receiver,
                                             const char* member,
                                             const QKeySequence& shortcut) {
    QAction* action;
    
    if (receiver && member) {
        action = destinationMenu->addAction(actionName, receiver, member, shortcut);
    } else {
        action = destinationMenu->addAction(actionName);
        action->setShortcut(shortcut);
    }
    
    _actionHash.insert(actionName, action);
    
    return action;
}

QAction* Menu::addCheckableActionToQMenuAndActionHash(QMenu* destinationMenu,
                                                      const QString actionName,
                                                      const QObject* receiver,
                                                      const char* member,
                                                      const QKeySequence& shortcut,
                                                      const bool checked) {
    QAction* action = addActionToQMenuAndActionHash(destinationMenu, actionName, receiver, member, shortcut);
    action->setCheckable(true);
    action->setChecked(checked);
    
    return action;
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

void Menu::editPreferences() {
    Application *applicationInstance = Application::getInstance();
    QDialog dialog(applicationInstance->getGLWidget());
    dialog.setWindowTitle("Interface Preferences");
    QBoxLayout* layout = new QBoxLayout(QBoxLayout::TopToBottom);
    dialog.setLayout(layout);
    
    QFormLayout* form = new QFormLayout();
    layout->addLayout(form, 1);
    
    const int QLINE_MINIMUM_WIDTH = 400;
    
    QLineEdit* domainServerHostname = new QLineEdit(QString(NodeList::getInstance()->getDomainHostname()));
    domainServerHostname->setMinimumWidth(QLINE_MINIMUM_WIDTH);
    form->addRow("Domain server:", domainServerHostname);
    
    QLineEdit* avatarURL = new QLineEdit(applicationInstance->getAvatar()->getVoxels()->getVoxelURL().toString());
    avatarURL->setMinimumWidth(QLINE_MINIMUM_WIDTH);
    form->addRow("Avatar URL:", avatarURL);
    
    QSpinBox* fieldOfView = new QSpinBox();
    fieldOfView->setMaximum(180);
    fieldOfView->setMinimum(1);
    fieldOfView->setValue(_fieldOfView);
    form->addRow("Vertical Field of View (Degrees):", fieldOfView);
    
    QDoubleSpinBox* gyroCameraSensitivity = new QDoubleSpinBox();
    gyroCameraSensitivity->setValue(_gyroCameraSensitivity);
    form->addRow("Gyro Camera Sensitivity (0 - 1):", gyroCameraSensitivity);
    
    QDoubleSpinBox* leanScale = new QDoubleSpinBox();
    leanScale->setValue(applicationInstance->getAvatar()->getLeanScale());
    form->addRow("Lean Scale:", leanScale);
    
    QSpinBox* audioJitterBufferSamples = new QSpinBox();
    audioJitterBufferSamples->setMaximum(10000);
    audioJitterBufferSamples->setMinimum(-10000);
    audioJitterBufferSamples->setValue(_audioJitterBufferSamples);
    form->addRow("Audio Jitter Buffer Samples (0 for automatic):", audioJitterBufferSamples);
    
    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    dialog.connect(buttons, SIGNAL(accepted()), SLOT(accept()));
    dialog.connect(buttons, SIGNAL(rejected()), SLOT(reject()));
    layout->addWidget(buttons);
    
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    
    QByteArray newHostname;
    
    if (domainServerHostname->text().size() > 0) {
        // the user input a new hostname, use that
        newHostname = domainServerHostname->text().toLocal8Bit();
    } else {
        // the user left the field blank, use the default hostname
        newHostname = QByteArray(DEFAULT_DOMAIN_HOSTNAME);
    }
    
    // check if the domain server hostname is new
    if (memcmp(NodeList::getInstance()->getDomainHostname(), newHostname.constData(), newHostname.size()) != 0) {
        
        NodeList::getInstance()->clear();
        
        // kill the local voxels
        applicationInstance->getVoxels()->killLocalVoxels();
        
        // reset the environment to default
        applicationInstance->getEnvironment()->resetToDefault();
        
        // set the new hostname
        NodeList::getInstance()->setDomainHostname(newHostname.constData());
    }
    
    QUrl url(avatarURL->text());
    applicationInstance->getAvatar()->getVoxels()->setVoxelURL(url);
    Avatar::sendAvatarVoxelURLMessage(url);
    
    _gyroCameraSensitivity = gyroCameraSensitivity->value();
    
    applicationInstance->getAvatar()->setLeanScale(leanScale->value());
    
    _audioJitterBufferSamples = audioJitterBufferSamples->value();
    
    if (_audioJitterBufferSamples != 0) {
        applicationInstance->getAudio()->setJitterBufferSamples(_audioJitterBufferSamples);
    }
    
    _fieldOfView = fieldOfView->value();
    applicationInstance->resizeGL(applicationInstance->getGLWidget()->width(), applicationInstance->getGLWidget()->height());
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
    delete _bandwidthDialog;
    _bandwidthDialog = NULL;
}

void Menu::cycleFrustumRenderMode() {
    _frustumDrawMode = (FrustumDrawMode)((_frustumDrawMode + 1) % FRUSTUM_DRAW_MODE_COUNT);
    updateFrustumRenderModeAction();
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
//    _window->activateWindow();
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