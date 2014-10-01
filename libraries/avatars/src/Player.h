//
//  Player.h
//
//
//  Created by Clement on 9/17/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Player_h
#define hifi_Player_h

#include <AudioInjector.h>

#include <QElapsedTimer>

#include "Recording.h"

class AvatarData;
class Player;

typedef QSharedPointer<Player> PlayerPointer;
typedef QWeakPointer<Player> WeakPlayerPointer;

/// Plays back a recording
class Player {
public:
    Player(AvatarData* avatar);
    
    bool isPlaying() const;
    bool isPaused() const;
    qint64 elapsed() const;
    
    RecordingPointer getRecording() const { return _recording; }
    int getCurrentFrame() const { return _currentFrame; }
    
public slots:
    void startPlaying();
    void stopPlaying();
    void pausePlayer();
    void loadFromFile(const QString& file);
    void loadRecording(RecordingPointer recording);
    void play();
    
    void setCurrentFrame(unsigned int currentFrame);
    void setCurrentTime(unsigned int currentTime);

    void setVolume(float volume);
    void setAudioOffset(int audioOffset);
    
    void setPlayFromCurrentLocation(bool playFromCurrentPosition);
    void setLoop(bool loop) { _loop = loop; }
    void useAttachements(bool useAttachments) { _useAttachments = useAttachments; }
    void useDisplayName(bool useDisplayName) { _useDisplayName = useDisplayName; }
    void useHeadModel(bool useHeadURL) { _useHeadURL = useHeadURL; }
    void useSkeletonModel(bool useSkeletonURL) { _useSkeletonURL = useSkeletonURL; }
    
private:
    void setupAudioThread();
    void cleanupAudioThread();
    void loopRecording();
    void setAudionInjectorPosition();
    bool computeCurrentFrame();
    
    AvatarData* _avatar;
    RecordingPointer _recording;
    int _currentFrame;
    float _frameInterpolationFactor;
    int _pausedFrame;

    QElapsedTimer _timer;
    int _timerOffset;
    int _audioOffset;
    
    QThread* _audioThread;
    QSharedPointer<AudioInjector> _injector;
    AudioInjectorOptions _options;
    
    RecordingContext _currentContext;
    bool _playFromCurrentPosition;
    bool _loop;
    bool _useAttachments;
    bool _useDisplayName;
    bool _useHeadURL;
    bool _useSkeletonURL;
};

#endif // hifi_Player_h