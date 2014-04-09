//
//  Menu.h
//  interface/src
//
//  Created by Stephen Birarda on 8/12/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Menu_h
#define hifi_Menu_h

#include <QMenuBar>
#include <QHash>
#include <QKeySequence>
#include <QPointer>

#include <EventTypes.h>
#include <MenuItemProperties.h>
#include <OctreeConstants.h>

#include "location/LocationManager.h"
#include "ui/ChatWindow.h"

const float ADJUST_LOD_DOWN_FPS = 40.0;
const float ADJUST_LOD_UP_FPS = 55.0;

const quint64 ADJUST_LOD_DOWN_DELAY = 1000 * 1000 * 5;
const quint64 ADJUST_LOD_UP_DELAY = ADJUST_LOD_DOWN_DELAY * 2;

const float ADJUST_LOD_DOWN_BY = 0.9f;
const float ADJUST_LOD_UP_BY = 1.1f;

const float ADJUST_LOD_MIN_SIZE_SCALE = DEFAULT_OCTREE_SIZE_SCALE * 0.25f;
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
class ChatWindow;
class OctreeStatsDialog;
class MenuItemProperties;

class Menu : public QMenuBar {
    Q_OBJECT
public:
    static Menu* getInstance();
    ~Menu();

    void triggerOption(const QString& menuOption);
    QAction* getActionForOption(const QString& menuOption);

    float getAudioJitterBufferSamples() const { return _audioJitterBufferSamples; }
    float getFieldOfView() const { return _fieldOfView; }
    float getFaceshiftEyeDeflection() const { return _faceshiftEyeDeflection; }
    BandwidthDialog* getBandwidthDialog() const { return _bandwidthDialog; }
    FrustumDrawMode getFrustumDrawMode() const { return _frustumDrawMode; }
    ViewFrustumOffset getViewFrustumOffset() const { return _viewFrustumOffset; }
    OctreeStatsDialog* getOctreeStatsDialog() const { return _octreeStatsDialog; }
    LodToolsDialog* getLodToolsDialog() const { return _lodToolsDialog; }
    int getMaxVoxels() const { return _maxVoxels; }
    QAction* getUseVoxelShader() const { return _useVoxelShader; }

    void handleViewFrustumOffsetKeyModifier(int key);

    // User Tweakable LOD Items
    QString getLODFeedbackText();
    void autoAdjustLOD(float currentFPS);
    void resetLODAdjust();
    void setVoxelSizeScale(float sizeScale);
    float getVoxelSizeScale() const { return _voxelSizeScale; }
    float getAvatarLODDistanceMultiplier() const { return _avatarLODDistanceMultiplier; }
    void setBoundaryLevelAdjust(int boundaryLevelAdjust);
    int getBoundaryLevelAdjust() const { return _boundaryLevelAdjust; }

    // User Tweakable PPS from Voxel Server
    int getMaxVoxelPacketsPerSecond() const { return _maxVoxelPacketsPerSecond; }

    QAction* addActionToQMenuAndActionHash(QMenu* destinationMenu,
                                           const QString& actionName,
                                           const QKeySequence& shortcut = 0,
                                           const QObject* receiver = NULL,
                                           const char* member = NULL,
                                           QAction::MenuRole role = QAction::NoRole,
                                           int menuItemLocation = UNSPECIFIED_POSITION);
    QAction* addActionToQMenuAndActionHash(QMenu* destinationMenu,
                                           QAction* action,
                                           const QString& actionName = QString(),
                                           const QKeySequence& shortcut = 0,
                                           QAction::MenuRole role = QAction::NoRole,
                                           int menuItemLocation = UNSPECIFIED_POSITION);
    
    void removeAction(QMenu* menu, const QString& actionName);

    bool goToDestination(QString destination);
    void goToOrientation(QString orientation);
    void goToDomain(const QString newDomain);
    void goTo(QString destination);

public slots:

    void loginForCurrentDomain();
    void bandwidthDetails();
    void octreeStatsDetails();
    void lodTools();
    void loadSettings(QSettings* settings = NULL);
    void saveSettings(QSettings* settings = NULL);
    void importSettings();
    void exportSettings();
    void goTo();
    void goToUser(const QString& user);
    void pasteToVoxel();

    void toggleLoginMenuItem();

    QMenu* addMenu(const QString& menuName);
    void removeMenu(const QString& menuName);
    void addSeparator(const QString& menuName, const QString& separatorName);
    void removeSeparator(const QString& menuName, const QString& separatorName);
    void addMenuItem(const MenuItemProperties& properties);
    void removeMenuItem(const QString& menuName, const QString& menuitem);
    bool isOptionChecked(const QString& menuOption);
    void setIsOptionChecked(const QString& menuOption, bool isChecked);

private slots:
    void aboutApp();
    void editPreferences();
    void goToDomainDialog();
    void goToLocation();
    void nameLocation();
    void bandwidthDetailsClosed();
    void octreeStatsDetailsClosed();
    void lodToolsClosed();
    void cycleFrustumRenderMode();
    void runTests();
    void showMetavoxelEditor();
    void showChat();
    void toggleChat();
    void audioMuteToggled();
    void namedLocationCreated(LocationManager::NamedLocationCreateResponse response);
    void multipleDestinationsDecision(const QJsonObject& userData, const QJsonObject& placeData);

private:
    static Menu* _instance;

    Menu();

    typedef void(*settingsAction)(QSettings*, QAction*);
    static void loadAction(QSettings* set, QAction* action);
    static void saveAction(QSettings* set, QAction* action);
    void scanMenuBar(settingsAction modifySetting, QSettings* set);
    void scanMenu(QMenu* menu, settingsAction modifySetting, QSettings* set);

    /// helper method to have separators with labels that are also compatible with OS X
    void addDisabledActionAndSeparator(QMenu* destinationMenu, const QString& actionName,
                                                int menuItemLocation = UNSPECIFIED_POSITION);

    QAction* addCheckableActionToQMenuAndActionHash(QMenu* destinationMenu,
                                                    const QString& actionName,
                                                    const QKeySequence& shortcut = 0,
                                                    const bool checked = false,
                                                    const QObject* receiver = NULL,
                                                    const char* member = NULL,
                                                    int menuItemLocation = UNSPECIFIED_POSITION);

    void updateFrustumRenderModeAction();

    void addAvatarCollisionSubMenu(QMenu* overMenu);

    QAction* getActionFromName(const QString& menuName, QMenu* menu);
    QMenu* getSubMenuFromName(const QString& menuName, QMenu* menu);
    QMenu* getMenuParent(const QString& menuName, QString& finalMenuPart);

    QAction* getMenuAction(const QString& menuName);
    int findPositionOfMenuItem(QMenu* menu, const QString& searchMenuItem);
    int positionBeforeSeparatorIfNeeded(QMenu* menu, int requestedPosition);
    QMenu* getMenu(const QString& menuName);


    QHash<QString, QAction*> _actionHash;
    int _audioJitterBufferSamples; /// number of extra samples to wait before starting audio playback
    BandwidthDialog* _bandwidthDialog;
    float _fieldOfView; /// in Degrees, doesn't apply to HMD like Oculus
    float _faceshiftEyeDeflection;
    FrustumDrawMode _frustumDrawMode;
    ViewFrustumOffset _viewFrustumOffset;
    QPointer<MetavoxelEditor> _MetavoxelEditor;
    QPointer<ChatWindow> _chatWindow;
    OctreeStatsDialog* _octreeStatsDialog;
    LodToolsDialog* _lodToolsDialog;
    int _maxVoxels;
    float _voxelSizeScale;
    float _avatarLODDistanceMultiplier;
    int _boundaryLevelAdjust;
    QAction* _useVoxelShader;
    int _maxVoxelPacketsPerSecond;
    QString replaceLastOccurrence(QChar search, QChar replace, QString string);
    quint64 _lastAdjust;
    quint64 _lastAvatarDetailDrop;
    SimpleMovingAverage _fpsAverage;
    SimpleMovingAverage _fastFPSAverage;
    QAction* _loginAction;
    QAction* _chatAction;
};

namespace MenuOption {
    const QString AboutApp = "About Interface";
    const QString AmbientOcclusion = "Ambient Occlusion";
    const QString Atmosphere = "Atmosphere";
    const QString AudioNoiseReduction = "Audio Noise Reduction";
    const QString AudioToneInjection = "Inject Test Tone";
    const QString Avatars = "Avatars";
    const QString Bandwidth = "Bandwidth Display";
    const QString BandwidthDetails = "Bandwidth Details";
    const QString BuckyBalls = "Bucky Balls";
    const QString Chat = "Chat...";
    const QString ChatCircling = "Chat Circling";
    const QString CollideWithAvatars = "Collide With Avatars";
    const QString CollideWithEnvironment = "Collide With World Boundaries";
    const QString CollideWithParticles = "Collide With Particles";
    const QString CollideWithVoxels = "Collide With Voxels";
    const QString Collisions = "Collisions";
    const QString CullSharedFaces = "Cull Shared Voxel Faces";
    const QString DecreaseAvatarSize = "Decrease Avatar Size";
    const QString DecreaseVoxelSize = "Decrease Voxel Size";
    const QString DisableAutoAdjustLOD = "Disable Automatically Adjusting LOD";
    const QString DisplayFrustum = "Display Frustum";
    const QString DisplayHands = "Display Hands";
    const QString DisplayHandTargets = "Display Hand Targets";
    const QString DontFadeOnVoxelServerChanges = "Don't Fade In/Out on Voxel Server Changes";
    const QString EchoLocalAudio = "Echo Local Audio";
    const QString EchoServerAudio = "Echo Server Audio";
    const QString Enable3DTVMode = "Enable 3DTV Mode";
    const QString Faceshift = "Faceshift";
    const QString FilterSixense = "Smooth Sixense Movement";
    const QString FirstPerson = "First Person";
    const QString FrameTimer = "Show Timer";
    const QString FrustumRenderMode = "Render Mode";
    const QString Fullscreen = "Fullscreen";
    const QString FullscreenMirror = "Fullscreen Mirror";
    const QString GlowMode = "Cycle Glow Mode";
    const QString GoHome = "Go Home";
    const QString GoTo = "Go To...";
    const QString GoToDomain = "Go To Domain...";
    const QString GoToLocation = "Go To Location...";
    const QString Gravity = "Use Gravity";
    const QString HandsCollideWithSelf = "Collide With Self";
    const QString HeadMouse = "Head Mouse";
    const QString IncreaseAvatarSize = "Increase Avatar Size";
    const QString IncreaseVoxelSize = "Increase Voxel Size";
    const QString LoadScript = "Open and Run Script File...";
    const QString LoadScriptURL = "Open and Run Script from URL...";
    const QString LodTools = "LOD Tools";
    const QString Log = "Log";
    const QString Login = "Login";
    const QString Logout = "Logout";
    const QString LookAtVectors = "Look-at Vectors";
    const QString MetavoxelEditor = "Metavoxel Editor...";
    const QString Metavoxels = "Metavoxels";
    const QString Mirror = "Mirror";
    const QString MoveWithLean = "Move with Lean";
    const QString MuteAudio = "Mute Microphone";
    const QString NameLocation = "Name this location";
    const QString NewVoxelCullingMode = "New Voxel Culling Mode";
    const QString OctreeStats = "Voxel and Particle Statistics";
    const QString OffAxisProjection = "Off-Axis Projection";
    const QString OldVoxelCullingMode = "Old Voxel Culling Mode";
    const QString Oscilloscope = "Audio Oscilloscope";
    const QString Pair = "Pair";
    const QString Particles = "Particles";
    const QString PasteToVoxel = "Paste to Voxel...";
    const QString PipelineWarnings = "Show Render Pipeline Warnings";
    const QString PlaySlaps = "Play Slaps";
    const QString Preferences = "Preferences...";
    const QString Quit =  "Quit";
    const QString ReloadAllScripts = "Reload All Scripts";
    const QString RenderBoundingCollisionShapes = "Bounding Collision Shapes";
    const QString RenderHeadCollisionShapes = "Head Collision Shapes";
    const QString RenderSkeletonCollisionShapes = "Skeleton Collision Shapes";
    const QString ResetAvatarSize = "Reset Avatar Size";
    const QString RunningScripts = "Running Scripts";
    const QString RunTimingTests = "Run Timing Tests";
    const QString SettingsExport = "Export Settings";
    const QString SettingsImport = "Import Settings";
    const QString Shadows = "Shadows";
    const QString ShowCulledSharedFaces = "Show Culled Shared Voxel Faces";
    const QString Stars = "Stars";
    const QString Stats = "Stats";
    const QString StopAllScripts = "Stop All Scripts";
    const QString SuppressShortTimings = "Suppress Timings Less than 10ms";
    const QString TestPing = "Test Ping";
    const QString TransmitterDrive = "Transmitter Drive";
    const QString TurnWithHead = "Turn using Head";
    const QString UploadHead = "Upload Head Model";
    const QString UploadSkeleton = "Upload Skeleton Model";
    const QString Visage = "Visage";
    const QString VoxelMode = "Cycle Voxel Mode";
    const QString Voxels = "Voxels";
    const QString VoxelTextures = "Voxel Textures";
}

void sendFakeEnterEvent();

#endif // hifi_Menu_h
