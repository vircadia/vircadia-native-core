
#include "MixedAudioStream.h"

MixedAudioStream::MixedAudioStream(int numFrameSamples, int numFramesCapacity, bool dynamicJitterBuffers, int staticDesiredJitterBufferFrames, int maxFramesOverDesired, bool useStDevForJitterCalc)
    : InboundAudioStream(numFrameSamples, numFramesCapacity, dynamicJitterBuffers, staticDesiredJitterBufferFrames, maxFramesOverDesired, useStDevForJitterCalc)
{
}

int MixedAudioStream::parseStreamProperties(PacketType type, const QByteArray& packetAfterSeqNum, int& numAudioSamples) {
    // mixed audio packets do not have any info between the seq num and the audio data.
    numAudioSamples = packetAfterSeqNum.size() / sizeof(int16_t);
    return 0;
}
