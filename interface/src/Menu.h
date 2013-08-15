//
//  Menu.h
//  hifi
//
//  Created by Stephen Birarda on 8/12/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#ifndef __hifi__Menu__
#define __hifi__Menu__

#include <QMenuBar>
#include <QHash>
#include <QKeySequence>

enum FrustumDrawMode {
    FRUSTUM_DRAW_MODE_ALL,
    FRUSTUM_DRAW_MODE_VECTORS,
    FRUSTUM_DRAW_MODE_PLANES,
    FRUSTUM_DRAW_MODE_NEAR_PLANE,
    FRUSTUM_DRAW_MODE_FAR_PLANE,
    FRUSTUM_DRAW_MODE_KEYHOLE,
    FRUSTUM_DRAW_MODE_COUNT
};

struct ViewFrustumOffset {
    float yaw;
    float pitch;
    float roll;
    float distance;
    float up;
};

class Menu : public QMenuBar {
    Q_OBJECT
public:
    static Menu* getInstance();
    
    bool isOptionChecked(const QString& menuOption);
    void triggerOption(const QString& menuOption);
    QAction* getActionForOption(const QString& menuOption);
    bool isVoxelModeActionChecked();
    
    float getAudioJitterBufferSamples() const { return _audioJitterBufferSamples; }
    float getFieldOfView() const { return _fieldOfView; }
    float getGyroCameraSensitivity() const { return _gyroCameraSensitivity; }
    BandwidthDialog* getBandwidthDialog() const { return _bandwidthDialog; }
    FrustumDrawMode getFrustumDrawMode() const { return _frustumDrawMode; }
    ViewFrustumOffset getViewFrustumOffset() const { return _viewFrustumOffset; }
    VoxelStatsDialog* getVoxelStatsDialog() const { return _voxelStatsDialog; }
    
    void loadSettings(QSettings* settings = NULL);
    void saveSettings(QSettings* settings = NULL);
    void importSettings();
    void exportSettings();
    
    void handleViewFrustumOffsetKeyModifier(int key);
    
public slots:
    void bandwidthDetails();
    void voxelStatsDetails();
    
private slots:
    void editPreferences();
    void bandwidthDetailsClosed();
    void voxelStatsDetailsClosed();
    void cycleFrustumRenderMode();
    void chooseVoxelPaintColor();
    
private:
    static Menu* _instance;
    
    Menu();
    
    typedef void(*settingsAction)(QSettings*, QAction*);
    static void loadAction(QSettings* set, QAction* action);
    static void saveAction(QSettings* set, QAction* action);
    void scanMenuBar(settingsAction modifySetting, QSettings* set);
    void scanMenu(QMenu* menu, settingsAction modifySetting, QSettings* set);
    
    QAction* addActionToQMenuAndActionHash(QMenu* destinationMenu,
                                           const QString actionName,
                                           const QKeySequence& shortcut = 0,
                                           const QObject* receiver = NULL,
                                           const char* member = NULL);
    QAction* addCheckableActionToQMenuAndActionHash(QMenu* destinationMenu,
                                                    const QString actionName,
                                                    const QKeySequence& shortcut = 0,
                                                    const bool checked = false,
                                                    const QObject* receiver = NULL,
                                                    const char* member = NULL);
    
    void updateFrustumRenderModeAction();
    
    QHash<QString, QAction*> _actionHash;
    int _audioJitterBufferSamples; /// number of extra samples to wait before starting audio playback
    BandwidthDialog* _bandwidthDialog;
    float _fieldOfView; /// in Degrees, doesn't apply to HMD like Oculus
    FrustumDrawMode _frustumDrawMode;
    float _gyroCameraSensitivity;
    ViewFrustumOffset _viewFrustumOffset;
    QActionGroup* _voxelModeActionsGroup;
    VoxelStatsDialog* _voxelStatsDialog;
};

namespace MenuOption {
    
    const QString Avatars = "Avatars";
    const QString AvatarAsBalls = "Avatar as Balls";
    const QString Atmosphere = "Atmosphere";
    const QString Bandwidth = "Bandwidth Display";
    const QString BandwidthDetails = "Bandwidth Details";
    const QString Collisions = "Collisions";
    const QString CopyVoxels = "Copy Voxels";
    const QString CoverageMap = "Render Coverage Map";
    const QString CoverageMapV2 = "Render Coverage Map V2";
    const QString CutVoxels = "Cut Voxels";
    const QString DecreaseAvatarSize = "Decrease Avatar Size";
    const QString DecreaseVoxelSize = "Decrease Voxel Size";
    const QString DestructiveAddVoxel = "Create Voxel is Destructive";
    const QString DisableDeltaSending = "Disable Delta Sending";
    const QString DisableLowRes = "Disable Lower Resolution While Moving";
    const QString DisableOcclusionCulling = "Disable Occlusion Culling";
    const QString DisplayFrustum = "Display Frustum";
    const QString EchoAudio = "Echo Audio";
    const QString ExportVoxels = "Export Voxels";
    const QString HeadMouse = "Head Mouse";
    const QString FaceMode = "Cycle Face Mode";
    const QString FalseColorByDistance = "FALSE Color By Distance";
    const QString FalseColorBySource = "FALSE Color By Source";
    const QString FalseColorEveryOtherVoxel = "FALSE Color Every Other Randomly";
    const QString FalseColorOccluded = "FALSE Color Occluded Voxels";
    const QString FalseColorOccludedV2 = "FALSE Color Occluded V2 Voxels";
    const QString FalseColorOutOfView = "FALSE Color Voxel Out of View";
    const QString FalseColorRandomly = "FALSE Color Voxels Randomly";
    const QString FirstPerson = "First Person";
    const QString FrameTimer = "Show Timer";
    const QString FrustumRenderMode = "Render Mode";
    const QString Fullscreen = "Fullscreen";
    const QString ImportVoxels = "Import Voxels";
    const QString ImportVoxelsClipboard = "Import Voxels to Clipboard";
    const QString IncreaseAvatarSize = "Increase Avatar Size";
    const QString IncreaseVoxelSize = "Increase Voxel Size";
    const QString KillLocalVoxels = "Kill Local Voxels";
    const QString GoHome = "Go Home";
    const QString Gravity = "Use Gravity";
    const QString GroundPlane = "Ground Plane";
    const QString GyroLook = "Smooth Gyro Look";
    const QString ListenModeNormal = "Listen Mode Normal";
    const QString ListenModePoint = "Listen Mode Point";
    const QString ListenModeSingleSource = "Listen Mode Single Source";
    const QString Log = "Log";
    const QString LookAtIndicator = "Look-at Indicator";
    const QString LookAtVectors = "Look-at Vectors";
    const QString LowPassFilter = "Low-pass Filter";
    const QString Mirror = "Mirror";
    const QString Monochrome = "Monochrome";
    const QString Oscilloscope = "Audio Oscilloscope";
    const QString Pair = "Pair";
    const QString ParticleSystem = "Particle System";
    const QString PasteVoxels = "Paste Voxels";
    const QString PipelineWarnings = "Show Render Pipeline Warnings";
    const QString Preferences = "Preferences...";
    const QString RandomizeVoxelColors = "Randomize Voxel TRUE Colors";
    const QString ResetAvatarSize = "Reset Avatar Size";
    const QString ResetSwatchColors = "Reset Swatch Colors";
    const QString RunTimingTests = "Run Timing Tests";
    const QString SettingsAutosave = "Autosave";
    const QString SettingsLoad = "Load Settings";
    const QString SettingsSave = "Save Settings";
    const QString SettingsImport = "Import Settings";
    const QString SettingsExport = "Export Settings";
    const QString ShowTrueColors = "Show TRUE Colors";
    const QString SimulateLeapHand = "Simulate Leap Hand";
    const QString SkeletonTracking = "Skeleton Tracking";
    const QString Stars = "Stars";
    const QString Stats = "Stats";
    const QString TestPing = "Test Ping";
    const QString TestRaveGlove = "Test Rave Glove";
    const QString TreeStats = "Calculate Tree Stats";
    const QString TransmitterDrive = "Transmitter Drive";
    const QString Quit =  "Quit";
    const QString Webcam = "Webcam";
    const QString WebcamMode = "Cycle Webcam Send Mode";
    const QString WebcamTexture = "Webcam Texture";
    const QString Voxels = "Voxels";
    const QString VoxelAddMode = "Add Voxel Mode";
    const QString VoxelColorMode = "Color Voxel Mode";
    const QString VoxelDeleteMode = "Delete Voxel Mode";
    const QString VoxelGetColorMode = "Get Color Mode";
    const QString VoxelMode = "Cycle Voxel Mode";
    const QString VoxelPaintColor = "Voxel Paint Color";
    const QString VoxelSelectMode = "Select Voxel Mode";
    const QString VoxelStats = "Voxel Stats";
    const QString VoxelTextures = "Voxel Textures";
}

#endif /* defined(__hifi__Menu__) */
