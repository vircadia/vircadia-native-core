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

class FileDownloader : public QObject {
    Q_OBJECT
    
public:
    FileDownloader(const QUrl dataURL, QObject* parent = NULL);
    
    void waitForFile(int timeout = 0);
    
    QByteArray getData() const { return _downloadedData; }
    bool done() { return _done; }
    
    static QByteArray download(const QUrl dataURL, int timeout = 0);
    
signals:
    void done(QNetworkReply::NetworkError);
    
private slots:
    void processReply(QNetworkReply* reply);
    
private:
    QNetworkAccessManager _networkAccessManager;
    QByteArray _downloadedData;
    
    bool _done;
};


#endif /* defined(__hifi__FileDownloader__) */
