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

#include <QFile>
#include <QMetaObject>
#include <QObject>

#include "AvatarData.h"
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

Recording::Recording() : _audio(NULL) {
}

Recording::~Recording() {
    delete _audio;
}

void Recording::addFrame(int timestamp, RecordingFrame &frame) {
    _timestamps << timestamp;
    _frames << frame;
}

void Recording::addAudioPacket(QByteArray byteArray) {
    if (!_audio) {
        _audio = new Sound(byteArray);
    }
    _audio->append(byteArray);
}

void Recording::clear() {
    _timestamps.clear();
    _frames.clear();
    delete _audio;
    _audio = NULL;
}

Recorder::Recorder(AvatarData* avatar) :
    _recording(new Recording()),
    _avatar(avatar)
{
    _timer.invalidate();
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
    
    const HeadData* head = _avatar->getHeadData();
    glm::quat rotation = glm::quat(glm::radians(glm::vec3(head->getFinalPitch(),
                                                          head->getFinalYaw(),
                                                          head->getFinalRoll())));
    frame.setHeadRotation(rotation);
    frame.setLeanForward(_avatar->getHeadData()->getLeanForward());
    frame.setLeanSideways(_avatar->getHeadData()->getLeanSideways());
    
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
    
    writeRecordingToFile(_recording, file);
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
        
        
        const HeadData* head = _avatar->getHeadData();
        glm::quat rotation = glm::quat(glm::radians(glm::vec3(head->getFinalPitch(),
                                                              head->getFinalYaw(),
                                                              head->getFinalRoll())));
        frame.setHeadRotation(rotation);
        frame.setLeanForward(_avatar->getHeadData()->getLeanForward());
        frame.setLeanSideways(_avatar->getHeadData()->getLeanSideways());
        
        _recording->addFrame(_timer.elapsed(), frame);
    }
}

void Recorder::record(char* samples, int size) {
    QByteArray byteArray(samples, size);
    _recording->addAudioPacket(byteArray);
}


Player::Player(AvatarData* avatar) :
    _recording(new Recording()),
    _avatar(avatar),
    _audioThread(NULL)
{
    _timer.invalidate();
    _options.setLoop(false);
    _options.setVolume(1.0f);
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

glm::quat Player::getHeadRotation() {
    if (!computeCurrentFrame()) {
        qWarning() << "Incorrect use of Player::getHeadRotation()";
        return glm::quat();
    }
    
    if (_currentFrame == 0) {
        return _recording->getFrame(_currentFrame).getHeadRotation();
    }
    return _recording->getFrame(0).getHeadRotation() *
           _recording->getFrame(_currentFrame).getHeadRotation();
}

float Player::getLeanSideways() {
    if (!computeCurrentFrame()) {
        qWarning() << "Incorrect use of Player::getLeanSideways()";
        return 0.0f;
    }
    
    return _recording->getFrame(_currentFrame).getLeanSideways();
}

float Player::getLeanForward() {
    if (!computeCurrentFrame()) {
        qWarning() << "Incorrect use of Player::getLeanForward()";
        return 0.0f;
    }
    
    return _recording->getFrame(_currentFrame).getLeanForward();
}

void Player::startPlaying() {
    if (_recording && _recording->getFrameNumber() > 0) {
        qDebug() << "Recorder::startPlaying()";
        _currentFrame = 0;
        
        // Setup audio thread
        _audioThread = new QThread();
        _options.setPosition(_avatar->getPosition());
        _options.setOrientation(_avatar->getOrientation());
        _injector.reset(new AudioInjector(_recording->getAudio(), _options), &QObject::deleteLater);
        _injector->moveToThread(_audioThread);
        _audioThread->start();
        QMetaObject::invokeMethod(_injector.data(), "injectAudio", Qt::QueuedConnection);
        
        // Fake faceshift connection
        _avatar->setForceFaceshiftConnected(true);
        
        _timer.start();
    }
}

void Player::stopPlaying() {
    if (!isPlaying()) {
        return;
    }
    
    _timer.invalidate();
    
    _avatar->clearJointsData();
    
    // Cleanup audio thread
    _injector->stop();
    QObject::connect(_injector.data(), &AudioInjector::finished,
                     _injector.data(), &AudioInjector::deleteLater);
    QObject::connect(_injector.data(), &AudioInjector::destroyed,
                     _audioThread, &QThread::quit);
    QObject::connect(_audioThread, &QThread::finished,
                     _audioThread, &QThread::deleteLater);
    _injector.clear();
    _audioThread = NULL;
    
    // Turn off fake faceshift connection
    _avatar->setForceFaceshiftConnected(false);
    
    qDebug() << "Recorder::stopPlaying()";
}

void Player::loadFromFile(QString file) {
    if (_recording) {
        _recording->clear();
    } else {
        _recording = RecordingPointer(new Recording());
    }
    readRecordingFromFile(_recording, file);
}

void Player::loadRecording(RecordingPointer recording) {
    _recording = recording;
}

void Player::play() {
    computeCurrentFrame();
    if (_currentFrame < 0 || _currentFrame >= _recording->getFrameNumber() - 1) {
        // If it's the end of the recording, stop playing
        stopPlaying();
        return;
    }
    
    if (_currentFrame == 0) {
        _avatar->setPosition(_recording->getFrame(_currentFrame).getTranslation());
        _avatar->setOrientation(_recording->getFrame(_currentFrame).getRotation());
        _avatar->setTargetScale(_recording->getFrame(_currentFrame).getScale());
        _avatar->setJointRotations(_recording->getFrame(_currentFrame).getJointRotations());
    } else {
        _avatar->setPosition(_recording->getFrame(0).getTranslation() +
                             _recording->getFrame(_currentFrame).getTranslation());
        _avatar->setOrientation(_recording->getFrame(0).getRotation() *
                                _recording->getFrame(_currentFrame).getRotation());
        _avatar->setTargetScale(_recording->getFrame(0).getScale() *
                                _recording->getFrame(_currentFrame).getScale());
        _avatar->setJointRotations(_recording->getFrame(_currentFrame).getJointRotations());
    }
    HeadData* head = const_cast<HeadData*>(_avatar->getHeadData());
    if (head) {
        head->setBlendshapeCoefficients(_recording->getFrame(_currentFrame).getBlendshapeCoefficients());
        head->setLeanSideways(_recording->getFrame(_currentFrame).getLeanSideways());
        head->setLeanForward(_recording->getFrame(_currentFrame).getLeanForward());
        glm::vec3 eulers = glm::degrees(safeEulerAngles(_recording->getFrame(_currentFrame).getHeadRotation()));
        head->setFinalPitch(eulers.x);
        head->setFinalYaw(eulers.y);
        head->setFinalRoll(eulers.z);
    }
    
    _options.setPosition(_avatar->getPosition());
    _options.setOrientation(_avatar->getOrientation());
    _injector->setOptions(_options);
}

bool Player::computeCurrentFrame() {
    if (!isPlaying()) {
        _currentFrame = -1;
        return false;
    }
    if (_currentFrame < 0) {
        _currentFrame = 0;
    }
    
    while (_currentFrame < _recording->getFrameNumber() - 1 &&
           _recording->getFrameTimestamp(_currentFrame) < _timer.elapsed()) {
        ++_currentFrame;
    }
    
    return true;
}

void writeRecordingToFile(RecordingPointer recording, QString filename) {
    if (!recording || recording->getFrameNumber() < 1) {
        qDebug() << "Can't save empty recording";
        return;
    }
    
    qDebug() << "Writing recording to " << filename << ".";
    QElapsedTimer timer;
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly)){
        return;
    }
    timer.start();
    
    
    QDataStream fileStream(&file);
    
    fileStream << recording->_timestamps;
    
    RecordingFrame& baseFrame = recording->_frames[0];
    int totalLength = 0;
    
    // Blendshape coefficients
    fileStream << baseFrame._blendshapeCoefficients;
    totalLength += baseFrame._blendshapeCoefficients.size();
    
    // Joint Rotations
    int jointRotationSize = baseFrame._jointRotations.size();
    fileStream << jointRotationSize;
    for (int i = 0; i < jointRotationSize; ++i) {
        fileStream << baseFrame._jointRotations[i].x << baseFrame._jointRotations[i].y << baseFrame._jointRotations[i].z << baseFrame._jointRotations[i].w;
    }
    totalLength += jointRotationSize;
    
    // Translation
    fileStream << baseFrame._translation.x << baseFrame._translation.y << baseFrame._translation.z;
    totalLength += 1;
    
    // Rotation
    fileStream << baseFrame._rotation.x << baseFrame._rotation.y << baseFrame._rotation.z << baseFrame._rotation.w;
    totalLength += 1;
    
    // Scale
    fileStream << baseFrame._scale;
    totalLength += 1;
    
    // Head Rotation
    fileStream << baseFrame._headRotation.x << baseFrame._headRotation.y << baseFrame._headRotation.z << baseFrame._headRotation.w;
    totalLength += 1;
    
    // Lean Sideways
    fileStream << baseFrame._leanSideways;
    totalLength += 1;
    
    // Lean Forward
    fileStream << baseFrame._leanForward;
    totalLength += 1;
    
    for (int i = 1; i < recording->_timestamps.size(); ++i) {
        QBitArray mask(totalLength);
        int maskIndex = 0;
        QByteArray buffer;
        QDataStream stream(&buffer, QIODevice::WriteOnly);
        RecordingFrame& previousFrame = recording->_frames[i - 1];
        RecordingFrame& frame = recording->_frames[i];
        
        // Blendshape coefficients
        for (int i = 0; i < frame._blendshapeCoefficients.size(); ++i) {
            if (frame._blendshapeCoefficients[i] != previousFrame._blendshapeCoefficients[i]) {
                stream << frame._blendshapeCoefficients[i];
                mask.setBit(maskIndex);
            }
            maskIndex++;
        }
        
        // Joint Rotations
        for (int i = 0; i < frame._jointRotations.size(); ++i) {
            if (frame._jointRotations[i] != previousFrame._jointRotations[i]) {
                stream << frame._jointRotations[i].x << frame._jointRotations[i].y << frame._jointRotations[i].z << frame._jointRotations[i].w;
                mask.setBit(maskIndex);
            }
            maskIndex++;
        }
        
        // Translation
        if (frame._translation != previousFrame._translation) {
            stream << frame._translation.x << frame._translation.y << frame._translation.z;
            mask.setBit(maskIndex);
        }
        maskIndex++;
        
        // Rotation
        if (frame._rotation != previousFrame._rotation) {
            stream << frame._rotation.x << frame._rotation.y << frame._rotation.z << frame._rotation.w;
            mask.setBit(maskIndex);
        }
        maskIndex++;
        
        // Scale
        if (frame._scale != previousFrame._scale) {
            stream << frame._scale;
            mask.setBit(maskIndex);
        }
        maskIndex++;
        
        // Head Rotation
        if (frame._headRotation != previousFrame._headRotation) {
            stream << frame._headRotation.x << frame._headRotation.y << frame._headRotation.z << frame._headRotation.w;
            mask.setBit(maskIndex);
        }
        maskIndex++;
        
        // Lean Sideways
        if (frame._leanSideways != previousFrame._leanSideways) {
            stream << frame._leanSideways;
            mask.setBit(maskIndex);
        }
        maskIndex++;
        
        // Lean Forward
        if (frame._leanForward != previousFrame._leanForward) {
            stream << frame._leanForward;
            mask.setBit(maskIndex);
        }
        maskIndex++;
        
        fileStream << mask;
        fileStream << buffer;
    }
    
    fileStream << recording->_audio->getByteArray();
    
    qDebug() << "Wrote " << file.size() << " bytes in " << timer.elapsed() << " ms.";
}

RecordingPointer readRecordingFromFile(RecordingPointer recording, QString filename) {
    qDebug() << "Reading recording from " << filename << ".";
    if (!recording) {
        recording.reset(new Recording());
    }
    
    QElapsedTimer timer;
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)){
        return recording;
    }
    timer.start();
    QDataStream fileStream(&file);
    
    fileStream >> recording->_timestamps;
    RecordingFrame baseFrame;
    
    // Blendshape coefficients
    fileStream >> baseFrame._blendshapeCoefficients;
    
    // Joint Rotations
    int jointRotationSize;
    fileStream >> jointRotationSize;
    baseFrame._jointRotations.resize(jointRotationSize);
    for (int i = 0; i < jointRotationSize; ++i) {
        fileStream >> baseFrame._jointRotations[i].x >> baseFrame._jointRotations[i].y >> baseFrame._jointRotations[i].z >> baseFrame._jointRotations[i].w;
    }
    
    fileStream >> baseFrame._translation.x >> baseFrame._translation.y >> baseFrame._translation.z;
    fileStream >> baseFrame._rotation.x >> baseFrame._rotation.y >> baseFrame._rotation.z >> baseFrame._rotation.w;
    fileStream >> baseFrame._scale;
    fileStream >> baseFrame._headRotation.x >> baseFrame._headRotation.y >> baseFrame._headRotation.z >> baseFrame._headRotation.w;
    fileStream >> baseFrame._leanSideways;
    fileStream >> baseFrame._leanForward;
    
    recording->_frames << baseFrame;
    
    for (int i = 1; i < recording->_timestamps.size(); ++i) {
        QBitArray mask;
        QByteArray buffer;
        QDataStream stream(&buffer, QIODevice::ReadOnly);
        RecordingFrame frame;
        RecordingFrame& previousFrame = recording->_frames.last();
        
        fileStream >> mask;
        fileStream >> buffer;
        int maskIndex = 0;
        
        // Blendshape Coefficients
        frame._blendshapeCoefficients.resize(baseFrame._blendshapeCoefficients.size());
        for (int i = 0; i < baseFrame._blendshapeCoefficients.size(); ++i) {
            if (mask[maskIndex++]) {
                stream >> frame._blendshapeCoefficients[i];
            } else {
                frame._blendshapeCoefficients[i] = previousFrame._blendshapeCoefficients[i];
            }
        }
        
        // Joint Rotations
        frame._jointRotations.resize(baseFrame._jointRotations.size());
        for (int i = 0; i < baseFrame._jointRotations.size(); ++i) {
            if (mask[maskIndex++]) {
                stream >> frame._jointRotations[i].x >> frame._jointRotations[i].y >> frame._jointRotations[i].z >> frame._jointRotations[i].w;
            } else {
                frame._jointRotations[i] = previousFrame._jointRotations[i];
            }
        }
        
        if (mask[maskIndex++]) {
            stream >> frame._translation.x >> frame._translation.y >> frame._translation.z;
        } else {
            frame._translation = previousFrame._translation;
        }
        
        if (mask[maskIndex++]) {
            stream >> frame._rotation.x >> frame._rotation.y >> frame._rotation.z >> frame._rotation.w;
        } else {
            frame._rotation = previousFrame._rotation;
        }
        
        if (mask[maskIndex++]) {
            stream >> frame._scale;
        } else {
            frame._scale = previousFrame._scale;
        }
        
        if (mask[maskIndex++]) {
            stream >> frame._headRotation.x >> frame._headRotation.y >> frame._headRotation.z >> frame._headRotation.w;
        } else {
            frame._headRotation = previousFrame._headRotation;
        }
        
        if (mask[maskIndex++]) {
            stream >> frame._leanSideways;
        } else {
            frame._leanSideways = previousFrame._leanSideways;
        }
        
        if (mask[maskIndex++]) {
            stream >> frame._leanForward;
        } else {
            frame._leanForward = previousFrame._leanForward;
        }
        
        recording->_frames << frame;
    }
    
    QByteArray audioArray;
    fileStream >> audioArray;
    recording->addAudioPacket(audioArray);
    
    
    qDebug() << "Read " << file.size()  << " bytes in " << timer.elapsed() << " ms.";
    return recording;
}


