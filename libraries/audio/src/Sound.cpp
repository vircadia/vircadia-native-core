//
//  Sound.cpp
//  libraries/audio/src
//
//  Created by Stephen Birarda on 1/2/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
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
