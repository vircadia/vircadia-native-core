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

void RecordingFrame::setBlendshapeCoefficients(QVector<float> blendshapeCoefficients) {
    _blendshapeCoefficients = blendshapeCoefficients;
}

void RecordingFrame::setJointRotations(QVector<glm::quat> jointRotations) {
    _jointRotations = jointRotations;
}

void RecordingFrame::setTranslation(glm::vec3 translation) {
    _translation = translation;
}

void Recording::addFrame(int timestamp, RecordingFrame &frame) {
    _timestamps << timestamp;
    _frames << frame;
}

void Recording::clear() {
    _timestamps.clear();
    _frames.clear();
}

Recorder::Recorder(AvatarData* avatar) : _avatar(avatar) {
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

void Recorder::record() {
    qDebug() << "Recording " << _avatar;
    RecordingFrame frame;
    frame.setBlendshapeCoefficients(_avatar->_)
}

Player::Player(AvatarData* avatar) : _avatar(avatar) {
}

bool Player::isPlaying() const {
    return _timer.isValid();
}

qint64 Player::elapsed() const {
    if (isPlaying()) {
        return _timer.elapsed();
    } else {
        return 0;
    }
}

void Player::startPlaying() {
    _timer.start();
}

void Player::stopPlaying() {
    _timer.invalidate();
}

void Player::loadFromFile(QString file) {
    _recording.clear();
    readRecordingFromFile(_recording, file);
}

void Player::play() {
    qDebug() << "Playing " << _avatar;
}

void writeRecordingToFile(Recording& recording, QString file) {
    // TODO
    qDebug() << "Writing recording to " << file;
}

Recording& readRecordingFromFile(Recording& recording, QString file) {
    // TODO
    qDebug() << "Reading recording from " << file;
    return recording;
}