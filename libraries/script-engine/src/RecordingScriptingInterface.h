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
#include <mutex>

#include <QtCore/QObject>

#include <DependencyManager.h>
#include <recording/Forward.h>
#include <recording/Frame.h>

class QScriptValue;

class RecordingScriptingInterface : public QObject, public Dependency {
    Q_OBJECT

public:
    RecordingScriptingInterface();

public slots:
    bool loadRecording(const QString& url);

    void startPlaying();
    void pausePlayer();
    void stopPlaying();
    bool isPlaying() const;
    bool isPaused() const;

    float playerElapsed() const;
    float playerLength() const;

    void setPlayerVolume(float volume);
    void setPlayerAudioOffset(float audioOffset);
    void setPlayerTime(float time);
    void setPlayerLoop(bool loop);

    void setPlayerUseDisplayName(bool useDisplayName);
    void setPlayerUseAttachments(bool useAttachments);
    void setPlayerUseHeadModel(bool useHeadModel);
    void setPlayerUseSkeletonModel(bool useSkeletonModel);
    void setPlayFromCurrentLocation(bool playFromCurrentLocation);

    bool getPlayerUseDisplayName() { return _useDisplayName; }
    bool getPlayerUseAttachments() { return _useAttachments; }
    bool getPlayerUseHeadModel() { return _useHeadModel; }
    bool getPlayerUseSkeletonModel() { return _useSkeletonModel; }
    bool getPlayFromCurrentLocation() { return _playFromCurrentLocation; }

    void startRecording();
    void stopRecording();
    bool isRecording() const;

    float recorderElapsed() const;

    void saveRecording(const QString& filename);
    bool saveRecordingToAsset(QScriptValue getClipAtpUrl);
    void loadLastRecording();

protected:
    using Mutex = std::recursive_mutex;
    using Locker = std::unique_lock<Mutex>;
    using Flag = std::atomic<bool>;

    QSharedPointer<recording::Deck> _player;
    QSharedPointer<recording::Recorder> _recorder;
    
    Flag _playFromCurrentLocation { true };
    Flag _useDisplayName { false };
    Flag _useHeadModel { false };
    Flag _useAttachments { false };
    Flag _useSkeletonModel { false };
    recording::ClipPointer _lastClip;
};

#endif // hifi_RecordingScriptingInterface_h
