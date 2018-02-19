//
//  AudioWavFile.h
//  libraries/audio-client/src
//
//  Created by Luis Cuenca on 12/1/2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AudioFileWav_h
#define hifi_AudioFileWav_h

#include <QObject>
#include <QFile>
#include <QDataStream>
#include <QVector>
#include <QAudioFormat>

class AudioFileWav : public QObject {
    Q_OBJECT
public:
    AudioFileWav() {}
    bool create(const QAudioFormat& audioFormat, const QString& filepath);
    bool addRawAudioChunk(char* chunk, int size);
    void close();

private:
    void addHeader(const QAudioFormat& audioFormat);
    QFile _file;
};

#endif // hifi_AudioFileWav_h