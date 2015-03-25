//
//  LODManager.h
//
//
//  Created by Clement on 1/16/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_LODManager_h
#define hifi_LODManager_h

#include <DependencyManager.h>
#include <OctreeConstants.h>
#include <SharedUtil.h>
#include <SimpleMovingAverage.h>

const float DEFAULT_DESKTOP_LOD_DOWN_FPS = 30.0;
const float DEFAULT_HMD_LOD_DOWN_FPS = 60.0;
const float INCREASE_LOD_GAP = 5.0f;

const quint64 ADJUST_LOD_DOWN_DELAY = 1000 * 1000 * 0.5; // Consider adjusting LOD down after half a second
const quint64 ADJUST_LOD_UP_DELAY = ADJUST_LOD_DOWN_DELAY * 2;

const float ADJUST_LOD_DOWN_BY = 0.9f;
const float ADJUST_LOD_UP_BY = 1.1f;

// This controls how low the auto-adjust LOD will go a value of 1 means it will adjust to a point where you must be 0.25
// meters away from an object of TREE_SCALE before you can see it (which is effectively completely blind). The default value
// DEFAULT_OCTREE_SIZE_SCALE means you can be 400 meters away from a 1 meter object in order to see it (which is ~20:20 vision).
const float ADJUST_LOD_MIN_SIZE_SCALE = 1.0f;
const float ADJUST_LOD_MAX_SIZE_SCALE = DEFAULT_OCTREE_SIZE_SCALE;

// The ratio of "visibility" of avatars to other content. A value larger than 1 will mean Avatars "cull" later than entities
// do. But both are still culled using the same angular size logic.
const float AVATAR_TO_ENTITY_RATIO = 2.0f;

const int ONE_SECOND_OF_FRAMES = 60;
const int FIVE_SECONDS_OF_FRAMES = 5 * ONE_SECOND_OF_FRAMES;


class LODManager : public QObject, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY
    
public:
    Q_INVOKABLE void setAutomaticLODAdjust(bool value) { _automaticLODAdjust = value; }
    Q_INVOKABLE bool getAutomaticLODAdjust() const { return _automaticLODAdjust; }

    Q_INVOKABLE void setDesktopLODDecreaseFPS(float value) { _desktopLODDecreaseFPS = value; }
    Q_INVOKABLE float getDesktopLODDecreaseFPS() const { return _desktopLODDecreaseFPS; }
    Q_INVOKABLE float getDesktopLODIncreaseFPS() const { return _desktopLODDecreaseFPS + INCREASE_LOD_GAP; }

    Q_INVOKABLE void setHMDLODDecreaseFPS(float value) { _hmdLODDecreaseFPS = value; }
    Q_INVOKABLE float getHMDLODDecreaseFPS() const { return _hmdLODDecreaseFPS; }
    Q_INVOKABLE float getHMDLODIncreaseFPS() const { return _hmdLODDecreaseFPS + INCREASE_LOD_GAP; }

    Q_INVOKABLE float getAvatarLODDistanceMultiplier() const { return _avatarLODDistanceMultiplier; }
    
    // User Tweakable LOD Items
    Q_INVOKABLE QString getLODFeedbackText();
    Q_INVOKABLE void setOctreeSizeScale(float sizeScale);
    Q_INVOKABLE float getOctreeSizeScale() const { return _octreeSizeScale; }
    
    Q_INVOKABLE void setBoundaryLevelAdjust(int boundaryLevelAdjust);
    Q_INVOKABLE int getBoundaryLevelAdjust() const { return _boundaryLevelAdjust; }
    
    Q_INVOKABLE void resetLODAdjust();
    Q_INVOKABLE float getFPSAverage() const { return _fpsAverage.getAverage(); }
    Q_INVOKABLE float getFastFPSAverage() const { return _fastFPSAverage.getAverage(); }
    
    Q_INVOKABLE float getLODDecreaseFPS();
    Q_INVOKABLE float getLODIncreaseFPS();
    
    bool shouldRenderMesh(float largestDimension, float distanceToCamera);
    void autoAdjustLOD(float currentFPS);
    
    void loadSettings();
    void saveSettings();
    
signals:
    void LODIncreased();
    void LODDecreased();
    
private:
    LODManager();
    void calculateAvatarLODDistanceMultiplier();
    
    bool _automaticLODAdjust = true;
    float _desktopLODDecreaseFPS = DEFAULT_DESKTOP_LOD_DOWN_FPS;
    float _hmdLODDecreaseFPS = DEFAULT_HMD_LOD_DOWN_FPS;

    float _avatarLODDistanceMultiplier;
    float _octreeSizeScale = DEFAULT_OCTREE_SIZE_SCALE;
    int _boundaryLevelAdjust = 0;
    
    quint64 _lastAdjust = 0;
    SimpleMovingAverage _fpsAverage = FIVE_SECONDS_OF_FRAMES;
    SimpleMovingAverage _fastFPSAverage = ONE_SECOND_OF_FRAMES;
    
    bool _shouldRenderTableNeedsRebuilding = true;
    QMap<float, float> _shouldRenderTable;
};

#endif // hifi_LODManager_h