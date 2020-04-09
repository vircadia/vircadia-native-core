//
//  AudioMixerSlave.cpp
//  assignment-client/src/audio
//
//  Created by Zach Pomerantz on 11/22/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AudioMixerSlave.h"

#include <algorithm>

#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtx/vector_angle.hpp>

#include <LogHandler.h>
#include <NetworkAccessManager.h>
#include <NodeList.h>
#include <Node.h>
#include <OctreeConstants.h>
#include <plugins/PluginManager.h>
#include <plugins/CodecPlugin.h>
#include <udt/PacketHeaders.h>
#include <SharedUtil.h>
#include <StDev.h>
#include <UUID.h>

#include "AudioRingBuffer.h"
#include "AudioMixer.h"
#include "AudioMixerClientData.h"
#include "AvatarAudioStream.h"
#include "InjectedAudioStream.h"
#include "AudioHelpers.h"

using namespace std;
using AudioStreamVector = AudioMixerClientData::AudioStreamVector;
using MixableStream = AudioMixerClientData::MixableStream;
using MixableStreamsVector = AudioMixerClientData::MixableStreamsVector;

// packet helpers
std::unique_ptr<NLPacket> createAudioPacket(PacketType type, int size, quint16 sequence, QString codec);
void sendMixPacket(const SharedNodePointer& node, AudioMixerClientData& data, QByteArray& buffer);
void sendSilentPacket(const SharedNodePointer& node, AudioMixerClientData& data);
void sendMutePacket(const SharedNodePointer& node, AudioMixerClientData&);
void sendEnvironmentPacket(const SharedNodePointer& node, AudioMixerClientData& data);

// mix helpers
inline float approximateGain(const AvatarAudioStream& listeningNodeStream, const PositionalAudioStream& streamToAdd);
inline float computeGain(float masterAvatarGain, float masterInjectorGain, const AvatarAudioStream& listeningNodeStream,
        const PositionalAudioStream& streamToAdd, const glm::vec3& relativePosition, float distance);
inline float computeAzimuth(const AvatarAudioStream& listeningNodeStream, const PositionalAudioStream& streamToAdd,
        const glm::vec3& relativePosition);

void AudioMixerSlave::processPackets(const SharedNodePointer& node) {
    AudioMixerClientData* data = (AudioMixerClientData*)node->getLinkedData();
    if (data) {
        // process packets and collect the number of streams available for this frame
        stats.sumStreams += data->processPackets(_sharedData.addedStreams);
    }
}

void AudioMixerSlave::configureMix(ConstIter begin, ConstIter end, unsigned int frame, int numToRetain) {
    _begin = begin;
    _end = end;
    _frame = frame;
    _numToRetain = numToRetain;
}

void AudioMixerSlave::mix(const SharedNodePointer& node) {
    // check that the node is valid
    AudioMixerClientData* data = (AudioMixerClientData*)node->getLinkedData();
    if (data == nullptr) {
        return;
    }

    if (node->isUpstream()) {
        return;
    }

    // check that the stream is valid
    auto avatarStream = data->getAvatarAudioStream();
    if (avatarStream == nullptr) {
        return;
    }

    // send mute packet, if necessary
    if (AudioMixer::shouldMute(avatarStream->getQuietestFrameLoudness()) || data->shouldMuteClient()) {
        sendMutePacket(node, *data);
    }

    // send audio packets, if necessary
    if (node->getType() == NodeType::Agent && node->getActiveSocket()) {
        ++stats.sumListeners;

        // mix the audio
        bool mixHasAudio = prepareMix(node);

        // send audio packet
        if (mixHasAudio || data->shouldFlushEncoder()) {
            QByteArray encodedBuffer;
            if (mixHasAudio) {
                // encode the audio
                QByteArray decodedBuffer(reinterpret_cast<char*>(_bufferSamples), AudioConstants::NETWORK_FRAME_BYTES_STEREO);
                data->encode(decodedBuffer, encodedBuffer);
            } else {
                // time to flush (resets shouldFlush until the next encode)
                data->encodeFrameOfZeros(encodedBuffer);
            }

            sendMixPacket(node, *data, encodedBuffer);
        } else {
            ++stats.sumListenersSilent;
            sendSilentPacket(node, *data);
        }

        // send environment packet
        sendEnvironmentPacket(node, *data);

        // send stats packet (about every second)
        const unsigned int NUM_FRAMES_PER_SEC = (int)ceil(AudioConstants::NETWORK_FRAMES_PER_SEC);
        if (data->shouldSendStats(_frame % NUM_FRAMES_PER_SEC)) {
            data->sendAudioStreamStatsPackets(node);
        }
    }
}


template <class Container, class Predicate>
void erase_if(Container& cont, Predicate&& pred) {
    auto it = remove_if(begin(cont), end(cont), std::forward<Predicate>(pred));
    cont.erase(it, end(cont));
}

template <class Container>
bool contains(const Container& cont, typename Container::value_type value) {
    return std::any_of(begin(cont), end(cont), [&value](const auto& element) {
        return value == element;
    });
}

// This class lets you do an erase if in several segments
// that use different predicates
template <class Container>
class SegmentedEraseIf {
public:
    using iterator = typename Container::iterator;

    SegmentedEraseIf(Container& cont) : _cont(cont) {
        _first = begin(_cont);
        _it = _first;
    }
    ~SegmentedEraseIf() {
        assert(_it == end(_cont));
        _cont.erase(_first, _it);
    }

    template <class Predicate>
    void iterateTo(iterator last, Predicate pred) {
        while (_it != last) {
            if (!pred(*_it)) {
                if (_first != _it) {
                    *_first = move(*_it);
                }
                ++_first;
            }
            ++_it;
        }
    }

private:
    iterator _first;
    iterator _it;
    Container& _cont;
};


void AudioMixerSlave::addStreams(Node& listener, AudioMixerClientData& listenerData) {
    auto& ignoredNodeIDs = listener.getIgnoredNodeIDs();
    auto& ignoringNodeIDs = listenerData.getIgnoringNodeIDs();

    auto& streams = listenerData.getStreams();

    // add data for newly created streams to our vector
    if (!listenerData.getHasReceivedFirstMix()) {
        // when this listener is new, we need to fill its added streams object with all available streams
        std::for_each(_begin, _end, [&](const SharedNodePointer& node) {
            AudioMixerClientData* nodeData = static_cast<AudioMixerClientData*>(node->getLinkedData());
            if (nodeData) {
                for (auto& stream : nodeData->getAudioStreams()) {
                    bool ignoredByListener = contains(ignoredNodeIDs, node->getUUID());
                    bool ignoringListener = contains(ignoringNodeIDs, node->getUUID());

                    if (ignoredByListener || ignoringListener) {
                        streams.skipped.emplace_back(node->getUUID(), node->getLocalID(),
                                                    stream->getStreamIdentifier(), stream.get());

                        // pre-populate ignored and ignoring flags for this stream
                        streams.skipped.back().ignoredByListener = ignoredByListener;
                        streams.skipped.back().ignoringListener = ignoringListener;
                    } else {
                        streams.active.emplace_back(node->getUUID(), node->getLocalID(),
                                                     stream->getStreamIdentifier(), stream.get());
                    }
                }
            }
        });

        // flag this listener as having received their first mix so we know we don't need to enumerate all nodes again
        listenerData.setHasReceivedFirstMix(true);
    } else {
        for (const auto& newStream : _sharedData.addedStreams) {
            bool ignoredByListener = contains(ignoredNodeIDs, newStream.nodeIDStreamID.nodeID);
            bool ignoringListener = contains(ignoringNodeIDs, newStream.nodeIDStreamID.nodeID);

            if (ignoredByListener || ignoringListener) {
                streams.skipped.emplace_back(newStream.nodeIDStreamID, newStream.positionalStream);

                // pre-populate ignored and ignoring flags for this stream
                streams.skipped.back().ignoredByListener = ignoredByListener;
                streams.skipped.back().ignoringListener = ignoringListener;
            } else {
                streams.active.emplace_back(newStream.nodeIDStreamID, newStream.positionalStream);
            }
        }
    }
}

bool shouldBeRemoved(const MixableStream& stream, const AudioMixerSlave::SharedData& sharedData) {
    return (contains(sharedData.removedNodes, stream.nodeStreamID.nodeLocalID) ||
            contains(sharedData.removedStreams, stream.nodeStreamID));
};

bool shouldBeInactive(MixableStream& stream) {
    return (!stream.positionalStream->lastPopSucceeded() ||
            stream.positionalStream->getLastPopOutputLoudness() == 0.0f);
};

bool shouldBeSkipped(MixableStream& stream, const Node& listener,
                     const AvatarAudioStream& listenerAudioStream,
                     const AudioMixerClientData& listenerData) {

    if (stream.nodeStreamID.nodeLocalID == listener.getLocalID()) {
        return !stream.positionalStream->shouldLoopbackForNode();
    }

    // grab the unprocessed ignores and unignores from and for this listener
    const auto& nodesIgnoredByListener = listenerData.getNewIgnoredNodeIDs();
    const auto& nodesUnignoredByListener = listenerData.getNewUnignoredNodeIDs();
    const auto& nodesIgnoringListener = listenerData.getNewIgnoringNodeIDs();
    const auto& nodesUnignoringListener = listenerData.getNewUnignoringNodeIDs();

    // this stream was previously not ignored by the listener and we have some newly ignored streams
    // check now if it is one of the ignored streams and flag it as such
    if (stream.ignoredByListener) {
        stream.ignoredByListener = !contains(nodesUnignoredByListener, stream.nodeStreamID.nodeID);
    } else {
        stream.ignoredByListener = contains(nodesIgnoredByListener, stream.nodeStreamID.nodeID);
    }

    if (stream.ignoringListener) {
        stream.ignoringListener = !contains(nodesUnignoringListener, stream.nodeStreamID.nodeID);
    } else {
        stream.ignoringListener = contains(nodesIgnoringListener, stream.nodeStreamID.nodeID);
    }

    bool listenerIsAdmin = listenerData.getRequestsDomainListData() && listener.getCanKick();
    if (stream.ignoredByListener || (stream.ignoringListener && !listenerIsAdmin)) {
        return true;
    }

    if (!listenerData.getSoloedNodes().empty()) {
        return !contains(listenerData.getSoloedNodes(), stream.nodeStreamID.nodeID);
    }

    bool shouldCheckIgnoreBox = (listenerAudioStream.isIgnoreBoxEnabled() ||
                                 stream.positionalStream->isIgnoreBoxEnabled());
    if (shouldCheckIgnoreBox &&
        listenerAudioStream.getIgnoreBox().touches(stream.positionalStream->getIgnoreBox())) {
        return true;
    }

    return false;
};

float approximateVolume(const MixableStream& stream, const AvatarAudioStream* listenerAudioStream) {
    if (stream.positionalStream->getLastPopOutputTrailingLoudness() == 0.0f) {
        return 0.0f;
    }

    if (stream.positionalStream == listenerAudioStream) {
        return 1.0f;
    }

    // approximate the gain
    float gain = approximateGain(*listenerAudioStream, *(stream.positionalStream));

    // for avatar streams, modify by the set gain adjustment
    if (stream.nodeStreamID.streamID.isNull()) {
        gain *= stream.hrtf->getGainAdjustment();
    }

    return stream.positionalStream->getLastPopOutputTrailingLoudness() * gain;
};

bool AudioMixerSlave::prepareMix(const SharedNodePointer& listener) {
    AvatarAudioStream* listenerAudioStream = static_cast<AudioMixerClientData*>(listener->getLinkedData())->getAvatarAudioStream();
    AudioMixerClientData* listenerData = static_cast<AudioMixerClientData*>(listener->getLinkedData());

    // zero out the mix for this listener
    memset(_mixSamples, 0, sizeof(_mixSamples));

    bool isThrottling = _numToRetain != -1;
    bool isSoloing = !listenerData->getSoloedNodes().empty();

    auto& streams = listenerData->getStreams();

    addStreams(*listener, *listenerData);

    // Process skipped streams
    erase_if(streams.skipped, [&](MixableStream& stream) {
        if (shouldBeRemoved(stream, _sharedData)) {
            return true;
        }

        if (!shouldBeSkipped(stream, *listener, *listenerAudioStream, *listenerData)) {
            if (shouldBeInactive(stream)) {
                streams.inactive.push_back(move(stream));
                ++stats.skippedToInactive;
            } else {
                streams.active.push_back(move(stream));
                ++stats.skippedToActive;
            }
            return true;
        }

        if (!isThrottling) {
            updateHRTFParameters(stream, *listenerAudioStream, listenerData->getMasterAvatarGain(),
                                 listenerData->getMasterInjectorGain());
        }
        return false;
    });

    // Process inactive streams
    erase_if(streams.inactive, [&](MixableStream& stream) {
        if (shouldBeRemoved(stream, _sharedData)) {
            return true;
        }

        if (shouldBeSkipped(stream, *listener, *listenerAudioStream, *listenerData)) {
            streams.skipped.push_back(move(stream));
            ++stats.inactiveToSkipped;
            return true;
        }

        if (!shouldBeInactive(stream)) {
            streams.active.push_back(move(stream));
            ++stats.inactiveToActive;
            return true;
        }

        if (!isThrottling) {
            updateHRTFParameters(stream, *listenerAudioStream, listenerData->getMasterAvatarGain(),
                                 listenerData->getMasterInjectorGain());
        }
        return false;
    });

    // Process active streams
    erase_if(streams.active, [&](MixableStream& stream) {
        if (shouldBeRemoved(stream, _sharedData)) {
            return true;
        }

        if (isThrottling) {
            // we're throttling, so we need to update the approximate volume for any un-skipped streams
            // unless this is simply for an echo (in which case the approx volume is 1.0)
            stream.approximateVolume = approximateVolume(stream, listenerAudioStream);
        } else {
            if (shouldBeSkipped(stream, *listener, *listenerAudioStream, *listenerData)) {
                addStream(stream, *listenerAudioStream, 0.0f, 0.0f, isSoloing);
                streams.skipped.push_back(move(stream));
                ++stats.activeToSkipped;
                return true;
            }

            addStream(stream, *listenerAudioStream, listenerData->getMasterAvatarGain(), listenerData->getMasterInjectorGain(),
                      isSoloing);

            if (shouldBeInactive(stream)) {
                // To reduce artifacts we still call render to flush the HRTF for every silent
                // sources on the first frame where the source becomes silent
                // this ensures the correct tail from last mixed block
                streams.inactive.push_back(move(stream));
                ++stats.activeToInactive;
                return true;
            }
        }

        return false;
    });

    if (isThrottling) {
        // since we're throttling, we need to partition the mixable into throttled and unthrottled streams
        int numToRetain = min(_numToRetain, (int)streams.active.size()); // Make sure we don't overflow
        auto throttlePoint = begin(streams.active) + numToRetain;

        std::nth_element(streams.active.begin(), throttlePoint, streams.active.end(),
                         [](const auto& a, const auto& b)
                         {
                             return a.approximateVolume > b.approximateVolume;
                         });

        SegmentedEraseIf<MixableStreamsVector> erase(streams.active);
        erase.iterateTo(throttlePoint, [&](MixableStream& stream) {
            if (shouldBeSkipped(stream, *listener, *listenerAudioStream, *listenerData)) {
                resetHRTFState(stream);
                streams.skipped.push_back(move(stream));
                ++stats.activeToSkipped;
                return true;
            }

            addStream(stream, *listenerAudioStream, listenerData->getMasterAvatarGain(), listenerData->getMasterInjectorGain(),
                      isSoloing);

            if (shouldBeInactive(stream)) {
                // To reduce artifacts we still call render to flush the HRTF for every silent
                // sources on the first frame where the source becomes silent
                // this ensures the correct tail from last mixed block
                streams.inactive.push_back(move(stream));
                ++stats.activeToInactive;
                return true;
            }

            return false;
        });
        erase.iterateTo(end(streams.active), [&](MixableStream& stream) {
            // To reduce artifacts we reset the HRTF state for every throttled
            // sources on the first frame where the source becomes throttled
            // this ensures at least remove the tail from last mixed block
            // preventing excessive artifacts on the next first block
            resetHRTFState(stream);

            if (shouldBeSkipped(stream, *listener, *listenerAudioStream, *listenerData)) {
                streams.skipped.push_back(move(stream));
                ++stats.activeToSkipped;
                return true;
            }

            if (shouldBeInactive(stream)) {
                streams.inactive.push_back(move(stream));
                ++stats.activeToInactive;
                return true;
            }

            return false;
        });
    }

    stats.skipped += (int)streams.skipped.size();
    stats.inactive += (int)streams.inactive.size();
    stats.active += (int)streams.active.size();

    // clear the newly ignored, un-ignored, ignoring, and un-ignoring streams now that we've processed them
    listenerData->clearStagedIgnoreChanges();

#ifdef HIFI_AUDIO_MIXER_DEBUG
    auto mixEnd = p_high_resolution_clock::now();
    auto mixTime = std::chrono::duration_cast<std::chrono::nanoseconds>(mixEnd - mixStart);
    stats.mixTime += mixTime.count();
#endif

    // check for silent audio before limiting
    // limiting uses a dither and can only guarantee abs(sample) <= 1
    bool hasAudio = false;
    for (int i = 0; i < AudioConstants::NETWORK_FRAME_SAMPLES_STEREO; ++i) {
        if (_mixSamples[i] != 0.0f) {
            hasAudio = true;
            break;
        }
    }

    // use the per listener AudioLimiter to render the mixed data
    listenerData->audioLimiter.render(_mixSamples, _bufferSamples, AudioConstants::NETWORK_FRAME_SAMPLES_PER_CHANNEL);

    return hasAudio;
}

void AudioMixerSlave::addStream(AudioMixerClientData::MixableStream& mixableStream,
                                AvatarAudioStream& listeningNodeStream,
                                float masterAvatarGain,
                                float masterInjectorGain,
                                bool isSoloing) {
    ++stats.totalMixes;

    auto streamToAdd = mixableStream.positionalStream;

    // check if this is a server echo of a source back to itself
    bool isEcho = (streamToAdd == &listeningNodeStream);

    glm::vec3 relativePosition = streamToAdd->getPosition() - listeningNodeStream.getPosition();

    float distance = glm::max(glm::length(relativePosition), EPSILON);
    float gain = isEcho ? 1.0f
                        : (isSoloing ? masterAvatarGain
                                     : computeGain(masterAvatarGain, masterInjectorGain, listeningNodeStream, *streamToAdd,
                                                   relativePosition, distance));
    float azimuth = isEcho ? 0.0f : computeAzimuth(listeningNodeStream, listeningNodeStream, relativePosition);

    const int HRTF_DATASET_INDEX = 1;

    if (!streamToAdd->lastPopSucceeded()) {
        bool forceSilentBlock = true;

        if (!streamToAdd->getLastPopOutput().isNull()) {
            bool isInjector = dynamic_cast<const InjectedAudioStream*>(streamToAdd);

            // in an injector, just go silent - the injector has likely ended
            // in other inputs (microphone, &c.), repeat with fade to avoid the harsh jump to silence
            if (!isInjector) {
                // calculate its fade factor, which depends on how many times it's already been repeated.
                float fadeFactor = calculateRepeatedFrameFadeFactor(streamToAdd->getConsecutiveNotMixedCount() - 1);
                if (fadeFactor > 0.0f) {
                    // apply the fadeFactor to the gain
                    gain *= fadeFactor;
                    forceSilentBlock = false;
                }
            }
        }

        if (forceSilentBlock) {
            // call renderSilent with a forced silent block to reduce artifacts
            // (this is not done for stereo streams since they do not go through the HRTF)
            if (!streamToAdd->isStereo() && !isEcho) {
                static int16_t silentMonoBlock[AudioConstants::NETWORK_FRAME_SAMPLES_PER_CHANNEL] = {};
                mixableStream.hrtf->render(silentMonoBlock, _mixSamples, HRTF_DATASET_INDEX, azimuth, distance, gain,
                                           AudioConstants::NETWORK_FRAME_SAMPLES_PER_CHANNEL);

                ++stats.hrtfRenders;
            }

            return;
        }
    }

    // grab the stream from the ring buffer
    AudioRingBuffer::ConstIterator streamPopOutput = streamToAdd->getLastPopOutput();

    if (streamToAdd->isStereo()) {

        streamPopOutput.readSamples(_bufferSamples, AudioConstants::NETWORK_FRAME_SAMPLES_STEREO);

        // stereo sources are not passed through HRTF
        mixableStream.hrtf->mixStereo(_bufferSamples, _mixSamples, gain, AudioConstants::NETWORK_FRAME_SAMPLES_PER_CHANNEL);

        ++stats.manualStereoMixes;
    } else if (isEcho) {

        streamPopOutput.readSamples(_bufferSamples, AudioConstants::NETWORK_FRAME_SAMPLES_PER_CHANNEL);

        // echo sources are not passed through HRTF
        mixableStream.hrtf->mixMono(_bufferSamples, _mixSamples, gain, AudioConstants::NETWORK_FRAME_SAMPLES_PER_CHANNEL);

        ++stats.manualEchoMixes;
    } else {

        streamPopOutput.readSamples(_bufferSamples, AudioConstants::NETWORK_FRAME_SAMPLES_PER_CHANNEL);

        mixableStream.hrtf->render(_bufferSamples, _mixSamples, HRTF_DATASET_INDEX, azimuth, distance, gain,
                                   AudioConstants::NETWORK_FRAME_SAMPLES_PER_CHANNEL);
        ++stats.hrtfRenders;
    }
}

void AudioMixerSlave::updateHRTFParameters(AudioMixerClientData::MixableStream& mixableStream,
                                           AvatarAudioStream& listeningNodeStream,
                                           float masterAvatarGain,
                                           float masterInjectorGain) {
    auto streamToAdd = mixableStream.positionalStream;

    // check if this is a server echo of a source back to itself
    bool isEcho = (streamToAdd == &listeningNodeStream);

    glm::vec3 relativePosition = streamToAdd->getPosition() - listeningNodeStream.getPosition();

    float distance = glm::max(glm::length(relativePosition), EPSILON);
    float gain = isEcho ? 1.0f : computeGain(masterAvatarGain, masterInjectorGain, listeningNodeStream, *streamToAdd, 
                                             relativePosition, distance);
    float azimuth = isEcho ? 0.0f : computeAzimuth(listeningNodeStream, listeningNodeStream, relativePosition);

    mixableStream.hrtf->setParameterHistory(azimuth, distance, gain);

    ++stats.hrtfUpdates;
}

void AudioMixerSlave::resetHRTFState(AudioMixerClientData::MixableStream& mixableStream) {
     mixableStream.hrtf->reset();
    ++stats.hrtfResets;
}

std::unique_ptr<NLPacket> createAudioPacket(PacketType type, int size, quint16 sequence, QString codec) {
    auto audioPacket = NLPacket::create(type, size);
    audioPacket->writePrimitive(sequence);
    audioPacket->writeString(codec);
    return audioPacket;
}

void sendMixPacket(const SharedNodePointer& node, AudioMixerClientData& data, QByteArray& buffer) {
    const int MIX_PACKET_SIZE =
        sizeof(quint16) + AudioConstants::MAX_CODEC_NAME_LENGTH_ON_WIRE + AudioConstants::NETWORK_FRAME_BYTES_STEREO;
    quint16 sequence = data.getOutgoingSequenceNumber();
    QString codec = data.getCodecName();
    auto mixPacket = createAudioPacket(PacketType::MixedAudio, MIX_PACKET_SIZE, sequence, codec);

    // pack samples
    mixPacket->write(buffer.constData(), buffer.size());

    // send packet
    DependencyManager::get<NodeList>()->sendPacket(std::move(mixPacket), *node);
    data.incrementOutgoingMixedAudioSequenceNumber();
}

void sendSilentPacket(const SharedNodePointer& node, AudioMixerClientData& data) {
    const int SILENT_PACKET_SIZE =
        sizeof(quint16) + AudioConstants::MAX_CODEC_NAME_LENGTH_ON_WIRE + sizeof(quint16);
    quint16 sequence = data.getOutgoingSequenceNumber();
    QString codec = data.getCodecName();
    auto mixPacket = createAudioPacket(PacketType::SilentAudioFrame, SILENT_PACKET_SIZE, sequence, codec);

    // pack number of samples
    mixPacket->writePrimitive(AudioConstants::NETWORK_FRAME_SAMPLES_STEREO);

    // send packet
    DependencyManager::get<NodeList>()->sendPacket(std::move(mixPacket), *node);
    data.incrementOutgoingMixedAudioSequenceNumber();
}

void sendMutePacket(const SharedNodePointer& node, AudioMixerClientData& data) {
    auto mutePacket = NLPacket::create(PacketType::NoisyMute, 0);
    DependencyManager::get<NodeList>()->sendPacket(std::move(mutePacket), *node);

    // probably now we just reset the flag, once should do it (?)
    data.setShouldMuteClient(false);
}

void sendEnvironmentPacket(const SharedNodePointer& node, AudioMixerClientData& data) {
    bool hasReverb = false;
    float reverbTime, wetLevel;

    auto& reverbSettings = AudioMixer::getReverbSettings();
    auto& audioZones = AudioMixer::getAudioZones();

    AvatarAudioStream* stream = data.getAvatarAudioStream();
    glm::vec3 streamPosition = stream->getPosition();

    // find reverb properties
    for (const auto& settings : reverbSettings) {
        AABox box = audioZones[settings.zone].area;
        if (box.contains(streamPosition)) {
            hasReverb = true;
            reverbTime = settings.reverbTime;
            wetLevel = settings.wetLevel;
            break;
        }
    }

    // check if data changed
    bool dataChanged = (stream->hasReverb() != hasReverb) ||
        (stream->hasReverb() && (stream->getRevebTime() != reverbTime || stream->getWetLevel() != wetLevel));
    if (dataChanged) {
        // update stream
        if (hasReverb) {
            stream->setReverb(reverbTime, wetLevel);
        } else {
            stream->clearReverb();
        }
    }

    // send packet at change or every so often
    float CHANCE_OF_SEND = 0.01f;
    bool sendData = dataChanged || (randFloat() < CHANCE_OF_SEND);

    if (sendData) {
        // size the packet
        unsigned char bitset = 0;
        int packetSize = sizeof(bitset);
        if (hasReverb) {
            packetSize += sizeof(reverbTime) + sizeof(wetLevel);
        }

        // write the packet
        auto envPacket = NLPacket::create(PacketType::AudioEnvironment, packetSize);
        if (hasReverb) {
            setAtBit(bitset, HAS_REVERB_BIT);
        }
        envPacket->writePrimitive(bitset);
        if (hasReverb) {
            envPacket->writePrimitive(reverbTime);
            envPacket->writePrimitive(wetLevel);
        }

        // send the packet
        DependencyManager::get<NodeList>()->sendPacket(std::move(envPacket), *node);
    }
}

float approximateGain(const AvatarAudioStream& listeningNodeStream, const PositionalAudioStream& streamToAdd) {
    float gain = 1.0f;

    // injector: apply attenuation
    if (streamToAdd.getType() == PositionalAudioStream::Injector) {
        gain *= reinterpret_cast<const InjectedAudioStream*>(&streamToAdd)->getAttenuationRatio();
        // injector: skip master gain
    }

    // avatar: skip attenuation - it is too costly to approximate

    // distance attenuation: approximate, ignore zone-specific attenuations
    glm::vec3 relativePosition = streamToAdd.getPosition() - listeningNodeStream.getPosition();
    float distance = glm::length(relativePosition);
    return gain / distance;

    // avatar: skip master gain
}

float computeGain(float masterAvatarGain,
                  float masterInjectorGain,
                  const AvatarAudioStream& listeningNodeStream,
                  const PositionalAudioStream& streamToAdd,
                  const glm::vec3& relativePosition,
                  float distance) {
    float gain = 1.0f;

    // injector: apply attenuation
    if (streamToAdd.getType() == PositionalAudioStream::Injector) {
        gain *= reinterpret_cast<const InjectedAudioStream*>(&streamToAdd)->getAttenuationRatio();
        // apply master gain
        gain *= masterInjectorGain;

    // avatar: apply fixed off-axis attenuation to make them quieter as they turn away
    } else if (streamToAdd.getType() == PositionalAudioStream::Microphone) {
        glm::vec3 rotatedListenerPosition = glm::inverse(streamToAdd.getOrientation()) * relativePosition;

        // source directivity is based on angle of emission, in local coordinates
        glm::vec3 direction = glm::normalize(rotatedListenerPosition);
        float angleOfDelivery = fastAcosf(glm::clamp(-direction.z, -1.0f, 1.0f));   // UNIT_NEG_Z is "forward"

        const float MAX_OFF_AXIS_ATTENUATION = 0.2f;
        const float OFF_AXIS_ATTENUATION_STEP = (1 - MAX_OFF_AXIS_ATTENUATION) / 2.0f;
        float offAxisCoefficient = MAX_OFF_AXIS_ATTENUATION + (angleOfDelivery * (OFF_AXIS_ATTENUATION_STEP / PI_OVER_TWO));

        gain *= offAxisCoefficient;

        // apply master gain
        gain *= masterAvatarGain;
    }

    auto& audioZones = AudioMixer::getAudioZones();
    auto& zoneSettings = AudioMixer::getZoneSettings();

    // find distance attenuation coefficient
    float attenuationPerDoublingInDistance = AudioMixer::getAttenuationPerDoublingInDistance();
    for (const auto& settings : zoneSettings) {
        if (audioZones[settings.source].area.contains(streamToAdd.getPosition()) &&
            audioZones[settings.listener].area.contains(listeningNodeStream.getPosition())) {
            attenuationPerDoublingInDistance = settings.coefficient;
            break;
        }
    }

    if (attenuationPerDoublingInDistance < 0.0f) {
        // translate a negative zone setting to distance limit
        const float MIN_DISTANCE_LIMIT = ATTN_DISTANCE_REF + 1.0f;  // silent after 1m
        float distanceLimit = std::max(-attenuationPerDoublingInDistance, MIN_DISTANCE_LIMIT);

        // calculate the LINEAR attenuation using the distance to this node
        // reference attenuation of 0dB at distance = ATTN_DISTANCE_REF
        float d = distance - ATTN_DISTANCE_REF;
        gain *= std::max(1.0f - d / (distanceLimit - ATTN_DISTANCE_REF), 0.0f);
        gain = std::min(gain, ATTN_GAIN_MAX);

    } else if (attenuationPerDoublingInDistance < 1.0f) {
        // translate a positive zone setting to gain per log2(distance)
        const float MIN_ATTENUATION_COEFFICIENT = 0.001f;   // -60dB per log2(distance)
        float g = glm::clamp(1.0f - attenuationPerDoublingInDistance, MIN_ATTENUATION_COEFFICIENT, 1.0f);

        // calculate the LOGARITHMIC attenuation using the distance to this node
        // reference attenuation of 0dB at distance = ATTN_DISTANCE_REF
        float d = (1.0f / ATTN_DISTANCE_REF) * std::max(distance, HRTF_NEARFIELD_MIN);
        gain *= fastExp2f(fastLog2f(g) * fastLog2f(d));
        gain = std::min(gain, ATTN_GAIN_MAX);

    } else {
        // translate a zone setting of 1.0 be silent at any distance
        gain = 0.0f;
    }

    return gain;
}

float computeAzimuth(const AvatarAudioStream& listeningNodeStream,
                     const PositionalAudioStream& streamToAdd,
                     const glm::vec3& relativePosition) {
    glm::quat inverseOrientation = glm::inverse(listeningNodeStream.getOrientation());

    glm::vec3 rotatedSourcePosition = inverseOrientation * relativePosition;

    // project the rotated source position vector onto the XZ plane
    rotatedSourcePosition.y = 0.0f;

    const float SOURCE_DISTANCE_THRESHOLD = 1e-30f;

    float rotatedSourcePositionLength2 = glm::length2(rotatedSourcePosition);
    if (rotatedSourcePositionLength2 > SOURCE_DISTANCE_THRESHOLD) {
        
        // produce an oriented angle about the y-axis
        glm::vec3 direction = rotatedSourcePosition * (1.0f / fastSqrtf(rotatedSourcePositionLength2));
        float angle = fastAcosf(glm::clamp(-direction.z, -1.0f, 1.0f)); // UNIT_NEG_Z is "forward"
        return (direction.x < 0.0f) ? -angle : angle;

    } else {   
        // no azimuth if they are in same spot
        return 0.0f; 
    }
}
