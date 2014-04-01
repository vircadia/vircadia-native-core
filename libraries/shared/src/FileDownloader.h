//
//  FileDownloader.h
//  hifi
//
//  Created by Clement Brisset on 3/14/14.
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//
//

#ifndef __hifi__FileDownloader__
#define __hifi__FileDownloader__

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QThread>

class FileDownloader : public QObject {
    Q_OBJECT
    
public:
    FileDownloader(QObject* parent = NULL);
    QByteArray getData() const { return _downloadedData; }
    
    
signals:
    void done(QNetworkReply::NetworkError error);
    
public slots:
    void download(const QUrl dataURL, QNetworkAccessManager::Operation operation = QNetworkAccessManager::GetOperation);
    
private slots:
    void processReply(QNetworkReply* reply);
    
private:
    QNetworkAccessManager _networkAccessManager;
    QByteArray _downloadedData;
};


#endif /* defined(__hifi__FileDownloader__) */
