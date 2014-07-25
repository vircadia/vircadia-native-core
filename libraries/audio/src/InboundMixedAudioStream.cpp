
#include "InboundMixedAudioStream.h"

InboundMixedAudioStream::InboundMixedAudioStream(int numFrameSamples, int numFramesCapacity, bool dynamicJitterBuffers)
    : InboundAudioStream(numFrameSamples, numFramesCapacity, dynamicJitterBuffers)
{
}

int InboundMixedAudioStream::parseStreamProperties(PacketType type, const QByteArray& packetAfterSeqNum, int& numAudioSamples) {
    // mixed audio packets do not have any info between the seq num and the audio data.
    numAudioSamples = packetAfterSeqNum.size() / sizeof(int16_t);
    return 0;
}

int InboundMixedAudioStream::parseAudioData(PacketType type, const QByteArray& packetAfterStreamProperties, int numAudioSamples) {
    return _ringBuffer.writeData(packetAfterStreamProperties.data(), numAudioSamples * sizeof(int16_t));
}
