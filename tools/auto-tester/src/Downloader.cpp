//
//  Downloader.cpp
//
//  Created by Nissim Hadar on 1 Mar 2018.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "Downloader.h"

Downloader::Downloader(QUrl imageUrl, QObject *parent) : QObject(parent) {
    connect(
        &_networkAccessManager, SIGNAL (finished(QNetworkReply*)),
        this, SLOT (fileDownloaded(QNetworkReply*))
    );

    QNetworkRequest request(imageUrl);
    _networkAccessManager.get(request);
}

void Downloader::fileDownloaded(QNetworkReply* reply) {
    _downloadedData = reply->readAll();

    //emit a signal
    reply->deleteLater();
    emit downloaded();
}

QByteArray Downloader::downloadedData() const {
    return _downloadedData;
}