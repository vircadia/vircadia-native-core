#pragma once

#include <QtCore/QObject>
#include <QtNetwork/QNetworkAccessManager>

class FileDownloader : public QObject {
    Q_OBJECT

public:
    using Handler = std::function<void(const QByteArray& data)>;

    FileDownloader(QUrl url, const Handler& handler, QObject* parent = 0);

    void waitForDownload();

private slots:
    void fileDownloaded(QNetworkReply* pReply);

private:
    QNetworkAccessManager _accessManager;
    Handler _handler;
    bool _complete { false };
};
