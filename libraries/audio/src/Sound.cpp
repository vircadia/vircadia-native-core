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
    _byteArray = reply->readAll();
}