//
//  Created by Bradley Austin Davis 2015/11/04
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_Recording_Deck_h
#define hifi_Recording_Deck_h

#include <utility>
#include <list>
#include <mutex>

#include <QtCore/QObject>
#include <QtCore/QTimer>
#include <QtCore/QList>

#include <DependencyManager.h>

#include "Forward.h"
#include "Frame.h"


namespace recording {

class Deck : public QObject, public ::Dependency {
    Q_OBJECT
public:
    using ClipList = std::list<ClipPointer>;
    using Pointer = std::shared_ptr<Deck>;

    Deck(QObject* parent = nullptr);

    // Place a clip on the deck for recording or playback
    void queueClip(ClipPointer clip, float timeOffset = 0.0f);
    void removeClip(const ClipConstPointer& clip);
    void removeClip(const QString& clipName);
    void removeAllClips();
    ClipList getClips(const QString& clipName) const;

    void play();
    bool isPlaying();

    void pause();
    bool isPaused() const;

    void stop();

    float length() const;

    void loop(bool enable = true);
    bool isLooping() const;

    float position() const;
    void seek(float position);

    float getVolume() const { return _volume; }
    void setVolume(float volume);

signals:
    void playbackStateChanged();
    void looped();

private:
    using Mutex = std::recursive_mutex;
    using Locker = std::unique_lock<Mutex>;

    ClipPointer getNextClip();
    void processFrames();

    mutable Mutex _mutex;
    QTimer _timer;
    ClipList _clips;
    quint64 _startEpoch { 0 };
    Frame::Time _position { 0 };
    bool _pause { true };
    bool _loop { false };
    float _length { 0 };
    float _volume { 1.0f };
};

}

#endif
