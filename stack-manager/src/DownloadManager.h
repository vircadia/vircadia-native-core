//
//  DownloadManager.h
//  StackManagerQt/src
//
//  Created by Mohammed Nafees on 07/09/14.
//  Copyright (c) 2014 High Fidelity. All rights reserved.
//

#ifndef hifi_DownloadManager_h
#define hifi_DownloadManager_h

#include <QWidget>
#include <QTableWidget>
#include <QHash>
#include <QEvent>
#include <QNetworkAccessManager>

#include "Downloader.h"

class DownloadManager : public QWidget {
    Q_OBJECT
public:
    DownloadManager(QNetworkAccessManager* manager, QWidget* parent = 0);
    ~DownloadManager();

    void downloadFile(const QUrl& url);

private slots:
    void onDownloadStarted(Downloader* downloader, const QUrl& url);
    void onDownloadCompleted(const QUrl& url);
    void onDownloadProgress(const QUrl& url, int percentage);
    void onDownloadFailed(const QUrl& url);
    void onInstallingFiles(const QUrl& url);
    void onFilesSuccessfullyInstalled(const QUrl& url);
    void onFilesInstallationFailed(const QUrl& url);

protected:
    void closeEvent(QCloseEvent*);

signals:
    void fileSuccessfullyInstalled(const QUrl& url);

private:
    QTableWidget* _table;
    QNetworkAccessManager* _manager;
    QHash<Downloader*, int> _downloaderHash;

    int downloaderRowIndexForUrl(const QUrl& url);
    Downloader* downloaderForUrl(const QUrl& url);
};

#endif
