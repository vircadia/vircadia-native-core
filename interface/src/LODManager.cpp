//
//  LODManager.cpp
//
//
//  Created by Clement on 1/16/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <Util.h>

#include "Application.h"
#include "ui/DialogsManager.h"

#include "LODManager.h"

namespace SettingHandles {
    const SettingHandle<bool> automaticAvatarLOD("automaticAvatarLOD", true);
    const SettingHandle<float> avatarLODDecreaseFPS("avatarLODDecreaseFPS", DEFAULT_ADJUST_AVATAR_LOD_DOWN_FPS);
    const SettingHandle<float> avatarLODIncreaseFPS("avatarLODIncreaseFPS",  ADJUST_LOD_UP_FPS);
    const SettingHandle<float> avatarLODDistanceMultiplier("avatarLODDistanceMultiplier",
                                                           DEFAULT_AVATAR_LOD_DISTANCE_MULTIPLIER);
    const SettingHandle<int> boundaryLevelAdjust("boundaryLevelAdjust", 0);
    const SettingHandle<float> octreeSizeScale("octreeSizeScale", DEFAULT_OCTREE_SIZE_SCALE);
}

void LODManager::autoAdjustLOD(float currentFPS) {
    // NOTE: our first ~100 samples at app startup are completely all over the place, and we don't
    // really want to count them in our average, so we will ignore the real frame rates and stuff
    // our moving average with simulated good data
    const int IGNORE_THESE_SAMPLES = 100;
    const float ASSUMED_FPS = 60.0f;
    if (_fpsAverage.getSampleCount() < IGNORE_THESE_SAMPLES) {
        currentFPS = ASSUMED_FPS;
    }
    _fpsAverage.updateAverage(currentFPS);
    _fastFPSAverage.updateAverage(currentFPS);
    
    quint64 now = usecTimestampNow();
    
    const quint64 ADJUST_AVATAR_LOD_DOWN_DELAY = 1000 * 1000;
    if (_automaticAvatarLOD) {
        if (_fastFPSAverage.getAverage() < _avatarLODDecreaseFPS) {
            if (now - _lastAvatarDetailDrop > ADJUST_AVATAR_LOD_DOWN_DELAY) {
                // attempt to lower the detail in proportion to the fps difference
                float targetFps = (_avatarLODDecreaseFPS + _avatarLODIncreaseFPS) * 0.5f;
                float averageFps = _fastFPSAverage.getAverage();
                const float MAXIMUM_MULTIPLIER_SCALE = 2.0f;
                _avatarLODDistanceMultiplier = qMin(MAXIMUM_AVATAR_LOD_DISTANCE_MULTIPLIER, _avatarLODDistanceMultiplier *
                                                    (averageFps < EPSILON ? MAXIMUM_MULTIPLIER_SCALE :
                                                     qMin(MAXIMUM_MULTIPLIER_SCALE, targetFps / averageFps)));
                _lastAvatarDetailDrop = now;
            }
        } else if (_fastFPSAverage.getAverage() > _avatarLODIncreaseFPS) {
            // let the detail level creep slowly upwards
            const float DISTANCE_DECREASE_RATE = 0.05f;
            _avatarLODDistanceMultiplier = qMax(MINIMUM_AVATAR_LOD_DISTANCE_MULTIPLIER,
                                                _avatarLODDistanceMultiplier - DISTANCE_DECREASE_RATE);
        }
    }
    
    bool changed = false;
    quint64 elapsed = now - _lastAdjust;
    
    if (elapsed > ADJUST_LOD_DOWN_DELAY && _fpsAverage.getAverage() < ADJUST_LOD_DOWN_FPS
        && _octreeSizeScale > ADJUST_LOD_MIN_SIZE_SCALE) {
        
        _octreeSizeScale *= ADJUST_LOD_DOWN_BY;
        
        if (_octreeSizeScale < ADJUST_LOD_MIN_SIZE_SCALE) {
            _octreeSizeScale = ADJUST_LOD_MIN_SIZE_SCALE;
        }
        changed = true;
        _lastAdjust = now;
        qDebug() << "adjusting LOD down... average fps for last approximately 5 seconds=" << _fpsAverage.getAverage()
        << "_octreeSizeScale=" << _octreeSizeScale;
    }
    
    if (elapsed > ADJUST_LOD_UP_DELAY && _fpsAverage.getAverage() > ADJUST_LOD_UP_FPS
        && _octreeSizeScale < ADJUST_LOD_MAX_SIZE_SCALE) {
        _octreeSizeScale *= ADJUST_LOD_UP_BY;
        if (_octreeSizeScale > ADJUST_LOD_MAX_SIZE_SCALE) {
            _octreeSizeScale = ADJUST_LOD_MAX_SIZE_SCALE;
        }
        changed = true;
        _lastAdjust = now;
        qDebug() << "adjusting LOD up... average fps for last approximately 5 seconds=" << _fpsAverage.getAverage()
        << "_octreeSizeScale=" << _octreeSizeScale;
    }
    
    if (changed) {
        _shouldRenderTableNeedsRebuilding = true;
        auto lodToolsDialog = DependencyManager::get<DialogsManager>()->getLodToolsDialog();
        if (lodToolsDialog) {
            lodToolsDialog->reloadSliders();
        }
    }
}

void LODManager::resetLODAdjust() {
    _fpsAverage.reset();
    _fastFPSAverage.reset();
    _lastAvatarDetailDrop = _lastAdjust = usecTimestampNow();
}

QString LODManager::getLODFeedbackText() {
    // determine granularity feedback
    int boundaryLevelAdjust = getBoundaryLevelAdjust();
    QString granularityFeedback;
    
    switch (boundaryLevelAdjust) {
        case 0: {
            granularityFeedback = QString("at standard granularity.");
        } break;
        case 1: {
            granularityFeedback = QString("at half of standard granularity.");
        } break;
        case 2: {
            granularityFeedback = QString("at a third of standard granularity.");
        } break;
        default: {
            granularityFeedback = QString("at 1/%1th of standard granularity.").arg(boundaryLevelAdjust + 1);
        } break;
    }
    
    // distance feedback
    float octreeSizeScale = getOctreeSizeScale();
    float relativeToDefault = octreeSizeScale / DEFAULT_OCTREE_SIZE_SCALE;
    QString result;
    if (relativeToDefault > 1.01) {
        result = QString("%1 further %2").arg(relativeToDefault,8,'f',2).arg(granularityFeedback);
    } else if (relativeToDefault > 0.99) {
        result = QString("the default distance %1").arg(granularityFeedback);
    } else {
        result = QString("%1 of default %2").arg(relativeToDefault,8,'f',3).arg(granularityFeedback);
    }
    return result;
}

// TODO: This is essentially the same logic used to render octree cells, but since models are more detailed then octree cells
//       I've added a voxelToModelRatio that adjusts how much closer to a model you have to be to see it.
bool LODManager::shouldRenderMesh(float largestDimension, float distanceToCamera) {
    const float octreeToMeshRatio = 4.0f; // must be this many times closer to a mesh than a voxel to see it.
    float octreeSizeScale = getOctreeSizeScale();
    int boundaryLevelAdjust = getBoundaryLevelAdjust();
    float maxScale = (float)TREE_SCALE;
    float visibleDistanceAtMaxScale = boundaryDistanceForRenderLevel(boundaryLevelAdjust, octreeSizeScale) / octreeToMeshRatio;
    
    if (_shouldRenderTableNeedsRebuilding) {
        _shouldRenderTable.clear();
        
        float SMALLEST_SCALE_IN_TABLE = 0.001f; // 1mm is plenty small
        float scale = maxScale;
        float visibleDistanceAtScale = visibleDistanceAtMaxScale;
        
        while (scale > SMALLEST_SCALE_IN_TABLE) {
            scale /= 2.0f;
            visibleDistanceAtScale /= 2.0f;
            _shouldRenderTable[scale] = visibleDistanceAtScale;
        }
        _shouldRenderTableNeedsRebuilding = false;
    }
    
    float closestScale = maxScale;
    float visibleDistanceAtClosestScale = visibleDistanceAtMaxScale;
    QMap<float, float>::const_iterator lowerBound = _shouldRenderTable.lowerBound(largestDimension);
    if (lowerBound != _shouldRenderTable.constEnd()) {
        closestScale = lowerBound.key();
        visibleDistanceAtClosestScale = lowerBound.value();
    }
    
    if (closestScale < largestDimension) {
        visibleDistanceAtClosestScale *= 2.0f;
    }
    
    return (distanceToCamera <= visibleDistanceAtClosestScale);
}

void LODManager::setOctreeSizeScale(float sizeScale) {
    _octreeSizeScale = sizeScale;
    _shouldRenderTableNeedsRebuilding = true;
}

void LODManager::setBoundaryLevelAdjust(int boundaryLevelAdjust) {
    _boundaryLevelAdjust = boundaryLevelAdjust;
    _shouldRenderTableNeedsRebuilding = true;
}


void LODManager::loadSettings() {
    setAutomaticAvatarLOD(SettingHandles::automaticAvatarLOD.get());
    setAvatarLODDecreaseFPS(SettingHandles::avatarLODDecreaseFPS.get());
    setAvatarLODIncreaseFPS(SettingHandles::avatarLODIncreaseFPS.get());
    setAvatarLODDistanceMultiplier(SettingHandles::avatarLODDistanceMultiplier.get());
    setBoundaryLevelAdjust(SettingHandles::boundaryLevelAdjust.get());
    setOctreeSizeScale(SettingHandles::octreeSizeScale.get());
}

void LODManager::saveSettings() {
    SettingHandles::automaticAvatarLOD.set(getAutomaticAvatarLOD());
    SettingHandles::avatarLODDecreaseFPS.set(getAvatarLODDecreaseFPS());
    SettingHandles::avatarLODIncreaseFPS.set(getAvatarLODIncreaseFPS());
    SettingHandles::avatarLODDistanceMultiplier.set(getAvatarLODDistanceMultiplier());
    SettingHandles::boundaryLevelAdjust.set(getBoundaryLevelAdjust());
    SettingHandles::octreeSizeScale.set(getOctreeSizeScale());
}


