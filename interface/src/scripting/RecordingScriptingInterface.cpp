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
    return (float)_player->position() / MSECS_PER_SECOND;
}

float RecordingScriptingInterface::playerLength() {
    return _player->length() / MSECS_PER_SECOND;
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
    _player->seek(time * MSECS_PER_SECOND);
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

    _audioRecordingBuffer.clear();
    auto myAvatar = DependencyManager::get<AvatarManager>()->getMyAvatar();
    myAvatar->setRecordingBasis();
    _recorder->start();
}

float calculateAudioTime(const QByteArray& audio) {
    static const float AUDIO_BYTES_PER_SECOND = AudioConstants::SAMPLE_RATE * sizeof(AudioConstants::AudioSample);
    return (float)audio.size() / AUDIO_BYTES_PER_SECOND;
}

void injectAudioFrame(Clip::Pointer& clip, Frame::Time time, const QByteArray& audio) {
    static const recording::FrameType AUDIO_FRAME_TYPE = recording::Frame::registerFrameType(AUDIO_FRAME_NAME);
    clip->addFrame(std::make_shared<Frame>(AUDIO_FRAME_TYPE, time, audio));
}

// Detect too much audio in a single frame, or too much deviation between 
// the expected audio length and the computed audio length
bool shouldStartNewAudioFrame(const QByteArray& currentAudioFrame, float expectedAudioLength) {
    if (currentAudioFrame.isEmpty()) {
        return true;
    }

    // 100 milliseconds
    float actualAudioLength = calculateAudioTime(currentAudioFrame);
    static const float MAX_AUDIO_PACKET_DURATION = 1.0f;
    if (actualAudioLength >= MAX_AUDIO_PACKET_DURATION) {
        return true;
    }


    float deviation = std::abs(actualAudioLength - expectedAudioLength);

    qDebug() << "Checking buffer deviation current length ";
    qDebug() << "Actual:    " << actualAudioLength;
    qDebug() << "Expected:  " << expectedAudioLength;
    qDebug() << "Deviation: " << deviation;

    static const float MAX_AUDIO_DEVIATION = 0.1f;
    if (deviation >= MAX_AUDIO_PACKET_DURATION) {
        return true;
    }

    return false;
}


void injectAudioFrames(Clip::Pointer& clip, const QList<QPair<recording::Frame::Time, QByteArray>>& audioBuffer) {
    Frame::Time lastAudioStartTime = 0;
    QByteArray audioFrameBuffer;
    for (const auto& audioPacket : audioBuffer) {
        float expectedAudioLength = Frame::frameTimeToSeconds(audioPacket.first - lastAudioStartTime);
        if (shouldStartNewAudioFrame(audioFrameBuffer, expectedAudioLength)) {
            // Time to start a new frame, inject the old one if it exists
            if (audioFrameBuffer.size()) {
                injectAudioFrame(clip, lastAudioStartTime, audioFrameBuffer);
                audioFrameBuffer.clear();
            }
            lastAudioStartTime = audioPacket.first;
        }
        audioFrameBuffer.append(audioPacket.second);
    }
}


void RecordingScriptingInterface::stopRecording() {
    _recorder->stop();

    _lastClip = _recorder->getClip();
    // post-process the audio into discreet chunks based on times of received samples
    injectAudioFrames(_lastClip, _audioRecordingBuffer);
    _audioRecordingBuffer.clear();
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
        auto audioFrameTime = Frame::frameTimeFromEpoch(_recordingEpoch);
        _audioRecordingBuffer.push_back({ audioFrameTime, audio });
        qDebug() << "Got sound packet of size " << audio.size() << " At time " << audioFrameTime;
    }
}

void RecordingScriptingInterface::processAudioFrame(const recording::FrameConstPointer& frame) {
    AudioInjectorOptions options;
    auto myAvatar = DependencyManager::get<AvatarManager>()->getMyAvatar();
    options.position = myAvatar->getPosition();
    options.orientation = myAvatar->getOrientation();
    // FIXME store the audio format (sample rate, bits, stereo) in the frame
    options.stereo = false;
    // FIXME move audio injector to a thread pool model?
    AudioInjector::playSoundAndDelete(frame->data, options, nullptr);
}
