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

const float ADJUST_LOD_DOWN_FPS = 40.0;
const float ADJUST_LOD_UP_FPS = 55.0;
const float DEFAULT_ADJUST_AVATAR_LOD_DOWN_FPS = 30.0f;

const quint64 ADJUST_LOD_DOWN_DELAY = 1000 * 1000 * 0.5; // Consider adjusting LOD down after half a second
const quint64 ADJUST_LOD_UP_DELAY = ADJUST_LOD_DOWN_DELAY * 2;

const float ADJUST_LOD_DOWN_BY = 0.9f;
const float ADJUST_LOD_UP_BY = 1.1f;

// This controls how low the auto-adjust LOD will go a value of 1 means it will adjust to a point where you must be 0.25
// meters away from an object of TREE_SCALE before you can see it (which is effectively completely blind). The default value
// DEFAULT_OCTREE_SIZE_SCALE means you can be 400 meters away from a 1 meter object in order to see it (which is ~20:20 vision).
const float ADJUST_LOD_MIN_SIZE_SCALE = 1.0f;
const float ADJUST_LOD_MAX_SIZE_SCALE = DEFAULT_OCTREE_SIZE_SCALE;

const float MINIMUM_AVATAR_LOD_DISTANCE_MULTIPLIER = 0.1f;
const float MAXIMUM_AVATAR_LOD_DISTANCE_MULTIPLIER = 15.0f;
const float DEFAULT_AVATAR_LOD_DISTANCE_MULTIPLIER = 1.0f;

const int ONE_SECOND_OF_FRAMES = 60;
const int FIVE_SECONDS_OF_FRAMES = 5 * ONE_SECOND_OF_FRAMES;


class LODManager : public QObject, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY
    
public:
    void setAutomaticAvatarLOD(bool automaticAvatarLOD) { _automaticAvatarLOD = automaticAvatarLOD; }
    bool getAutomaticAvatarLOD() const { return _automaticAvatarLOD; }
    void setAvatarLODDecreaseFPS(float avatarLODDecreaseFPS) { _avatarLODDecreaseFPS = avatarLODDecreaseFPS; }
    float getAvatarLODDecreaseFPS() const { return _avatarLODDecreaseFPS; }
    void setAvatarLODIncreaseFPS(float avatarLODIncreaseFPS) { _avatarLODIncreaseFPS = avatarLODIncreaseFPS; }
    float getAvatarLODIncreaseFPS() const { return _avatarLODIncreaseFPS; }
    void setAvatarLODDistanceMultiplier(float multiplier) { _avatarLODDistanceMultiplier = multiplier; }
    float getAvatarLODDistanceMultiplier() const { return _avatarLODDistanceMultiplier; }
    
    // User Tweakable LOD Items
    Q_INVOKABLE QString getLODFeedbackText();
    Q_INVOKABLE void setOctreeSizeScale(float sizeScale);
    Q_INVOKABLE float getOctreeSizeScale() const { return _octreeSizeScale; }
    
    Q_INVOKABLE void setBoundaryLevelAdjust(int boundaryLevelAdjust);
    Q_INVOKABLE int getBoundaryLevelAdjust() const { return _boundaryLevelAdjust; }
    
    void autoAdjustLOD(float currentFPS);
    Q_INVOKABLE void resetLODAdjust();
    Q_INVOKABLE float getFPSAverage() const { return _fpsAverage.getAverage(); }
    Q_INVOKABLE float getFastFPSAverage() const { return _fastFPSAverage.getAverage(); }
    
    bool shouldRenderMesh(float largestDimension, float distanceToCamera);
    
    void loadSettings();
    void saveSettings();
    
signals:
    void LODIncreased();
    void LODDecreased();
    
private:
    LODManager() {}
    
    bool _automaticAvatarLOD = true;
    float _avatarLODDecreaseFPS = DEFAULT_ADJUST_AVATAR_LOD_DOWN_FPS;
    float _avatarLODIncreaseFPS = ADJUST_LOD_UP_FPS;
    float _avatarLODDistanceMultiplier = DEFAULT_AVATAR_LOD_DISTANCE_MULTIPLIER;
    
    float _octreeSizeScale = DEFAULT_OCTREE_SIZE_SCALE;
    int _boundaryLevelAdjust = 0;
    
    quint64 _lastAdjust = 0;
    quint64 _lastAvatarDetailDrop = 0;
    SimpleMovingAverage _fpsAverage = FIVE_SECONDS_OF_FRAMES;
    SimpleMovingAverage _fastFPSAverage = ONE_SECOND_OF_FRAMES;
    
    bool _shouldRenderTableNeedsRebuilding = true;
    QMap<float, float> _shouldRenderTable;
};

#endif // hifi_LODManager_h