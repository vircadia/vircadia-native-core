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

#include <QtCore/QObject>
#include <QtCore/QTimer>

#include "Forward.h"


namespace recording {

class Deck : public QObject {
    Q_OBJECT
public:
    using Pointer = std::shared_ptr<Deck>;
    Deck(QObject* parent = nullptr) : QObject(parent) {}

    // Place a clip on the deck for recording or playback
    void queueClip(ClipPointer clip, Time timeOffset = 0.0f);

    void play();
    bool isPlaying() { return !_pause; }

    void pause();
    bool isPaused() const { return _pause; }

    void stop() { pause(); seek(0.0f); }

    Time length() const { return _length; }

    void loop(bool enable = true) { _loop = enable; }
    bool isLooping() const { return _loop; }

    Time position() const;
    void seek(Time position);

signals:
    void playbackStateChanged();

private:
    using Clips = std::list<ClipPointer>;

    ClipPointer getNextClip();
    void processFrames();

    QTimer _timer;
    Clips _clips;
    quint64 _startEpoch { 0 };
    Time _position { 0 };
    bool _pause { true };
    bool _loop { false };
    Time _length { 0 };
};

}

#endif
