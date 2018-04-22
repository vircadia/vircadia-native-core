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
	* @property {int} scopeInput - To Be Completed
    * @property {int} scopeOutputLeft - To Be Completed
    * @property {int} scopeOutputRight - To Be Completed
    * @property {int} triggerInput - To Be Completed
    * @property {int} triggerOutputLeft - To Be Completed
    * @property {int} triggerOutputRight - To Be Completed
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
	* To Be Completed
	* @function AudioScope.toggle
	*/
    void toggle() { setVisible(!_isEnabled); }
    /**jsdoc
    * To Be Completed
	* @param {boolean} visible
    * @function AudioScope.setVisible
    */
    void setVisible(bool visible);
    /**jsdoc
    * To Be Completed
    * @param {boolean} visible
    * @function AudioScope.getVisible
	* @returns {boolean} 
    */
    bool getVisible() const { return _isEnabled; }
    /**jsdoc
    * To Be Completed
    * @function AudioScope.togglePause
    */
    void togglePause() { setPause(!_isPaused); }
    /**jsdoc
    * To Be Completed
    * @param {boolean} paused
    * @function AudioScope.setPause
    */
    void setPause(bool paused) { _isPaused = paused; emit pauseChanged(); }
    /**jsdoc
    * To Be Completed
    * @function AudioScope.getPause
	* @returns {boolean} isPaused
    */
    bool getPause() { return _isPaused; }
    /**jsdoc
    * To Be Completed
    * @function AudioScope.toggleTrigger
    */
    void toggleTrigger() { _autoTrigger = !_autoTrigger; }
    /**jsdoc
    * To Be Completed
    * @function AudioScope.getAutoTrigger
	* @returns {boolean} autoTrigger
    */
    bool getAutoTrigger() { return _autoTrigger; }
    /**jsdoc
    * To Be Completed
    * @function AudioScope.setAutoTrigger
	* @param {boolean} autoTrigger 
    */
    void setAutoTrigger(bool autoTrigger) { _isTriggered = false; _autoTrigger = autoTrigger; }
    /**jsdoc
    * To Be Completed
    * @function AudioScope.setTriggerValues
    * @param {number} x
    * @param {number} y
    */
    void setTriggerValues(int x, int y) { _triggerValues.x = x; _triggerValues.y = y; }
    /**jsdoc
    * To Be Completed
    * @function AudioScope.setTriggered
    * @param {boolean} triggered
    */
    void setTriggered(bool triggered) { _isTriggered = triggered; }
    /**jsdoc
    * To Be Completed
    * @function AudioScope.getTriggered
    * @returns {boolean}
    */
    bool getTriggered() { return _isTriggered; }

    /**jsdoc
    * To Be Completed
    * @function AudioScope.getFramesPerSecond
    * @returns {number}
    */
    float getFramesPerSecond();
    /**jsdoc
    * To Be Completed
    * @function AudioScope.getFramesPerScope
    * @returns {number}
    */
    int getFramesPerScope() { return _framesPerScope; }

    /**jsdoc
    * To Be Completed
    * @function AudioScope.selectAudioScopeFiveFrames
    */
    void selectAudioScopeFiveFrames();
    /**jsdoc
    * To Be Completed
    * @function AudioScope.selectAudioScopeTwentyFrames
    */
    void selectAudioScopeTwentyFrames();
    /**jsdoc
    * To Be Completed
    * @function AudioScope.selectAudioScopeFiftyFrames
    */
    void selectAudioScopeFiftyFrames();

    /**jsdoc
    * To Be Completed
    * @function AudioScope.getScopeInput
	* @returns {number} 
    */
    QVector<int> getScopeInput() { return _scopeInputData; };
    /**jsdoc
    * To Be Completed
    * @function AudioScope.getScopeOutputLeft
    * @returns {number}
    */
    QVector<int> getScopeOutputLeft() { return _scopeOutputLeftData; };
    /**jsdoc
    * To Be Completed
    * @function AudioScope.getScopeOutputRight
    * @returns {number}
    */
    QVector<int> getScopeOutputRight() { return _scopeOutputRightData; };

    /**jsdoc
    * To Be Completed
    * @function AudioScope.getTriggerInput
    * @returns {number}
    */
    QVector<int> getTriggerInput() { return _triggerInputData; };
    /**jsdoc
    * To Be Completed
    * @function AudioScope.getTriggerOutputLeft
    * @returns {number}
    */
    QVector<int> getTriggerOutputLeft() { return _triggerOutputLeftData; };
    /**jsdoc
    * To Be Completed
    * @function AudioScope.getTriggerOutputRight
    * @returns {number}
    */
    QVector<int> getTriggerOutputRight() { return _triggerOutputRightData; };

    /**jsdoc
    * To Be Completed
    * @function AudioScope.setLocalEcho
    * @parm {boolean} serverEcho
    */
    void setLocalEcho(bool serverEcho);
    /**jsdoc
    * To Be Completed
    * @function AudioScope.setServerEcho
    * @parm {boolean} serverEcho
    */
    void setServerEcho(bool serverEcho);

signals:
    /**jsdoc
    * To Be Completed
    * @function AudioScope.pauseChanged
	* @returns {Signal}
    */
    void pauseChanged();
    /**jsdoc
    * To Be Completed
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
