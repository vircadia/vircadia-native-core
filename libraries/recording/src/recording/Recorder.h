//
//  Created by Bradley Austin Davis 2015/11/05
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_Recording_Recorder_h
#define hifi_Recording_Recorder_h

#include <mutex>

#include <QtCore/QObject>
#include <QtCore/QElapsedTimer>

#include <DependencyManager.h>

#include "Forward.h"

namespace recording {

// An interface for interacting with clips, creating them by recording or
// playing them back.  Also serialization to and from files / network sources
class Recorder : public QObject, public Dependency {
    Q_OBJECT
public:
    Recorder(QObject* parent = nullptr);

    float position();

    // Start recording frames
    void start();
    // Stop recording
    void stop();

    // Test if recording is active
    bool isRecording();

    // Erase the currently recorded content
    void clear();

    void recordFrame(FrameType type, QByteArray frameData);

    // Return the currently recorded content
    ClipPointer getClip();

signals:
    void recordingStateChanged();

private:
    using Mutex = std::recursive_mutex;
    using Locker = std::unique_lock<Mutex>;

    Mutex _mutex;
    QElapsedTimer _timer;
    ClipPointer _clip;
    quint64 _elapsed { 0 };
    quint64 _startEpoch { 0 };
    bool _recording { false };
};

}

#endif
