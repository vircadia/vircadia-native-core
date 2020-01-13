#ifndef OPUSDECODER_H
#define OPUSDECODER_H


#include <plugins/CodecPlugin.h>
#include "opus/opus.h"


class AthenaOpusDecoder : public Decoder {
public:
    AthenaOpusDecoder(int sampleRate, int numChannels);
    ~AthenaOpusDecoder() override;


    virtual void decode(const QByteArray& encodedBuffer, QByteArray& decodedBuffer) override;
    virtual void lostFrame( QByteArray &decodedBuffer) override;


private:
    int _encodedSize;

    OpusDecoder *_decoder = nullptr;
    int _opus_sample_rate = 0;
    int _opus_num_channels = 0;
    int _decoded_size = 0;
};


#endif // OPUSDECODER_H
