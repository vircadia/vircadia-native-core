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
#include <Settings.h>
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

namespace SettingHandles {
    const SettingHandle<bool> automaticAvatarLOD("automaticAvatarLOD", true);
    const SettingHandle<float> avatarLODDecreaseFPS("avatarLODDecreaseFPS", DEFAULT_ADJUST_AVATAR_LOD_DOWN_FPS);
    const SettingHandle<float> avatarLODIncreaseFPS("avatarLODIncreaseFPS",  ADJUST_LOD_UP_FPS);
    const SettingHandle<float> avatarLODDistanceMultiplier("avatarLODDistanceMultiplier",
                                                           DEFAULT_AVATAR_LOD_DISTANCE_MULTIPLIER);
}

class LODManager : public Dependency {
    SINGLETON_DEPENDENCY
    
public:
    // TODO: Once the SettingWatcher is implemented, replace them with normal SettingHandles.
    float getOctreeSizeScale() const;
    void setOctreeSizeScale(float sizeScale);
    int getBoundaryLevelAdjust() const;
    void setBoundaryLevelAdjust(int boundaryLevelAdjust);
    
    // User Tweakable LOD Items
    QString getLODFeedbackText();
    
    void autoAdjustLOD(float currentFPS);
    void resetLODAdjust();
    
    bool shouldRenderMesh(float largestDimension, float distanceToCamera);
    
private:
    LODManager() {}
    
    quint64 _lastAdjust = 0;
    quint64 _lastAvatarDetailDrop = 0;
    SimpleMovingAverage _fpsAverage = FIVE_SECONDS_OF_FRAMES;
    SimpleMovingAverage _fastFPSAverage = ONE_SECOND_OF_FRAMES;
    
    bool _shouldRenderTableNeedsRebuilding = true;
    QMap<float, float> _shouldRenderTable;
};

#endif // hifi_LODManager_h