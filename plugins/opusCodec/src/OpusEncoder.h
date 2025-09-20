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

    const QString getName() const override { return "opus"; }

    int getComplexity() const override;
    void setComplexity(int complexity) override;

    int getBitrate() const override;
    void setBitrate(int bitrate) override;

    bool getVBR() const override;
    void setVBR(bool vbr) override;

    int getVBRConstraint() const;
    void setVBRConstraint(int vbrConstraint);

    int getMaxBandwidth() const;
    void setMaxBandwidth(int maxBandwidth);

    int getBandwidth() const;
    void setBandwidth(int bandwidth);

    Encoder::SignalType getSignalType() const override;
    void setSignalType(Encoder::SignalType signal) override;

    ApplicationType getApplication() const override;
    void setApplication(ApplicationType application) override;

    int getLookahead() const;

    bool getFEC() const override;
    void setFEC(bool inBandFEC) override;

    int getPacketLossPercent() const override;
    void setPacketLossPercent(int percentage) override;

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
