
#include "MixedAudioStream.h"

MixedAudioStream::MixedAudioStream(int numFrameSamples, int numFramesCapacity, bool dynamicJitterBuffers, int maxFramesOverDesired, bool useStDevForJitterCalc)
    : InboundAudioStream(numFrameSamples, numFramesCapacity, dynamicJitterBuffers, maxFramesOverDesired, useStDevForJitterCalc)
{
}

int MixedAudioStream::parseStreamProperties(PacketType type, const QByteArray& packetAfterSeqNum, int& numAudioSamples) {
    // mixed audio packets do not have any info between the seq num and the audio data.
    numAudioSamples = packetAfterSeqNum.size() / sizeof(int16_t);
    return 0;
}

int MixedAudioStream::parseAudioData(PacketType type, const QByteArray& packetAfterStreamProperties, int numAudioSamples) {
    return _ringBuffer.writeData(packetAfterStreamProperties.data(), numAudioSamples * sizeof(int16_t));
}
