//
//  FileDownloader.cpp
//  hifi
//
//  Created by Clement Brisset on 3/14/14.
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//
//

#include <QUrl>
#include <QNetworkRequest>
#include <QEventLoop>
#include <QTimer>

#include "FileDownloader.h"

FileDownloader::FileDownloader(QObject* parent) : QObject(parent) {
    connect(&_networkAccessManager, SIGNAL(finished(QNetworkReply*)), SLOT(processReply(QNetworkReply*)));
}

void FileDownloader::download(const QUrl& dataURL, QNetworkAccessManager::Operation operation) {
    QNetworkRequest request(dataURL);
    
    _downloadedData.clear();
    switch (operation) {
        case QNetworkAccessManager::GetOperation:
            _networkAccessManager.get(request);
            break;
        case QNetworkAccessManager::HeadOperation:
            _networkAccessManager.head(request);
            break;
        default:
            emit done(QNetworkReply::ProtocolInvalidOperationError);
            break;
    }
}

void FileDownloader::processReply(QNetworkReply *reply) {
    if (reply->error() == QNetworkReply::NoError) {
        _downloadedData = reply->readAll();
    }
    
    reply->deleteLater();
    emit done(reply->error());
}