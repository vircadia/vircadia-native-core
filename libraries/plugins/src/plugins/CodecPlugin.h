//
//  CodecPlugin.h
//  plugins/src/plugins
//
//  Created by Brad Hefta-Gaub on 6/9/2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include <algorithm>
#include "Plugin.h"
#include <QDebug>

#ifndef VIRCADIA_CODECPLUGIN_H
#define VIRCADIA_CODECPLUGIN_H

class Encoder {
public:
    enum class EncoderBandpass {
        Narrowband, Mediumband, Wideband, Superwideband, Fullband
    };

    enum class EncoderSignal {
        Auto, Voice, Music
    };

    enum class EncoderApplication {
        VOIP, Audio, LowDelay
    };

/*
    enum class Capabilities : int {
        None = 0x00,
        Lossless = 0x01,
        Complexity = 0x02,
        Bitrate = 0x04,
        FEC = 0x08,
        Bandpass = 0x10,
        SignalType = 0x20,
        VBR = 0x40
    };
*/


    virtual ~Encoder() { }
    virtual void encode(const QByteArray& decodedBuffer, QByteArray& encodedBuffer) = 0;

    /**
     * @brief Get the codec's name
     *
     * Returns the name of the codec, for usage in debug output and UI.
     *
     * @return QString Codec's name
     */
    virtual const QString getName() const { return "Encoder"; }

    virtual bool isLossless() const { return false; }

    /**
     * @brief Get the codec's capabilities
     *
     * This is a bit field of which of the possible settings have effect on this codec.
     *
     * For example, a codec capable of adding redundancy to compensate for packet loss will have the FEC flag set.
     *
     * @return Capabilities
     */
    //virtual Capabilities getCapabilities() { return Capabilities::None; }

    virtual void setApplication(EncoderApplication app) { _application = app; }
    virtual bool hasApplication() const { return false; }
    EncoderApplication getApplication() const { return _application; }

    /**
     * @brief Set the codec's complexity (CPU usage)
     *
     * This defines how much CPU time is spent on encoding. Lower values are expected to have
     * lower quality or higher bandwidth usage, depending on the codec.
     *
     * @param complexity A value from 0 to 100.
     */
    virtual void setComplexity(int complexity) { _complexity = qBound(complexity, 0, 100); }
    virtual bool hasComplexity() const { return false; }
    int getComplexity() const { return _complexity; }

    /**
     * @brief Set the bitrate
     *
     * How many bits per second the codec is going to use. This only applies to lossy codecs.
     *
     * @param bitrate A rate in bits per second from 500 to 512000
     */
    virtual void setBitrate(int bitrate) { _bitrate = qBound(bitrate, 500, 512000); }
    virtual bool hasBitrate() const { return false; }
    int getBitrate() const { return _bitrate; }

    /**
     * @brief Sets whether to use Forward Error Correction
     *
     * When enabled, redundant data is included in the codec's output to compensate for packet loss
     * @param enabled
     */
    virtual void setFEC(bool enabled) { _useFEC = enabled; }
    virtual bool hasFEC() const { return false; }
    bool getFEC() const { return _useFEC; }

    /**
     * @brief Set how much packet loss to tolerate
     *
     * This is how much loss we expect to have. Higher values may consume bandwidth or lower quality to compensate.
     *
     * @param percent 0 to 100
     */
    virtual void setPacketLossPercent(int percent) { _packetLossPercent = qBound(percent, 0, 100); }
    virtual bool hasPacketLossPercent() const { return false; }
    int getPacketLossPercent() const { return _packetLossPercent; }

    /**
     * @brief Set the bandpass
     *
     * @param bandpass
     */
    virtual void setBandpass(EncoderBandpass bandpass) { _bandpass = bandpass;}
    virtual bool hasBandpass() const { return false; }
    EncoderBandpass getBandpass() const { return _bandpass; }

    /**
     * @brief Set the type of audio signal
     *
     * @param signal
     */
    virtual void setSignalType(EncoderSignal signal) { _signal = signal;}
    virtual bool hasSignalType() const { return false; }
    EncoderSignal getSignalType() const { return _signal; }

    /**
     * @brief Whether to use variable bitrate
     *
     * @param use_vbr
     */
    virtual void setVBR(bool use_vbr) { _useVBR = use_vbr; }
    virtual bool hasVBR() const { return false; }
    bool getVBR() const { return _useVBR; }

    QDebug operator<<(QDebug debug) {
        debug << "{ Codec = " << getName();
        if ( hasComplexity() ) {
            debug << "; Complexity = " << getComplexity();
        }

        if ( hasBitrate() ) {
            debug << "; Bitrate = " << getBitrate();
        }

        if ( hasFEC() ) {
            debug << "; FEC = " << getFEC();
        }

        if ( hasPacketLossPercent() ) {
            debug << "; PacketLoss = " << getPacketLossPercent();
        }

        if ( hasBandpass() ) {
            debug << "; Bandpass = " << (int)getBandpass();
        }

        if ( hasSignalType() ) {
            debug << "; Signal = " << (int)getSignalType();
        }

        if ( hasVBR() ) {
            debug << "; VBR = " << getVBR();
        }

        debug << "}";

        return debug;
    }

/*
    Encoder::Capabilities operator|(Encoder::Capabilities lhs, Encoder::Capabilities rhs) {
    return static_cast<Encoder::Capabilities>(
        static_cast<std::underlying_type<Encoder::Capabilities>::type>(lhs) |
        static_cast<std::underlying_type<Encoder::Capabilities>::type>(rhs)
    );

}
   */
protected:
    int _complexity;
    int _bitrate;
    bool _useFEC;
    bool _useVBR;
    int _packetLossPercent;
    EncoderBandpass _bandpass;
    EncoderSignal _signal;
    EncoderApplication _application;
};



class Decoder {
public:
    virtual ~Decoder() { }
    virtual void decode(const QByteArray& encodedBuffer, QByteArray& decodedBuffer) = 0;

    virtual void lostFrame(QByteArray& decodedBuffer) = 0;
};




class CodecPlugin : public Plugin {
public:
    virtual Encoder* createEncoder(int sampleRate, int numChannels) = 0;
    virtual Decoder* createDecoder(int sampleRate, int numChannels) = 0;
    virtual void releaseEncoder(Encoder* encoder) = 0;
    virtual void releaseDecoder(Decoder* decoder) = 0;
};

#endif
