//
//  AudioInjector.cpp
//  hifi
//
//  Created by Stephen Birarda on 12/19/2013.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>

#include "AbstractAudioInterface.h"

#include "AudioInjector.h"

AudioInjector::AudioInjector(const QUrl& sampleURL, QObject* parent) :
	QObject(parent)
{
    // assume we have a QApplication or QCoreApplication instance and use the
    // QNetworkAccess manager to grab the raw audio file at the given URL
    
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)),
            this, SLOT(replyFinished(QNetworkReply*)));
    
    manager->get(QNetworkRequest(sampleURL));
}

void AudioInjector::replyFinished(QNetworkReply* reply) {
    // replace our samples array with the downloaded data
    _sampleByteArray = reply->readAll();
}

void AudioInjector::injectViaThread(AbstractAudioInterface* localAudioInterface) {
    // make sure we actually have samples downloaded to inject
    if (_sampleByteArray.size()) {
        // give our sample byte array to the local audio interface, if we have it, so it can be handled locally
        if (localAudioInterface) {
            // assume that localAudioInterface could be on a separate thread
            QMetaObject::invokeMethod(localAudioInterface, "handleAudioByteArray",
                                      Qt::QueuedConnection,
                                      Q_ARG(QByteArray, _sampleByteArray));

        }
        
         // setup a new thread we can use for the injection
    }
}