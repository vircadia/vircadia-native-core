//
//  OpusCodecManager.h
//  plugins/opusCodec/src
//
//  Copyright 2020 Dale Glass
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef OPUSENCODER_H
#define OPUSENCODER_H
#include <plugins/CodecPlugin.h>
#include <opus/opus.h>


class AthenaOpusEncoder : public Encoder {
public:

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
    void setVBRConstraint(int vbrConstraint);

    int getMaxBandwidth() const;
    void setMaxBandwidth(int maxBandwidth);

    int getBandwidth() const;
    void setBandwidth(int bandwidth);

    int getSignal() const;
    void setSignal(int signal);

    int getApplication() const;
    void setApplication(int application);

    int getLookahead() const;

    int getInbandFEC() const;
    void setInbandFEC(int inBandFEC);

    int getExpectedPacketLossPercentage() const;
    void setExpectedPacketLossPercentage(int percentage);

    int getDTX() const;
    void setDTX(int dtx);


private:

    const int DEFAULT_BITRATE = 128000;
    const int DEFAULT_COMPLEXITY = 10;
    const int DEFAULT_APPLICATION = OPUS_APPLICATION_VOIP;
    const int DEFAULT_SIGNAL = OPUS_AUTO;

    int _opusSampleRate = 0;
    int _opusChannels = 0;
    int _opusExpectedLoss = 0;


    OpusEncoder* _encoder = nullptr;
};


#endif // OPUSENCODER_H
