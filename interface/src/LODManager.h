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

const float DEFAULT_DESKTOP_LOD_DOWN_FPS = 20.0;
const float DEFAULT_HMD_LOD_DOWN_FPS = 20.0;
const float MAX_LIKELY_DESKTOP_FPS = 59.0; // this is essentially, V-synch - 1 fps
const float MAX_LIKELY_HMD_FPS = 74.0; // this is essentially, V-synch - 1 fps
const float INCREASE_LOD_GAP = 15.0f;

const float START_DELAY_WINDOW_IN_SECS = 3.0f; // wait at least this long after steady state/last upshift to consider downshifts
const float DOWN_SHIFT_WINDOW_IN_SECS = 0.5f;
const float UP_SHIFT_WINDOW_IN_SECS = 2.5f;

const int ASSUMED_FPS = 60;
const quint64 START_SHIFT_ELPASED = USECS_PER_SECOND * START_DELAY_WINDOW_IN_SECS;
const quint64 DOWN_SHIFT_ELPASED = USECS_PER_SECOND * DOWN_SHIFT_WINDOW_IN_SECS; // Consider adjusting LOD down after half a second
const quint64 UP_SHIFT_ELPASED = USECS_PER_SECOND * UP_SHIFT_WINDOW_IN_SECS;

const int START_DELAY_SAMPLES_OF_FRAMES = ASSUMED_FPS * START_DELAY_WINDOW_IN_SECS;
const int DOWN_SHIFT_SAMPLES_OF_FRAMES = ASSUMED_FPS * DOWN_SHIFT_WINDOW_IN_SECS;
const int UP_SHIFT_SAMPLES_OF_FRAMES = ASSUMED_FPS * UP_SHIFT_WINDOW_IN_SECS;

const float ADJUST_LOD_DOWN_BY = 0.9f;
const float ADJUST_LOD_UP_BY = 1.1f;

// The default value DEFAULT_OCTREE_SIZE_SCALE means you can be 400 meters away from a 1 meter object in order to see it (which is ~20:20 vision).
const float ADJUST_LOD_MAX_SIZE_SCALE = DEFAULT_OCTREE_SIZE_SCALE;
// This controls how low the auto-adjust LOD will go. We want a minimum vision of ~20:500 or 0.04 of default
const float ADJUST_LOD_MIN_SIZE_SCALE = DEFAULT_OCTREE_SIZE_SCALE * 0.04f;

class RenderArgs;
class AABox;

class LODManager : public QObject, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY
    
public:
    Q_INVOKABLE void setAutomaticLODAdjust(bool value) { _automaticLODAdjust = value; }
    Q_INVOKABLE bool getAutomaticLODAdjust() const { return _automaticLODAdjust; }

    Q_INVOKABLE void setDesktopLODDecreaseFPS(float value) { _desktopLODDecreaseFPS = value; }
    Q_INVOKABLE float getDesktopLODDecreaseFPS() const { return _desktopLODDecreaseFPS; }
    Q_INVOKABLE float getDesktopLODIncreaseFPS() const { return glm::min(_desktopLODDecreaseFPS + INCREASE_LOD_GAP, MAX_LIKELY_DESKTOP_FPS); }

    Q_INVOKABLE void setHMDLODDecreaseFPS(float value) { _hmdLODDecreaseFPS = value; }
    Q_INVOKABLE float getHMDLODDecreaseFPS() const { return _hmdLODDecreaseFPS; }
    Q_INVOKABLE float getHMDLODIncreaseFPS() const { return glm::min(_hmdLODDecreaseFPS + INCREASE_LOD_GAP, MAX_LIKELY_HMD_FPS); }
    
    // User Tweakable LOD Items
    Q_INVOKABLE QString getLODFeedbackText();
    Q_INVOKABLE void setOctreeSizeScale(float sizeScale);
    Q_INVOKABLE float getOctreeSizeScale() const { return _octreeSizeScale; }
    
    Q_INVOKABLE void setBoundaryLevelAdjust(int boundaryLevelAdjust);
    Q_INVOKABLE int getBoundaryLevelAdjust() const { return _boundaryLevelAdjust; }
    
    Q_INVOKABLE float getLODDecreaseFPS();
    Q_INVOKABLE float getLODIncreaseFPS();
    
    static bool shouldRender(const RenderArgs* args, const AABox& bounds);
    void autoAdjustLOD(float currentFPS);
    
    void loadSettings();
    void saveSettings();
    void resetLODAdjust();
    
signals:
    void LODIncreased();
    void LODDecreased();
    
private:
    LODManager();
    
    bool _automaticLODAdjust = true;
    float _desktopLODDecreaseFPS = DEFAULT_DESKTOP_LOD_DOWN_FPS;
    float _hmdLODDecreaseFPS = DEFAULT_HMD_LOD_DOWN_FPS;

    float _octreeSizeScale = DEFAULT_OCTREE_SIZE_SCALE;
    int _boundaryLevelAdjust = 0;
    
    quint64 _lastDownShift = 0;
    quint64 _lastUpShift = 0;
    quint64 _lastStable = 0;
    bool _isDownshifting = false; // start out as if we're not downshifting
    
    SimpleMovingAverage _fpsAverageStartWindow = START_DELAY_SAMPLES_OF_FRAMES;
    SimpleMovingAverage _fpsAverageDownWindow = DOWN_SHIFT_SAMPLES_OF_FRAMES;
    SimpleMovingAverage _fpsAverageUpWindow = UP_SHIFT_SAMPLES_OF_FRAMES;
};

#endif // hifi_LODManager_h
