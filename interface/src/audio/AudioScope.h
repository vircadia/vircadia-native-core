//
//  AudioScope.h
//  interace/src/audio
//
//  Created by Stephen Birarda on 2014-12-16.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AudioScope_h
#define hifi_AudioScope_h

#include <glm/glm.hpp>

#include <QByteArray>
#include <QObject>

#include <DependencyManager.h>
#include <gpu/Batch.h>


class AudioScope : public QObject, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY
    
    /*@jsdoc
     * The <code>AudioScope</code> API provides facilities for an audio scope.
     *
     * @namespace AudioScope
     *
     * @deprecated This API doesn't work properly. It is deprecated and will be removed.
     *
     * @hifi-interface
     * @hifi-client-entity
     * @hifi-avatar
     *
     * @property {number[]} scopeInput - Scope input. <em>Read-only.</em>
     * @property {number[]} scopeOutputLeft - Scope left output. <em>Read-only.</em>
     * @property {number[]} scopeOutputRight - Scope right output. <em>Read-only.</em>
     * @property {number[]} triggerInput - Trigger input. <em>Read-only.</em>
     * @property {number[]} triggerOutputLeft - Trigger left output. <em>Read-only.</em>
     * @property {number[]} triggerOutputRight - Trigger right output. <em>Read-only.</em>
     */

    Q_PROPERTY(QVector<int> scopeInput READ getScopeInput)
    Q_PROPERTY(QVector<int> scopeOutputLeft READ getScopeOutputLeft)
    Q_PROPERTY(QVector<int> scopeOutputRight READ getScopeOutputRight)

    Q_PROPERTY(QVector<int> triggerInput READ getTriggerInput)
    Q_PROPERTY(QVector<int> triggerOutputLeft READ getTriggerOutputLeft)
    Q_PROPERTY(QVector<int> triggerOutputRight READ getTriggerOutputRight)

public:
    // Audio scope methods for allocation/deallocation
    void allocateScope();
    void freeScope();
    void reallocateScope(int frames);
    
public slots:

    /*@jsdoc
     * Toggle.
     * @function AudioScope.toggle
     */
    void toggle() { setVisible(!_isEnabled); }
     
    /*@jsdoc
     * Set visible.
     * @function AudioScope.setVisible
     * @param {boolean} visible - Visible.
     */
    void setVisible(bool visible);

    /*@jsdoc
     * Get visible.
     * @function AudioScope.getVisible
     * @returns {boolean} Visible.
     */
    bool getVisible() const { return _isEnabled; }

    /*@jsdoc
     * Toggle pause.
     * @function AudioScope.togglePause
     */
    void togglePause() { setPause(!_isPaused); }

    /*@jsdoc
     * Set pause.
     * @function AudioScope.setPause
     * @param {boolean} pause - Pause.
     */
    void setPause(bool paused) { _isPaused = paused; emit pauseChanged(); }

    /*@jsdoc
     * Get pause.
     * @function AudioScope.getPause
     * @returns {boolean} Pause.
     */
    bool getPause() { return _isPaused; }

    /*@jsdoc
     * Toggle trigger.
     * @function AudioScope.toggleTrigger
     */
    void toggleTrigger() { _autoTrigger = !_autoTrigger; }

    /*@jsdoc
     * Get auto trigger.
     * @function AudioScope.getAutoTrigger
     * @returns {boolean} Auto trigger.
     */
    bool getAutoTrigger() { return _autoTrigger; }

    /*@jsdoc
     * Set auto trigger.
     * @function AudioScope.setAutoTrigger
     * @param {boolean} autoTrigger - Auto trigger.
     */
    void setAutoTrigger(bool autoTrigger) { _isTriggered = false; _autoTrigger = autoTrigger; }

    /*@jsdoc
     * Set trigger values.
     * @function AudioScope.setTriggerValues
     * @param {number} x - X.
     * @param {number} y - Y.
     */
    void setTriggerValues(int x, int y) { _triggerValues.x = x; _triggerValues.y = y; }
    
    /*@jsdoc
     * Set triggered.
     * @function AudioScope.setTriggered
     * @param {boolean} triggered - Triggered.
     */
    void setTriggered(bool triggered) { _isTriggered = triggered; }
    
    /*@jsdoc
     * Get triggered.
     * @function AudioScope.getTriggered
     * @returns {boolean} Triggered.
     */
    bool getTriggered() { return _isTriggered; }

    /*@jsdoc
     * Get frames per second.
     * @function AudioScope.getFramesPerSecond
     * @returns {number} Frames per second.
     */
    float getFramesPerSecond();

    /*@jsdoc
     * Get frames per scope.
     * @function AudioScope.getFramesPerScope
     * @returns {number} Frames per scope.
     */
    int getFramesPerScope() { return _framesPerScope; }

    /*@jsdoc
     * Select five frames audio scope.
     * @function AudioScope.selectAudioScopeFiveFrames
     */
    void selectAudioScopeFiveFrames();

    /*@jsdoc
     * Select twenty frames audio scope.
     * @function AudioScope.selectAudioScopeTwentyFrames
     */
    void selectAudioScopeTwentyFrames();

    /*@jsdoc
     * Select fifty frames audio scope.
     * @function AudioScope.selectAudioScopeFiftyFrames
     */
    void selectAudioScopeFiftyFrames();

    /*@jsdoc
     * Get scope input.
     * @function AudioScope.getScopeInput
     * @returns {number[]} Scope input.
     */
    QVector<int> getScopeInput() { return _scopeInputData; };

    /*@jsdoc
     * Get scope left output.
     * @function AudioScope.getScopeOutputLeft
     * @returns {number[]} Scope left output.
     */
    QVector<int> getScopeOutputLeft() { return _scopeOutputLeftData; };

    /*@jsdoc
     * Get scope right output.
     * @function AudioScope.getScopeOutputRight
     * @returns {number[]} Scope right output.
     */
    QVector<int> getScopeOutputRight() { return _scopeOutputRightData; };

    /*@jsdoc
     * Get trigger input.
     * @function AudioScope.getTriggerInput
     * @returns {number[]} Trigger input.
     */
    QVector<int> getTriggerInput() { return _triggerInputData; };

    /*@jsdoc
     * Get left trigger output.
     * @function AudioScope.getTriggerOutputLeft
     * @returns {number[]} Left trigger output.
     */
    QVector<int> getTriggerOutputLeft() { return _triggerOutputLeftData; };
   
    /*@jsdoc
     * Get right trigger output.
     * @function AudioScope.getTriggerOutputRight
     * @returns {number[]} Right trigger output.
     */
    QVector<int> getTriggerOutputRight() { return _triggerOutputRightData; };

    /*@jsdoc
     * Set local echo.
     * @function AudioScope.setLocalEcho
     * @parm {boolean} localEcho - Local echo.
     */
    void setLocalEcho(bool localEcho);

    /*@jsdoc
     * Set server echo.
     * @function AudioScope.setServerEcho
     * @parm {boolean} serverEcho - Server echo.
     */
    void setServerEcho(bool serverEcho);

signals:

    /*@jsdoc
     * Triggered when pause changes.
     * @function AudioScope.pauseChanged
     * @returns {Signal}
     */
    void pauseChanged();

    /*@jsdoc
     * Triggered when scope is triggered.
     * @function AudioScope.triggered
     * @returns {Signal}
     */
    void triggered();

protected:
    AudioScope();
    
private slots:
    void addStereoSilenceToScope(int silentSamplesPerChannel);
    void addLastFrameRepeatedWithFadeToScope(int samplesPerChannel);
    void addStereoSamplesToScope(const QByteArray& samples);
    void addInputToScope(const QByteArray& inputSamples);
    
private:
    
    // Audio scope methods for data acquisition
    int addBufferToScope(QByteArray* byteArray, int frameOffset, const int16_t* source, int sourceSamples,
                         unsigned int sourceChannel, unsigned int sourceNumberOfChannels, float fade = 1.0f);
    int addSilenceToScope(QByteArray* byteArray, int frameOffset, int silentSamples);
    
    QVector<int> getScopeVector(const QByteArray* scope, int offset);

    bool shouldTrigger(const QVector<int>& scope);
    void computeInputData();
    void computeOutputData();

    void storeTriggerValues();

    bool _isEnabled;
    bool _isPaused;
    bool _isTriggered;
    bool _autoTrigger;
    int _scopeInputOffset;
    int _scopeOutputOffset;
    int _framesPerScope;
    int _samplesPerScope;

    QByteArray* _scopeInput;
    QByteArray* _scopeOutputLeft;
    QByteArray* _scopeOutputRight;
    QByteArray _scopeLastFrame;
    
    QVector<int> _scopeInputData;
    QVector<int> _scopeOutputLeftData;
    QVector<int> _scopeOutputRightData;

    QVector<int> _triggerInputData;
    QVector<int> _triggerOutputLeftData;
    QVector<int> _triggerOutputRightData;

    
    glm::ivec2 _triggerValues;

    int _audioScopeBackground;
    int _audioScopeGrid;
    int _inputID;
    int _outputLeftID;
    int _outputRightD;
};

#endif // hifi_AudioScope_h
