//
//  Player.cpp
//
//
//  Created by Clement on 9/17/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <AudioRingBuffer.h>
#include <GLMHelpers.h>
#include <NodeList.h>
#include <StreamUtils.h>

#include "AvatarData.h"
#include "Player.h"

static const int INVALID_FRAME = -1;

Player::Player(AvatarData* avatar) :
    _avatar(avatar),
    _recording(new Recording()),
    _currentFrame(INVALID_FRAME),
    _frameInterpolationFactor(0.0f),
    _pausedFrame(INVALID_FRAME),
    _timerOffset(0),
    _audioOffset(0),
    _audioThread(NULL),
    _playFromCurrentPosition(true),
    _loop(false),
    _useAttachments(true),
    _useDisplayName(true),
    _useHeadURL(true),
    _useSkeletonURL(true)
{
    _timer.invalidate();
}

bool Player::isPlaying() const {
    return _timer.isValid();
}

bool Player::isPaused() const {
    return (_pausedFrame != INVALID_FRAME);
}

qint64 Player::elapsed() const {
    if (isPlaying()) {
        return _timerOffset + _timer.elapsed();
    } else if (isPaused()) {
        return _timerOffset;
    } else {
        return 0;
    }
}

void Player::startPlaying() {
    if (!_recording || _recording->getFrameNumber() <= 1) {
        return;
    }
    
    if (!isPaused()) {
        _currentContext.globalTimestamp = usecTimestampNow();
        _currentContext.domain = NodeList::getInstance()->getDomainHandler().getHostname();
        _currentContext.position = _avatar->getPosition();
        _currentContext.orientation = _avatar->getOrientation();
        _currentContext.scale = _avatar->getTargetScale();
        _currentContext.headModel = _avatar->getFaceModelURL().toString();
        _currentContext.skeletonModel = _avatar->getSkeletonModelURL().toString();
        _currentContext.displayName = _avatar->getDisplayName();
        _currentContext.attachments = _avatar->getAttachmentData();
        
        _currentContext.orientationInv = glm::inverse(_currentContext.orientation);
        
        RecordingContext& context = _recording->getContext();
        if (_useAttachments) {
            _avatar->setAttachmentData(context.attachments);
        }
        if (_useDisplayName) {
            _avatar->setDisplayName(context.displayName);
        }
        if (_useHeadURL) {
            _avatar->setFaceModelURL(context.headModel);
        }
        if (_useSkeletonURL) {
            _avatar->setSkeletonModelURL(context.skeletonModel);
        }
        
        bool wantDebug = false;
        if (wantDebug) {
            qDebug() << "Player::startPlaying(): Recording Context";
            qDebug() << "Domain:" << _currentContext.domain;
            qDebug() << "Position:" << _currentContext.position;
            qDebug() << "Orientation:" << _currentContext.orientation;
            qDebug() << "Scale:" << _currentContext.scale;
            qDebug() << "Head URL:" << _currentContext.headModel;
            qDebug() << "Skeleton URL:" << _currentContext.skeletonModel;
            qDebug() << "Display Name:" << _currentContext.displayName;
            qDebug() << "Num Attachments:" << _currentContext.attachments.size();
            
            for (int i = 0; i < _currentContext.attachments.size(); ++i) {
                qDebug() << "Model URL:" << _currentContext.attachments[i].modelURL;
                qDebug() << "Joint Name:" << _currentContext.attachments[i].jointName;
                qDebug() << "Translation:" << _currentContext.attachments[i].translation;
                qDebug() << "Rotation:" << _currentContext.attachments[i].rotation;
                qDebug() << "Scale:" << _currentContext.attachments[i].scale;
            }
        }
        
        // Fake faceshift connection
        _avatar->setForceFaceshiftConnected(true);
        
        qDebug() << "Recorder::startPlaying()";
        setupAudioThread();
        _currentFrame = 0;
        _timerOffset = 0;
        _timer.start();
    } else {
        qDebug() << "Recorder::startPlaying(): Unpause";
        setupAudioThread();
        _timer.start();
        
        setCurrentFrame(_pausedFrame);
        _pausedFrame = INVALID_FRAME;
    }
}

void Player::stopPlaying() {
    if (!isPlaying()) {
        return;
    }
    _pausedFrame = INVALID_FRAME;
    _timer.invalidate();
    cleanupAudioThread();
    _avatar->clearJointsData();
    
    // Turn off fake faceshift connection
    _avatar->setForceFaceshiftConnected(false);
    
    if (_useAttachments) {
        _avatar->setAttachmentData(_currentContext.attachments);
    }
    if (_useDisplayName) {
        _avatar->setDisplayName(_currentContext.displayName);
    }
    if (_useHeadURL) {
        _avatar->setFaceModelURL(_currentContext.headModel);
    }
    if (_useSkeletonURL) {
        _avatar->setSkeletonModelURL(_currentContext.skeletonModel);
    }
    
    qDebug() << "Recorder::stopPlaying()";
}

void Player::pausePlayer() {
    _timerOffset = elapsed();
    _timer.invalidate();
    cleanupAudioThread();
    
    _pausedFrame = _currentFrame;
    qDebug() << "Recorder::pausePlayer()";
}

void Player::setupAudioThread() {
    _audioThread = new QThread();
    _options.position = _avatar->getPosition();
    _options.orientation = _avatar->getOrientation();
    _injector.reset(new AudioInjector(_recording->getAudio(), _options), &QObject::deleteLater);
    _injector->moveToThread(_audioThread);
    _audioThread->start();
    QMetaObject::invokeMethod(_injector.data(), "injectAudio", Qt::QueuedConnection);
}

void Player::cleanupAudioThread() {
    _injector->stop();
    QObject::connect(_injector.data(), &AudioInjector::finished,
                     _injector.data(), &AudioInjector::deleteLater);
    QObject::connect(_injector.data(), &AudioInjector::destroyed,
                     _audioThread, &QThread::quit);
    QObject::connect(_audioThread, &QThread::finished,
                     _audioThread, &QThread::deleteLater);
    _injector.clear();
    _audioThread = NULL;
}

void Player::loopRecording() {
    cleanupAudioThread();
    setupAudioThread();
    _currentFrame = 0;
    _timerOffset = 0;
    _timer.restart();
}

void Player::loadFromFile(const QString& file) {
    if (_recording) {
        _recording->clear();
    } else {
        _recording = RecordingPointer(new Recording());
    }
    readRecordingFromFile(_recording, file);
    
    _pausedFrame = INVALID_FRAME;
}

void Player::loadRecording(RecordingPointer recording) {
    _recording = recording;
    _pausedFrame = INVALID_FRAME;
}

void Player::play() {
    computeCurrentFrame();
    if (_currentFrame < 0 || (_currentFrame >= _recording->getFrameNumber() - 2)) { // -2 because of interpolation
        if (_loop) {
            loopRecording();
        } else {
            stopPlaying();
        }
        return;
    }
    
    const RecordingContext* context = &_recording->getContext();
    if (_playFromCurrentPosition) {
        context = &_currentContext;
    }
    const RecordingFrame& currentFrame = _recording->getFrame(_currentFrame);
    const RecordingFrame& nextFrame = _recording->getFrame(_currentFrame + 1);
    
    glm::vec3 translation = glm::mix(currentFrame.getTranslation(),
                                     nextFrame.getTranslation(),
                                     _frameInterpolationFactor);
    _avatar->setPosition(context->position + context->orientation * translation);
    
    glm::quat rotation = safeMix(currentFrame.getRotation(),
                                 nextFrame.getRotation(),
                                 _frameInterpolationFactor);
    _avatar->setOrientation(context->orientation * rotation);
    
    float scale = glm::mix(currentFrame.getScale(),
                           nextFrame.getScale(),
                           _frameInterpolationFactor);
    _avatar->setTargetScale(context->scale * scale);
    
    
    QVector<glm::quat> jointRotations(currentFrame.getJointRotations().size());
    for (int i = 0; i < currentFrame.getJointRotations().size(); ++i) {
        jointRotations[i] = safeMix(currentFrame.getJointRotations()[i],
                                    nextFrame.getJointRotations()[i],
                                    _frameInterpolationFactor);
    }
    _avatar->setJointRotations(jointRotations);
    
    HeadData* head = const_cast<HeadData*>(_avatar->getHeadData());
    if (head) {
        // Make sure fake faceshift connection doesn't get turned off
        _avatar->setForceFaceshiftConnected(true);
        
        QVector<float> blendCoef(currentFrame.getBlendshapeCoefficients().size());
        for (int i = 0; i < currentFrame.getBlendshapeCoefficients().size(); ++i) {
            blendCoef[i] = glm::mix(currentFrame.getBlendshapeCoefficients()[i],
                                    nextFrame.getBlendshapeCoefficients()[i],
                                    _frameInterpolationFactor);
        }
        head->setBlendshapeCoefficients(blendCoef);
        
        float leanSideways = glm::mix(currentFrame.getLeanSideways(),
                                      nextFrame.getLeanSideways(),
                                      _frameInterpolationFactor);
        head->setLeanSideways(leanSideways);
        
        float leanForward = glm::mix(currentFrame.getLeanForward(),
                                     nextFrame.getLeanForward(),
                                     _frameInterpolationFactor);
        head->setLeanForward(leanForward);
        
        glm::quat headRotation = safeMix(currentFrame.getHeadRotation(),
                                         nextFrame.getHeadRotation(),
                                         _frameInterpolationFactor);
        glm::vec3 eulers = glm::degrees(safeEulerAngles(headRotation));
        head->setFinalPitch(eulers.x);
        head->setFinalYaw(eulers.y);
        head->setFinalRoll(eulers.z);
        
        
        glm::vec3 lookAt = glm::mix(currentFrame.getLookAtPosition(),
                                    nextFrame.getLookAtPosition(),
                                    _frameInterpolationFactor);
        head->setLookAtPosition(context->position + context->orientation * lookAt);
    } else {
        qDebug() << "WARNING: Player couldn't find head data.";
    }
    
    _options.position = _avatar->getPosition();
    _options.orientation = _avatar->getOrientation();
    _injector->setOptions(_options);
}

void Player::setCurrentFrame(unsigned int currentFrame) {
    if (_recording && currentFrame >= _recording->getFrameNumber()) {
        stopPlaying();
        return;
    }
    
    _currentFrame = currentFrame;
    _timerOffset = _recording->getFrameTimestamp(_currentFrame);
    
    if (isPlaying()) {
        _timer.start();
        setAudionInjectorPosition();
    } else {
        _pausedFrame = _currentFrame;
    }
}

void Player::setCurrentTime(unsigned int currentTime) {
    if (currentTime >= _recording->getLength()) {
        stopPlaying();
        return;
    }
    
    // Find correct frame
    int lowestBound = 0;
    int highestBound = _recording->getFrameNumber() - 1;
    while (lowestBound + 1 != highestBound) {
        assert(lowestBound < highestBound);
        
        int bestGuess = lowestBound +
                    (highestBound - lowestBound) *
                    (float)(currentTime - _recording->getFrameTimestamp(lowestBound)) /
                    (float)(_recording->getFrameTimestamp(highestBound) - _recording->getFrameTimestamp(lowestBound));
        
        if (_recording->getFrameTimestamp(bestGuess) <= currentTime) {
            if (currentTime < _recording->getFrameTimestamp(bestGuess + 1)) {
                lowestBound = bestGuess;
                highestBound = bestGuess + 1;
            } else {
                lowestBound = bestGuess + 1;
            }
        } else {
            if (_recording->getFrameTimestamp(bestGuess - 1) <= currentTime) {
                lowestBound = bestGuess - 1;
                highestBound = bestGuess;
            } else {
                highestBound = bestGuess - 1;
            }
        }
    }
    
    _currentFrame = lowestBound;
    _timerOffset = _recording->getFrameTimestamp(lowestBound);
    
    if (isPlaying()) {
        _timer.start();
        setAudionInjectorPosition();
    } else {
        _pausedFrame = lowestBound;
    }
}

void Player::setVolume(float volume) {
    _options.volume = volume;
    if (_injector) {
        _injector->setOptions(_options);
    }
    qDebug() << "New volume: " << volume;
}

void Player::setAudioOffset(int audioOffset) {
    _audioOffset = audioOffset;
}

void Player::setAudionInjectorPosition() {
    int MSEC_PER_SEC = 1000;
    int SAMPLE_SIZE = 2; // 16 bits
    int CHANNEL_COUNT = 1;
    int FRAME_SIZE = SAMPLE_SIZE * CHANNEL_COUNT;
    int currentAudioFrame = elapsed() * FRAME_SIZE * (SAMPLE_RATE / MSEC_PER_SEC);
    _injector->setCurrentSendPosition(currentAudioFrame);
}

void Player::setPlayFromCurrentLocation(bool playFromCurrentLocation) {
    _playFromCurrentPosition = playFromCurrentLocation;
}

bool Player::computeCurrentFrame() {
    if (!isPlaying()) {
        _currentFrame = INVALID_FRAME;
        return false;
    }
    if (_currentFrame < 0) {
        _currentFrame = 0;
    }
    
    quint64 elapsed = glm::clamp(Player::elapsed() - _audioOffset, (qint64)0, (qint64)_recording->getLength());
    while(_currentFrame >= 0 &&
          _recording->getFrameTimestamp(_currentFrame) > elapsed) {
        --_currentFrame;
    }
    
    while (_currentFrame < _recording->getFrameNumber() &&
           _recording->getFrameTimestamp(_currentFrame) < elapsed) {
        ++_currentFrame;
    }
    --_currentFrame;
    
    if (_currentFrame == _recording->getFrameNumber() - 1) {
        --_currentFrame;
        _frameInterpolationFactor = 1.0f;
    } else {
        qint64 currentTimestamps = _recording->getFrameTimestamp(_currentFrame);
        qint64 nextTimestamps = _recording->getFrameTimestamp(_currentFrame + 1);
        _frameInterpolationFactor = (float)(elapsed - currentTimestamps) /
                                    (float)(nextTimestamps - currentTimestamps);
    }
    
    if (_frameInterpolationFactor < 0.0f || _frameInterpolationFactor > 1.0f) {
        _frameInterpolationFactor = 0.0f;
        qDebug() << "Invalid frame interpolation value: overriding";
    }
    return true;
}
