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
#include "scripting/WindowScriptingInterface.h"

// filename format: hifi-snap-by-%username%-on-%date%_%time%_@-%location%.jpg
// %1 <= username, %2 <= date and time, %3 <= current location
const QString FILENAME_PATH_FORMAT = "hifi-snap-by-%1-on-%2.jpg";

const QString DATETIME_FORMAT = "yyyy-MM-dd_hh-mm-ss";
const QString SNAPSHOTS_DIRECTORY = "Snapshots";

const QString URL = "highfidelity_url";

Setting::Handle<QString> Snapshot::snapshotsLocation("snapshotsLocation",
    QStandardPaths::writableLocation(QStandardPaths::DesktopLocation));
Setting::Handle<bool> Snapshot::hasSetSnapshotsLocation("hasSetSnapshotsLocation", false);

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

    QString username = DependencyManager::get<AccountManager>()->getAccountInfo().getUsername();
    // normalize username, replace all non alphanumeric with '-'
    username.replace(QRegExp("[^A-Za-z0-9_]"), "-");

    QDateTime now = QDateTime::currentDateTime();

    QString filename = FILENAME_PATH_FORMAT.arg(username, now.toString(DATETIME_FORMAT));

    const int IMAGE_QUALITY = 100;

    if (!isTemporary) {
        QString snapshotFullPath;
        if (!hasSetSnapshotsLocation.get()) {
            snapshotFullPath = QFileDialog::getExistingDirectory(nullptr, "Choose Snapshots Directory", snapshotsLocation.get());
            hasSetSnapshotsLocation.set(true);
            snapshotsLocation.set(snapshotFullPath);
        } else {
            snapshotFullPath = snapshotsLocation.get();
        }

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

void Snapshot::uploadSnapshot(const QString& filename) {

    const QString SNAPSHOT_UPLOAD_URL = "/api/v1/snapshots";
    static SnapshotUploader uploader;
    
    qDebug() << "uploading snapshot " << filename;

    QFile* file = new QFile(filename);
    Q_ASSERT(file->exists());
    file->open(QIODevice::ReadOnly);

    QHttpPart imagePart;
    imagePart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("image/jpeg"));
    imagePart.setHeader(QNetworkRequest::ContentDispositionHeader,
                        QVariant("form-data; name=\"image\"; filename=\"" + file->fileName() + "\""));
    imagePart.setBodyDevice(file);
    
    QHttpMultiPart* multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    file->setParent(multiPart); // we cannot delete the file now, so delete it with the multiPart
    multiPart->append(imagePart);
    
    auto accountManager = DependencyManager::get<AccountManager>();
    JSONCallbackParameters callbackParams;
    callbackParams.jsonCallbackReceiver = &uploader;
    callbackParams.jsonCallbackMethod = "uploadSuccess";
    callbackParams.errorCallbackReceiver = &uploader;
    callbackParams.errorCallbackMethod = "uploadFailure";

    accountManager->sendRequest(SNAPSHOT_UPLOAD_URL,
                                AccountManagerAuth::Required,
                                QNetworkAccessManager::PostOperation,
                                callbackParams,
                                nullptr,
                                multiPart);
}

void SnapshotUploader::uploadSuccess(QNetworkReply& reply) {
    const QString STORY_UPLOAD_URL = "/api/v1/user_stories";
    static SnapshotUploader uploader;

    // parse the reply for the thumbnail_url
    QByteArray contents = reply.readAll();
    QJsonParseError jsonError;
    auto doc = QJsonDocument::fromJson(contents, &jsonError);
    if (jsonError.error == QJsonParseError::NoError) {
        QString thumbnail_url = doc.object().value("thumbnail_url").toString();

        // create json post data
        QJsonObject rootObject;
        QJsonObject userStoryObject;
        userStoryObject.insert("thumbnail_url", thumbnail_url);
        userStoryObject.insert("action", "snapshot");
        rootObject.insert("user_story", userStoryObject);

        auto accountManager = DependencyManager::get<AccountManager>();
        JSONCallbackParameters callbackParams;
        callbackParams.jsonCallbackReceiver = &uploader;
        callbackParams.jsonCallbackMethod = "createStorySuccess";
        callbackParams.errorCallbackReceiver = &uploader;
        callbackParams.errorCallbackMethod = "createStoryFailure";

        accountManager->sendRequest(STORY_UPLOAD_URL,
                                    AccountManagerAuth::Required,
                                    QNetworkAccessManager::PostOperation,
                                    callbackParams,
                                    QJsonDocument(rootObject).toJson());
                                    
    } else {
        qDebug() << "Error parsing upload response: " << jsonError.errorString();
        emit DependencyManager::get<WindowScriptingInterface>()->snapshotShared(false);
    }
}

void SnapshotUploader::uploadFailure(QNetworkReply& reply) {
    // TODO: parse response, potentially helpful for logging (?)
    emit DependencyManager::get<WindowScriptingInterface>()->snapshotShared(false);
}

void SnapshotUploader::createStorySuccess(QNetworkReply& reply) {    
    emit DependencyManager::get<WindowScriptingInterface>()->snapshotShared(true);
}

void SnapshotUploader::createStoryFailure(QNetworkReply& reply) {
    // TODO: parse response, potentially helpful for logging (?)
    emit DependencyManager::get<WindowScriptingInterface>()->snapshotShared(false);
}
