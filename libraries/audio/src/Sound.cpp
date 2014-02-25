//
//  Sound.cpp
//  hifi
//
//  Created by Stephen Birarda on 1/2/2014.
//  Modified by Athanasios Gaitatzes to add WAVE file support.
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//

#include <stdint.h>

#include <QDataStream>
#include <QtCore/QDebug>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <qendian.h>

#include "Sound.h"

Sound::Sound(const QUrl& sampleURL, QObject* parent) :
    QObject(parent)
{
    // assume we have a QApplication or QCoreApplication instance and use the
    // QNetworkAccess manager to grab the raw audio file at the given URL

    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)),
            this, SLOT(replyFinished(QNetworkReply*)));

    qDebug() << "Requesting audio file" << sampleURL.toDisplayString();
    manager->get(QNetworkRequest(sampleURL));
}

void Sound::replyFinished(QNetworkReply* reply) {

    // replace our byte array with the downloaded data
    QByteArray rawAudioByteArray = reply->readAll();

    // foreach(QByteArray b, reply->rawHeaderList())
    //     qDebug() << b.constData() << ": " << reply->rawHeader(b).constData();

    if (reply->hasRawHeader("Content-Type")) {

        QByteArray headerContentType = reply->rawHeader("Content-Type");

        // WAV audio file encountered
        if (headerContentType == "audio/x-wav"
            || headerContentType == "audio/wav"
            || headerContentType == "audio/wave") {

            QByteArray outputAudioByteArray;

            interpretAsWav(rawAudioByteArray, outputAudioByteArray);
            downSample(outputAudioByteArray);
        } else {
            //  Process as RAW file
            downSample(rawAudioByteArray);
        }
    } else {
        qDebug() << "Network reply without 'Content-Type'.";
    }
}

void Sound::downSample(const QByteArray& rawAudioByteArray) {

    // assume that this was a RAW file and is now an array of samples that are
    // signed, 16-bit, 48Khz, mono

    // we want to convert it to the format that the audio-mixer wants
    // which is signed, 16-bit, 24Khz, mono

    _byteArray.resize(rawAudioByteArray.size() / 2);

    int numSourceSamples = rawAudioByteArray.size() / sizeof(int16_t);
    int16_t* sourceSamples = (int16_t*) rawAudioByteArray.data();
    int16_t* destinationSamples = (int16_t*) _byteArray.data();

    for (int i = 1; i < numSourceSamples; i += 2) {
        if (i + 1 >= numSourceSamples) {
            destinationSamples[(i - 1) / 2] = (sourceSamples[i - 1] / 2) + (sourceSamples[i] / 2);
        } else {
            destinationSamples[(i - 1) / 2] = (sourceSamples[i - 1] / 4) + (sourceSamples[i] / 2) + (sourceSamples[i + 1] / 4);
        }
    }
}

//
// Format description from https://ccrma.stanford.edu/courses/422/projects/WaveFormat/
//
// The header for a WAV file looks like this:
// Positions     Sample Value     Description
//   00-03         "RIFF"       Marks the file as a riff file. Characters are each 1 byte long.
//   04-07         File size (int) Size of the overall file - 8 bytes, in bytes (32-bit integer).
//   08-11         "WAVE"       File Type Header. For our purposes, it always equals "WAVE".
//   12-15         "fmt "       Format chunk marker.
//   16-19         16           Length of format data as listed above
//   20-21         1            Type of format: (1=PCM, 257=Mu-Law, 258=A-Law, 259=ADPCM) - 2 byte integer
//   22-23         2            Number of Channels - 2 byte integer
//   24-27         44100        Sample Rate - 32 byte integer. Sample Rate = Number of Samples per second, or Hertz.
//   28-31         176400       (Sample Rate * BitsPerSample * Channels) / 8.
//   32-33         4            (BitsPerSample * Channels) / 8 - 8 bit mono2 - 8 bit stereo/16 bit mono4 - 16 bit stereo
//   34-35         16           Bits per sample
//   36-39         "data"       Chunk header. Marks the beginning of the data section.
//   40-43         File size (int) Size of the data section.
//   44-??                      Actual sound data
// Sample values are given above for a 16-bit stereo source.
//

struct chunk
{
    char        id[4];
    quint32     size;
};

struct RIFFHeader
{
    chunk       descriptor;     // "RIFF"
    char        type[4];        // "WAVE"
};

struct WAVEHeader
{
    chunk       descriptor;
    quint16     audioFormat;    // Format type: 1=PCM, 257=Mu-Law, 258=A-Law, 259=ADPCM
    quint16     numChannels;    // Number of channels: 1=mono, 2=stereo
    quint32     sampleRate;
    quint32     byteRate;       // Sample rate * Number of Channels * Bits per sample / 8
    quint16     blockAlign;     // (Number of Channels * Bits per sample) / 8.1
    quint16     bitsPerSample;
};

struct DATAHeader
{
    chunk       descriptor;
};

struct CombinedHeader
{
    RIFFHeader  riff;
    WAVEHeader  wave;
};

void Sound::interpretAsWav(const QByteArray& inputAudioByteArray, QByteArray& outputAudioByteArray) {

    CombinedHeader fileHeader;

    // Create a data stream to analyze the data
    QDataStream waveStream(const_cast<QByteArray *>(&inputAudioByteArray), QIODevice::ReadOnly);
    if (waveStream.readRawData(reinterpret_cast<char *>(&fileHeader), sizeof(CombinedHeader)) == sizeof(CombinedHeader)) {

        if (strncmp(fileHeader.riff.descriptor.id, "RIFF", 4) == 0) {
            waveStream.setByteOrder(QDataStream::LittleEndian);
        } else {
            // descriptor.id == "RIFX" also signifies BigEndian file
            // waveStream.setByteOrder(QDataStream::BigEndian);
            qDebug() << "Currently not supporting big-endian audio files.";
            return;
        }

        if (strncmp(fileHeader.riff.type, "WAVE", 4) != 0
            || strncmp(fileHeader.wave.descriptor.id, "fmt", 3) != 0) {
            qDebug() << "Not a WAVE Audio file.";
            return;
        }

        // added the endianess check as an extra level of security

        if (qFromLittleEndian<quint16>(fileHeader.wave.audioFormat) != 1) {
            qDebug() << "Currently not supporting non PCM audio files.";
            return;
        }
        if (qFromLittleEndian<quint16>(fileHeader.wave.numChannels) != 1) {
            qDebug() << "Currently not supporting stereo audio files.";
            return;
        }
        if (qFromLittleEndian<quint16>(fileHeader.wave.bitsPerSample) != 16) {
            qDebug() << "Currently not supporting non 16bit audio files.";
            return;
        }
        if (qFromLittleEndian<quint32>(fileHeader.wave.sampleRate) != 48000) {
            qDebug() << "Currently not supporting non 48KHz audio files.";
            return;
        }

        // Read off remaining header information
        DATAHeader dataHeader;
        if (waveStream.readRawData(reinterpret_cast<char *>(&dataHeader), sizeof(DATAHeader)) == sizeof(DATAHeader)) {
            if (strncmp(dataHeader.descriptor.id, "data", 4) != 0) {
                qDebug() << "Invalid wav audio data header.";
                return;
            }
        } else {
            qDebug() << "Could not read wav audio data header.";
            return;
        }

        if (qFromLittleEndian<quint32>(fileHeader.riff.descriptor.size) != qFromLittleEndian<quint32>(dataHeader.descriptor.size) + 36) {
            qDebug() << "Did not read audio file chank headers correctly.";
            return;
        }

        // Now pull out the data
        quint32 outputAudioByteArraySize = qFromLittleEndian<quint32>(dataHeader.descriptor.size);
        outputAudioByteArray.resize(outputAudioByteArraySize);
        waveStream.readRawData(outputAudioByteArray.data(), outputAudioByteArraySize);

    } else {
        qDebug() << "Could not read wav audio file header.";
        return;
    }
}
