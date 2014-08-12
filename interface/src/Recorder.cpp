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

#include <GLMHelpers.h>

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

void RecordingFrame::setRotation(glm::quat rotation) {
    _rotation = rotation;
}

void RecordingFrame::setScale(float scale) {
    _scale = scale;
}

void RecordingFrame::setHeadRotation(glm::quat headRotation) {
    _headRotation = headRotation;
}

void RecordingFrame::setLeanSideways(float leanSideways) {
    _leanSideways = leanSideways;
}

void RecordingFrame::setLeanForward(float leanForward) {
    _leanForward = leanForward;
}

void RecordingFrame::setEstimatedEyePitch(float estimatedEyePitch) {
    _estimatedEyePitch = estimatedEyePitch;
}

void RecordingFrame::setEstimatedEyeYaw(float estimatedEyeYaw) {
    _estimatedEyeYaw = estimatedEyeYaw;
}

void Recording::addFrame(int timestamp, RecordingFrame &frame) {
    _timestamps << timestamp;
    _frames << frame;
}

void Recording::clear() {
    _timestamps.clear();
    _frames.clear();
}

Recorder::Recorder(AvatarData* avatar) :
    _recording(new Recording()),
    _avatar(avatar)
{
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
    qDebug() << "Recorder::startRecording()";
    _recording->clear();
    _timer.start();
    
    RecordingFrame frame;
    frame.setBlendshapeCoefficients(_avatar->getHeadData()->getBlendshapeCoefficients());
    frame.setJointRotations(_avatar->getJointRotations());
    frame.setTranslation(_avatar->getPosition());
    frame.setRotation(_avatar->getOrientation());
    frame.setScale(_avatar->getTargetScale());
    
    // TODO
    const HeadData* head = _avatar->getHeadData();
    glm::quat rotation = glm::quat(glm::radians(glm::vec3(head->getFinalPitch(),
                                                          head->getFinalYaw(),
                                                          head->getFinalRoll())));
    frame.setHeadRotation(rotation);
    // TODO
    //frame.setEstimatedEyePitch();
    //frame.setEstimatedEyeYaw();
    
    _recording->addFrame(0, frame);
}

void Recorder::stopRecording() {
    qDebug() << "Recorder::stopRecording()";
    _timer.invalidate();
    
    qDebug().nospace() << "Recorded " << _recording->getFrameNumber() << " during " << _recording->getLength() << " msec (" << _recording->getFrameNumber() / (_recording->getLength() / 1000.0f) << " fps)";
}

void Recorder::saveToFile(QString file) {
    if (_recording->isEmpty()) {
        qDebug() << "Cannot save recording to file, recording is empty.";
    }
    
    writeRecordingToFile(*_recording, file);
}

void Recorder::record() {
    if (isRecording()) {
        const RecordingFrame& referenceFrame = _recording->getFrame(0);
        RecordingFrame frame;
        frame.setBlendshapeCoefficients(_avatar->getHeadData()->getBlendshapeCoefficients());
        frame.setJointRotations(_avatar->getJointRotations());
        frame.setTranslation(_avatar->getPosition() - referenceFrame.getTranslation());
        frame.setRotation(glm::inverse(referenceFrame.getRotation()) * _avatar->getOrientation());
        frame.setScale(_avatar->getTargetScale() / referenceFrame.getScale());
        // TODO
        //frame.setHeadTranslation();
        const HeadData* head = _avatar->getHeadData();
        glm::quat rotation = glm::quat(glm::radians(glm::vec3(head->getFinalPitch(),
                                                              head->getFinalYaw(),
                                                              head->getFinalRoll())));
        frame.setHeadRotation(glm::inverse(referenceFrame.getHeadRotation()) * rotation);
        // TODO
        //frame.setEstimatedEyePitch();
        //frame.setEstimatedEyeYaw();
        _recording->addFrame(_timer.elapsed(), frame);
    }
}

Player::Player(AvatarData* avatar) :
    _recording(new Recording()),
    _avatar(avatar)
{
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

QVector<float> Player::getBlendshapeCoefficients() {
    computeCurrentFrame();
    return _recording->getFrame(_currentFrame).getBlendshapeCoefficients();
}

QVector<glm::quat> Player::getJointRotations() {
    computeCurrentFrame();
    return _recording->getFrame(_currentFrame).getJointRotations();
}

glm::quat Player::getRotation() {
    computeCurrentFrame();
    return _recording->getFrame(_currentFrame).getRotation();
}

float Player::getScale() {
    computeCurrentFrame();
    return _recording->getFrame(_currentFrame).getScale();
}

glm::quat Player::getHeadRotation() {
    computeCurrentFrame();
    if (_currentFrame >= 0 && _currentFrame <= _recording->getFrameNumber()) {
        if (_currentFrame == _recording->getFrameNumber()) {
            return _recording->getFrame(0).getHeadRotation() *
                   _recording->getFrame(_currentFrame - 1).getHeadRotation();
        }
        if (_currentFrame == 0) {
            return _recording->getFrame(_currentFrame).getHeadRotation();
        }
        
        return _recording->getFrame(0).getHeadRotation() *
               _recording->getFrame(_currentFrame).getHeadRotation();
    }
    qWarning() << "Incorrect use of Player::getHeadRotation()";
    return glm::quat();
}

float Player::getEstimatedEyePitch() {
    computeCurrentFrame();
    return _recording->getFrame(_currentFrame).getEstimatedEyePitch();
}

float Player::getEstimatedEyeYaw() {
    computeCurrentFrame();
    return _recording->getFrame(_currentFrame).getEstimatedEyeYaw();
}


void Player::startPlaying() {
    if (_recording && _recording->getFrameNumber() > 0) {
        qDebug() << "Recorder::startPlaying()";
        _timer.start();
        _currentFrame = 0;
    }
}

void Player::stopPlaying() {
    qDebug() << "Recorder::stopPlaying()";
    _timer.invalidate();
}

void Player::loadFromFile(QString file) {
    if (_recording) {
        _recording->clear();
    } else {
        _recording = RecordingPointer(new Recording());
    }
    readRecordingFromFile(*_recording, file);
}

void Player::loadRecording(RecordingPointer recording) {
    _recording = recording;
}

void Player::play() {
    qDebug() << "Playing " << _timer.elapsed() / 1000.0f;
    computeCurrentFrame();
    if (_currentFrame < 0 || _currentFrame >= _recording->getFrameNumber()) {
        // If it's the end of the recording, stop playing
        stopPlaying();
        return;
    }
    if (_currentFrame == 0) {
        _avatar->setPosition(_recording->getFrame(_currentFrame).getTranslation());
        _avatar->setOrientation(_recording->getFrame(_currentFrame).getRotation());
        _avatar->setTargetScale(_recording->getFrame(_currentFrame).getScale());
        HeadData* head = const_cast<HeadData*>(_avatar->getHeadData());
        head->setBlendshapeCoefficients(_recording->getFrame(_currentFrame).getBlendshapeCoefficients());
        // TODO
        // HEAD: Coeff, Translation, estimated eye rotations
        // BODY: Joint Rotations
    } else {
        _avatar->setPosition(_recording->getFrame(0).getTranslation() +
                             _recording->getFrame(_currentFrame).getTranslation());
        _avatar->setOrientation(_recording->getFrame(0).getRotation() *
                                _recording->getFrame(_currentFrame).getRotation());
        _avatar->setTargetScale(_recording->getFrame(0).getScale() *
                                _recording->getFrame(_currentFrame).getScale());
        HeadData* head = const_cast<HeadData*>(_avatar->getHeadData());
        head->setBlendshapeCoefficients(_recording->getFrame(_currentFrame).getBlendshapeCoefficients());
        // TODO
        // HEAD: Coeff, Translation, estimated eye rotations
        // BODY: Joint Rotations
    }
}

void Player::computeCurrentFrame() {
    if (!isPlaying()) {
        qDebug() << "Not Playing";
        _currentFrame = -1;
        return;
    }
    if (_currentFrame < 0) {
        qDebug() << "Reset to 0";
        _currentFrame = 0;
    }
    
    while (_currentFrame < _recording->getFrameNumber() &&
           _recording->getFrameTimestamp(_currentFrame) < _timer.elapsed()) {
        qDebug() << "Loop";
        ++_currentFrame;
    }
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