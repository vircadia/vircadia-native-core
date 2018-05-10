#include "FileDownloader.h"

#include <QtCore/QCoreApplication>
#include <QtNetwork/QNetworkReply>

FileDownloader::FileDownloader(QUrl url, const Handler& handler, QObject* parent) : QObject(parent), _handler(handler) {
    connect(&_accessManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(fileDownloaded(QNetworkReply*)));
    _accessManager.get(QNetworkRequest(url));
}

void FileDownloader::waitForDownload() {
    while (!_complete) {
        QCoreApplication::processEvents();
    }
}

void FileDownloader::fileDownloaded(QNetworkReply* pReply) {
    _handler(pReply->readAll());
    pReply->deleteLater();
    _complete = true;
}
