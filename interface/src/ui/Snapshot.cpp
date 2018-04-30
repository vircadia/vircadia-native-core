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
#include <QtConcurrent/QtConcurrentRun>

#include <AccountManager.h>
#include <AddressManager.h>
#include <avatar/AvatarManager.h>
#include <avatar/MyAvatar.h>
#include <shared/FileUtils.h>
#include <NodeList.h>
#include <OffscreenUi.h>
#include <SharedUtil.h>
#include <SecondaryCamera.h>
#include <plugins/DisplayPlugin.h>

#include "Application.h"
#include "scripting/WindowScriptingInterface.h"
#include "MainWindow.h"
#include "Snapshot.h"
#include "SnapshotUploader.h"
#include "ToneMappingEffect.h"

// filename format: hifi-snap-by-%username%-on-%date%_%time%_@-%location%.jpg
// %1 <= username, %2 <= date and time, %3 <= current location
const QString FILENAME_PATH_FORMAT = "hifi-snap-by-%1-on-%2.jpg";

const QString DATETIME_FORMAT = "yyyy-MM-dd_hh-mm-ss";
const QString SNAPSHOTS_DIRECTORY = "Snapshots";

const QString URL = "highfidelity_url";

Setting::Handle<QString> Snapshot::snapshotsLocation("snapshotsLocation");

SnapshotMetaData* Snapshot::parseSnapshotData(QString snapshotPath) {

    if (!QFile(snapshotPath).exists()) {
        return NULL;
    }

    QUrl url;

    if (snapshotPath.right(3) == "jpg") {
        QImage shot(snapshotPath);

        // no location data stored
        if (shot.text(URL).isEmpty()) {
            return NULL;
        }

        // parsing URL
        url = QUrl(shot.text(URL), QUrl::ParsingMode::StrictMode);
    } else {
        return NULL;
    }

    SnapshotMetaData* data = new SnapshotMetaData();
    data->setURL(url);

    return data;
}

QString Snapshot::saveSnapshot(QImage image, const QString& filename) {

    QFile* snapshotFile = savedFileForSnapshot(image, false, filename);

    // we don't need the snapshot file, so close it, grab its filename and delete it
    snapshotFile->close();

    QString snapshotPath = QFileInfo(*snapshotFile).absoluteFilePath();

    delete snapshotFile;

    return snapshotPath;
}

QTimer Snapshot::snapshotTimer;

qint16 Snapshot::snapshotIndex = 0;
QVariant Snapshot::oldAttachedEntityId = 0;
QVariant Snapshot::oldOrientation = 0;
QVariant Snapshot::oldvFoV = 0;
QVariant Snapshot::oldNearClipPlaneDistance = 0;
QVariant Snapshot::oldFarClipPlaneDistance = 0;

QImage Snapshot::downImage;
QImage Snapshot::frontImage;
QImage Snapshot::leftImage;
QImage Snapshot::backImage;
QImage Snapshot::rightImage;
QImage Snapshot::upImage;

void Snapshot::save360Snapshot(const glm::vec3& cameraPosition, const QString& filename) {
    SecondaryCameraJobConfig* secondaryCameraRenderConfig = static_cast<SecondaryCameraJobConfig*>(qApp->getRenderEngine()->getConfiguration()->getConfig("SecondaryCamera"));

    // Save initial values of secondary camera render config
    oldAttachedEntityId = secondaryCameraRenderConfig->property("attachedEntityId");
    oldOrientation = secondaryCameraRenderConfig->property("orientation");
    oldvFoV = secondaryCameraRenderConfig->property("vFoV");
    oldNearClipPlaneDistance = secondaryCameraRenderConfig->property("nearClipPlaneDistance");
    oldFarClipPlaneDistance = secondaryCameraRenderConfig->property("farClipPlaneDistance");

    // Initialize some secondary camera render config options for 360 snapshot capture
    static_cast<ToneMappingConfig*>(qApp->getRenderEngine()->getConfiguration()->getConfig("SecondaryCameraJob.ToneMapping"))->setCurve(0);

    secondaryCameraRenderConfig->resetSizeSpectatorCamera(2048, 2048);
    secondaryCameraRenderConfig->setProperty("attachedEntityId", "");
    secondaryCameraRenderConfig->setPosition(cameraPosition);
    secondaryCameraRenderConfig->setProperty("vFoV", 90.0f);
    secondaryCameraRenderConfig->setProperty("nearClipPlaneDistance", 0.3f);
    secondaryCameraRenderConfig->setProperty("farClipPlaneDistance", 5000.0f);

    secondaryCameraRenderConfig->setOrientation(glm::quat(glm::radians(glm::vec3(-90.0f, 0.0f, 0.0f))));

    snapshotIndex = 0;

    snapshotTimer.setSingleShot(false);
    snapshotTimer.setInterval(250);
    connect(&snapshotTimer, &QTimer::timeout, [] {
        SecondaryCameraJobConfig* config = static_cast<SecondaryCameraJobConfig*>(qApp->getRenderEngine()->getConfiguration()->getConfig("SecondaryCamera"));
        if (snapshotIndex == 0) {
            downImage = qApp->getActiveDisplayPlugin()->getSecondaryCameraScreenshot();
            config->setOrientation(glm::quat(glm::radians(glm::vec3(0.0f, 0.0f, 0.0f))));
        } else if (snapshotIndex == 1) {
            frontImage = qApp->getActiveDisplayPlugin()->getSecondaryCameraScreenshot();
            config->setOrientation(glm::quat(glm::radians(glm::vec3(0.0f, 90.0f, 0.0f))));
        } else if (snapshotIndex == 2) {
            leftImage = qApp->getActiveDisplayPlugin()->getSecondaryCameraScreenshot();
            config->setOrientation(glm::quat(glm::radians(glm::vec3(0.0f, 180.0f, 0.0f))));
        } else if (snapshotIndex == 3) {
            backImage = qApp->getActiveDisplayPlugin()->getSecondaryCameraScreenshot();
            config->setOrientation(glm::quat(glm::radians(glm::vec3(0.0f, 270.0f, 0.0f))));
        } else if (snapshotIndex == 4) {
            rightImage = qApp->getActiveDisplayPlugin()->getSecondaryCameraScreenshot();
            config->setOrientation(glm::quat(glm::radians(glm::vec3(90.0f, 0.0f, 0.0f))));
        } else if (snapshotIndex == 5) {
            upImage = qApp->getActiveDisplayPlugin()->getSecondaryCameraScreenshot();
        } else {
            // Reset secondary camera render config
            static_cast<ToneMappingConfig*>(qApp->getRenderEngine()->getConfiguration()->getConfig("SecondaryCameraJob.ToneMapping"))->setCurve(1);
            config->resetSizeSpectatorCamera(qApp->getWindow()->geometry().width(), qApp->getWindow()->geometry().height());
            config->setProperty("attachedEntityId", oldAttachedEntityId);
            config->setProperty("vFoV", oldvFoV);
            config->setProperty("nearClipPlaneDistance", oldNearClipPlaneDistance);
            config->setProperty("farClipPlaneDistance", oldFarClipPlaneDistance);

            // Process six QImages
            QtConcurrent::run(convertToEquirectangular);

            snapshotTimer.stop();
        }

        snapshotIndex++;
    });
    snapshotTimer.start();
}
void Snapshot::convertToEquirectangular() {
    // I got help from StackOverflow while writing this code:
    // https://stackoverflow.com/questions/34250742/converting-a-cubemap-into-equirectangular-panorama

    float outputImageWidth = 8192.0f;
    float outputImageHeight = 4096.0f;
    QImage outputImage(outputImageWidth, outputImageHeight, QImage::Format_RGB32);
    outputImage.fill(0);
    QRgb sourceColorValue;
    float phi, theta;
    int cubeFaceWidth = 2048.0f;
    int cubeFaceHeight = 2048.0f;

    for (int j = 0; j < outputImageHeight; j++) {
        theta = (1.0f - ((float)j / outputImageHeight)) * PI;

        for (int i = 0; i < outputImageWidth; i++) {
            phi = ((float)i / outputImageWidth) * 2.0f * PI;

            float x, y, z;
            x = glm::sin(phi) * glm::sin(theta) * -1.0f;
            y = glm::cos(theta);
            z = glm::cos(phi) * glm::sin(theta) * -1.0f;

            float xa, ya, za;
            float a;

            a = std::max(std::max(std::abs(x), std::abs(y)), std::abs(z));

            xa = x / a;
            ya = y / a;
            za = z / a;

            // Pixel in the source images
            int xPixel, yPixel;
            QImage sourceImage;

            if (xa == 1) {
                // Right image
                xPixel = (int)((((za + 1.0f) / 2.0f) - 1.0f) * cubeFaceWidth);
                yPixel = (int)((((ya + 1.0f) / 2.0f)) * cubeFaceHeight);
                sourceImage = rightImage;
            } else if (xa == -1) {
                // Left image
                xPixel = (int)((((za + 1.0f) / 2.0f)) * cubeFaceWidth);
                yPixel = (int)((((ya + 1.0f) / 2.0f)) * cubeFaceHeight);
                sourceImage = leftImage;
            } else if (ya == 1) {
                // Down image
                xPixel = (int)((((xa + 1.0f) / 2.0f)) * cubeFaceWidth);
                yPixel = (int)((((za + 1.0f) / 2.0f) - 1.0f) * cubeFaceHeight);
                sourceImage = downImage;
            } else if (ya == -1) {
                // Up image
                xPixel = (int)((((xa + 1.0f) / 2.0f)) * cubeFaceWidth);
                yPixel = (int)((((za + 1.0f) / 2.0f)) * cubeFaceHeight);
                sourceImage = upImage;
            } else if (za == 1) {
                // Front image
                xPixel = (int)((((xa + 1.0f) / 2.0f)) * cubeFaceWidth);
                yPixel = (int)((((ya + 1.0f) / 2.0f)) * cubeFaceHeight);
                sourceImage = frontImage;
            } else if (za == -1) {
                // Back image
                xPixel = (int)((((xa + 1.0f) / 2.0f) - 1.0f) * cubeFaceWidth);
                yPixel = (int)((((ya + 1.0f) / 2.0f)) * cubeFaceHeight);
                sourceImage = backImage;
            } else {
                qDebug() << "Unknown face encountered when processing 360 Snapshot";
                xPixel = 0;
                yPixel = 0;
            }

            xPixel = std::min(std::abs(xPixel), 2047);
            yPixel = std::min(std::abs(yPixel), 2047);

            sourceColorValue = sourceImage.pixel(xPixel, yPixel);
            outputImage.setPixel(i, j, sourceColorValue);
        }
    }

    emit DependencyManager::get<WindowScriptingInterface>()->equirectangularSnapshotTaken(saveSnapshot(outputImage, QString()), true);
}

QTemporaryFile* Snapshot::saveTempSnapshot(QImage image) {
    // return whatever we get back from saved file for snapshot
    return static_cast<QTemporaryFile*>(savedFileForSnapshot(image, true));
}

QFile* Snapshot::savedFileForSnapshot(QImage & shot, bool isTemporary, const QString& userSelectedFilename) {

    // adding URL to snapshot
    QUrl currentURL = DependencyManager::get<AddressManager>()->currentPublicAddress();
    shot.setText(URL, currentURL.toString());

    QString username = DependencyManager::get<AccountManager>()->getAccountInfo().getUsername();
    // normalize username, replace all non alphanumeric with '-'
    username.replace(QRegExp("[^A-Za-z0-9_]"), "-");

    QDateTime now = QDateTime::currentDateTime();

    // If user has requested specific filename then use it, else create the filename
	// 'jpg" is appended, as the image is saved in jpg format.  This is the case for all snapshots
	//       (see definition of FILENAME_PATH_FORMAT)
    QString filename;
    if (!userSelectedFilename.isNull()) {
        filename = userSelectedFilename + ".jpg";
    } else {
        filename = FILENAME_PATH_FORMAT.arg(username, now.toString(DATETIME_FORMAT));
    }

    const int IMAGE_QUALITY = 100;

    if (!isTemporary) {
        QString snapshotFullPath = snapshotsLocation.get();

        if (snapshotFullPath.isEmpty()) {
            snapshotFullPath = OffscreenUi::getExistingDirectory(nullptr, "Choose Snapshots Directory", QStandardPaths::writableLocation(QStandardPaths::DesktopLocation));
            snapshotsLocation.set(snapshotFullPath);
        }

        if (!snapshotFullPath.isEmpty()) { // not cancelled

            if (!snapshotFullPath.endsWith(QDir::separator())) {
                snapshotFullPath.append(QDir::separator());
            }

            snapshotFullPath.append(filename);

            QFile* imageFile = new QFile(snapshotFullPath);
            imageFile->open(QIODevice::WriteOnly);

            shot.save(imageFile, 0, IMAGE_QUALITY);
            imageFile->close();

            return imageFile;
        }

    }
    // Either we were asked for a tempororary, or the user didn't set a directory.
    QTemporaryFile* imageTempFile = new QTemporaryFile(QDir::tempPath() + "/XXXXXX-" + filename);

    if (!imageTempFile->open()) {
        qDebug() << "Unable to open QTemporaryFile for temp snapshot. Will not save.";
        return NULL;
    }
    imageTempFile->setAutoRemove(isTemporary);

    shot.save(imageTempFile, 0, IMAGE_QUALITY);
    imageTempFile->close();

    return imageTempFile;
}

void Snapshot::uploadSnapshot(const QString& filename, const QUrl& href) {

    const QString SNAPSHOT_UPLOAD_URL = "/api/v1/snapshots";
    QUrl url = href;
    if (url.isEmpty()) {
        SnapshotMetaData* snapshotData = Snapshot::parseSnapshotData(filename);
        if (snapshotData) {
            url = snapshotData->getURL();
        }
        delete snapshotData;
    }
    if (url.isEmpty()) {
        url = QUrl(DependencyManager::get<AddressManager>()->currentShareableAddress());
    }
    SnapshotUploader* uploader = new SnapshotUploader(url, filename);
    
    QFile* file = new QFile(filename);
    Q_ASSERT(file->exists());
    file->open(QIODevice::ReadOnly);

    QHttpPart imagePart;
    if (filename.right(3) == "gif") {
        imagePart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("image/gif"));
    } else {
        imagePart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("image/jpeg"));
    }
    imagePart.setHeader(QNetworkRequest::ContentDispositionHeader,
                        QVariant("form-data; name=\"image\"; filename=\"" + file->fileName() + "\""));
    imagePart.setBodyDevice(file);
    
    QHttpMultiPart* multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    file->setParent(multiPart); // we cannot delete the file now, so delete it with the multiPart
    multiPart->append(imagePart);
    
    auto accountManager = DependencyManager::get<AccountManager>();
    JSONCallbackParameters callbackParams(uploader, "uploadSuccess", uploader, "uploadFailure");

    accountManager->sendRequest(SNAPSHOT_UPLOAD_URL,
                                AccountManagerAuth::Required,
                                QNetworkAccessManager::PostOperation,
                                callbackParams,
                                nullptr,
                                multiPart);
}

QString Snapshot::getSnapshotsLocation() {
    return snapshotsLocation.get("");
}

void Snapshot::setSnapshotsLocation(const QString& location) {
    snapshotsLocation.set(location);
}
