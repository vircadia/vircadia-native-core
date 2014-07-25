
#include "InboundAudioStream.h"
#include "PacketHeaders.h"

class InboundMixedAudioStream : public InboundAudioStream {
public:
    InboundMixedAudioStream(int numFrameSamples, int numFramesCapacity, bool dynamicJitterBuffers);
protected:
    int parseStreamProperties(PacketType type, const QByteArray& packetAfterSeqNum, int& numAudioSamples);
    int parseAudioData(PacketType type, const QByteArray& packetAfterStreamProperties, int numAudioSamples);
};
