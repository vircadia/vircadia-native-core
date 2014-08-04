
#include "RawMixedAudioStream.h"

RawMixedAudioStream ::RawMixedAudioStream (int numFrameSamples, int numFramesCapacity, bool dynamicJitterBuffers, int staticDesiredJitterBufferFrames, int maxFramesOverDesired, bool useStDevForJitterCalc)
    : InboundAudioStream(numFrameSamples, numFramesCapacity, dynamicJitterBuffers, staticDesiredJitterBufferFrames, maxFramesOverDesired, useStDevForJitterCalc)
{
}

int RawMixedAudioStream ::parseStreamProperties(PacketType type, const QByteArray& packetAfterSeqNum, int& numAudioSamples) {
    // mixed audio packets do not have any info between the seq num and the audio data.
    numAudioSamples = packetAfterSeqNum.size() / sizeof(int16_t);
    return 0;
}

int RawMixedAudioStream ::parseAudioData(PacketType type, const QByteArray& packetAfterStreamProperties, int numAudioSamples) {

    QByteArray outputBuffer;
    emit processSamples(packetAfterStreamProperties, outputBuffer);

    int bytesWritten = _ringBuffer.writeData(outputBuffer.data(), outputBuffer.size());
    printf("wrote %d samples to ringbuffer\n", bytesWritten / 2);

    return packetAfterStreamProperties.size();
}
