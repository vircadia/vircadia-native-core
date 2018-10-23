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

#include <QtWidgets/QMessageBox>

Downloader::Downloader(QUrl fileURL, QObject *parent) : QObject(parent) {
    connect(
        &_networkAccessManager, SIGNAL (finished(QNetworkReply*)),
        this, SLOT (fileDownloaded(QNetworkReply*))
    );

    QNetworkRequest request(fileURL);
    _networkAccessManager.get(request);
}

void Downloader::fileDownloaded(QNetworkReply* reply) {
    QNetworkReply::NetworkError error = reply->error();
    if (error != QNetworkReply::NetworkError::NoError) {
        QMessageBox::information(0, "Test Aborted", "Failed to download file: " + reply->errorString());
        return;
    }

    _downloadedData = reply->readAll();

    //emit a signal
    reply->deleteLater();
    emit downloaded();
}

QByteArray Downloader::downloadedData() const {
    return _downloadedData;
}