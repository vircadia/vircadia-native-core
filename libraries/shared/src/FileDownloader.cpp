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

FileDownloader::FileDownloader(const QUrl dataURL, QObject* parent) :
    QObject(parent),
    _done(false)
{
    connect(&_networkAccessManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(processReply(QNetworkReply*)));

    QNetworkRequest request(dataURL);
    _networkAccessManager.get(request);
}

void FileDownloader::processReply(QNetworkReply *reply) {
    if (reply->error() == QNetworkReply::NoError) {
        _downloadedData = reply->readAll();
    }
    
    reply->deleteLater();
    _done = true;
    emit done(reply->error());
}

void FileDownloader::waitForFile(int timeout) {
    QTimer timer;
    QEventLoop loop;
    connect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));
    connect(this, SIGNAL(done(QNetworkReply::NetworkError)), &loop, SLOT(quit()));
    
    if (!_done) {
        if (timeout > 0) {
            timer.start(timeout);
        }
        loop.exec();
    }
}

QByteArray FileDownloader::download(const QUrl dataURL, int timeout) {
    QTimer timer;
    QEventLoop loop;
    connect(&timer, SIGNAL(timeout()), &loop, SLOT(quit));
    
    FileDownloader downloader(dataURL);
    connect(&downloader, SIGNAL(done(QNetworkReply::NetworkError)), &loop, SLOT(quit()));
    
    if (timeout > 0) {
        timer.start(timeout);
    }
    loop.exec();
    
    return downloader.getData();
}