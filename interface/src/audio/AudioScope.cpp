//
//  AudioScope.cpp
//  interface/src/audio
//
//  Created by Stephen Birarda on 2014-12-16.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <limits>

#include <AudioClient.h>
#include <AudioConstants.h>
#include <GeometryCache.h>
#include <TextureCache.h>
#include <gpu/Context.h>
#include <GLMHelpers.h>

#include "AudioScope.h"

static const unsigned int DEFAULT_FRAMES_PER_SCOPE = 5;
static const unsigned int SCOPE_WIDTH = AudioConstants::NETWORK_FRAME_SAMPLES_PER_CHANNEL * DEFAULT_FRAMES_PER_SCOPE;
static const unsigned int MULTIPLIER_SCOPE_HEIGHT = 20;
static const unsigned int SCOPE_HEIGHT = 2 * 15 * MULTIPLIER_SCOPE_HEIGHT;

AudioScope::AudioScope() :
    _isEnabled(false),
    _isPaused(false),
    _scopeInputOffset(0),
    _scopeOutputOffset(0),
    _framesPerScope(DEFAULT_FRAMES_PER_SCOPE),
    _samplesPerScope(AudioConstants::NETWORK_FRAME_SAMPLES_PER_CHANNEL * _framesPerScope),
    _scopeInput(NULL),
    _scopeOutputLeft(NULL),
    _scopeOutputRight(NULL),
    _scopeLastFrame(),
    _audioScopeBackground(DependencyManager::get<GeometryCache>()->allocateID()),
    _audioScopeGrid(DependencyManager::get<GeometryCache>()->allocateID()),
    _inputID(DependencyManager::get<GeometryCache>()->allocateID()),
    _outputLeftID(DependencyManager::get<GeometryCache>()->allocateID()),
    _outputRightD(DependencyManager::get<GeometryCache>()->allocateID())
{
    auto audioIO = DependencyManager::get<AudioClient>();
    connect(&audioIO->getReceivedAudioStream(), &MixedProcessedAudioStream::addedSilence,
            this, &AudioScope::addStereoSilenceToScope);
    connect(&audioIO->getReceivedAudioStream(), &MixedProcessedAudioStream::addedLastFrameRepeatedWithFade,
            this, &AudioScope::addLastFrameRepeatedWithFadeToScope);
    connect(&audioIO->getReceivedAudioStream(), &MixedProcessedAudioStream::addedStereoSamples,
            this, &AudioScope::addStereoSamplesToScope);
    connect(audioIO.data(), &AudioClient::inputReceived, this, &AudioScope::addInputToScope);
}

void AudioScope::setVisible(bool visible) {
    if (_isEnabled != visible) {
        _isEnabled = visible;
        if (_isEnabled) {
            allocateScope();
        } else {
            freeScope();
        }
    }
}

void AudioScope::selectAudioScopeFiveFrames() {
    reallocateScope(5);
}

void AudioScope::selectAudioScopeTwentyFrames() {
    reallocateScope(20);
}

void AudioScope::selectAudioScopeFiftyFrames() {
    reallocateScope(50);
}

void AudioScope::allocateScope() {
    _scopeInputOffset = 0;
    _scopeOutputOffset = 0;
    int num = _samplesPerScope * sizeof(int16_t);
    _scopeInput = new QByteArray(num, 0);
    _scopeOutputLeft = new QByteArray(num, 0);
    _scopeOutputRight = new QByteArray(num, 0);
}

void AudioScope::reallocateScope(int frames) {
    if (_framesPerScope != frames) {
        _framesPerScope = frames;
        _samplesPerScope = AudioConstants::NETWORK_FRAME_SAMPLES_PER_CHANNEL * _framesPerScope;
        freeScope();
        allocateScope();
    }
}

void AudioScope::freeScope() {
    if (_scopeInput) {
        delete _scopeInput;
        _scopeInput = 0;
    }
    if (_scopeOutputLeft) {
        delete _scopeOutputLeft;
        _scopeOutputLeft = 0;
    }
    if (_scopeOutputRight) {
        delete _scopeOutputRight;
        _scopeOutputRight = 0;
    }
}

void AudioScope::render(RenderArgs* renderArgs, int width, int height) {
    
    if (!_isEnabled) {
        return;
    }
    
    static const glm::vec4 backgroundColor = { 0.4f, 0.4f, 0.4f, 0.6f };
    static const glm::vec4 gridColor = { 0.7f, 0.7f, 0.7f, 1.0f };
    static const glm::vec4 inputColor = { 0.3f, 1.0f, 0.3f, 1.0f };
    static const glm::vec4 outputLeftColor = { 1.0f, 0.3f, 0.3f, 1.0f };
    static const glm::vec4 outputRightColor = { 0.3f, 0.3f, 1.0f, 1.0f };
    static const int gridCols = 2;
    int gridRows = _framesPerScope;
    
    int x = (width - (int)SCOPE_WIDTH) / 2;
    int y = (height - (int)SCOPE_HEIGHT) / 2;
    int w = (int)SCOPE_WIDTH;
    int h = (int)SCOPE_HEIGHT;

    gpu::Batch& batch = *renderArgs->_batch;
    auto geometryCache = DependencyManager::get<GeometryCache>();

    // Grid uses its own pipeline, so draw it before setting another
    const float GRID_EDGE = 0.005f;
    geometryCache->renderGrid(batch, glm::vec2(x, y), glm::vec2(x + w, y + h),
        gridRows, gridCols, GRID_EDGE, gridColor, true, _audioScopeGrid);

    geometryCache->useSimpleDrawPipeline(batch);
    auto textureCache = DependencyManager::get<TextureCache>();
    batch.setResourceTexture(0, textureCache->getWhiteTexture());
    
    // FIXME - do we really need to reset this here? we know that we're called inside of ApplicationOverlay::renderOverlays
    //         which already set up our batch for us to have these settings
    mat4 legacyProjection = glm::ortho<float>(0, width, height, 0, -1000, 1000);
    batch.setProjectionTransform(legacyProjection);
    batch.setModelTransform(Transform());
    batch.resetViewTransform();

    geometryCache->renderQuad(batch, x, y, w, h, backgroundColor, _audioScopeBackground);
    renderLineStrip(batch, _inputID, inputColor, x, y, _samplesPerScope, _scopeInputOffset, _scopeInput);
    renderLineStrip(batch, _outputLeftID, outputLeftColor, x, y, _samplesPerScope, _scopeOutputOffset, _scopeOutputLeft);
    renderLineStrip(batch, _outputRightD, outputRightColor, x, y, _samplesPerScope, _scopeOutputOffset, _scopeOutputRight);
}

void AudioScope::renderLineStrip(gpu::Batch& batch, int id, const glm::vec4& color, int x, int y, int n, int offset, const QByteArray* byteArray) {

    int16_t sample;
    int16_t* samples = ((int16_t*) byteArray->data()) + offset;
    int numSamplesToAverage = _framesPerScope / DEFAULT_FRAMES_PER_SCOPE;
    int count = (n - offset) / numSamplesToAverage;
    int remainder = (n - offset) % numSamplesToAverage;
    y += SCOPE_HEIGHT / 2;
    
    auto geometryCache = DependencyManager::get<GeometryCache>();

    QVector<glm::vec2> points;
    

    // Compute and draw the sample averages from the offset position
    for (int i = count; --i >= 0; ) {
        sample = 0;
        for (int j = numSamplesToAverage; --j >= 0; ) {
            sample += *samples++;
        }
        sample /= numSamplesToAverage;
        points << glm::vec2(x++, y - sample);
    }

    // Compute and draw the sample average across the wrap boundary
    if (remainder != 0) {
        sample = 0;
        for (int j = remainder; --j >= 0; ) {
            sample += *samples++;
        }
    
        samples = (int16_t*) byteArray->data();

        for (int j = numSamplesToAverage - remainder; --j >= 0; ) {
            sample += *samples++;
        }
        sample /= numSamplesToAverage;
        points << glm::vec2(x++, y - sample);
    } else {
        samples = (int16_t*) byteArray->data();
    }

    // Compute and draw the sample average from the beginning to the offset
    count = (offset - remainder) / numSamplesToAverage;
    for (int i = count; --i >= 0; ) {
        sample = 0;
        for (int j = numSamplesToAverage; --j >= 0; ) {
            sample += *samples++;
        }
        sample /= numSamplesToAverage;
        points << glm::vec2(x++, y - sample);
    }
    
    
    geometryCache->updateVertices(id, points, color);
    geometryCache->renderVertices(batch, gpu::LINE_STRIP, id);
}

int AudioScope::addBufferToScope(QByteArray* byteArray, int frameOffset, const int16_t* source, int sourceSamplesPerChannel,
                            unsigned int sourceChannel, unsigned int sourceNumberOfChannels, float fade) {
    if (!_isEnabled || _isPaused) {
        return 0;
    }
    
    // Temporary variable receives sample value
    float sample;
    
    // Short int pointer to mapped samples in byte array
    int16_t* destination = (int16_t*) byteArray->data();
    
    for (int i = 0; i < sourceSamplesPerChannel; i++) {
        sample = (float)source[i * sourceNumberOfChannels + sourceChannel];
        destination[frameOffset] = sample / (float) AudioConstants::MAX_SAMPLE_VALUE * (float)SCOPE_HEIGHT / 2.0f;
        frameOffset = (frameOffset == _samplesPerScope - 1) ? 0 : frameOffset + 1;
    }
    return frameOffset;
}

int AudioScope::addSilenceToScope(QByteArray* byteArray, int frameOffset, int silentSamples) {
                                                                                                                                                                                                                                    
    // Short int pointer to mapped samples in byte array
    int16_t* destination = (int16_t*)byteArray->data();
    
    if (silentSamples >= _samplesPerScope) {
        memset(destination, 0, byteArray->size());
        return frameOffset;
    }
    
    int samplesToBufferEnd = _samplesPerScope - frameOffset;
    if (silentSamples > samplesToBufferEnd) {
        memset(destination + frameOffset, 0, samplesToBufferEnd * sizeof(int16_t));
        memset(destination, 0, (silentSamples - samplesToBufferEnd) * sizeof(int16_t));
    } else {
        memset(destination + frameOffset, 0, silentSamples * sizeof(int16_t));
    }
    
    return (frameOffset + silentSamples) % _samplesPerScope;
}


void AudioScope::addStereoSilenceToScope(int silentSamplesPerChannel) {
    if (!_isEnabled || _isPaused) {
        return;
    }
    addSilenceToScope(_scopeOutputLeft, _scopeOutputOffset, silentSamplesPerChannel);
    _scopeOutputOffset = addSilenceToScope(_scopeOutputRight, _scopeOutputOffset, silentSamplesPerChannel);
}

void AudioScope::addStereoSamplesToScope(const QByteArray& samples) {
    if (!_isEnabled || _isPaused) {
        return;
    }
    const int16_t* samplesData = reinterpret_cast<const int16_t*>(samples.data());
    int samplesPerChannel = samples.size() / sizeof(int16_t) / AudioConstants::STEREO;
    
    addBufferToScope(_scopeOutputLeft, _scopeOutputOffset, samplesData, samplesPerChannel, 0, AudioConstants::STEREO);
    _scopeOutputOffset = addBufferToScope(_scopeOutputRight, _scopeOutputOffset, samplesData, samplesPerChannel, 1, AudioConstants::STEREO);
    
    _scopeLastFrame = samples.right(AudioConstants::NETWORK_FRAME_BYTES_STEREO);
}

void AudioScope::addLastFrameRepeatedWithFadeToScope(int samplesPerChannel) {
    const int16_t* lastFrameData = reinterpret_cast<const int16_t*>(_scopeLastFrame.data());
    
    int samplesRemaining = samplesPerChannel;
    int indexOfRepeat = 0;
    do {
        int samplesToWriteThisIteration = std::min(samplesRemaining, (int) AudioConstants::NETWORK_FRAME_SAMPLES_PER_CHANNEL);
        float fade = calculateRepeatedFrameFadeFactor(indexOfRepeat);
        addBufferToScope(_scopeOutputLeft, _scopeOutputOffset, lastFrameData,
                         samplesToWriteThisIteration, 0, AudioConstants::STEREO, fade);
        _scopeOutputOffset = addBufferToScope(_scopeOutputRight, _scopeOutputOffset,
                                              lastFrameData, samplesToWriteThisIteration, 1, AudioConstants::STEREO, fade);
        
        samplesRemaining -= samplesToWriteThisIteration;
        indexOfRepeat++;
    } while (samplesRemaining > 0);
}

void AudioScope::addInputToScope(const QByteArray& inputSamples) {
    if (!_isEnabled || _isPaused) {
        return;
    }
    
    const int INPUT_AUDIO_CHANNEL = 0;
    const int NUM_INPUT_CHANNELS = 1;
    
    _scopeInputOffset = addBufferToScope(_scopeInput, _scopeInputOffset,
                                         reinterpret_cast<const int16_t*>(inputSamples.data()),
                                         inputSamples.size() / sizeof(int16_t), INPUT_AUDIO_CHANNEL, NUM_INPUT_CHANNELS);
}
