//
//  Sound.cpp
//  hifi
//
//  Created by Stephen Birarda on 1/2/2014.
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//

#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>

#include "Sound.h"

Sound::Sound(const QUrl& sampleURL, QObject* parent) :
    QObject(parent)
{
    // assume we have a QApplication or QCoreApplication instance and use the
    // QNetworkAccess manager to grab the raw audio file at the given URL
    
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)),
            this, SLOT(replyFinished(QNetworkReply*)));
    
    manager->get(QNetworkRequest(sampleURL));
}

void Sound::replyFinished(QNetworkReply* reply) {
    // replace our byte array with the downloaded data
    QByteArray rawAudioByteArray = reply->readAll();
    
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