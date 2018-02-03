//
//  LODManager.h
//  interface/src/LODManager.h
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
#include <NumericalConstants.h>
#include <OctreeConstants.h>
#include <PIDController.h>
#include <SimpleMovingAverage.h>
#include <render/Args.h>

const float DEFAULT_DESKTOP_LOD_DOWN_FPS = 30.0f;
const float DEFAULT_HMD_LOD_DOWN_FPS = 34.0f;
const float DEFAULT_DESKTOP_MAX_RENDER_TIME = (float)MSECS_PER_SECOND / DEFAULT_DESKTOP_LOD_DOWN_FPS; // msec
const float DEFAULT_HMD_MAX_RENDER_TIME = (float)MSECS_PER_SECOND / DEFAULT_HMD_LOD_DOWN_FPS; // msec
const float MAX_LIKELY_DESKTOP_FPS = 59.0f; // this is essentially, V-synch - 1 fps
const float MAX_LIKELY_HMD_FPS = 74.0f; // this is essentially, V-synch - 1 fps
const float INCREASE_LOD_GAP_FPS = 15.0f; // fps

// The default value DEFAULT_OCTREE_SIZE_SCALE means you can be 400 meters away from a 1 meter object in order to see it (which is ~20:20 vision).
const float ADJUST_LOD_MAX_SIZE_SCALE = DEFAULT_OCTREE_SIZE_SCALE;
// This controls how low the auto-adjust LOD will go. We want a minimum vision of ~20:500 or 0.04 of default
const float ADJUST_LOD_MIN_SIZE_SCALE = DEFAULT_OCTREE_SIZE_SCALE * 0.04f;

class AABox;

class LODManager : public QObject, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY

public:
    Q_INVOKABLE void setAutomaticLODAdjust(bool value) { _automaticLODAdjust = value; }
    Q_INVOKABLE bool getAutomaticLODAdjust() const { return _automaticLODAdjust; }

    Q_INVOKABLE void setDesktopLODDecreaseFPS(float value);
    Q_INVOKABLE float getDesktopLODDecreaseFPS() const;
    Q_INVOKABLE float getDesktopLODIncreaseFPS() const;

    Q_INVOKABLE void setHMDLODDecreaseFPS(float value);
    Q_INVOKABLE float getHMDLODDecreaseFPS() const;
    Q_INVOKABLE float getHMDLODIncreaseFPS() const;

    // User Tweakable LOD Items
    Q_INVOKABLE QString getLODFeedbackText();
    Q_INVOKABLE void setOctreeSizeScale(float sizeScale);
    Q_INVOKABLE float getOctreeSizeScale() const { return _octreeSizeScale; }

    Q_INVOKABLE void setBoundaryLevelAdjust(int boundaryLevelAdjust);
    Q_INVOKABLE int getBoundaryLevelAdjust() const { return _boundaryLevelAdjust; }

    Q_INVOKABLE float getLODDecreaseFPS() const;
    Q_INVOKABLE float getLODIncreaseFPS() const;

    Q_PROPERTY(float presentTime READ getPresentTime)
    Q_PROPERTY(float engineRunTime READ getEngineRunTime)
    Q_PROPERTY(float gpuTime READ getGPUTime)
    Q_PROPERTY(float avgRenderTime READ getAverageRenderTime)
    Q_PROPERTY(float fps READ getMaxTheoreticalFPS)
    Q_PROPERTY(float lodLevel READ getLODLevel)

    Q_PROPERTY(float lodDecreaseFPS READ getLODDecreaseFPS)
    Q_PROPERTY(float lodIncreaseFPS READ getLODIncreaseFPS)

    float getPresentTime() const { return _presentTime; }
    float getEngineRunTime() const { return _engineRunTime; }
    float getGPUTime() const { return _gpuTime; }

    static bool shouldRender(const RenderArgs* args, const AABox& bounds);
    void setRenderTimes(float presentTime, float engineRunTime, float gpuTime);
    void autoAdjustLOD(float realTimeDelta);

    void loadSettings();
    void saveSettings();
    void resetLODAdjust();

    float getAverageRenderTime() const { return _avgRenderTime; };
    float getMaxTheoreticalFPS() const { return (float)MSECS_PER_SECOND / _avgRenderTime; };
    float getLODLevel() const;

signals:
    void LODIncreased();
    void LODDecreased();

private:
    LODManager();

    bool _automaticLODAdjust = true;
    float _presentTime { 0.0f }; // msec
    float _engineRunTime { 0.0f }; // msec
    float _gpuTime { 0.0f }; // msec
    float _avgRenderTime { 0.0f }; // msec
    float _desktopMaxRenderTime { DEFAULT_DESKTOP_MAX_RENDER_TIME };
    float _hmdMaxRenderTime { DEFAULT_HMD_MAX_RENDER_TIME };

    float _octreeSizeScale = DEFAULT_OCTREE_SIZE_SCALE;
    int _boundaryLevelAdjust = 0;

    uint64_t _decreaseFPSExpiry { 0 };
    uint64_t _increaseFPSExpiry { 0 };
};

#endif // hifi_LODManager_h
