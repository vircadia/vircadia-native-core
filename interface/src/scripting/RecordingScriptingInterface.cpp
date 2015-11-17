//
//  Created by Bradley Austin Davis on 2015/11/13
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "RecordingScriptingInterface.h"

#include <recording/Deck.h>
#include <recording/Recorder.h>
#include <recording/Clip.h>
#include <recording/Frame.h>
#include <NumericalConstants.h>
#include <AudioClient.h>
#include <AudioConstants.h>

#include "avatar/AvatarManager.h"
#include "Application.h"
#include "InterfaceLogging.h"

typedef int16_t AudioSample;


using namespace recording;

// FIXME move to somewhere audio related?
static const QString AUDIO_FRAME_NAME = "com.highfidelity.recording.Audio";

RecordingScriptingInterface::RecordingScriptingInterface() {
    static const recording::FrameType AVATAR_FRAME_TYPE = recording::Frame::registerFrameType(AvatarData::FRAME_NAME);
    // FIXME how to deal with driving multiple avatars locally?  
    Frame::registerFrameHandler(AVATAR_FRAME_TYPE, [this](Frame::ConstPointer frame) {
        processAvatarFrame(frame);
    });

    static const recording::FrameType AUDIO_FRAME_TYPE = recording::Frame::registerFrameType(AUDIO_FRAME_NAME);
    Frame::registerFrameHandler(AUDIO_FRAME_TYPE, [this](Frame::ConstPointer frame) {
        processAudioFrame(frame);
    });

    _player = DependencyManager::get<Deck>();
    _recorder = DependencyManager::get<Recorder>();

    auto audioClient = DependencyManager::get<AudioClient>();
    connect(audioClient.data(), &AudioClient::inputReceived, this, &RecordingScriptingInterface::processAudioInput);
}

bool RecordingScriptingInterface::isPlaying() {
    return _player->isPlaying();
}

bool RecordingScriptingInterface::isPaused() {
    return _player->isPaused();
}

float RecordingScriptingInterface::playerElapsed() {
    return _player->position();
}

float RecordingScriptingInterface::playerLength() {
    return _player->length();
}

void RecordingScriptingInterface::loadRecording(const QString& filename) {
    using namespace recording;

    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "loadRecording", Qt::BlockingQueuedConnection,
            Q_ARG(QString, filename));
        return;
    }

    ClipPointer clip = Clip::fromFile(filename);
    if (!clip) {
        qWarning() << "Unable to load clip data from " << filename;
    }
    _player->queueClip(clip);
}

void RecordingScriptingInterface::startPlaying() {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "startPlaying", Qt::BlockingQueuedConnection);
        return;
    }
    auto myAvatar = DependencyManager::get<AvatarManager>()->getMyAvatar();
    // Playback from the current position
    if (_playFromCurrentLocation) {
        _dummyAvatar.setRecordingBasis(std::make_shared<Transform>(myAvatar->getTransform()));
    } else {
        _dummyAvatar.clearRecordingBasis();
    }
    _player->play();
}

void RecordingScriptingInterface::setPlayerVolume(float volume) {
    // FIXME 
}

void RecordingScriptingInterface::setPlayerAudioOffset(float audioOffset) {
    // FIXME 
}

void RecordingScriptingInterface::setPlayerTime(float time) {
    _player->seek(time);
}

void RecordingScriptingInterface::setPlayFromCurrentLocation(bool playFromCurrentLocation) {
    _playFromCurrentLocation = playFromCurrentLocation;
}

void RecordingScriptingInterface::setPlayerLoop(bool loop) {
    _player->loop(loop);
}

void RecordingScriptingInterface::setPlayerUseDisplayName(bool useDisplayName) {
    _useDisplayName = useDisplayName;
}

void RecordingScriptingInterface::setPlayerUseAttachments(bool useAttachments) {
    _useAttachments = useAttachments;
}

void RecordingScriptingInterface::setPlayerUseHeadModel(bool useHeadModel) {
    _useHeadModel = useHeadModel;
}

void RecordingScriptingInterface::setPlayerUseSkeletonModel(bool useSkeletonModel) {
    _useSkeletonModel = useSkeletonModel;
}

void RecordingScriptingInterface::play() {
    _player->play();
}

void RecordingScriptingInterface::pausePlayer() {
    _player->pause();
}

void RecordingScriptingInterface::stopPlaying() {
    _player->stop();
}

bool RecordingScriptingInterface::isRecording() {
    return _recorder->isRecording();
}

float RecordingScriptingInterface::recorderElapsed() {
    return _recorder->position();
}

void RecordingScriptingInterface::startRecording() {
    if (_recorder->isRecording()) {
        qCWarning(interfaceapp) << "Recorder is already running";
        return;
    }

    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "startRecording", Qt::BlockingQueuedConnection);
        return;
    }

    _recordingEpoch = Frame::epochForFrameTime(0);

    auto myAvatar = DependencyManager::get<AvatarManager>()->getMyAvatar();
    myAvatar->setRecordingBasis();
    _recorder->start();
}

void RecordingScriptingInterface::stopRecording() {
    _recorder->stop();

    _lastClip = _recorder->getClip();
    // post-process the audio into discreet chunks based on times of received samples
    _lastClip->seek(0);
    Frame::ConstPointer frame;
    while (frame = _lastClip->nextFrame()) {
        qDebug() << "Frame time " << frame->timeOffset << " size " << frame->data.size();
    }
    _lastClip->seek(0);

    auto myAvatar = DependencyManager::get<AvatarManager>()->getMyAvatar();
    myAvatar->clearRecordingBasis();
}

void RecordingScriptingInterface::saveRecording(const QString& filename) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "saveRecording", Qt::BlockingQueuedConnection,
            Q_ARG(QString, filename));
        return;
    }

    if (!_lastClip) {
        qWarning() << "There is no recording to save";
        return;
    }

    recording::Clip::toFile(filename, _lastClip);
}

void RecordingScriptingInterface::loadLastRecording() {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "loadLastRecording", Qt::BlockingQueuedConnection);
        return;
    }

    if (!_lastClip) {
        qCDebug(interfaceapp) << "There is no recording to load";
        return;
    }

    _player->queueClip(_lastClip);
    _player->play();
}

void RecordingScriptingInterface::processAvatarFrame(const Frame::ConstPointer& frame) {
    Q_ASSERT(QThread::currentThread() == thread());

    AvatarData::fromFrame(frame->data, _dummyAvatar);

    auto myAvatar = DependencyManager::get<AvatarManager>()->getMyAvatar();
    if (_useHeadModel && _dummyAvatar.getFaceModelURL().isValid() && 
        (_dummyAvatar.getFaceModelURL() != myAvatar->getFaceModelURL())) {
        // FIXME
        //myAvatar->setFaceModelURL(_dummyAvatar.getFaceModelURL());
    }

    if (_useSkeletonModel && _dummyAvatar.getSkeletonModelURL().isValid() &&
        (_dummyAvatar.getSkeletonModelURL() != myAvatar->getSkeletonModelURL())) {
        // FIXME
        //myAvatar->useFullAvatarURL()
    }

    if (_useDisplayName && _dummyAvatar.getDisplayName() != myAvatar->getDisplayName()) {
        myAvatar->setDisplayName(_dummyAvatar.getDisplayName());
    }

    myAvatar->setPosition(_dummyAvatar.getPosition());
    myAvatar->setOrientation(_dummyAvatar.getOrientation());

    // FIXME attachments
    // FIXME joints
    // FIXME head lean
    // FIXME head orientation
}

void RecordingScriptingInterface::processAudioInput(const QByteArray& audio) {
    if (_recorder->isRecording()) {
        static const recording::FrameType AUDIO_FRAME_TYPE = recording::Frame::registerFrameType(AUDIO_FRAME_NAME);
        _recorder->recordFrame(AUDIO_FRAME_TYPE, audio);
    }
}

void RecordingScriptingInterface::processAudioFrame(const recording::FrameConstPointer& frame) {
    auto audioClient = DependencyManager::get<AudioClient>();
    audioClient->handleRecordedAudioInput(frame->data);
}
