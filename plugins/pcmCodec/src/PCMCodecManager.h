//
//  PCMCodecManager.h
//  plugins/pcmCodec/src
//
//  Created by Brad Hefta-Gaub on 6/9/2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi__PCMCodecManager_h
#define hifi__PCMCodecManager_h

#include <plugins/CodecPlugin.h>

/*
class Encoder {
public:
virtual void encode(const QByteArray& decodedBuffer, QByteArray& encodedBuffer) = 0;
};

class Decoder {
public:
virtual void decode(const QByteArray& encodedBuffer, QByteArray& decodedBuffer) = 0;

// numFrames - number of samples (mono) or sample-pairs (stereo)
virtual void trackLostFrames(int numFrames) = 0;
};

class CodecPlugin : public Plugin {
public:
virtual Encoder* createEncoder(int sampleRate, int numChannels) = 0;
virtual Decoder* createDecoder(int sampleRate, int numChannels) = 0;
virtual void releaseEncoder(Encoder* encoder) = 0;
virtual void releaseDecoder(Decoder* decoder) = 0;
};
*/

class PCMCodec : public CodecPlugin {
    Q_OBJECT
    
public:
    // Plugin functions
    bool isSupported() const override;
    const QString& getName() const override { return NAME; }

    void init() override;
    void deinit() override;

    /// Called when a plugin is being activated for use.  May be called multiple times.
    bool activate() override;
    /// Called when a plugin is no longer being used.  May be called multiple times.
    void deactivate() override;

    virtual Encoder* createEncoder(int sampleRate, int numChannels) override;
    virtual Decoder* createDecoder(int sampleRate, int numChannels) override;
    virtual void releaseEncoder(Encoder* encoder) override;
    virtual void releaseDecoder(Decoder* decoder) override;

private:
    static const QString NAME;
};

class zLibCodec : public CodecPlugin {
    Q_OBJECT

public:
    // Plugin functions
    bool isSupported() const override;
    const QString& getName() const override { return NAME; }

    void init() override;
    void deinit() override;

    /// Called when a plugin is being activated for use.  May be called multiple times.
    bool activate() override;
    /// Called when a plugin is no longer being used.  May be called multiple times.
    void deactivate() override;

    virtual Encoder* createEncoder(int sampleRate, int numChannels) override;
    virtual Decoder* createDecoder(int sampleRate, int numChannels) override;
    virtual void releaseEncoder(Encoder* encoder) override;
    virtual void releaseDecoder(Decoder* decoder) override;


private:
    static const QString NAME;
};

#endif // hifi__PCMCodecManager_h
