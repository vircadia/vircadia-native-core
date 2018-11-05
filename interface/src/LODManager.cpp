//
//  LODManager.cpp
//  interface/src/LODManager.h
//
//  Created by Clement on 1/16/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "LODManager.h"

#include <SettingHandle.h>
#include <OctreeUtils.h>
#include <Util.h>

#include "Application.h"
#include "ui/DialogsManager.h"
#include "InterfaceLogging.h"

const float LODManager::DEFAULT_DESKTOP_LOD_DOWN_FPS = LOD_DEFAULT_QUALITY_LEVEL * LOD_MAX_LIKELY_DESKTOP_FPS;
const float LODManager::DEFAULT_HMD_LOD_DOWN_FPS = LOD_DEFAULT_QUALITY_LEVEL * LOD_MAX_LIKELY_HMD_FPS;

Setting::Handle<float> desktopLODDecreaseFPS("desktopLODDecreaseFPS", LODManager::DEFAULT_DESKTOP_LOD_DOWN_FPS);
Setting::Handle<float> hmdLODDecreaseFPS("hmdLODDecreaseFPS", LODManager::DEFAULT_HMD_LOD_DOWN_FPS);

LODManager::LODManager() {
}

// We use a "time-weighted running average" of the maxRenderTime and compare it against min/max thresholds
// to determine if we should adjust the level of detail (LOD).
//
// A time-weighted running average has a timescale which determines how fast the average tracks the measured
// value in real-time.  Given a step-function in the mesured value, and assuming measurements happen
// faster than the runningAverage is computed, the error between the value and its runningAverage will be
// reduced by 1/e every timescale of real-time that passes.
const float LOD_ADJUST_RUNNING_AVG_TIMESCALE = 0.08f; // sec

// batchTIme is always contained in presentTime.
// We favor using batchTime instead of presentTime as a representative value for rendering duration (on present thread) 
// if batchTime + cushionTime < presentTime.
// since we are shooting for fps around 60, 90Hz, the ideal frames are around 10ms
// so we are picking a cushion time of 3ms 
const float LOD_BATCH_TO_PRESENT_CUSHION_TIME = 3.0f; // msec

void LODManager::setRenderTimes(float presentTime, float engineRunTime, float batchTime, float gpuTime) {
    // Make sure the sampled time are positive values
    _presentTime = std::max(0.0f, presentTime);
    _engineRunTime = std::max(0.0f, engineRunTime);
    _batchTime = std::max(0.0f, batchTime);
    _gpuTime = std::max(0.0f, gpuTime);
}

void LODManager::autoAdjustLOD(float realTimeDelta) {
 
    // The "render time" is the worse of:
    // - engineRunTime: Time spent in the render thread in the engine producing the gpu::Frame N
    // - batchTime: Time spent in the present thread processing the batches of gpu::Frame N+1
    // - presentTime: Time spent in the present thread between the last 2 swap buffers considered the total time to submit gpu::Frame N+1
    // - gpuTime: Time spent in the GPU executing the gpu::Frame N + 2

    // But Present time is in reality synched with the monitor/display refresh rate, it s always longer than batchTime.
    // So if batchTime is fast enough relative to presentTime we are using it, otherwise we are using presentTime. got it ?
    auto presentTime = (_presentTime > _batchTime + LOD_BATCH_TO_PRESENT_CUSHION_TIME ? _batchTime + LOD_BATCH_TO_PRESENT_CUSHION_TIME : _presentTime);
    float maxRenderTime = glm::max(glm::max(presentTime, _engineRunTime), _gpuTime);

    // maxRenderTime must be a realistic valid duration in order for the regulation to work correctly.
    // We make sure it s a non zero positive value (1.0ms) under 1 sec
    maxRenderTime = std::max(1.0f, std::min(maxRenderTime, (float)MSECS_PER_SECOND));

    // realTimeDelta must be a realistic valid duration in order for the regulation to work correctly.
    // We make sure it a positive value under 1 sec
    // note that if real time delta is very small we will early exit to avoid division by zero
    realTimeDelta = std::max(0.0f, std::min(realTimeDelta, 1.0f));

    // compute time-weighted running average render time (now and smooth)
    // We MUST clamp the blend between 0.0 and 1.0 for stability
    float nowBlend = (realTimeDelta < LOD_ADJUST_RUNNING_AVG_TIMESCALE) ? realTimeDelta / LOD_ADJUST_RUNNING_AVG_TIMESCALE : 1.0f;
    float smoothBlend = (realTimeDelta <  LOD_ADJUST_RUNNING_AVG_TIMESCALE * _smoothScale) ? realTimeDelta / (LOD_ADJUST_RUNNING_AVG_TIMESCALE * _smoothScale) : 1.0f;

    //Evaluate the running averages for the render time
    // We must sanity check for the output average evaluated to be in a valid range to avoid issues 
    _nowRenderTime = (1.0f - nowBlend) * _nowRenderTime + nowBlend * maxRenderTime; // msec
    _nowRenderTime = std::max(0.0f, std::min(_nowRenderTime, (float)MSECS_PER_SECOND));
    _smoothRenderTime = (1.0f - smoothBlend) * _smoothRenderTime + smoothBlend * maxRenderTime; // msec
    _smoothRenderTime = std::max(0.0f, std::min(_smoothRenderTime, (float)MSECS_PER_SECOND));

    // Early exit if not regulating or if the simulation or render times don't matter
    if (!_automaticLODAdjust || realTimeDelta <= 0.0f || _nowRenderTime <= 0.0f || _smoothRenderTime <= 0.0f) {
        return;
    }

    // Previous values for output
    float oldOctreeSizeScale = getOctreeSizeScale();
    float oldLODAngle = getLODAngleDeg();

    // Target fps is slightly overshooted by 5hz
    float targetFPS = getLODTargetFPS() + LOD_OFFSET_FPS;

    // Current fps based on latest measurments
    float currentNowFPS = (float)MSECS_PER_SECOND / _nowRenderTime;
    float currentSmoothFPS = (float)MSECS_PER_SECOND / _smoothRenderTime;
 
    // Compute the Variance of the FPS signal (FPS - smouthFPS)^2
    // Also scale it by a percentage for fine tuning (default is 100%)
    float currentVarianceFPS = (currentSmoothFPS - currentNowFPS);
    currentVarianceFPS *= currentVarianceFPS;
    currentVarianceFPS *= _pidCoefs.w;

    // evaluate current error between the current smoothFPS and target FPS
    // and the sqaure of the error to compare against the Variance
    auto currentErrorFPS = (targetFPS - currentSmoothFPS);
    auto currentErrorFPSSquare = currentErrorFPS * currentErrorFPS;

    // Define a noiseCoef that is trying to adjust the error to the FPS target value based on its strength
    // relative to the current Variance of the FPS signal.
    // If the error is within the variance, just set to 0.
    // if its within 2x the variance scale the control
    // and full control if error is bigger than 2x variance
    auto noiseCoef = 1.0f;
    if (currentErrorFPSSquare < currentVarianceFPS) {
        noiseCoef = 0.0f;
    } else if (currentErrorFPSSquare < 2.0f * currentVarianceFPS) {
        noiseCoef = (currentErrorFPSSquare - currentVarianceFPS) / currentVarianceFPS;
    }

    // The final normalized error is the the error to the FPS target, weighted by the noiseCoef, then normailzed by the target FPS.
    // it s also clamped in the [-1, 1] range
    auto error = noiseCoef * currentErrorFPS / targetFPS;
    error = glm::clamp(error, -1.0f, 1.0f);

    // Now we are getting into the P.I.D. controler code
    // retreive the history of pid error and integral
    auto previous_error = _pidHistory.x;
    auto previous_integral = _pidHistory.y;

    // The dt used for temporal values of the controller is the current realTimedelta
    // clamped to a reasonable granularity to make sure we are not over reacting
    auto dt = std::min(realTimeDelta, LOD_ADJUST_RUNNING_AVG_TIMESCALE);

    // Compute the current integral and clamp to avoid accumulation
    auto integral = previous_integral + error * dt;
    glm::clamp(integral, -1.0f, 1.0f);

    // Compute derivative
    // dt is never zero because realTimeDelta would have early exit above, but if it ever was let's zero the derivative term
    auto derivative = (dt <= 0.0f ? 0.0f : (error - previous_error) / dt);

    // remember history
    _pidHistory.x = error;
    _pidHistory.y = integral;
    _pidHistory.z = derivative;

    // Compute the output of the PID and record intermediate results for tuning
    _pidOutputs.x = _pidCoefs.x * error;        // Kp * error
    _pidOutputs.y = _pidCoefs.y * integral;     // Ki * integral 
    _pidOutputs.z = _pidCoefs.z * derivative;   // Kd * derivative

    auto output = _pidOutputs.x + _pidOutputs.y + _pidOutputs.z;
    _pidOutputs.w = output;

    // And now add the output of the controller to the LODAngle where we will guarantee it is in the proper range
    setLODAngleDeg(oldLODAngle + output);

    if (oldOctreeSizeScale != _octreeSizeScale) {
        auto lodToolsDialog = DependencyManager::get<DialogsManager>()->getLodToolsDialog();
        if (lodToolsDialog) {
            lodToolsDialog->reloadSliders();
        }
    }
}

float LODManager::getLODAngleHalfTan() const {
    return getPerspectiveAccuracyAngleTan(_octreeSizeScale, _boundaryLevelAdjust);
}
float LODManager::getLODAngle() const {
    return 2.0f * atanf(getLODAngleHalfTan());
}
float LODManager::getLODAngleDeg() const {
    return glm::degrees(getLODAngle());
}

void LODManager::setLODAngleDeg(float lodAngle) {
    auto newSolidAngle = std::max(0.5f, std::min(lodAngle, 90.f));
    auto halTan = glm::tan(glm::radians(newSolidAngle * 0.5f));
    auto octreeSizeScale = TREE_SCALE * OCTREE_TO_MESH_RATIO / halTan;
    setOctreeSizeScale(octreeSizeScale);
}

void LODManager::setSmoothScale(float t) {
    _smoothScale = glm::max(1.0f, t);
}

float LODManager::getPidKp() const {
    return _pidCoefs.x;
}
float LODManager::getPidKi() const {
    return _pidCoefs.y;
}
float LODManager::getPidKd() const {
    return _pidCoefs.z;
}
float LODManager::getPidKv() const {
    return _pidCoefs.w;
}
void LODManager::setPidKp(float k) {
    _pidCoefs.x = k;
}
void LODManager::setPidKi(float k) {
    _pidCoefs.y = k;
}
void LODManager::setPidKd(float k) {
    _pidCoefs.z = k;
}
void LODManager::setPidKv(float t) {
    _pidCoefs.w = t;
}

float LODManager::getPidOp() const {
    return _pidOutputs.x;
}
float LODManager::getPidOi() const {
    return _pidOutputs.y;
}
float LODManager::getPidOd() const {
    return _pidOutputs.z;
}
float LODManager::getPidO() const {
    return _pidOutputs.w;
}

void LODManager::resetLODAdjust() {
}

void LODManager::setAutomaticLODAdjust(bool value) {
    _automaticLODAdjust = value;
    emit autoLODChanged();
}

bool LODManager::shouldRender(const RenderArgs* args, const AABox& bounds) {
    // To decide if the bound should be rendered or not at the specified Args->lodAngle,
    // we need to compute the apparent angle of the bound from the frustum origin,
    // and compare it against the lodAngle, if it is greater or equal we should render the content of that bound.
    // we abstract the bound as a sphere centered on the bound center and of radius half diagonal of the bound.

    // Instead of comparing  angles, we are comparing the tangent of the half angle which are more efficient to compute:
    // we are comparing the square of the half tangent apparent angle for the bound against the LODAngle Half tangent square
    // if smaller, the bound is too small and we should NOT render it, return true otherwise.

    // Tangent Adjacent side is eye to bound center vector length
    auto pos = args->getViewFrustum().getPosition() - bounds.calcCenter();
    auto halfTanAdjacentSq = glm::dot(pos, pos);

    // Tangent Opposite side is the half length of the dimensions vector of the bound
    auto dim = bounds.getDimensions();
    auto halfTanOppositeSq = 0.25f * glm::dot(dim, dim);

    // The test is:
    // isVisible = halfTanSq >= lodHalfTanSq = (halfTanOppositeSq / halfTanAdjacentSq) >= lodHalfTanSq
    // which we express as below to avoid division
    // (halfTanOppositeSq) >= lodHalfTanSq * halfTanAdjacentSq
    return (halfTanOppositeSq >= args->_lodAngleHalfTanSq * halfTanAdjacentSq);
};

void LODManager::setOctreeSizeScale(float sizeScale) {
    _octreeSizeScale = sizeScale;
}

void LODManager::setBoundaryLevelAdjust(int boundaryLevelAdjust) {
    _boundaryLevelAdjust = boundaryLevelAdjust;
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
    if (relativeToDefault > 1.01f) {
        result = QString("20:%1 or %2 times further than average vision%3").arg(relativeToTwentyTwenty).arg(relativeToDefault, 0, 'f', 2).arg(granularityFeedback);
    } else if (relativeToDefault > 0.99f) {
        result = QString("20:20 or the default distance for average vision%1").arg(granularityFeedback);
    } else if (relativeToDefault > 0.01f) {
        result = QString("20:%1 or %2 of default distance for average vision%3").arg(relativeToTwentyTwenty).arg(relativeToDefault, 0, 'f', 3).arg(granularityFeedback);
    } else {
        result = QString("%2 of default distance for average vision%3").arg(relativeToDefault, 0, 'f', 3).arg(granularityFeedback);
    }
    return result;
}

void LODManager::loadSettings() {
    setDesktopLODTargetFPS(desktopLODDecreaseFPS.get());
    setHMDLODTargetFPS(hmdLODDecreaseFPS.get());
}

void LODManager::saveSettings() {
    desktopLODDecreaseFPS.set(getDesktopLODTargetFPS());
    hmdLODDecreaseFPS.set(getHMDLODTargetFPS());
}

const float MIN_DECREASE_FPS = 0.5f;

void LODManager::setDesktopLODTargetFPS(float fps) {
    if (fps < MIN_DECREASE_FPS) {
        // avoid divide by zero
        fps = MIN_DECREASE_FPS;
    }
    _desktopTargetFPS = fps;
}

float LODManager::getDesktopLODTargetFPS() const {
    return _desktopTargetFPS;
}

void LODManager::setHMDLODTargetFPS(float fps) {
    if (fps < MIN_DECREASE_FPS) {
        // avoid divide by zero
        fps = MIN_DECREASE_FPS;
    }
    _hmdTargetFPS = fps;
}

float LODManager::getHMDLODTargetFPS() const {
    return _hmdTargetFPS;
}

float LODManager::getLODTargetFPS() const {
    if (qApp->isHMDMode()) {
        return getHMDLODTargetFPS();
    }
    return getDesktopLODTargetFPS();
}

void LODManager::setWorldDetailQuality(float quality) {
    static const float MIN_FPS = 10;
    static const float LOW = 0.25f;

    bool isLowestValue = quality == LOW;
    bool isHMDMode = qApp->isHMDMode();

    float maxFPS = isHMDMode ? LOD_MAX_LIKELY_HMD_FPS : LOD_MAX_LIKELY_DESKTOP_FPS;
    float desiredFPS = maxFPS;

    if (!isLowestValue) {
        float calculatedFPS = (maxFPS - (maxFPS * quality));
        desiredFPS = calculatedFPS < MIN_FPS ? MIN_FPS : calculatedFPS;
    }

    if (isHMDMode) {
        setHMDLODTargetFPS(desiredFPS);
    } else {
        setDesktopLODTargetFPS(desiredFPS);
    }

    emit worldDetailQualityChanged();
}

float LODManager::getWorldDetailQuality() const {

    static const float LOW = 0.25f;
    static const float MEDIUM = 0.5f;
    static const float HIGH = 0.75f;

    bool inHMD = qApp->isHMDMode();

    float targetFPS = 0.0f;
    if (inHMD) {
        targetFPS = getHMDLODTargetFPS();
    } else {
        targetFPS = getDesktopLODTargetFPS();
    }
    float maxFPS = inHMD ? LOD_MAX_LIKELY_HMD_FPS : LOD_MAX_LIKELY_DESKTOP_FPS;
    float percentage = 1.0f - targetFPS / maxFPS;

    if (percentage <= LOW) {
        return LOW;
    } else if (percentage <= MEDIUM) {
        return MEDIUM;
    }

    return HIGH;
}


void LODManager::setLODQualityLevel(float quality) {
    _lodQualityLevel = quality;
}

float LODManager::getLODQualityLevel() const {
    return _lodQualityLevel;
}
