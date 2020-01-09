#ifndef OPUSENCODER_H
#define OPUSENCODER_H
#include <plugins/CodecPlugin.h>
#include "OpusWrapper.h"
#include "opus/opus.h"


class AthenaOpusEncoder : public Encoder {
public:
    const int DEFAULT_BITRATE     = 128000;
    const int DEFAULT_COMPLEXITY  = 10;
    const int DEFAULT_APPLICATION = OPUS_APPLICATION_VOIP;
    const int DEFAULT_SIGNAL      = OPUS_AUTO;


    AthenaOpusEncoder(int sampleRate, int numChannels);
    ~AthenaOpusEncoder() override;

    virtual void encode(const QByteArray& decodedBuffer, QByteArray& encodedBuffer) override;


    int getComplexity() const;
    void setComplexity(int complexity);

    int getBitrate() const;
    void setBitrate(int bitrate);

    int getVBR() const;
    void setVBR(int vbr);

    int getVBRConstraint() const;
    void setVBRConstraint(int vbr_const);

    int getMaxBandwidth() const;
    void setMaxBandwidth(int maxbw);

    int getBandwidth() const;
    void setBandwidth(int bw);

    int getSignal() const;
    void setSignal(int signal);

    int getApplication() const;
    void setApplication(int app);

    int getLookahead() const;

    int getInbandFEC() const;
    void setInbandFEC(int fec);

    int getExpectedPacketLossPercent() const;
    void setExpectedPacketLossPercent(int perc);

    int getDTX() const;
    void setDTX(int dtx);


private:

    int _opus_sample_rate = 0;
    int _opus_channels = 0;
    int _opus_expected_loss = 0;


    OpusEncoder *_encoder = nullptr;
};


#endif // OPUSENCODER_H
