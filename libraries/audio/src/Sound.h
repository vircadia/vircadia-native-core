//
//  Sound.h
//  libraries/audio/src
//
//  Created by Stephen Birarda on 1/2/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Sound_h
#define hifi_Sound_h

#include <QtCore/QObject>
#include <QtNetwork/QNetworkReply>

class Sound : public QObject {
    Q_OBJECT
    
    Q_PROPERTY(bool downloaded READ hasDownloaded)
public:
    Sound(const QUrl& sampleURL, QObject* parent = NULL);
    Sound(float volume, float frequency, float duration, float decay, QObject* parent = NULL);
    Sound(const QByteArray byteArray, QObject* parent = NULL);
    void append(const QByteArray byteArray);
    
    bool hasDownloaded() const { return _hasDownloaded; }
    
    const QByteArray& getByteArray() { return _byteArray; }

private:
    QByteArray _byteArray;
    bool _hasDownloaded;
    
    void trimFrames();
    void downSample(const QByteArray& rawAudioByteArray);
    void interpretAsWav(const QByteArray& inputAudioByteArray, QByteArray& outputAudioByteArray);

private slots:
    void replyFinished();
    void replyError(QNetworkReply::NetworkError code);
};

#endif // hifi_Sound_h
