//
//  HiFiCodec.h
//  plugins/hifiCodec/src
//
//  Created by Brad Hefta-Gaub on 7/10/2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_HiFiCodec_h
#define hifi_HiFiCodec_h

#include <plugins/CodecPlugin.h>

class HiFiCodec : public CodecPlugin {
    Q_OBJECT

public:
    // Plugin functions
    bool isSupported() const override;
    const QString getName() const override { return NAME; }

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
    static const char* NAME;
};

#endif // hifi_HiFiCodec_h
