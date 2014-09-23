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
    qint64 elapsed() const;
    
    RecordingPointer getRecording() const { return _recording; }
    int getCurrentFrame() const { return _currentFrame; }
    
public slots:
    void startPlaying();
    void stopPlaying();
    void loadFromFile(const QString& file);
    void loadRecording(RecordingPointer recording);
    void play();
    
    void setCurrentFrame(int currentFrame);
    void setCurrentTime(qint64 currentTime);
    
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
    
    QElapsedTimer _timer;
    RecordingPointer _recording;
    int _currentFrame;
    qint64 _timerOffset;
    
    QSharedPointer<AudioInjector> _injector;
    AudioInjectorOptions _options;
    
    AvatarData* _avatar;
    QThread* _audioThread;
    
    
    RecordingContext _currentContext;
    bool _playFromCurrentPosition;
    bool _loop;
    bool _useAttachments;
    bool _useDisplayName;
    bool _useHeadURL;
    bool _useSkeletonURL;
    
};

#endif // hifi_Player_h