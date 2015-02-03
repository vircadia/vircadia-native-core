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
#include <SettingHandle.h>
#include <SharedUtil.h>
#include <SimpleMovingAverage.h>

const float ADJUST_LOD_DOWN_FPS = 40.0;
const float ADJUST_LOD_UP_FPS = 55.0;
const float DEFAULT_ADJUST_AVATAR_LOD_DOWN_FPS = 30.0f;

const quint64 ADJUST_LOD_DOWN_DELAY = 1000 * 1000 * 5;
const quint64 ADJUST_LOD_UP_DELAY = ADJUST_LOD_DOWN_DELAY * 2;

const float ADJUST_LOD_DOWN_BY = 0.9f;
const float ADJUST_LOD_UP_BY = 1.1f;

const float ADJUST_LOD_MIN_SIZE_SCALE = DEFAULT_OCTREE_SIZE_SCALE * 0.25f;
const float ADJUST_LOD_MAX_SIZE_SCALE = DEFAULT_OCTREE_SIZE_SCALE;

const float MINIMUM_AVATAR_LOD_DISTANCE_MULTIPLIER = 0.1f;
const float MAXIMUM_AVATAR_LOD_DISTANCE_MULTIPLIER = 15.0f;
const float DEFAULT_AVATAR_LOD_DISTANCE_MULTIPLIER = 1.0f;

const int ONE_SECOND_OF_FRAMES = 60;
const int FIVE_SECONDS_OF_FRAMES = 5 * ONE_SECOND_OF_FRAMES;


class LODManager : public Dependency {
    SINGLETON_DEPENDENCY
    
public:
    void setAutomaticAvatarLOD(bool automaticAvatarLOD) { _automaticAvatarLOD.set(automaticAvatarLOD); }
    bool getAutomaticAvatarLOD() { return _automaticAvatarLOD.get(); }
    void setAvatarLODDecreaseFPS(float avatarLODDecreaseFPS) { _avatarLODDecreaseFPS.set(avatarLODDecreaseFPS); }
    float getAvatarLODDecreaseFPS() { return _avatarLODDecreaseFPS.get(); }
    void setAvatarLODIncreaseFPS(float avatarLODIncreaseFPS) { _avatarLODIncreaseFPS.set(avatarLODIncreaseFPS); }
    float getAvatarLODIncreaseFPS() { return _avatarLODIncreaseFPS.get(); }
    void setAvatarLODDistanceMultiplier(float multiplier) { _avatarLODDistanceMultiplier.set(multiplier); }
    float getAvatarLODDistanceMultiplier() { return _avatarLODDistanceMultiplier.get(); }
    
    // User Tweakable LOD Items
    QString getLODFeedbackText();
    void setOctreeSizeScale(float sizeScale);
    float getOctreeSizeScale() { return _octreeSizeScale.get(); }
    
    void setBoundaryLevelAdjust(int boundaryLevelAdjust);
    int getBoundaryLevelAdjust() { return _boundaryLevelAdjust.get(); }
    
    void autoAdjustLOD(float currentFPS);
    void resetLODAdjust();
    
    bool shouldRenderMesh(float largestDimension, float distanceToCamera);
    
private:
    LODManager();
    
    Setting::Handle<bool> _automaticAvatarLOD;
    Setting::Handle<float> _avatarLODDecreaseFPS;
    Setting::Handle<float> _avatarLODIncreaseFPS;
    Setting::Handle<float> _avatarLODDistanceMultiplier;
    
    Setting::Handle<int> _boundaryLevelAdjust;
    Setting::Handle<float> _octreeSizeScale;
    
    quint64 _lastAdjust = 0;
    quint64 _lastAvatarDetailDrop = 0;
    SimpleMovingAverage _fpsAverage = FIVE_SECONDS_OF_FRAMES;
    SimpleMovingAverage _fastFPSAverage = ONE_SECOND_OF_FRAMES;
    
    bool _shouldRenderTableNeedsRebuilding = true;
    QMap<float, float> _shouldRenderTable;
};

#endif // hifi_LODManager_h