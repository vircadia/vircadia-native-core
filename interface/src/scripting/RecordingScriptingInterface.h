//
//  Created by Bradley Austin Davis on 2015/11/13
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_RecordingScriptingInterface_h
#define hifi_RecordingScriptingInterface_h

#include <atomic>

#include <QObject>

#include <DependencyManager.h>
#include <recording/Forward.h>
#include <recording/Frame.h>
#include <AvatarData.h>

class RecordingScriptingInterface : public QObject, public Dependency {
    Q_OBJECT

public:
    RecordingScriptingInterface();

public slots:
    bool isPlaying();
    bool isPaused();
    float playerElapsed();
    float playerLength();
    void loadRecording(const QString& filename);
    void startPlaying();
    void setPlayerVolume(float volume);
    void setPlayerAudioOffset(float audioOffset);
    void setPlayerTime(float time);
    void setPlayFromCurrentLocation(bool playFromCurrentLocation);
    void setPlayerLoop(bool loop);
    void setPlayerUseDisplayName(bool useDisplayName);
    void setPlayerUseAttachments(bool useAttachments);
    void setPlayerUseHeadModel(bool useHeadModel);
    void setPlayerUseSkeletonModel(bool useSkeletonModel);
    void play();
    void pausePlayer();
    void stopPlaying();
    bool isRecording();
    float recorderElapsed();
    void startRecording();
    void stopRecording();
    void saveRecording(const QString& filename);
    void loadLastRecording();

signals:
    void playbackStateChanged();
    // Should this occur for any frame or just for seek calls?
    void playbackPositionChanged();
    void looped();

private:
    using Mutex = std::recursive_mutex;
    using Locker = std::unique_lock<Mutex>;
    using Flag = std::atomic<bool>;
    void processAvatarFrame(const recording::FrameConstPointer& frame);
    void processAudioFrame(const recording::FrameConstPointer& frame);
    void processAudioInput(const QByteArray& audioData);
    QSharedPointer<recording::Deck> _player;
    QSharedPointer<recording::Recorder> _recorder;
    quint64 _recordingEpoch { 0 };
    
    Flag _playFromCurrentLocation { true };
    Flag _useDisplayName { false };
    Flag _useHeadModel { false };
    Flag _useAttachments { false };
    Flag _useSkeletonModel { false };
    recording::ClipPointer _lastClip;
    AvatarData _dummyAvatar;
};

#endif // hifi_RecordingScriptingInterface_h
