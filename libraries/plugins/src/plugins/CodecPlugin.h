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
#include <QJsonObject>


#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(codec_plugin)


/**
 * @brief Audio encoder
 *
 * This class is the base interface for all encoders. It provides an interface to encode audio, and exposes encoding
 * settings and data about the encoder.
 *
 * Some settings don't apply to some encoders and will be ignored. Methods are provided to determine whether a setting will apply.
 *
 * Settings may be clamped or fit into certain ranges. For instance an encoder may adjust the wanted bitrate to the nearest one it supports.
 *
 * @warning Both set and get functions must be implemented for each setting. The class doesn't store any data internally and it's up to the
 * derived class to deal with storage.
 *
 */
class Encoder {
public:
    enum class Bandpass {
        Narrowband, Mediumband, Wideband, Superwideband, Fullband
    };

    enum class SignalType {
        Auto, Voice, Music
    };

    enum class ApplicationType {
        VOIP, Audio, LowDelay
    };

    /**
     * @brief Represents the settings loaded from the domain's configuration page
     *
     * Encoder classes are created EntityScriptServer, Agent, AudioMixerClientData and AudioClient,
     * and they need to store configuration data internally for passing it to new Encoder objects
     * when they create them.
     *
     */
    struct CodecSettings {
        QString codec;
        Encoder::Bandpass bandpass = Encoder::Bandpass::Fullband;
        Encoder::ApplicationType applicationType = Encoder::ApplicationType::Audio;
        Encoder::SignalType signalType = Encoder::SignalType::Auto;
        int bitrate = 128000;
        int complexity = 100;
        bool enableFEC = 0;
        int packetLossPercent = 0;
        bool enableVBR = 1;


    };

    /**
     * @brief Parse the codec settings from a domain's config into a vector of CodecSettings.
     *
     * @param root A QJsonObject which contains a codec_settings dictionary with codec settings inside.
     * @return std::vector<CodecSettings>
     */
    static std::vector<CodecSettings> parseSettings(const QJsonObject &root);
    /**
     * @brief Configures a given encoder with the values parsed from the configuration
     *
     * This takes the whole vector as an argument, and automatically tries to find a matching
     * configuration in there.
     *
     * @param settings A vector of CodecSettings. The function will look for a matching configuration within.
     */
    void configure(const std::vector<CodecSettings> &settings);

    virtual ~Encoder() { }
    virtual void encode(const QByteArray& decodedBuffer, QByteArray& encodedBuffer) = 0;

    /**
     * @brief Get the codec's name
     *
     * Returns the name of the codec. Used for display, debugging and configuration.
     * This must match the values found within the configuration.
     *
     * @return QString Codec's name
     */
    virtual const QString getName() const { return "Encoder"; }

    /**
     * @brief Whether this codec is lossless
     *
     * @return true Codec is lossless
     * @return false Codec is lossy
     */
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

    virtual void setApplication(ApplicationType app) {  }
    virtual bool hasApplication() const { return false; }
    virtual ApplicationType getApplication() const { return ApplicationType::Audio; }

    /**
     * @brief Set the codec's complexity (CPU usage)
     *
     * This defines how much CPU time is spent on encoding. Lower values are expected to have
     * lower quality or higher bandwidth usage, depending on the codec.
     *
     * @param complexity A value from 0 to 100.
     */
    virtual void setComplexity(int complexity) {  }
    virtual bool hasComplexity() const { return false; }
    virtual int getComplexity() const { return 0; }

    /**
     * @brief Set the bitrate
     *
     * How many bits per second the codec is going to use. This only applies to lossy codecs.
     *
     * @param bitrate A rate in bits per second from 500 to 512000
     */
    virtual void setBitrate(int bitrate) { }
    virtual bool hasBitrate() const { return false; }
    virtual int getBitrate() const { return 0; }

    /**
     * @brief Sets whether to use Forward Error Correction
     *
     * When enabled, redundant data is included in the codec's output to compensate for packet loss
     * @param enabled
     */
    virtual void setFEC(bool enabled) {  }
    virtual bool hasFEC() const { return false; }
    virtual bool getFEC() const { return false; }

    /**
     * @brief Set how much packet loss to tolerate
     *
     * This is how much loss we expect to have. Higher values may consume bandwidth or lower quality to compensate.
     *
     * @param percent 0 to 100
     */
    virtual void setPacketLossPercent(int percent) { }
    virtual bool hasPacketLossPercent() const { return false; }
    virtual int getPacketLossPercent() const {  return 0; }

    /**
     * @brief Set the bandpass
     *
     * @param bandpass
     */
    virtual void setBandpass(Bandpass bandpass) { }
    virtual bool hasBandpass() const { return false; }
    virtual Bandpass getBandpass() const { return Bandpass::Fullband; }

    /**
     * @brief Set the type of audio signal
     *
     * @param signal
     */
    virtual void setSignalType(SignalType signal) {}
    virtual bool hasSignalType() const { return false; }
    virtual SignalType getSignalType() const { return SignalType::Auto; }

    /**
     * @brief Whether to use variable bitrate
     *
     * @param use_vbr
     */
    virtual void setVBR(bool use_vbr) { }
    virtual bool hasVBR() const { return false; }
    virtual bool getVBR() const { return false; }

    QDebug operator<<(QDebug debug);
};

QDebug operator<<(QDebug &debug, const Encoder::CodecSettings &settings);

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
