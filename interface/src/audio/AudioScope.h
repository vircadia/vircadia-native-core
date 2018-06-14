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
    
    /**jsdoc
     * The AudioScope API helps control the Audio Scope features in Interface
     * @namespace AudioScope
     *
     * @hifi-interface
     * @hifi-client-entity
     *
     * @property {number} scopeInput <em>Read-only.</em>
     * @property {number} scopeOutputLeft <em>Read-only.</em>
     * @property {number} scopeOutputRight <em>Read-only.</em>
     * @property {number} triggerInput <em>Read-only.</em>
     * @property {number} triggerOutputLeft <em>Read-only.</em>
     * @property {number} triggerOutputRight <em>Read-only.</em>
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

    /**jsdoc
     * @function AudioScope.toggle
     */
    void toggle() { setVisible(!_isEnabled); }
     
    /**jsdoc
     * @function AudioScope.setVisible
     * @param {boolean} visible
     */
    void setVisible(bool visible);

    /**jsdoc
     * @function AudioScope.getVisible
     * @returns {boolean} 
     */
    bool getVisible() const { return _isEnabled; }

    /**jsdoc
     * @function AudioScope.togglePause
     */
    void togglePause() { setPause(!_isPaused); }

    /**jsdoc
     * @function AudioScope.setPause
     * @param {boolean} paused
     */
    void setPause(bool paused) { _isPaused = paused; emit pauseChanged(); }

    /**jsdoc
     * @function AudioScope.getPause
     * @returns {boolean}
     */
    bool getPause() { return _isPaused; }

    /**jsdoc
     * @function AudioScope.toggleTrigger
     */
    void toggleTrigger() { _autoTrigger = !_autoTrigger; }

    /**jsdoc
     * @function AudioScope.getAutoTrigger
     * @returns {boolean}
     */
    bool getAutoTrigger() { return _autoTrigger; }

    /**jsdoc
     * @function AudioScope.setAutoTrigger
     * @param {boolean} autoTrigger 
     */
    void setAutoTrigger(bool autoTrigger) { _isTriggered = false; _autoTrigger = autoTrigger; }

    /**jsdoc
     * @function AudioScope.setTriggerValues
     * @param {number} x
     * @param {number} y
     */
    void setTriggerValues(int x, int y) { _triggerValues.x = x; _triggerValues.y = y; }
    
    /**jsdoc
     * @function AudioScope.setTriggered
     * @param {boolean} triggered
     */
    void setTriggered(bool triggered) { _isTriggered = triggered; }
    
    /**jsdoc
     * @function AudioScope.getTriggered
     * @returns {boolean}
     */
    bool getTriggered() { return _isTriggered; }

    /**jsdoc
     * @function AudioScope.getFramesPerSecond
     * @returns {number}
     */
    float getFramesPerSecond();

    /**jsdoc
     * @function AudioScope.getFramesPerScope
     * @returns {number}
     */
    int getFramesPerScope() { return _framesPerScope; }

    /**jsdoc
     * @function AudioScope.selectAudioScopeFiveFrames
     */
    void selectAudioScopeFiveFrames();

    /**jsdoc
     * @function AudioScope.selectAudioScopeTwentyFrames
     */
    void selectAudioScopeTwentyFrames();

    /**jsdoc
     * @function AudioScope.selectAudioScopeFiftyFrames
     */
    void selectAudioScopeFiftyFrames();

    /**jsdoc
     * @function AudioScope.getScopeInput
     * @returns {number[]} 
     */
    QVector<int> getScopeInput() { return _scopeInputData; };

    /**jsdoc
     * @function AudioScope.getScopeOutputLeft
     * @returns {number[]}
     */
    QVector<int> getScopeOutputLeft() { return _scopeOutputLeftData; };

    /**jsdoc
     * @function AudioScope.getScopeOutputRight
     * @returns {number[]}
     */
    QVector<int> getScopeOutputRight() { return _scopeOutputRightData; };

    /**jsdoc
     * @function AudioScope.getTriggerInput
     * @returns {number[]}
     */
    QVector<int> getTriggerInput() { return _triggerInputData; };

    /**jsdoc
     * @function AudioScope.getTriggerOutputLeft
     * @returns {number[]}
     */
    QVector<int> getTriggerOutputLeft() { return _triggerOutputLeftData; };
   
    /**jsdoc
     * @function AudioScope.getTriggerOutputRight
     * @returns {number[]}
     */
    QVector<int> getTriggerOutputRight() { return _triggerOutputRightData; };

    /**jsdoc
     * @function AudioScope.setLocalEcho
     * @parm {boolean} localEcho
     */
    void setLocalEcho(bool localEcho);

    /**jsdoc
     * @function AudioScope.setServerEcho
     * @parm {boolean} serverEcho
     */
    void setServerEcho(bool serverEcho);

signals:

    /**jsdoc
     * @function AudioScope.pauseChanged
     * @returns {Signal}
     */
    void pauseChanged();

    /**jsdoc
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
