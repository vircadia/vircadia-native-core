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
const float INCREASE_LOD_GAP_FPS = 10.0f; // fps

// The default value DEFAULT_OCTREE_SIZE_SCALE means you can be 400 meters away from a 1 meter object in order to see it (which is ~20:20 vision).
const float ADJUST_LOD_MAX_SIZE_SCALE = DEFAULT_OCTREE_SIZE_SCALE;
// This controls how low the auto-adjust LOD will go. We want a minimum vision of ~20:500 or 0.04 of default
const float ADJUST_LOD_MIN_SIZE_SCALE = DEFAULT_OCTREE_SIZE_SCALE * 0.04f;

class AABox;

/**jsdoc
 * The LOD class manages your Level of Detail functions within Interface.
 * @namespace LODManager
  *
 * @hifi-interface
 * @hifi-client-entity
 *
 * @property {number} presentTime <em>Read-only.</em>
 * @property {number} engineRunTime <em>Read-only.</em>
 * @property {number} gpuTime <em>Read-only.</em>
 * @property {number} avgRenderTime <em>Read-only.</em>
 * @property {number} fps <em>Read-only.</em>
 * @property {number} lodLevel <em>Read-only.</em>
 * @property {number} lodDecreaseFPS <em>Read-only.</em>
 * @property {number} lodIncreaseFPS <em>Read-only.</em>
 */

class LODManager : public QObject, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY

    Q_PROPERTY(float presentTime READ getPresentTime)
    Q_PROPERTY(float engineRunTime READ getEngineRunTime)
    Q_PROPERTY(float gpuTime READ getGPUTime)
    Q_PROPERTY(float avgRenderTime READ getAverageRenderTime)
    Q_PROPERTY(float fps READ getMaxTheoreticalFPS)
    Q_PROPERTY(float lodLevel READ getLODLevel)
    Q_PROPERTY(float lodDecreaseFPS READ getLODDecreaseFPS)
    Q_PROPERTY(float lodIncreaseFPS READ getLODIncreaseFPS)

public:
     
    /**jsdoc
     * @function LODManager.setAutomaticLODAdjust
     * @param {boolean} value
     */
    Q_INVOKABLE void setAutomaticLODAdjust(bool value) { _automaticLODAdjust = value; }

    /**jsdoc
     * @function LODManager.getAutomaticLODAdjust
     * @returns {boolean}
     */
    Q_INVOKABLE bool getAutomaticLODAdjust() const { return _automaticLODAdjust; }

    /**jsdoc
     * @function LODManager.setDesktopLODDecreaseFPS
     * @param {number} value
     */
    Q_INVOKABLE void setDesktopLODDecreaseFPS(float value);

    /**jsdoc
     * @function LODManager.getDesktopLODDecreaseFPS
     * @returns {number}
     */

    Q_INVOKABLE float getDesktopLODDecreaseFPS() const;

    /**jsdoc
     * @function LODManager.getDesktopLODIncreaseFPS
     * @returns {number}
     */
    Q_INVOKABLE float getDesktopLODIncreaseFPS() const;

    /**jsdoc
     * @function LODManager.setHMDLODDecreaseFPS
     * @param {number} value
     */
  
    Q_INVOKABLE void setHMDLODDecreaseFPS(float value);

    /**jsdoc
     * @function LODManager.getHMDLODDecreaseFPS
     * @returns {number}
     */
    Q_INVOKABLE float getHMDLODDecreaseFPS() const;

    /**jsdoc
     * @function LODManager.getHMDLODIncreaseFPS
     * @returns {number}
     */
    Q_INVOKABLE float getHMDLODIncreaseFPS() const;

    // User Tweakable LOD Items
    /**jsdoc
     * @function LODManager.getLODFeedbackText
     * @returns {string}
     */
    Q_INVOKABLE QString getLODFeedbackText();

    /**jsdoc
     * @function LODManager.setOctreeSizeScale
     * @param {number} sizeScale
     */
    Q_INVOKABLE void setOctreeSizeScale(float sizeScale);

    /**jsdoc
     * @function LODManager.getOctreeSizeScale
     * @returns {number}
     */
    Q_INVOKABLE float getOctreeSizeScale() const { return _octreeSizeScale; }

    /**jsdoc
     * @function LODManager.setBoundaryLevelAdjust
     * @param {number} boundaryLevelAdjust
     */
    Q_INVOKABLE void setBoundaryLevelAdjust(int boundaryLevelAdjust);

    /**jsdoc
     * @function LODManager.getBoundaryLevelAdjust
     * @returns {number}
     */
    Q_INVOKABLE int getBoundaryLevelAdjust() const { return _boundaryLevelAdjust; }

    /**jsdoc
     * @function LODManager.getLODDecreaseFPS
     * @returns {number}
     */
    Q_INVOKABLE float getLODDecreaseFPS() const;

    /**jsdoc
     * @function LODManager.getLODIncreaseFPS
     * @returns {number}
     */
    Q_INVOKABLE float getLODIncreaseFPS() const;

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

    /**jsdoc
     * @function LODManager.LODIncreased
     * @returns {Signal}
     */
    void LODIncreased();

    /**jsdoc
     * @function LODManager.LODDecreased
     * @returns {Signal}
     */
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
