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
#include <QPointer>

#include <AbstractMenuInterface.h>

const float ADJUST_LOD_DOWN_FPS = 40.0;
const float ADJUST_LOD_UP_FPS = 55.0;

const quint64 ADJUST_LOD_DOWN_DELAY = 1000 * 1000 * 5;
const quint64 ADJUST_LOD_UP_DELAY = ADJUST_LOD_DOWN_DELAY * 2;

const float ADJUST_LOD_DOWN_BY = 0.9f;
const float ADJUST_LOD_UP_BY = 1.1f;

const float ADJUST_LOD_MIN_SIZE_SCALE = TREE_SCALE * 1.0f;
const float ADJUST_LOD_MAX_SIZE_SCALE = DEFAULT_OCTREE_SIZE_SCALE;

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
class LodToolsDialog;
class MetavoxelEditor;
class VoxelStatsDialog;

class Menu : public QMenuBar, public AbstractMenuInterface {
    Q_OBJECT
public:
    static Menu* getInstance();
    ~Menu();

    bool isOptionChecked(const QString& menuOption);
    void setIsOptionChecked(const QString& menuOption, bool isChecked);
    void triggerOption(const QString& menuOption);
    QAction* getActionForOption(const QString& menuOption);
    bool isVoxelModeActionChecked();

    float getAudioJitterBufferSamples() const { return _audioJitterBufferSamples; }
    float getFieldOfView() const { return _fieldOfView; }
    float getFaceshiftEyeDeflection() const { return _faceshiftEyeDeflection; }
    BandwidthDialog* getBandwidthDialog() const { return _bandwidthDialog; }
    FrustumDrawMode getFrustumDrawMode() const { return _frustumDrawMode; }
    ViewFrustumOffset getViewFrustumOffset() const { return _viewFrustumOffset; }
    VoxelStatsDialog* getVoxelStatsDialog() const { return _voxelStatsDialog; }
    LodToolsDialog* getLodToolsDialog() const { return _lodToolsDialog; }
    int getMaxVoxels() const { return _maxVoxels; }
    QAction* getUseVoxelShader() const { return _useVoxelShader; }

    void handleViewFrustumOffsetKeyModifier(int key);

    // User Tweakable LOD Items
    void autoAdjustLOD(float currentFPS);
    void setVoxelSizeScale(float sizeScale);
    float getVoxelSizeScale() const { return _voxelSizeScale; }
    void setBoundaryLevelAdjust(int boundaryLevelAdjust);
    int getBoundaryLevelAdjust() const { return _boundaryLevelAdjust; }

    // User Tweakable PPS from Voxel Server
    int getMaxVoxelPacketsPerSecond() const { return _maxVoxelPacketsPerSecond; }

    virtual QMenu* getActiveScriptsMenu() { return _activeScriptsMenu;}
    virtual QAction* addActionToQMenuAndActionHash(QMenu* destinationMenu,
                                           const QString actionName,
                                           const QKeySequence& shortcut = 0,
                                           const QObject* receiver = NULL,
                                           const char* member = NULL,
                                           QAction::MenuRole role = QAction::NoRole);
    virtual void removeAction(QMenu* menu, const QString& actionName);
    bool goToDestination(QString destination);
    void goToOrientation(QString orientation);
    void goToDomain(const QString newDomain);

public slots:
    void loginForCurrentDomain();
    void bandwidthDetails();
    void voxelStatsDetails();
    void lodTools();
    void loadSettings(QSettings* settings = NULL);
    void saveSettings(QSettings* settings = NULL);
    void importSettings();
    void exportSettings();
    void goTo();
    void pasteToVoxel();
    
    void toggleLoginMenuItem();

private slots:
    void aboutApp();
    void editPreferences();
    void goToDomainDialog();
    void goToLocation();
    void bandwidthDetailsClosed();
    void voxelStatsDetailsClosed();
    void lodToolsClosed();
    void cycleFrustumRenderMode();
    void updateVoxelModeActions();
    void chooseVoxelPaintColor();
    void runTests();
    void resetSwatchColors();
    void showMetavoxelEditor();
    void audioMuteToggled();

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

    QAction* addCheckableActionToQMenuAndActionHash(QMenu* destinationMenu,
                                                    const QString actionName,
                                                    const QKeySequence& shortcut = 0,
                                                    const bool checked = false,
                                                    const QObject* receiver = NULL,
                                                    const char* member = NULL);

    void updateFrustumRenderModeAction();

    void addAvatarCollisionSubMenu(QMenu* overMenu);

    QHash<QString, QAction*> _actionHash;
    int _audioJitterBufferSamples; /// number of extra samples to wait before starting audio playback
    BandwidthDialog* _bandwidthDialog;
    float _fieldOfView; /// in Degrees, doesn't apply to HMD like Oculus
    float _faceshiftEyeDeflection;
    FrustumDrawMode _frustumDrawMode;
    ViewFrustumOffset _viewFrustumOffset;
    QActionGroup* _voxelModeActionsGroup;
    QPointer<MetavoxelEditor> _MetavoxelEditor;
    VoxelStatsDialog* _voxelStatsDialog;
    LodToolsDialog* _lodToolsDialog;
    int _maxVoxels;
    float _voxelSizeScale;
    int _boundaryLevelAdjust;
    QAction* _useVoxelShader;
    int _maxVoxelPacketsPerSecond;
    QMenu* _activeScriptsMenu;
    QString replaceLastOccurrence(QChar search, QChar replace, QString string);
    quint64 _lastAdjust;
    QAction* _loginAction;
};

namespace MenuOption {
    const QString AboutApp = "About Interface";
    const QString AmbientOcclusion = "Ambient Occlusion";
    const QString Avatars = "Avatars";
    const QString Atmosphere = "Atmosphere";
    const QString AutoAdjustLOD = "Automatically Adjust LOD";
    const QString AutomaticallyAuditTree = "Automatically Audit Tree Stats";
    const QString Bandwidth = "Bandwidth Display";
    const QString BandwidthDetails = "Bandwidth Details";
    const QString BuckyBalls = "Bucky Balls";
    const QString ChatCircling = "Chat Circling";
    const QString Collisions = "Collisions";
    const QString CollideWithAvatars = "Collide With Avatars";
    const QString CollideWithParticles = "Collide With Particles";
    const QString CollideWithVoxels = "Collide With Voxels";
    const QString CollideWithEnvironment = "Collide With World Boundaries";
    const QString CopyVoxels = "Copy";
    const QString CoverageMap = "Render Coverage Map";
    const QString CoverageMapV2 = "Render Coverage Map V2";
    const QString CullSharedFaces = "Cull Shared Voxel Faces";
    const QString CutVoxels = "Cut";
    const QString DecreaseAvatarSize = "Decrease Avatar Size";
    const QString DecreaseVoxelSize = "Decrease Voxel Size";
    const QString DeleteVoxels = "Delete";
    const QString DestructiveAddVoxel = "Create Voxel is Destructive";
    const QString DisableColorVoxels = "Disable Colored Voxels";
    const QString DisableDeltaSending = "Disable Delta Sending";
    const QString DisableLowRes = "Disable Lower Resolution While Moving";
    const QString DisplayFrustum = "Display Frustum";
    const QString DisplayHands = "Display Hands";
    const QString DisplayHandTargets = "Display Hand Targets";
    const QString FilterSixense = "Smooth Sixense Movement";
    const QString DontRenderVoxels = "Don't call _voxels.render()";
    const QString DontCallOpenGLForVoxels = "Don't call glDrawRangeElementsEXT() for Voxels";
    const QString Enable3DTVMode = "Enable 3DTV Mode";
    const QString EnableOcclusionCulling = "Enable Occlusion Culling";
    const QString EnableVoxelPacketCompression = "Enable Voxel Packet Compression";
    const QString AudioNoiseReduction = "Audio Noise Reduction";
    const QString EchoServerAudio = "Echo Server Audio";
    const QString EchoLocalAudio = "Echo Local Audio";
    const QString MuteAudio = "Mute Microphone";
    const QString ExportVoxels = "Export Voxels";
    const QString DontFadeOnVoxelServerChanges = "Don't Fade In/Out on Voxel Server Changes";
    const QString HeadMouse = "Head Mouse";
    const QString HandsCollideWithSelf = "Collide With Self";
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
    const QString GoTo = "Go To...";
    const QString ImportVoxels = "Import Voxels";
    const QString ImportVoxelsClipboard = "Import Voxels to Clipboard";
    const QString IncreaseAvatarSize = "Increase Avatar Size";
    const QString IncreaseVoxelSize = "Increase Voxel Size";
    const QString KillLocalVoxels = "Kill Local Voxels";
    const QString GoHome = "Go Home";
    const QString Gravity = "Use Gravity";
    const QString LodTools = "LOD Tools";
    const QString Log = "Log";
    const QString Login = "Login";
    const QString Logout = "Logout";
    const QString LookAtVectors = "Look-at Vectors";
    const QString MetavoxelEditor = "Metavoxel Editor...";
    const QString Metavoxels = "Metavoxels";
    const QString Mirror = "Mirror";
    const QString MoveWithLean = "Move with Lean";
    const QString NewVoxelCullingMode = "New Voxel Culling Mode";
    const QString NudgeVoxels = "Nudge";
    const QString OffAxisProjection = "Off-Axis Projection";
    const QString OldVoxelCullingMode = "Old Voxel Culling Mode";
    const QString TurnWithHead = "Turn using Head";
    const QString ClickToFly = "Fly to voxel on click";
    const QString LoadScript = "Open and Run Script...";
    const QString Oscilloscope = "Audio Oscilloscope";
    const QString Pair = "Pair";
    const QString Particles = "Particles";
    const QString PasteVoxels = "Paste";
    const QString PasteToVoxel = "Paste to Voxel...";
    const QString PipelineWarnings = "Show Render Pipeline Warnings";
    const QString PlaySlaps = "Play Slaps";
    const QString Preferences = "Preferences...";
    const QString RandomizeVoxelColors = "Randomize Voxel TRUE Colors";
    const QString ReloadAllScripts = "Reload All Scripts";
    const QString RenderSkeletonCollisionProxies = "Skeleton Collision Proxies";
    const QString RenderHeadCollisionProxies = "Head Collision Proxies";
    const QString ResetAvatarSize = "Reset Avatar Size";
    const QString ResetSwatchColors = "Reset Swatch Colors";
    const QString RunTimingTests = "Run Timing Tests";
    const QString SettingsImport = "Import Settings";
    const QString Shadows = "Shadows";
    const QString SettingsExport = "Export Settings";
    const QString ShowAllLocalVoxels = "Show All Local Voxels";
    const QString ShowCulledSharedFaces = "Show Culled Shared Voxel Faces";
    const QString ShowTrueColors = "Show TRUE Colors";
    const QString SuppressShortTimings = "Suppress Timings Less than 10ms";
    const QString Stars = "Stars";
    const QString Stats = "Stats";
    const QString StopAllScripts = "Stop All Scripts";
    const QString TestPing = "Test Ping";
    const QString TreeStats = "Calculate Tree Stats";
    const QString TransmitterDrive = "Transmitter Drive";
    const QString Quit =  "Quit";
    const QString UseVoxelShader = "Use Voxel Shader";
    const QString VoxelsAsPoints = "Draw Voxels as Points";
    const QString Voxels = "Voxels";
    const QString VoxelAddMode = "Add Voxel Mode";
    const QString VoxelColorMode = "Color Voxel Mode";
    const QString VoxelDeleteMode = "Delete Voxel Mode";
    const QString VoxelDrumming = "Voxel Drumming";
    const QString VoxelGetColorMode = "Get Color Mode";
    const QString VoxelMode = "Cycle Voxel Mode";
    const QString VoxelPaintColor = "Voxel Paint Color";
    const QString VoxelSelectMode = "Select Voxel Mode";
    const QString VoxelStats = "Voxel Stats";
    const QString VoxelTextures = "Voxel Textures";
}

#endif /* defined(__hifi__Menu__) */
