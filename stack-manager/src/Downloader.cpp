//
//  Downloader.cpp
//  StackManagerQt/src
//
//  Created by Mohammed Nafees on 07/09/14.
//  Copyright (c) 2014 High Fidelity. All rights reserved.
//

#include "Downloader.h"
#include "GlobalData.h"

#include <quazip.h>
#include <quazipfile.h>

#include <QNetworkRequest>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDebug>

Downloader::Downloader(const QUrl& url, QObject* parent) :
    QObject(parent)
{
    _url = url;
}

void Downloader::start(QNetworkAccessManager* manager) {
    qDebug() << "Downloader::start() for URL - " << _url;
    QNetworkRequest req(_url);
    QNetworkReply* reply = manager->get(req);
    emit downloadStarted(this, _url);
    connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), SLOT(error(QNetworkReply::NetworkError)));
    connect(reply, SIGNAL(downloadProgress(qint64,qint64)), SLOT(downloadProgress(qint64,qint64)));
    connect(reply, SIGNAL(finished()), SLOT(downloadFinished()));
}

void Downloader::error(QNetworkReply::NetworkError error) {
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    qDebug() << reply->errorString();
    reply->deleteLater();
}

void Downloader::downloadProgress(qint64 bytesReceived, qint64 bytesTotal) {
    int percentage = bytesReceived*100/bytesTotal;
    emit downloadProgress(_url, percentage);
}

void Downloader::downloadFinished() {
    qDebug() << "Downloader::downloadFinished() for URL - " << _url;
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << reply->errorString();
        emit downloadFailed(_url);
        return;
    }
    emit downloadCompleted(_url);

    QString fileName = QFileInfo(_url.toString()).fileName();
    QString fileDir = GlobalData::getInstance().getClientsLaunchPath();
    QString filePath = fileDir + fileName;

    QFile file(filePath);

    // remove file if already exists
    if (file.exists()) {
        file.remove();
    }

    if (file.open(QIODevice::WriteOnly)) {
        if (fileName == "assignment-client" || fileName == "assignment-client.exe" ||
            fileName == "domain-server" || fileName == "domain-server.exe") {
            file.setPermissions(QFile::ExeOwner | QFile::ReadOwner | QFile::WriteOwner);
        } else {
            file.setPermissions(QFile::ReadOwner | QFile::WriteOwner);
        }
        emit installingFiles(_url);
        file.write(reply->readAll());
        bool error = false;
        file.close();

        if (fileName.endsWith(".zip")) { // we need to unzip the file now
            QuaZip zip(QFileInfo(file).absoluteFilePath());
            if (zip.open(QuaZip::mdUnzip)) {
                QuaZipFile zipFile(&zip);
                for(bool f = zip.goToFirstFile(); f; f = zip.goToNextFile()) {
                    if (zipFile.open(QIODevice::ReadOnly)) {
                        QFile newFile(QDir::toNativeSeparators(fileDir + "/" + zipFile.getActualFileName()));
                        if (zipFile.getActualFileName().endsWith("/")) {
                            QDir().mkpath(QFileInfo(newFile).absolutePath());
                            zipFile.close();
                            continue;
                        }

                        // remove file if already exists
                        if (newFile.exists()) {
                            newFile.remove();
                        }

                        if (newFile.open(QIODevice::WriteOnly)) {
                            newFile.setPermissions(QFile::ReadOwner | QFile::WriteOwner);
                            newFile.write(zipFile.readAll());
                            newFile.close();
                        } else {
                            error = true;
                            qDebug() << "Could not open archive file for writing: " << zip.getCurrentFileName();
                            emit filesInstallationFailed(_url);
                            break;
                        }
                    } else {
                        error = true;
                        qDebug() << "Could not open archive file: " << zip.getCurrentFileName();
                        emit filesInstallationFailed(_url);
                        break;
                    }
                    zipFile.close();
                }
                zip.close();
            } else {
                error = true;
                emit filesInstallationFailed(_url);
                qDebug() << "Could not open zip file for extraction.";
            }
        }
        if (!error)
            emit filesSuccessfullyInstalled(_url);
    } else {
        emit filesInstallationFailed(_url);
        qDebug() << "Could not open file: " << filePath;
    }
    reply->deleteLater();
}


