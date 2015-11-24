//
//  Downloader.h
//  StackManagerQt/src
//
//  Created by Mohammed Nafees on 07/09/14.
//  Copyright (c) 2014 High Fidelity. All rights reserved.
//

#ifndef hifi_Downloader_h
#define hifi_Downloader_h

#include <QObject>
#include <QUrl>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class Downloader : public QObject
{
    Q_OBJECT
public:
    explicit Downloader(const QUrl& url, QObject* parent = 0);

    const QUrl& getUrl() { return _url; }

    void start(QNetworkAccessManager* manager);

private slots:
    void error(QNetworkReply::NetworkError error);
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void downloadFinished();

signals:
    void downloadStarted(Downloader* downloader, const QUrl& url);
    void downloadCompleted(const QUrl& url);
    void downloadProgress(const QUrl& url, int percentage);
    void downloadFailed(const QUrl& url);
    void installingFiles(const QUrl& url);
    void filesSuccessfullyInstalled(const QUrl& url);
    void filesInstallationFailed(const QUrl& url);

private:
    QUrl _url;
};

#endif
