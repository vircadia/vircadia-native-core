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

class QNetworkReply;

class Sound : public QObject {
    Q_OBJECT
public:
    Sound(const QUrl& sampleURL, QObject* parent = NULL);
    Sound(float volume, float frequency, float duration, float decay, QObject* parent = NULL);
    
    const QByteArray& getByteArray() { return _byteArray; }

private:
    QByteArray _byteArray;

    void downSample(const QByteArray& rawAudioByteArray);
    void interpretAsWav(const QByteArray& inputAudioByteArray, QByteArray& outputAudioByteArray);

private slots:
    void replyFinished(QNetworkReply* reply);
};

#endif // hifi_Sound_h
