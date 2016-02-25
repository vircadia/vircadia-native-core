//
//  Snapshot.cpp
//  interface/src/ui
//
//  Created by Stojce Slavkovski on 1/26/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QTemporaryFile>
#include <QtCore/QUrl>
#include <QtCore/QUrlQuery>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonArray>
#include <QtNetwork/QHttpMultiPart>
#include <QtGui/QImage>

#include <AccountManager.h>
#include <AddressManager.h>
#include <avatar/AvatarManager.h>
#include <avatar/MyAvatar.h>
#include <FileUtils.h>
#include <NodeList.h>
#include <OffscreenUi.h>
#include <SharedUtil.h>

#include "Application.h"
#include "Snapshot.h"

// filename format: hifi-snap-by-%username%-on-%date%_%time%_@-%location%.jpg
// %1 <= username, %2 <= date and time, %3 <= current location
const QString FILENAME_PATH_FORMAT = "hifi-snap-by-%1-on-%2.jpg";

const QString DATETIME_FORMAT = "yyyy-MM-dd_hh-mm-ss";
const QString SNAPSHOTS_DIRECTORY = "Snapshots";

const QString URL = "highfidelity_url";

Setting::Handle<QString> Snapshot::snapshotsLocation("snapshotsLocation",
    QStandardPaths::writableLocation(QStandardPaths::DesktopLocation));

SnapshotMetaData* Snapshot::parseSnapshotData(QString snapshotPath) {

    if (!QFile(snapshotPath).exists()) {
        return NULL;
    }

    QImage shot(snapshotPath);

    // no location data stored
    if (shot.text(URL).isEmpty()) {
        return NULL;
    }

    // parsing URL
    QUrl url = QUrl(shot.text(URL), QUrl::ParsingMode::StrictMode);

    SnapshotMetaData* data = new SnapshotMetaData();
    data->setURL(url);

    return data;
}

QString Snapshot::saveSnapshot(QImage image) {

    QFile* snapshotFile = savedFileForSnapshot(image, false);

    // we don't need the snapshot file, so close it, grab its filename and delete it
    snapshotFile->close();

    QString snapshotPath = QFileInfo(*snapshotFile).absoluteFilePath();

    delete snapshotFile;

    return snapshotPath;
}

QTemporaryFile* Snapshot::saveTempSnapshot(QImage image) {
    // return whatever we get back from saved file for snapshot
    return static_cast<QTemporaryFile*>(savedFileForSnapshot(image, true));
}

QFile* Snapshot::savedFileForSnapshot(QImage & shot, bool isTemporary) {

    // adding URL to snapshot
    QUrl currentURL = DependencyManager::get<AddressManager>()->currentAddress();
    shot.setText(URL, currentURL.toString());

    QString username = AccountManager::getInstance().getAccountInfo().getUsername();
    // normalize username, replace all non alphanumeric with '-'
    username.replace(QRegExp("[^A-Za-z0-9_]"), "-");

    QDateTime now = QDateTime::currentDateTime();

    QString filename = FILENAME_PATH_FORMAT.arg(username, now.toString(DATETIME_FORMAT));

    const int IMAGE_QUALITY = 100;

    if (!isTemporary) {
        QString snapshotFullPath = snapshotsLocation.get();

        if (!snapshotFullPath.endsWith(QDir::separator())) {
            snapshotFullPath.append(QDir::separator());
        }

        snapshotFullPath.append(filename);

        QFile* imageFile = new QFile(snapshotFullPath);
        imageFile->open(QIODevice::WriteOnly);

        shot.save(imageFile, 0, IMAGE_QUALITY);
        imageFile->close();

        return imageFile;

    } else {
        QTemporaryFile* imageTempFile = new QTemporaryFile(QDir::tempPath() + "/XXXXXX-" + filename);

        if (!imageTempFile->open()) {
            qDebug() << "Unable to open QTemporaryFile for temp snapshot. Will not save.";
            return NULL;
        }

        shot.save(imageTempFile, 0, IMAGE_QUALITY);
        imageTempFile->close();

        return imageTempFile;
    }
}

const QString FORUM_URL = "https://alphas.highfidelity.io";
const QString FORUM_UPLOADS_URL = FORUM_URL + "/uploads";
const QString FORUM_POST_URL = FORUM_URL + "/posts";
const QString FORUM_REPLY_TO_TOPIC = "244";
const QString FORUM_POST_TEMPLATE = "<img src='%1'/><p>%2</p>";
const QString SHARE_DEFAULT_ERROR = "The server isn't responding. Please try again in a few minutes.";
const QString SUCCESS_LABEL_TEMPLATE = "Success!!! Go check out your image ...<br/><a style='color:#333;text-decoration:none' href='%1'>%1</a>";


QString SnapshotUploader::uploadSnapshot(const QUrl& fileUrl) {
    if (AccountManager::getInstance().getAccountInfo().getDiscourseApiKey().isEmpty()) {
        OffscreenUi::warning(nullptr, "", "Your Discourse API key is missing, you cannot share snapshots. Please try to relog.");
        return QString();
    }

    QHttpPart apiKeyPart;
    apiKeyPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"api_key\""));
    apiKeyPart.setBody(AccountManager::getInstance().getAccountInfo().getDiscourseApiKey().toLatin1());

    QString filename = fileUrl.toLocalFile();
    qDebug() << filename;
    QFile* file = new QFile(filename);
    Q_ASSERT(file->exists());
    file->open(QIODevice::ReadOnly);

    QHttpPart imagePart;
    imagePart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("image/jpeg"));
    imagePart.setHeader(QNetworkRequest::ContentDispositionHeader,
        QVariant("form-data; name=\"file\"; filename=\"" + file->fileName() + "\""));
    imagePart.setBodyDevice(file);

    QHttpMultiPart* multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    file->setParent(multiPart); // we cannot delete the file now, so delete it with the multiPart
    multiPart->append(apiKeyPart);
    multiPart->append(imagePart);

    QUrl url(FORUM_UPLOADS_URL);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, HIGH_FIDELITY_USER_AGENT);

    QString result;
    QEventLoop loop;

    QSharedPointer<QNetworkReply> reply(NetworkAccessManager::getInstance().post(request, multiPart));
    QObject::connect(reply.data(), &QNetworkReply::finished, [&] {
        loop.quit();

        qDebug() << reply->errorString();
        for (const auto& header : reply->rawHeaderList()) {
            qDebug() << "Header " << QString(header);
        }
        auto replyResult = reply->readAll();
        qDebug() << QString(replyResult);
        QJsonDocument jsonResponse = QJsonDocument::fromJson(replyResult);
        const QJsonObject& responseObject = jsonResponse.object();
        if (!responseObject.contains("url")) {
            OffscreenUi::warning(this, "", SHARE_DEFAULT_ERROR);
            return;
        }
        result = responseObject["url"].toString();
    });
    loop.exec();
    return result;
}

QString SnapshotUploader::sendForumPost(const QString& snapshotPath, const QString& notes) {
    // post to Discourse forum
    QNetworkRequest request;
    request.setHeader(QNetworkRequest::UserAgentHeader, HIGH_FIDELITY_USER_AGENT);
    QUrl forumUrl(FORUM_POST_URL);

    QUrlQuery query;
    query.addQueryItem("api_key", AccountManager::getInstance().getAccountInfo().getDiscourseApiKey());
    query.addQueryItem("topic_id", FORUM_REPLY_TO_TOPIC);
    query.addQueryItem("raw", FORUM_POST_TEMPLATE.arg(snapshotPath, notes));
    forumUrl.setQuery(query);

    QByteArray postData = forumUrl.toEncoded(QUrl::RemoveFragment);
    request.setUrl(forumUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    QNetworkReply* requestReply = NetworkAccessManager::getInstance().post(request, postData);

    QEventLoop loop;
    QString result;
    connect(requestReply, &QNetworkReply::finished, [&] {
        loop.quit();
        QJsonDocument jsonResponse = QJsonDocument::fromJson(requestReply->readAll());
        requestReply->deleteLater();
        const QJsonObject& responseObject = jsonResponse.object();

        if (!responseObject.contains("id")) {
            QString errorMessage(SHARE_DEFAULT_ERROR);
            if (responseObject.contains("errors")) {
                QJsonArray errorArray = responseObject["errors"].toArray();
                if (!errorArray.first().toString().isEmpty()) {
                    errorMessage = errorArray.first().toString();
                }
            }
            OffscreenUi::warning(this, "", errorMessage);
            return;
        }

        const QString urlTemplate = "%1/t/%2/%3/%4";
        result = urlTemplate.arg(FORUM_URL,
            responseObject["topic_slug"].toString(),
            QString::number(responseObject["topic_id"].toDouble()),
            QString::number(responseObject["post_number"].toDouble()));
    });
    loop.exec();
    return result;
}

