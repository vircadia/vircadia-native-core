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

#include <SettingHandle.h>
#include <Util.h>

#include "Application.h"
#include "ui/DialogsManager.h"

#include "LODManager.h"

Setting::Handle<float> desktopLODDecreaseFPS("desktopLODDecreaseFPS", DEFAULT_DESKTOP_LOD_DOWN_FPS);
Setting::Handle<float> hmdLODDecreaseFPS("hmdLODDecreaseFPS", DEFAULT_HMD_LOD_DOWN_FPS);


Setting::Handle<float> avatarLODDistanceMultiplier("avatarLODDistanceMultiplier",
                                                           DEFAULT_AVATAR_LOD_DISTANCE_MULTIPLIER);
Setting::Handle<int> boundaryLevelAdjust("boundaryLevelAdjust", 0);
Setting::Handle<float> octreeSizeScale("octreeSizeScale", DEFAULT_OCTREE_SIZE_SCALE);


float LODManager::getLODDecreaseFPS() {
    if (Application::getInstance()->isHMDMode()) {
        return getHMDLODDecreaseFPS();
    }
    return getDesktopLODDecreaseFPS();
}

float LODManager::getLODIncreaseFPS() {
    if (Application::getInstance()->isHMDMode()) {
        return getHMDLODIncreaseFPS();
    }
    return getDesktopLODIncreaseFPS();
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

    bool changed = false;
    bool octreeChanged = false;
    quint64 elapsed = now - _lastAdjust;
    
    if (_automaticLODAdjust) {
        // LOD Downward adjustment    
        if (elapsed > ADJUST_LOD_DOWN_DELAY && _fpsAverage.getAverage() < getLODDecreaseFPS()) {

            // Avatars... attempt to lower the detail in proportion to the fps difference
            float targetFps = (getLODDecreaseFPS() + getLODIncreaseFPS()) * 0.5f;
            float averageFps = _fastFPSAverage.getAverage();
            const float MAXIMUM_MULTIPLIER_SCALE = 2.0f;
            float oldAvatarLODDistanceMultiplier = _avatarLODDistanceMultiplier;
            _avatarLODDistanceMultiplier = qMin(MAXIMUM_AVATAR_LOD_DISTANCE_MULTIPLIER, _avatarLODDistanceMultiplier *
                                                (averageFps < EPSILON ? MAXIMUM_MULTIPLIER_SCALE :
                                                 qMin(MAXIMUM_MULTIPLIER_SCALE, targetFps / averageFps)));
        
            if (oldAvatarLODDistanceMultiplier != _avatarLODDistanceMultiplier) {
                qDebug() << "adjusting LOD down... average fps for last approximately 5 seconds=" << _fpsAverage.getAverage()
                            << "_avatarLODDistanceMultiplier=" << _avatarLODDistanceMultiplier;
                changed = true;
            }

            // Octree items... stepwise adjustment
            if (_octreeSizeScale > ADJUST_LOD_MIN_SIZE_SCALE) {
                _octreeSizeScale *= ADJUST_LOD_DOWN_BY;
                if (_octreeSizeScale < ADJUST_LOD_MIN_SIZE_SCALE) {
                    _octreeSizeScale = ADJUST_LOD_MIN_SIZE_SCALE;
                }
                octreeChanged = changed = true;
            }

            if (changed) {        
                _lastAdjust = now;
                qDebug() << "adjusting LOD down... average fps for last approximately 5 seconds=" << _fpsAverage.getAverage()
                            << "_octreeSizeScale=" << _octreeSizeScale;
        
                emit LODDecreased();
            }
        }
    
        // LOD Upward adjustment
        if (elapsed > ADJUST_LOD_UP_DELAY && _fpsAverage.getAverage() > getLODIncreaseFPS()) {

            // Avatars... let the detail level creep slowly upwards
            if (_avatarLODDistanceMultiplier < MAXIMUM_AUTO_ADJUST_AVATAR_LOD_DISTANCE_MULTIPLIER) {
                const float DISTANCE_DECREASE_RATE = 0.05f;
                float oldAvatarLODDistanceMultiplier = _avatarLODDistanceMultiplier;
                _avatarLODDistanceMultiplier = qMax(MINIMUM_AVATAR_LOD_DISTANCE_MULTIPLIER,
                                                    _avatarLODDistanceMultiplier - DISTANCE_DECREASE_RATE);
                                                    
                if (oldAvatarLODDistanceMultiplier != _avatarLODDistanceMultiplier) {
                    qDebug() << "adjusting LOD up... average fps for last approximately 5 seconds=" << _fpsAverage.getAverage()
                                << "_avatarLODDistanceMultiplier=" << _avatarLODDistanceMultiplier;
                    changed = true;
                }
            }

            // Octee items... stepwise adjustment
            if (_octreeSizeScale < ADJUST_LOD_MAX_SIZE_SCALE) {
                if (_octreeSizeScale < ADJUST_LOD_MIN_SIZE_SCALE) {
                    _octreeSizeScale = ADJUST_LOD_MIN_SIZE_SCALE;
                } else {
                    _octreeSizeScale *= ADJUST_LOD_UP_BY;
                }
                if (_octreeSizeScale > ADJUST_LOD_MAX_SIZE_SCALE) {
                    _octreeSizeScale = ADJUST_LOD_MAX_SIZE_SCALE;
                }
                octreeChanged = changed = true;
            }
        
            if (changed) {
                _lastAdjust = now;
                qDebug() << "adjusting LOD up... average fps for last approximately 5 seconds=" << _fpsAverage.getAverage()
                            << "_octreeSizeScale=" << _octreeSizeScale;

                emit LODIncreased();
            }
        }
    
        if (changed) {
            _shouldRenderTableNeedsRebuilding = true;
            auto lodToolsDialog = DependencyManager::get<DialogsManager>()->getLodToolsDialog();
            if (lodToolsDialog) {
                lodToolsDialog->reloadSliders();
            }
        }
    }
}

void LODManager::resetLODAdjust() {
    _fpsAverage.reset();
    _fastFPSAverage.reset();
    _lastAdjust = usecTimestampNow();
}

QString LODManager::getLODFeedbackText() {
    // determine granularity feedback
    int boundaryLevelAdjust = getBoundaryLevelAdjust();
    QString granularityFeedback;
    
    switch (boundaryLevelAdjust) {
        case 0: {
            granularityFeedback = QString(".");
        } break;
        case 1: {
            granularityFeedback = QString(" at half of standard granularity.");
        } break;
        case 2: {
            granularityFeedback = QString(" at a third of standard granularity.");
        } break;
        default: {
            granularityFeedback = QString(" at 1/%1th of standard granularity.").arg(boundaryLevelAdjust + 1);
        } break;
    }
    
    // distance feedback
    float octreeSizeScale = getOctreeSizeScale();
    float relativeToDefault = octreeSizeScale / DEFAULT_OCTREE_SIZE_SCALE;
    int relativeToTwentyTwenty = 20 / relativeToDefault;

    QString result;
    if (relativeToDefault > 1.01) {
        result = QString("20:%1 or %2 times further than average vision%3").arg(relativeToTwentyTwenty).arg(relativeToDefault,0,'f',2).arg(granularityFeedback);
    } else if (relativeToDefault > 0.99) {
        result = QString("20:20 or the default distance for average vision%1").arg(granularityFeedback);
    } else if (relativeToDefault > 0.01) {
        result = QString("20:%1 or %2 of default distance for average vision%3").arg(relativeToTwentyTwenty).arg(relativeToDefault,0,'f',3).arg(granularityFeedback);
    } else {
        result = QString("%2 of default distance for average vision%3").arg(relativeToDefault,0,'f',3).arg(granularityFeedback);
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
    setDesktopLODDecreaseFPS(desktopLODDecreaseFPS.get());
    setHMDLODDecreaseFPS(hmdLODDecreaseFPS.get());

    setAvatarLODDistanceMultiplier(avatarLODDistanceMultiplier.get());
    setBoundaryLevelAdjust(boundaryLevelAdjust.get());
    setOctreeSizeScale(octreeSizeScale.get());
}

void LODManager::saveSettings() {
    desktopLODDecreaseFPS.set(getDesktopLODDecreaseFPS());
    hmdLODDecreaseFPS.set(getHMDLODDecreaseFPS());

    avatarLODDistanceMultiplier.set(getAvatarLODDistanceMultiplier());
    boundaryLevelAdjust.set(getBoundaryLevelAdjust());
    octreeSizeScale.set(getOctreeSizeScale());
}


