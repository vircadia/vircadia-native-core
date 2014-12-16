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

#include <DependencyManager.h>

#include <QByteArray>
#include <QObject>

class AudioScope : public QObject, public DependencyManager::Dependency {
    Q_OBJECT
public:
    // Audio scope methods for data acquisition
    int addBufferToScope(QByteArray* byteArray, int frameOffset, const int16_t* source, int sourceSamples,
                         unsigned int sourceChannel, unsigned int sourceNumberOfChannels, float fade = 1.0f);
    int addSilenceToScope(QByteArray* byteArray, int frameOffset, int silentSamples);
    
    // Audio scope methods for rendering
    static void renderBackground(const float* color, int x, int y, int width, int height);
    void renderGrid(const float* color, int x, int y, int width, int height, int rows, int cols);
    void renderLineStrip(const float* color, int x, int  y, int n, int offset, const QByteArray* byteArray);
    
    // Audio scope methods for allocation/deallocation
    void allocateScope();
    void freeScope();
    void reallocateScope(int frames);
    
    void render(int width, int height);
    
    friend class DependencyManager;
    
public slots:
    void toggleScope();
    void toggleScopePause() { _isPaused = !_isPaused; }
    void selectAudioScopeFiveFrames();
    void selectAudioScopeTwentyFrames();
    void selectAudioScopeFiftyFrames();
    void addStereoSilenceToScope(int silentSamplesPerChannel);
    void addLastFrameRepeatedWithFadeToScope(int samplesPerChannel);
    void addStereoSamplesToScope(const QByteArray& samples);
    
protected:
    AudioScope();
private:
    bool _isEnabled;
    bool _isPaused;
    int _scopeInputOffset;
    int _scopeOutputOffset;
    int _framesPerScope;
    int _samplesPerScope;
    QByteArray* _scopeInput;
    QByteArray* _scopeOutputLeft;
    QByteArray* _scopeOutputRight;
    QByteArray _scopeLastFrame;
    QMutex _guard;
};

#endif // hifi_AudioScope_h