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

#include "CodecPlugin.h"
#include <QJsonArray>

Q_LOGGING_CATEGORY(codec_plugin, "vircadia.codec_plugin")

std::vector<Encoder::CodecSettings> Encoder::parseSettings(const QJsonObject &root) {
    const QString CODEC_SETTINGS = "codec_settings";

    std::vector<Encoder::CodecSettings> list;

    if (root[CODEC_SETTINGS].isArray()) {
        qCWarning(codec_plugin) << "Parsing codec settings";

        const QJsonArray &codecSettings = root[CODEC_SETTINGS].toArray();

        const QString CODEC = "codec";
        const QString BITRATE = "bitrate";
        const QString COMPLEXITY = "complexity";
        const QString FEC = "fec";
        const QString PACKET_LOSS = "packet_loss";
        const QString VBR = "vbr";

        for (int i=0;i<codecSettings.count(); ++i) {
            QJsonObject codecObject = codecSettings[i].toObject();

            qCWarning(codec_plugin) << "Parsing codec" << i;

            if ( codecObject.contains(CODEC)) {
                Encoder::CodecSettings settings;

                settings.codec = codecObject.value(CODEC).toString();

                if ( codecObject.contains(BITRATE)) {
                    settings.bitrate = codecObject.value(BITRATE).toString().toInt();
                }

                if ( codecObject.contains(COMPLEXITY)) {
                    settings.complexity = codecObject.value(COMPLEXITY).toString().toInt();
                }

                if ( codecObject.contains(FEC) && codecObject.contains(PACKET_LOSS)) {
                    settings.enableFEC = codecObject.value(FEC).toBool();
                    settings.packetLossPercent = codecObject.value(PACKET_LOSS).toString().toInt();
                }

                if ( codecObject.contains(VBR)) {
                    settings.enableVBR = codecObject.value(VBR).toBool();
                }

                qCCritical(codec_plugin) << "Codec settings parsed:" << settings;
                list.push_back( settings );
            } else {
                qCCritical(codec_plugin) << "Failed to find" << CODEC << "element";
            }
        }
    } else {
        qCWarning(codec_plugin) << "Failed to find a" << CODEC_SETTINGS << "array inside" << root;
    }

    return list;
}

void Encoder::configure(const std::vector<Encoder::CodecSettings> &settings) {
    const QString myName = getName().trimmed().toLower();

    for( const auto &s : settings ) {
        if ( s.codec.trimmed().toLower() == myName ) {

            qCDebug(codec_plugin) << "Configuring encoder" << getName();

            if (hasApplication()) {
                setApplication(s.applicationType);
            }

            if (hasBandpass()) {
                setBandpass(s.bandpass);
            }

            if (hasSignalType()) {
                setSignalType(s.signalType);
            }

            if (hasBitrate()) {
                setBitrate(s.bitrate);
            }

            if (hasComplexity()) {
                setComplexity(s.complexity);
            }

            if (hasFEC()) {
                setFEC(s.enableFEC);
            }

            if (hasPacketLossPercent()) {
                setPacketLossPercent(s.packetLossPercent);
            }

            if (hasVBR()) {
                setVBR(s.enableVBR);
            }
        }
    }
}

QDebug operator<<(QDebug &debug, const Encoder::CodecSettings &settings) {
    debug << "{ Codec =" << settings.codec;
    debug << "; Complexity =" << settings.complexity;
    debug << "; Bitrate =" << settings.bitrate;
    debug << "; FEC =" << settings.enableFEC;
    debug << "; PacketLoss =" << settings.packetLossPercent;
    debug << "; Application =" << (int)settings.applicationType;
    debug << "; Bandpass =" << (int)settings.bandpass;
    debug << "; Signal =" << (int)settings.signalType;
    debug << "; VBR =" << settings.enableVBR;
    debug << "}";
    return debug;
}


QDebug Encoder::operator<<(QDebug debug) {
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

