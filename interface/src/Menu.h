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

class QSettings;

class BandwidthDialog;
class VoxelStatsDialog;
class LodToolsDialog;

class Menu : public QMenuBar {
    Q_OBJECT
public:
    static Menu* getInstance();
    ~Menu();
    
    bool isOptionChecked(const QString& menuOption);
    void triggerOption(const QString& menuOption);
    QAction* getActionForOption(const QString& menuOption);
    bool isVoxelModeActionChecked();
    
    float getAudioJitterBufferSamples() const { return _audioJitterBufferSamples; }
    float getFieldOfView() const { return _fieldOfView; }
    BandwidthDialog* getBandwidthDialog() const { return _bandwidthDialog; }
    FrustumDrawMode getFrustumDrawMode() const { return _frustumDrawMode; }
    ViewFrustumOffset getViewFrustumOffset() const { return _viewFrustumOffset; }
    VoxelStatsDialog* getVoxelStatsDialog() const { return _voxelStatsDialog; }
    LodToolsDialog* getLodToolsDialog() const { return _lodToolsDialog; }
    int getMaxVoxels() const { return _maxVoxels; }
    QAction* getUseVoxelShader() const { return _useVoxelShader; }

    
    void handleViewFrustumOffsetKeyModifier(int key);

    // User Tweakable LOD Items
    void setVoxelSizeScale(float sizeScale);
    float getVoxelSizeScale() const { return _voxelSizeScale; }
    void setBoundaryLevelAdjust(int boundaryLevelAdjust);
    int getBoundaryLevelAdjust() const { return _boundaryLevelAdjust; }
    
public slots:
    void bandwidthDetails();
    void voxelStatsDetails();
    void lodTools();
    void loadSettings(QSettings* settings = NULL);
    void saveSettings(QSettings* settings = NULL);
    void importSettings();
    void exportSettings();
    void goToUser();
    
private slots:
    void aboutApp();
    void login();
    void editPreferences();
    void goToDomain();
    void goToLocation();
    void bandwidthDetailsClosed();
    void voxelStatsDetailsClosed();
    void lodToolsClosed();
    void cycleFrustumRenderMode();
    void updateVoxelModeActions();
    void chooseVoxelPaintColor();
    void runTests();
    void resetSwatchColors();
    void setOldVoxelCullingMode(bool oldMode);
    void setNewVoxelCullingMode(bool newMode);
    
private:
    static Menu* _instance;
    
    Menu();
    
    typedef void(*settingsAction)(QSettings*, QAction*);
    static void loadAction(QSettings* set, QAction* action);
    static void saveAction(QSettings* set, QAction* action);
    void scanMenuBar(settingsAction modifySetting, QSettings* set);
    void scanMenu(QMenu* menu, settingsAction modifySetting, QSettings* set);
    
    /// helper method to have separators with labels that are also compatible with OS X
    void addDisabledActionAndSeparator(QMenu* destinationMenu, const QString& actionName);
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
    void setVoxelCullingMode(bool oldMode);
    
    QHash<QString, QAction*> _actionHash;
    int _audioJitterBufferSamples; /// number of extra samples to wait before starting audio playback
    BandwidthDialog* _bandwidthDialog;
    float _fieldOfView; /// in Degrees, doesn't apply to HMD like Oculus
    FrustumDrawMode _frustumDrawMode;
    ViewFrustumOffset _viewFrustumOffset;
    QActionGroup* _voxelModeActionsGroup;
    VoxelStatsDialog* _voxelStatsDialog;
    LodToolsDialog* _lodToolsDialog;
    int _maxVoxels;
    float _voxelSizeScale;
    int _boundaryLevelAdjust;
    QAction* _useVoxelShader;
};

namespace MenuOption {
    const QString AboutApp = "About Interface";
    const QString AmbientOcclusion = "Ambient Occlusion";
    const QString Avatars = "Avatars";
    const QString AvatarAsBalls = "Avatar as Balls";
    const QString Atmosphere = "Atmosphere";
    const QString AutomaticallyAuditTree = "Automatically Audit Tree Stats";
    const QString Bandwidth = "Bandwidth Display";
    const QString BandwidthDetails = "Bandwidth Details";
    const QString Collisions = "Collisions";
    const QString CopyVoxels = "Copy";
    const QString CoverageMap = "Render Coverage Map";
    const QString CoverageMapV2 = "Render Coverage Map V2";
    const QString CutVoxels = "Cut";
    const QString DecreaseAvatarSize = "Decrease Avatar Size";
    const QString DecreaseVoxelSize = "Decrease Voxel Size";
    const QString DeleteVoxels = "Delete";
    const QString DestructiveAddVoxel = "Create Voxel is Destructive";
    const QString DeltaSending = "Delta Sending";
    const QString DisableConstantCulling = "Disable Constant Culling";
    const QString DisableFastVoxelPipeline = "Disable Fast Voxel Pipeline";
    const QString DisplayFrustum = "Display Frustum";
    const QString DontRenderVoxels = "Don't call _voxels.render()";
    const QString DontCallOpenGLForVoxels = "Don't call glDrawRangeElementsEXT() for Voxels";
    const QString EchoAudio = "Echo Audio";
    const QString ExportVoxels = "Export Voxels";
    const QString ExtraDebugging = "Extra Debugging";
    const QString HeadMouse = "Head Mouse";
    const QString FaceMode = "Cycle Face Mode";
    const QString FaceshiftTCP = "Faceshift (TCP)";
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
    const QString FullscreenMirror = "Fullscreen Mirror";
    const QString GlowMode = "Cycle Glow Mode";
    const QString GoToDomain = "Go To Domain...";
    const QString GoToLocation = "Go To Location...";
    const QString DisableHideOutOfView = "Disable Hide Out of View Voxels";
    const QString GoToUser = "Go To User...";
    const QString ImportVoxels = "Import Voxels";
    const QString ImportVoxelsClipboard = "Import Voxels to Clipboard";
    const QString IncreaseAvatarSize = "Increase Avatar Size";
    const QString IncreaseVoxelSize = "Increase Voxel Size";
    const QString KillLocalVoxels = "Kill Local Voxels";
    const QString GoHome = "Go Home";
    const QString Gravity = "Use Gravity";
    const QString ParticleCloud = "Particle Cloud";
    const QString LodTools = "LOD Tools";
    const QString Log = "Log";
    const QString Login = "Login";
    const QString LookAtIndicator = "Look-at Indicator";
    const QString LookAtVectors = "Look-at Vectors";
    const QString LowRes = "Lower Resolution While Moving";
    const QString Mirror = "Mirror";
    const QString NewVoxelCullingMode = "New Voxel Culling Mode";
    const QString NudgeVoxels = "Nudge";
    const QString OcclusionCulling = "Occlusion Culling";
    const QString OffAxisProjection = "Off-Axis Projection";
    const QString OldVoxelCullingMode = "Old Voxel Culling Mode";
    const QString TurnWithHead = "Turn using Head";
    const QString Oscilloscope = "Audio Oscilloscope";
    const QString Pair = "Pair";
    const QString PasteVoxels = "Paste";
    const QString PipelineWarnings = "Show Render Pipeline Warnings";
    const QString Preferences = "Preferences...";
    const QString RandomizeVoxelColors = "Randomize Voxel TRUE Colors";
    const QString RemoveOutOfView = "Instead of Hide Remove Out of View Voxels";
    const QString ResetAvatarSize = "Reset Avatar Size";
    const QString ResetSwatchColors = "Reset Swatch Colors";
    const QString RunTimingTests = "Run Timing Tests";
    const QString SendVoxelColors = "Colored Voxels";
    const QString SettingsImport = "Import Settings";
    const QString SettingsExport = "Export Settings";
    const QString ShowAllLocalVoxels = "Show All Local Voxels";
    const QString ShowTrueColors = "Show TRUE Colors";
    const QString SimulateLeapHand = "Simulate Leap Hand";
    const QString SkeletonTracking = "Skeleton Tracking";
    const QString SuppressShortTimings = "Suppress Timings Less than 10ms";
    const QString LEDTracking = "LED Tracking";
    const QString Stars = "Stars";
    const QString Stats = "Stats";
    const QString TestPing = "Test Ping";
    const QString TestRaveGlove = "Test Rave Glove";
    const QString TreeStats = "Calculate Tree Stats";
    const QString TransmitterDrive = "Transmitter Drive";
    const QString Quit =  "Quit";
    const QString UseFullFrustumInHide = "Use Full View Frustums when Culling";
    const QString UseVoxelShader = "Use Voxel Shader";
    const QString VoxelsAsPoints = "Draw Voxels as Points";
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
    const QString Webcam = "Webcam";
    const QString WebcamMode = "Cycle Webcam Send Mode";
    const QString WebcamTexture = "Webcam Texture";
}

#endif /* defined(__hifi__Menu__) */
