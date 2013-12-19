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

int abstractAudioPointerMeta = qRegisterMetaType<AbstractAudioInterface*>("AbstractAudioInterface*");

AudioInjector::AudioInjector(const QUrl& sampleURL) :
    _sourceURL(sampleURL)
{
    // we want to live on our own thread
    moveToThread(&_thread);
    connect(&_thread, SIGNAL(started()), this, SLOT(startDownload()));
    _thread.start();
}

void AudioInjector::startDownload() {
    // assume we have a QApplication or QCoreApplication instance and use the
    // QNetworkAccess manager to grab the raw audio file at the given URL
    
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)),
            this, SLOT(replyFinished(QNetworkReply*)));
    
    manager->get(QNetworkRequest(_sourceURL));
}

void AudioInjector::replyFinished(QNetworkReply* reply) {
    // replace our samples array with the downloaded data
    _sampleByteArray = reply->readAll();
}

void AudioInjector::injectViaThread(AbstractAudioInterface* localAudioInterface) {
    // use Qt::AutoConnection so that this is called on our thread, if appropriate
    QMetaObject::invokeMethod(this, "injectAudio", Qt::AutoConnection, Q_ARG(AbstractAudioInterface*, localAudioInterface));
}

void AudioInjector::injectAudio(AbstractAudioInterface* localAudioInterface) {
    
    // make sure we actually have samples downloaded to inject
    if (_sampleByteArray.size()) {
        // give our sample byte array to the local audio interface, if we have it, so it can be handled locally
        if (localAudioInterface) {
            // assume that localAudioInterface could be on a separate thread, use Qt::AutoConnection to handle properly
            QMetaObject::invokeMethod(localAudioInterface, "handleAudioByteArray",
                                      Qt::AutoConnection,
                                      Q_ARG(QByteArray, _sampleByteArray));
            
        }
    }
}