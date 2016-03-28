//
//  Created by Bradley Austin Davis on 2015/11/13
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "RecordingScriptingInterface.h"

#include <QtCore/QThread>

#include <NumericalConstants.h>
#include <Transform.h>
#include <recording/Deck.h>
#include <recording/Recorder.h>
#include <recording/Clip.h>
#include <recording/Frame.h>
#include <recording/ClipCache.h>


#include <QtScript/QScriptValue>
#include <AssetClient.h>
#include <AssetUpload.h>
#include <QtCore/QUrl>
#include <QtWidgets/QFileDialog>

#include "ScriptEngineLogging.h"

using namespace recording;

static const QString HFR_EXTENSION = "hfr";

RecordingScriptingInterface::RecordingScriptingInterface() {
    _player = DependencyManager::get<Deck>();
    _recorder = DependencyManager::get<Recorder>();
}

bool RecordingScriptingInterface::isPlaying() const {
    return _player->isPlaying();
}

bool RecordingScriptingInterface::isPaused() const {
    return _player->isPaused();
}

float RecordingScriptingInterface::playerElapsed() const {
    return _player->position();
}

float RecordingScriptingInterface::playerLength() const {
    return _player->length();
}

bool RecordingScriptingInterface::loadRecording(const QString& url) {
    using namespace recording;

    auto loader = ClipCache::instance().getClipLoader(url);
    if (!loader->isLoaded()) {
        QEventLoop loop;
        QObject::connect(loader.data(), &Resource::loaded, &loop, &QEventLoop::quit);
        QObject::connect(loader.data(), &Resource::failed, &loop, &QEventLoop::quit);
        loop.exec();
    }

    if (!loader->isLoaded()) {
        qWarning() << "Clip failed to load from " << url;
        return false;
    }

    _player->queueClip(loader->getClip());
    return true;
}


void RecordingScriptingInterface::startPlaying() {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "startPlaying", Qt::BlockingQueuedConnection);
        return;
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
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "setPlayerTime", Qt::BlockingQueuedConnection, Q_ARG(float, time));
        return;
    }
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

void RecordingScriptingInterface::pausePlayer() {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "pausePlayer", Qt::BlockingQueuedConnection);
        return;
    }
    _player->pause();
}

void RecordingScriptingInterface::stopPlaying() {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "stopPlaying", Qt::BlockingQueuedConnection);
        return;
    }
    _player->stop();
}

bool RecordingScriptingInterface::isRecording() const {
    return _recorder->isRecording();
}

float RecordingScriptingInterface::recorderElapsed() const {
    return _recorder->position();
}

void RecordingScriptingInterface::startRecording() {
    if (_recorder->isRecording()) {
        qCWarning(scriptengine) << "Recorder is already running";
        return;
    }

    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "startRecording", Qt::BlockingQueuedConnection);
        return;
    }

    _recorder->start();
}

void RecordingScriptingInterface::stopRecording() {
    _recorder->stop();
    _lastClip = _recorder->getClip();
    _lastClip->seek(0);
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

bool RecordingScriptingInterface::saveRecordingToAsset(QScriptValue getClipAtpUrl) {
    if (!getClipAtpUrl.isFunction()) {
        qCWarning(scriptengine) << "The argument is not a function.";
        return false;
    }

    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "saveRecordingToAsset", Qt::BlockingQueuedConnection,
            Q_ARG(QScriptValue, getClipAtpUrl));
        return false;
    }

    if (!_lastClip) {
        qWarning() << "There is no recording to save";
        return false;
    }

    if (auto upload = DependencyManager::get<AssetClient>()->createUpload(recording::Clip::toBuffer(_lastClip))) {
        QObject::connect(upload, &AssetUpload::finished, this, [=](AssetUpload* upload, const QString& hash) mutable {
            QString clip_atp_url = "";

            if (upload->getError() == AssetUpload::NoError) {

                clip_atp_url = QString("%1:%2").arg(URL_SCHEME_ATP, hash);
                upload->deleteLater();
            } else {
                qCWarning(scriptengine) << "Error during the Asset upload.";
            }

            QScriptValueList args;
            args << clip_atp_url;
            getClipAtpUrl.call(QScriptValue(), args);
        });
        upload->start();
        return true;
    }

    qCWarning(scriptengine) << "Saving on asset failed.";
    return false;
}

void RecordingScriptingInterface::loadLastRecording() {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "loadLastRecording", Qt::BlockingQueuedConnection);
        return;
    }

    if (!_lastClip) {
        qCDebug(scriptengine) << "There is no recording to load";
        return;
    }

    _player->queueClip(_lastClip);
    _player->play();
}

