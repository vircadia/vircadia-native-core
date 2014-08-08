//
//  Recorder.cpp
//
//
//  Created by Clement on 8/7/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Recorder.h"

void Recording::addFrame(int timestamp, RecordingFrame &frame) {
    _timestamps << timestamp;
    _frames << frame;
}

void Recording::clear() {
    _timestamps.clear();
    _frames.clear();
}

void writeRecordingToFile(Recording& recording, QString file) {
    qDebug() << "Writing recording to " << file;
}

Recording& readRecordingFromFile(QString file) {
    qDebug() << "Reading recording from " << file;
    return *(new Recording());
}


bool Recorder::isRecording() const {
    return _timer.isValid();
}

qint64 Recorder::elapsed() const {
    if (isRecording()) {
        return _timer.elapsed();
    } else {
        return 0;
    }
}

void Recorder::startRecording() {
    _recording.clear();
    _timer.start();
}

void Recorder::stopRecording() {
    _timer.invalidate();
}

void Recorder::saveToFile(QString file) {
    if (_recording.isEmpty()) {
        qDebug() << "Cannot save recording to file, recording is empty.";
    }
    
    writeRecordingToFile(_recording, file);
}