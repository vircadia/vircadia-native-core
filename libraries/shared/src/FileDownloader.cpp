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
#include <QNetworkReply>
#include <QEventLoop>

#include "FileDownloader.h"

FileDownloader::FileDownloader(const QUrl dataURL, QObject* parent) : QObject(parent) {
    connect(&_networkAccessManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(processReply(QNetworkReply*)));

    QNetworkRequest request(dataURL);
    _networkAccessManager.get(request);
}

void FileDownloader::processReply(QNetworkReply *reply) {
    if (reply->error() != QNetworkReply::NoError) {
        emit done(reply->error());
        return;
    }
    
    _downloadedData = reply->readAll();
    reply->deleteLater();
    
    emit done(QNetworkReply::NoError);
}

QByteArray FileDownloader::download(const QUrl dataURL) {
    QEventLoop loop;
    FileDownloader downloader(dataURL);
    connect(&downloader, SIGNAL(done()), &loop, SLOT(quit()));
    loop.exec();
    return downloader.getData();
}