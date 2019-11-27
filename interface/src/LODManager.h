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

#include <mutex>

#include <DependencyManager.h>
#include <NumericalConstants.h>
#include <OctreeConstants.h>
#include <OctreeUtils.h>
#include <PIDController.h>
#include <SimpleMovingAverage.h>
#include <render/Args.h>


/**jsdoc
 * <p>The world detail quality rendered.</p>
 * <table>
 *   <thead>
 *     <tr><th>Value</th><th>Description</th></tr>
 *   </thead>
 *   <tbody>
 *     <tr><td><code>0</code></td><td>Low world detail quality.</td></tr>
 *     <tr><td><code>1</code></td><td>Medium world detail quality.</td></tr>
 *     <tr><td><code>2</code></td><td>High world detail quality.</td></tr>
 *   </tbody>
 * </table>
 * @typedef {number} LODManager.WorldDetailQuality
 */
enum WorldDetailQuality {
    WORLD_DETAIL_LOW = 0,
    WORLD_DETAIL_MEDIUM,
    WORLD_DETAIL_HIGH
};
Q_DECLARE_METATYPE(WorldDetailQuality);

#ifdef Q_OS_ANDROID
const float LOD_DEFAULT_QUALITY_LEVEL = 0.2f; // default quality level setting is High (lower framerate)
#else
const float LOD_DEFAULT_QUALITY_LEVEL = 0.5f; // default quality level setting is Mid
#endif

#ifdef Q_OS_ANDROID
const WorldDetailQuality DEFAULT_WORLD_DETAIL_QUALITY = WORLD_DETAIL_LOW;
const std::vector<float> QUALITY_TO_FPS_DESKTOP = { 60.0f, 30.0f, 15.0f };
const std::vector<float> QUALITY_TO_FPS_HMD = { 25.0f, 16.0f, 10.0f };
#else
const WorldDetailQuality DEFAULT_WORLD_DETAIL_QUALITY = WORLD_DETAIL_MEDIUM;
const std::vector<float> QUALITY_TO_FPS_DESKTOP = { 60.0f, 30.0f, 15.0f };
const std::vector<float> QUALITY_TO_FPS_HMD = { 90.0f, 45.0f, 22.5f };
#endif

const float LOD_OFFSET_FPS = 5.0f; // offset of FPS to add for computing the target framerate

class AABox;


/**jsdoc
 * The LOD class manages your Level of Detail functions within Interface.
 * @namespace LODManager
 *
 * @hifi-interface
 * @hifi-client-entity
 * @hifi-avatar
 *
 * @property {number} presentTime <em>Read-only.</em>
 * @property {number} engineRunTime <em>Read-only.</em>
 * @property {number} gpuTime <em>Read-only.</em>
 */
class LODManager : public QObject, public Dependency {
    Q_OBJECT
        SINGLETON_DEPENDENCY

        Q_PROPERTY(WorldDetailQuality worldDetailQuality READ getWorldDetailQuality WRITE setWorldDetailQuality 
            NOTIFY worldDetailQualityChanged)

        Q_PROPERTY(float lodQualityLevel READ getLODQualityLevel WRITE setLODQualityLevel NOTIFY lodQualityLevelChanged)

        Q_PROPERTY(bool automaticLODAdjust READ getAutomaticLODAdjust WRITE setAutomaticLODAdjust NOTIFY autoLODChanged)

        Q_PROPERTY(float presentTime READ getPresentTime)
        Q_PROPERTY(float engineRunTime READ getEngineRunTime)
        Q_PROPERTY(float batchTime READ getBatchTime)
        Q_PROPERTY(float gpuTime READ getGPUTime)

        Q_PROPERTY(float nowRenderTime READ getNowRenderTime)
        Q_PROPERTY(float nowRenderFPS READ getNowRenderFPS)

        Q_PROPERTY(float smoothScale READ getSmoothScale WRITE setSmoothScale)
        Q_PROPERTY(float smoothRenderTime READ getSmoothRenderTime)
        Q_PROPERTY(float smoothRenderFPS READ getSmoothRenderFPS)

        Q_PROPERTY(float lodTargetFPS READ getLODTargetFPS)

        Q_PROPERTY(float lodAngleDeg READ getLODAngleDeg WRITE setLODAngleDeg)

        Q_PROPERTY(float pidKp READ getPidKp WRITE setPidKp)
        Q_PROPERTY(float pidKi READ getPidKi WRITE setPidKi)
        Q_PROPERTY(float pidKd READ getPidKd WRITE setPidKd)
        Q_PROPERTY(float pidKv READ getPidKv WRITE setPidKv)

        Q_PROPERTY(float pidOp READ getPidOp)
        Q_PROPERTY(float pidOi READ getPidOi)
        Q_PROPERTY(float pidOd READ getPidOd)
        Q_PROPERTY(float pidO READ getPidO)

public:

    /**jsdoc
     * @function LODManager.setAutomaticLODAdjust
     * @param {boolean} value
     */
    Q_INVOKABLE void setAutomaticLODAdjust(bool value);

    /**jsdoc
     * @function LODManager.getAutomaticLODAdjust
     * @returns {boolean}
     */
    Q_INVOKABLE bool getAutomaticLODAdjust() const { return _automaticLODAdjust; }

    /**jsdoc
     * @function LODManager.setDesktopLODTargetFPS
     * @param {number} value
     */
    Q_INVOKABLE void setDesktopLODTargetFPS(float value);

    /**jsdoc
     * @function LODManager.getDesktopLODTargetFPS
     * @returns {number}
     */

    Q_INVOKABLE float getDesktopLODTargetFPS() const;

    /**jsdoc
     * @function LODManager.setHMDLODTargetFPS
     * @param {number} value
     */

    Q_INVOKABLE void setHMDLODTargetFPS(float value);

    /**jsdoc
     * @function LODManager.getHMDLODTargetFPS
     * @returns {number}
     */
    Q_INVOKABLE float getHMDLODTargetFPS() const;


    // User Tweakable LOD Items
    /**jsdoc
     * @function LODManager.getLODFeedbackText
     * @returns {string}
     */
    Q_INVOKABLE QString getLODFeedbackText();

    /**jsdoc
     * @function LODManager.setOctreeSizeScale
     * @param {number} sizeScale
     * @deprecated This function is deprecated and will be removed. Use the {@link LODManager.lodAngleDeg} property instead.
     */
    Q_INVOKABLE void setOctreeSizeScale(float sizeScale);

    /**jsdoc
     * @function LODManager.getOctreeSizeScale
     * @returns {number}
     * @deprecated This function is deprecated and will be removed. Use the {@link LODManager.lodAngleDeg} property instead.
     */
    Q_INVOKABLE float getOctreeSizeScale() const;

    /**jsdoc
     * @function LODManager.setBoundaryLevelAdjust
     * @param {number} boundaryLevelAdjust
     * @deprecated This function is deprecated and will be removed.
     */
    Q_INVOKABLE void setBoundaryLevelAdjust(int boundaryLevelAdjust);

    /**jsdoc
     * @function LODManager.getBoundaryLevelAdjust
     * @returns {number}
     * @deprecated This function is deprecated and will be removed.
     */
    Q_INVOKABLE int getBoundaryLevelAdjust() const { return _boundaryLevelAdjust; }

    /**jsdoc
    * @function LODManager.getLODTargetFPS
    * @returns {number}
    */
    Q_INVOKABLE float getLODTargetFPS() const;


    float getPresentTime() const { return _presentTime; }
    float getEngineRunTime() const { return _engineRunTime; }
    float getBatchTime() const { return _batchTime; }
    float getGPUTime() const { return _gpuTime; }

    static bool shouldRender(const RenderArgs* args, const AABox& bounds);
    void setRenderTimes(float presentTime, float engineRunTime, float batchTime, float gpuTime);
    void autoAdjustLOD(float realTimeDelta);

    void loadSettings();
    void saveSettings();
    void resetLODAdjust();

    float getNowRenderTime() const { return _nowRenderTime; };
    float getNowRenderFPS() const { return (_nowRenderTime > 0.0f ? (float)MSECS_PER_SECOND / _nowRenderTime : 0.0f); };

    void setSmoothScale(float t);
    float getSmoothScale() const { return _smoothScale; }

    float getSmoothRenderTime() const { return _smoothRenderTime; };
    float getSmoothRenderFPS() const { return (_smoothRenderTime > 0.0f ? (float)MSECS_PER_SECOND / _smoothRenderTime : 0.0f); };

    void setWorldDetailQuality(WorldDetailQuality quality);
    WorldDetailQuality getWorldDetailQuality() const;

    void setLODQualityLevel(float quality);
    float getLODQualityLevel() const;

    float getLODAngleDeg() const;
    void setLODAngleDeg(float lodAngle);
    float getLODHalfAngleTan() const;
    float getLODAngle() const;
    float getVisibilityDistance() const;
    void setVisibilityDistance(float distance);

    float getPidKp() const;
    float getPidKi() const;
    float getPidKd() const;
    float getPidKv() const;
    void setPidKp(float k);
    void setPidKi(float k);
    void setPidKd(float k);
    void setPidKv(float t);

    float getPidOp() const;
    float getPidOi() const;
    float getPidOd() const;
    float getPidO() const;

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

    void autoLODChanged();
    void lodQualityLevelChanged();
    void worldDetailQualityChanged();

private:
    LODManager();

    void setWorldDetailQuality(WorldDetailQuality quality, bool isHMDMode);

    std::mutex _automaticLODLock;
    bool _automaticLODAdjust = true;

    float _presentTime{ 0.0f }; // msec
    float _engineRunTime{ 0.0f }; // msec
    float _batchTime{ 0.0f }; // msec
    float _gpuTime{ 0.0f }; // msec

    float _nowRenderTime{ 0.0f }; // msec
    float _smoothScale{ 10.0f }; // smooth is evaluated over 10 times longer than now
    float _smoothRenderTime{ 0.0f }; // msec

    float _lodQualityLevel{ LOD_DEFAULT_QUALITY_LEVEL };

    WorldDetailQuality _desktopWorldDetailQuality { DEFAULT_WORLD_DETAIL_QUALITY };
    WorldDetailQuality _hmdWorldDetailQuality { DEFAULT_WORLD_DETAIL_QUALITY };

    float _desktopTargetFPS { QUALITY_TO_FPS_DESKTOP[_desktopWorldDetailQuality] };
    float _hmdTargetFPS { QUALITY_TO_FPS_HMD[_hmdWorldDetailQuality] };

    float _lodHalfAngle = getHalfAngleFromVisibilityDistance(DEFAULT_VISIBILITY_DISTANCE_FOR_UNIT_ELEMENT);
    int _boundaryLevelAdjust = 0;

    glm::vec4 _pidCoefs{ 1.0f, 0.0f, 0.0f, 1.0f }; // Kp, Ki, Kd, Kv
    glm::vec4 _pidHistory{ 0.0f };
    glm::vec4 _pidOutputs{ 0.0f };
};

QScriptValue worldDetailQualityToScriptValue(QScriptEngine* engine, const WorldDetailQuality& worldDetailQuality);
void worldDetailQualityFromScriptValue(const QScriptValue& object, WorldDetailQuality& worldDetailQuality);

#endif // hifi_LODManager_h
