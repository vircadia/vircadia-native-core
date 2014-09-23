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

Player::Player(AvatarData* avatar) :
    _recording(new Recording()),
    _timerOffset(0),
    _avatar(avatar),
    _audioThread(NULL),
    _playFromCurrentPosition(true),
    _loop(false),
    _useAttachments(true),
    _useDisplayName(true),
    _useHeadURL(true),
    _useSkeletonURL(true)
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
        return _timerOffset + _timer.elapsed();
    } else {
        return 0;
    }
}

void Player::startPlaying() {
    if (_recording && _recording->getFrameNumber() > 0) {
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
    }
}

void Player::stopPlaying() {
    if (!isPlaying()) {
        return;
    }
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

void Player::setupAudioThread() {
    _audioThread = new QThread();
    _options.setPosition(_avatar->getPosition());
    _options.setOrientation(_avatar->getOrientation());
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
}

void Player::loadRecording(RecordingPointer recording) {
    _recording = recording;
}

void Player::play() {
    computeCurrentFrame();
    if (_currentFrame < 0 || (_currentFrame >= _recording->getFrameNumber() - 1)) {
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
    
    _avatar->setPosition(context->position + context->orientation * currentFrame.getTranslation());
    _avatar->setOrientation(context->orientation * currentFrame.getRotation());
    _avatar->setTargetScale(context->scale * currentFrame.getScale());
    _avatar->setJointRotations(currentFrame.getJointRotations());
    
    HeadData* head = const_cast<HeadData*>(_avatar->getHeadData());
    if (head) {
        head->setBlendshapeCoefficients(currentFrame.getBlendshapeCoefficients());
        head->setLeanSideways(currentFrame.getLeanSideways());
        head->setLeanForward(currentFrame.getLeanForward());
        glm::vec3 eulers = glm::degrees(safeEulerAngles(currentFrame.getHeadRotation()));
        head->setFinalPitch(eulers.x);
        head->setFinalYaw(eulers.y);
        head->setFinalRoll(eulers.z);
        head->setLookAtPosition(context->position + context->orientation * currentFrame.getLookAtPosition());
    } else {
        qDebug() << "WARNING: Player couldn't find head data.";
    }
    
    _options.setPosition(_avatar->getPosition());
    _options.setOrientation(_avatar->getOrientation());
    _injector->setOptions(_options);
}

void Player::setCurrentFrame(int currentFrame) {
    if (_recording && currentFrame >= _recording->getFrameNumber()) {
        stopPlaying();
        return;
    }
    
    _currentFrame = currentFrame;
    _timerOffset = _recording->getFrameTimestamp(_currentFrame);
    _timer.restart();
    
    setAudionInjectorPosition();
}

void Player::setCurrentTime(qint64 currentTime) {
    if (currentTime < 0 || currentTime >= _recording->getLength()) {
        stopPlaying();
        return;
    }
    
    // Find correct frame
    int lowestBound = 0;
    int highestBound = _recording->getFrameNumber() - 1;
    int bestGuess = 0;
    while (!(_recording->getFrameTimestamp(bestGuess) <= currentTime &&
             _recording->getFrameTimestamp(bestGuess + 1) > currentTime)) {
        
        if (_recording->getFrameTimestamp(bestGuess) <= currentTime) {
            lowestBound = bestGuess;
        } else {
            highestBound = bestGuess;
        }
        
        bestGuess = lowestBound +
                    (highestBound - lowestBound) *
                    (float)(currentTime - _recording->getFrameTimestamp(lowestBound)) /
                    (float)(_recording->getFrameTimestamp(highestBound) - _recording->getFrameTimestamp(lowestBound));
    }
    
    _currentFrame = bestGuess;
    _timerOffset = _recording->getFrameTimestamp(bestGuess);
    _timer.restart();
    
    setAudionInjectorPosition();
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
        _currentFrame = -1;
        return false;
    }
    if (_currentFrame < 0) {
        _currentFrame = 0;
    }
    
    while (_currentFrame < _recording->getFrameNumber() - 1 &&
           _recording->getFrameTimestamp(_currentFrame) < elapsed()) {
        ++_currentFrame;
    }
    
    return true;
}
