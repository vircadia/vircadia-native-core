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

class FileDownloader : public QObject {
public:
    FileDownloader(const QUrl dataURL, QObject* parent = NULL);
    QByteArray getData() const { return _downloadedData; }
    
    static QByteArray download(const QUrl dataURL);
    
signals:
    void done(QNetworkReply::NetworkError);
    
private slots:
    void processReply(QNetworkReply* reply);
    
private:
    QNetworkAccessManager _networkAccessManager;
    QByteArray _downloadedData;
};


#endif /* defined(__hifi__FileDownloader__) */
