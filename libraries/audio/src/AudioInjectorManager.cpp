//
//  AudioInjectorManager.cpp
//  libraries/audio/src
//
//  Created by Stephen Birarda on 2015-11-16.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AudioInjectorManager.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QSharedPointer>

#include <SharedUtil.h>
#include <shared/QtHelpers.h>
#include <ThreadHelpers.h>

#include "AudioConstants.h"
#include "AudioInjector.h"
#include "AudioLogging.h"

#include "AudioSRC.h"

AudioInjectorManager::~AudioInjectorManager() {
    _shouldStop = true;

    Lock lock(_injectorsMutex);

    // make sure any still living injectors are stopped and deleted
    while (!_injectors.empty()) {
        // grab the injector at the front
        auto& timePointerPair = _injectors.top();

        // ask it to stop and be deleted
        timePointerPair.second->finish();

        _injectors.pop();
    }

    // get rid of the lock now that we've stopped all living injectors
    lock.unlock();

    // in case the thread is waiting for injectors wake it up now
    _injectorReady.notify_one();

    // quit and wait on the manager thread, if we ever created it
    if (_thread) {
        _thread->quit();
        _thread->wait();
    }

    moveToThread(qApp->thread());
}

void AudioInjectorManager::createThread() {
    _thread = new QThread();
    _thread->setObjectName("Audio Injector Thread");

    // when the thread is started, have it call our run to handle injection of audio
    connect(_thread, &QThread::started, this, [this] {
        setThreadName("AudioInjectorManager");
        run();
    }, Qt::DirectConnection);

    moveToThread(_thread);

    // start the thread
    _thread->start();
}

void AudioInjectorManager::run() {
    while (!_shouldStop) {
        // wait until the next injector is ready, or until we get a new injector given to us
        Lock lock(_injectorsMutex);

        if (_injectors.size() > 0) {
            // when does the next injector need to send a frame?
            // do we get to wait or should we just go for it now?

            auto timeInjectorPair = _injectors.top();

            auto nextTimestamp = timeInjectorPair.first;
            int64_t difference = int64_t(nextTimestamp - usecTimestampNow());

            if (difference > 0) {
                _injectorReady.wait_for(lock, std::chrono::microseconds(difference));
            }

            if (_injectors.size() > 0) {
                // loop through the injectors in the map and send whatever frames need to go out
                auto front = _injectors.top();

                // create an InjectorQueue to hold injectors to be queued
                // this allows us to call processEvents even if a single injector wants to be re-queued immediately
                std::vector<TimeInjectorPointerPair> heldInjectors;
                heldInjectors.reserve(_injectors.size());

                while (_injectors.size() > 0 && front.first <= usecTimestampNow()) {
                    // either way we're popping this injector off - get a copy first
                    auto injector = front.second;
                    _injectors.pop();

                    if (!injector.isNull()) {
                        // this is an injector that's ready to go, have it send a frame now
                        auto nextCallDelta = injector->injectNextFrame();

                        if (nextCallDelta >= 0 && !injector->isFinished()) {
                            // enqueue the injector with the correct timing in our holding queue
                            heldInjectors.emplace(heldInjectors.end(), usecTimestampNow() + nextCallDelta, injector);
                        } else {
                            injector->sendStopInjectorPacket();
                        }
                    }

                    if (_injectors.size() > 0) {
                        front = _injectors.top();
                    } else {
                        // no more injectors to look at, break
                        break;
                    }
                }

                // if there are injectors in the holding queue, push them to our persistent queue now
                while (!heldInjectors.empty()) {
                    _injectors.push(heldInjectors.back());
                    heldInjectors.pop_back();
                }
            }

        } else {
            // we have no current injectors, wait until we get at least one before we do anything
            _injectorReady.wait(lock);
        }

        // unlock the lock in case something in process events needs to modify the queue
        lock.unlock();

        QCoreApplication::processEvents();
    }
}

static const int MAX_INJECTORS_PER_THREAD = 40; // calculated based on AudioInjector time to send frame, with sufficient padding

bool AudioInjectorManager::wouldExceedLimits() { // Should be called inside of a lock.
    if (_injectors.size() >= MAX_INJECTORS_PER_THREAD) {
        qCDebug(audio)  << "AudioInjectorManager::threadInjector could not thread AudioInjector - at max of"
            << MAX_INJECTORS_PER_THREAD << "current audio injectors.";
        return true;
    }
    return false;
}

bool AudioInjectorManager::threadInjector(const AudioInjectorPointer& injector) {
    if (_shouldStop) {
        qCDebug(audio) << "AudioInjectorManager::threadInjector asked to thread injector but is shutting down.";
        return false;
    }

    // guard the injectors vector with a mutex
    Lock lock(_injectorsMutex);

    if (wouldExceedLimits()) {
        return false;
    } else {
        // add the injector to the queue with a send timestamp of now
        _injectors.emplace(usecTimestampNow(), injector);

        // notify our wait condition so we can inject two frames for this injector immediately
        _injectorReady.notify_one();
    }
    return true;
}

AudioInjectorPointer AudioInjectorManager::playSound(const SharedSoundPointer& sound, const AudioInjectorOptions& options, bool setPendingDelete) {
    if (_shouldStop) {
        qCDebug(audio) << "AudioInjectorManager::threadInjector asked to thread injector but is shutting down.";
        return nullptr;
    }

    AudioInjectorPointer injector = nullptr;
    if (sound && sound->isReady()) {
        if (options.pitch == 1.0f) {
            injector = QSharedPointer<AudioInjector>(new AudioInjector(sound, options), &AudioInjector::deleteLater);
        } else {
            using AudioConstants::AudioSample;
            using AudioConstants::SAMPLE_RATE;
            const int standardRate = SAMPLE_RATE;
            // limit pitch to 4 octaves
            const float pitch = glm::clamp(options.pitch, 1 / 16.0f, 16.0f);
            const int resampledRate = glm::round(SAMPLE_RATE / pitch);

            auto audioData = sound->getAudioData();
            auto numChannels = audioData->getNumChannels();
            auto numFrames = audioData->getNumFrames();

            AudioSRC resampler(standardRate, resampledRate, numChannels);

            // create a resampled buffer that is guaranteed to be large enough
            const int maxOutputFrames = resampler.getMaxOutput(numFrames);
            const int maxOutputSize = maxOutputFrames * numChannels * sizeof(AudioSample);
            QByteArray resampledBuffer(maxOutputSize, '\0');
            auto bufferPtr = reinterpret_cast<AudioSample*>(resampledBuffer.data());

            resampler.render(audioData->data(), bufferPtr, numFrames);

            int numSamples = maxOutputFrames * numChannels;
            auto newAudioData = AudioData::make(numSamples, numChannels, bufferPtr);

            injector = QSharedPointer<AudioInjector>(new AudioInjector(newAudioData, options), &AudioInjector::deleteLater);
        }
    }

    if (!injector) {
        return nullptr;
    }

    if (setPendingDelete) {
        injector->_state |= AudioInjectorState::PendingDelete;
    }

    injector->moveToThread(_thread);
    injector->inject(&AudioInjectorManager::threadInjector);

    return injector;
}

AudioInjectorPointer AudioInjectorManager::playSound(const AudioDataPointer& audioData, const AudioInjectorOptions& options, bool setPendingDelete) {
    if (_shouldStop) {
        qCDebug(audio) << "AudioInjectorManager::threadInjector asked to thread injector but is shutting down.";
        return nullptr;
    }

    AudioInjectorPointer injector = nullptr;
    if (options.pitch == 1.0f) {
        injector = QSharedPointer<AudioInjector>(new AudioInjector(audioData, options), &AudioInjector::deleteLater);
    } else {
        using AudioConstants::AudioSample;
        using AudioConstants::SAMPLE_RATE;
        const int standardRate = SAMPLE_RATE;
        // limit pitch to 4 octaves
        const float pitch = glm::clamp(options.pitch, 1 / 16.0f, 16.0f);
        const int resampledRate = glm::round(SAMPLE_RATE / pitch);

        auto numChannels = audioData->getNumChannels();
        auto numFrames = audioData->getNumFrames();

        AudioSRC resampler(standardRate, resampledRate, numChannels);

        // create a resampled buffer that is guaranteed to be large enough
        const int maxOutputFrames = resampler.getMaxOutput(numFrames);
        const int maxOutputSize = maxOutputFrames * numChannels * sizeof(AudioSample);
        QByteArray resampledBuffer(maxOutputSize, '\0');
        auto bufferPtr = reinterpret_cast<AudioSample*>(resampledBuffer.data());

        resampler.render(audioData->data(), bufferPtr, numFrames);

        int numSamples = maxOutputFrames * numChannels;
        auto newAudioData = AudioData::make(numSamples, numChannels, bufferPtr);

        injector = QSharedPointer<AudioInjector>(new AudioInjector(newAudioData, options), &AudioInjector::deleteLater);
    }

    if (!injector) {
        return nullptr;
    }

    if (setPendingDelete) {
        injector->_state |= AudioInjectorState::PendingDelete;
    }

    injector->moveToThread(_thread);
    injector->inject(&AudioInjectorManager::threadInjector);

    return injector;
}

void AudioInjectorManager::setOptionsAndRestart(const AudioInjectorPointer& injector, const AudioInjectorOptions& options) {
    if (!injector) {
        return;
    }

    if (QThread::currentThread() != _thread) {
        QMetaObject::invokeMethod(this, "setOptionsAndRestart", Q_ARG(const AudioInjectorPointer&, injector), Q_ARG(const AudioInjectorOptions&, options));
        _injectorReady.notify_one();
        return;
    }

    injector->setOptions(options);
    injector->restart();
}

void AudioInjectorManager::restart(const AudioInjectorPointer& injector) {
    if (!injector) {
        return;
    }

    if (QThread::currentThread() != _thread) {
        QMetaObject::invokeMethod(this, "restart", Q_ARG(const AudioInjectorPointer&, injector));
        _injectorReady.notify_one();
        return;
    }

    injector->restart();
}

void AudioInjectorManager::setOptions(const AudioInjectorPointer& injector, const AudioInjectorOptions& options) {
    if (!injector) {
        return;
    }

    if (QThread::currentThread() != _thread) {
        QMetaObject::invokeMethod(this, "setOptions", Q_ARG(const AudioInjectorPointer&, injector), Q_ARG(const AudioInjectorOptions&, options));
        _injectorReady.notify_one();
        return;
    }

    injector->setOptions(options);
}

AudioInjectorOptions AudioInjectorManager::getOptions(const AudioInjectorPointer& injector) {
    if (!injector) {
        return AudioInjectorOptions();
    }

    return injector->getOptions();
}

float AudioInjectorManager::getLoudness(const AudioInjectorPointer& injector) {
    if (!injector) {
        return 0.0f;
    }

    return injector->getLoudness();
}

bool AudioInjectorManager::isPlaying(const AudioInjectorPointer& injector) {
    if (!injector) {
        return false;
    }

    return injector->isPlaying();
}

void AudioInjectorManager::stop(const AudioInjectorPointer& injector) {
    if (!injector) {
        return;
    }

    if (QThread::currentThread() != _thread) {
        QMetaObject::invokeMethod(this, "stop", Q_ARG(const AudioInjectorPointer&, injector));
        _injectorReady.notify_one();
        return;
    }

    injector->finish();
}

size_t AudioInjectorManager::getNumInjectors() {
    Lock lock(_injectorsMutex);
    return _injectors.size();
}
