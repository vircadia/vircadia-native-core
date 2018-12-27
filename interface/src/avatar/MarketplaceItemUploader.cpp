//
//  MarketplaceItemUploader.cpp
//
//
//  Created by Ryan Huffman on 12/10/2018
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "MarketplaceItemUploader.h"

#include <AccountManager.h>
#include <DependencyManager.h>

#include <QBuffer>
#include <quazip5/quazip.h>
#include <quazip5/quazipfile.h>

#include <qtimer.h>
#include <QFile>
#include <QFileInfo>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUuid>

MarketplaceItemUploader::MarketplaceItemUploader(QString title,
                                                 QString description,
                                                 QString rootFilename,
                                                 QUuid marketplaceID,
                                                 QStringList filePaths) :
    _title(title),
    _description(description), _rootFilename(rootFilename), _marketplaceID(marketplaceID), _filePaths(filePaths) {
    qWarning() << "File paths: " << _filePaths.join(", ");
    //_marketplaceID = QUuid::fromString(QLatin1String("{50dbd62f-cb6b-4be4-afb8-1ef8bd2dffa8}"));
}

void MarketplaceItemUploader::setState(State newState) {
    Q_ASSERT(_state != State::Complete);
    Q_ASSERT(_error == Error::None);
    Q_ASSERT(newState != _state);
    qDebug() << "Setting uploader state to: " << newState;

    _state = newState;
    emit stateChanged(newState);
    if (newState == State::Complete) {
        emit completed();
        emit finishedChanged();
    }
}

void MarketplaceItemUploader::setError(Error error) {
    Q_ASSERT(_state != State::Complete);
    Q_ASSERT(_error == Error::None);

    _error = error;
    emit errorChanged(error);
    emit finishedChanged();
}

void MarketplaceItemUploader::send() {
    doGetCategories();
}

void MarketplaceItemUploader::doGetCategories() {
    setState(State::GettingCategories);

    static const QString path = "/api/v1/marketplace/categories";

    auto accountManager = DependencyManager::get<AccountManager>();
    auto request = accountManager->createRequest(path, AccountManagerAuth::None);

    qWarning() << "Request url is: " << request.url();

    QNetworkAccessManager& networkAccessManager = NetworkAccessManager::getInstance();

    QNetworkReply* reply = networkAccessManager.get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        auto error = reply->error();
        if (error == QNetworkReply::NoError) {
            auto doc = QJsonDocument::fromJson(reply->readAll());
            auto extractCategoryID = [&doc]() -> std::pair<bool, int> {
                auto items = doc.object()["data"].toObject()["items"];
                if (!items.isArray()) {
                    qWarning() << "Categories parse error: data.items is not an array";
                    return { false, 0 };
                }

                auto itemsArray = items.toArray();
                for (const auto item : itemsArray) {
                    if (!item.isObject()) {
                        qWarning() << "Categories parse error: item is not an object";
                        return { false, 0 };
                    }

                    auto itemObject = item.toObject();
                    if (itemObject["name"].toString() == "Avatars") {
                        auto idValue = itemObject["id"];
                        if (!idValue.isDouble()) {
                            qWarning() << "Categories parse error: id is not a number";
                            return { false, 0 };
                        }
                        return { true, (int)idValue.toDouble() };
                    }
                }

                qWarning() << "Categories parse error: could not find a category for 'Avatar'";
                return { false, 0 };
            };

            bool success;
            int id;
            std::tie(success, id) = extractCategoryID();
            qDebug() << "Done " << success << id;
            if (!success) {
                qWarning() << "Failed to find marketplace category id";
                setError(Error::Unknown);
            } else {
                doUploadAvatar();
            }
        } else {
            setError(Error::Unknown);
        }
    });
}

void MarketplaceItemUploader::doUploadAvatar() {
    QBuffer buffer{ &_fileData };
    //buffer.open(QIODevice::WriteOnly);
    QuaZip zip{ &buffer };
    if (!zip.open(QuaZip::Mode::mdAdd)) {
        qWarning() << "Failed to open zip!!";
    }

    for (auto& filePath : _filePaths) {
        qWarning() << "Zipping: " << filePath;
        QFileInfo fileInfo{ filePath };

        QuaZipFile zipFile{ &zip };
        if (!zipFile.open(QIODevice::WriteOnly, QuaZipNewInfo(fileInfo.fileName()))) {
            qWarning() << "Could not open zip file:" << zipFile.getZipError();
            setError(Error::Unknown);
            return;
        }
        QFile file{ filePath };
        if (file.open(QIODevice::ReadOnly)) {
            zipFile.write(file.readAll());
        } else {
            qWarning() << "Failed to open: " << filePath;
        }
        file.close();
        zipFile.close();
        if (zipFile.getZipError() != UNZ_OK) {
            qWarning() << "Could not close zip file: " << zipFile.getZipError();
            setState(State::Complete);
            return;
        }
    }

    zip.close();

    qDebug() << "Finished zipping, size: " << (buffer.size() / (1000.0f)) << "KB";

    QString path = "/api/v1/marketplace/items";
    bool creating = true;
    if (!_marketplaceID.isNull()) {
        creating = false;
        auto idWithBraces = _marketplaceID.toString();
        auto idWithoutBraces = idWithBraces.mid(1, idWithBraces.length() - 2);
        path += "/" + idWithoutBraces;
    }
    auto accountManager = DependencyManager::get<AccountManager>();
    auto request = accountManager->createRequest(path, AccountManagerAuth::Required);
    qWarning() << "Request url is: " << request.url();

    QJsonObject root{ { "marketplace_item",
                        QJsonObject{ { "title", _title },
                                     { "description", _description },
                                     { "root_file_key", _rootFilename },
                                     { "category_ids", QJsonArray({ 5 }) },
                                     { "license", 0 },
                                     { "files", QString::fromLatin1(_fileData.toBase64()) } } } };
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/json");
    QJsonDocument doc{ root };

    qWarning() << "data: " << doc.toJson();

    _fileData.toBase64();
    QNetworkAccessManager& networkAccessManager = NetworkAccessManager::getInstance();

    QNetworkReply* reply{ nullptr };
    if (creating) {
        reply = networkAccessManager.post(request, doc.toJson());
    } else {
        reply = networkAccessManager.put(request, doc.toJson());
    }

    connect(reply, &QNetworkReply::uploadProgress, this, [this](float bytesSent, float bytesTotal) {
        if (_state == State::UploadingAvatar) {
            emit uploadProgress(bytesSent, bytesTotal);
            if (bytesSent >= bytesTotal) {
                setState(State::WaitingForUploadResponse);
            }
        }
    });

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        _responseData = reply->readAll();
        qWarning() << "Finished request " << _responseData;

        auto error = reply->error();
        if (error == QNetworkReply::NoError) {

            auto doc = QJsonDocument::fromJson(_responseData.toLatin1());
            auto status = doc.object()["status"].toString();
            if (status == "success") {
                _marketplaceID = QUuid::fromString(doc["data"].toObject()["marketplace_id"].toString());
                _itemVersion = doc["data"].toObject()["version"].toDouble();
                setState(State::WaitingForInventory);
                doWaitForInventory();
            } else {
                setError(Error::Unknown);
            }
        } else {
            setError(Error::Unknown);
        }
    });

    setState(State::UploadingAvatar);
}

void MarketplaceItemUploader::doWaitForInventory() {
    static const QString path = "/api/v1/commerce/inventory";

    auto accountManager = DependencyManager::get<AccountManager>();
    auto request = accountManager->createRequest(path, AccountManagerAuth::Required);

    QNetworkAccessManager& networkAccessManager = NetworkAccessManager::getInstance();

    QNetworkReply* reply = networkAccessManager.post(request, "");

    _numRequestsForInventory++;

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        auto data = reply->readAll();

        bool success = false;

        auto error = reply->error();
        if (error == QNetworkReply::NoError) {
            // Parse response data
            auto doc = QJsonDocument::fromJson(data);
            auto isAssetAvailable = [this, &doc]() -> bool {
                if (!doc.isObject()) {
                    return false;
                }
                auto root = doc.object();
                auto status = root["status"].toString();
                if (status != "success") {
                    return false;
                }
                auto data = root["data"];
                if (!data.isObject()) {
                    return false; 
                }
                auto assets = data.toObject()["assets"];
                if (!assets.isArray()) {
                    return false;
                }
                for (auto asset : assets.toArray()) {
                    auto assetObject = asset.toObject();
                    auto id = QUuid::fromString(assetObject["id"].toString());
                    if (id.isNull()) {
                        continue;
                    }
                    if (id == _marketplaceID) {
                        auto version = assetObject["version"];
                        if (version.isDouble()) {
                            int versionInt = version.toDouble();
                            if (versionInt >= _itemVersion) {
                                return true;
                            }
                        }
                    }
                }
                return false;
            };

            success = isAssetAvailable();
        }
        if (success) {
            setState(State::Complete);
        } else {
            qDebug() << "Failed to find item in inventory";
            if (_numRequestsForInventory > 8) {
                setError(Error::Unknown);
            } else {
                QTimer::singleShot(5000, [this]() { doWaitForInventory(); });
            }
        }
    });
}
