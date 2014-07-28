
#include "InboundAudioStream.h"
#include "PacketHeaders.h"

class InboundMixedAudioStream : public InboundAudioStream {
public:
    InboundMixedAudioStream(int numFrameSamples, int numFramesCapacity, bool dynamicJitterBuffers, bool useStDevForJitterCalc = false);

    float getNextOutputFrameLoudness() const { return _ringBuffer.getNextOutputFrameLoudness(); }

protected:
    int parseStreamProperties(PacketType type, const QByteArray& packetAfterSeqNum, int& numAudioSamples);
    int parseAudioData(PacketType type, const QByteArray& packetAfterStreamProperties, int numAudioSamples);
};
