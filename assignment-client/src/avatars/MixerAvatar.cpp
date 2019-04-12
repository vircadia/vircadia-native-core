//
// MixerAvatar.cpp
// assignment-client/src/avatars
//
// Created by Simon Walton April 2019
// Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QRegularExpression>
#include <QJsonObject>
#include <QJsonArray>
#include <QJSonDocument>
#include <QNetworkReply>
#include <QCryptographicHash>

#include <ResourceManager.h>
#include <NetworkAccessManager.h>
#include <NetworkingConstants.h>
#include <EntityItem.h>
#include <EntityItemProperties.h>
#include "MixerAvatar.h"
#include "AvatarLogging.h"

void MixerAvatar::fetchAvatarFST() {
    _avatarFSTValid = false;
    auto resourceManager = DependencyManager::get<ResourceManager>();
    QUrl avatarURL = getSkeletonModelURL();
    if (avatarURL.isEmpty()) {
        return;
    }
    auto avatarURLString = avatarURL.toDisplayString();
    // Match UUID + version
    static const QRegularExpression marketIdRegex{
        "^https://metaverse.highfidelity.com/api/v.+/commerce/entity_edition/([-0-9a-z]{36}).*certificate_id=([\\w/+%]+)"
    };
    auto marketIdMatch = marketIdRegex.match(avatarURL.toDisplayString());
    if (marketIdMatch.hasMatch()) {
        QMutexLocker certifyLocker(&_avatarCertifyLock);
        _marketplaceIdString = marketIdMatch.captured(1);
        _certificateIdFromURL = QUrl::fromPercentEncoding(marketIdMatch.captured(2).toUtf8());
    }

    ResourceRequest* fstRequest = resourceManager->createResourceRequest(this, avatarURL);
    if (fstRequest) {
        QMutexLocker certifyLocker(&_avatarCertifyLock);

        if (_avatarRequest) {
            _avatarRequest->deleteLater();
        }
        _avatarRequest = fstRequest;
        connect(fstRequest, &ResourceRequest::finished, this, &MixerAvatar::fstRequestComplete);
        _verifyState = kRequestingFST;
        fstRequest->send();
    } else {
        qCDebug(avatars) << "Couldn't create FST request for" << avatarURL;
    }
}

void MixerAvatar::fstRequestComplete() {
    ResourceRequest* fstRequest = static_cast<ResourceRequest*>(QObject::sender());
    QMutexLocker certifyLocker(&_avatarCertifyLock);
    if (fstRequest == _avatarRequest) {
        auto result = fstRequest->getResult();
        if (result != ResourceRequest::Success) {
            _verifyState = kError;
            qCDebug(avatars) << "FST request for" << fstRequest->getUrl() << "failed:" << result;
        } else {
            _avatarFSTContents = fstRequest->getData();
            _verifyState = kReceivedFST;
            generateFSTHash();
            QString& marketplacePublicKey = EntityItem::_marketplacePublicKey;
            bool staticVerification = validateFSTHash(marketplacePublicKey);
            _verifyState = staticVerification ? kStaticValidation : kNoncertified;

            if (_verifyState == kStaticValidation) {
                static const QString POP_MARKETPLACE_API{ "/api/v1/commerce/proof_of_purchase_status/transfer" };
                auto& networkAccessManager = NetworkAccessManager::getInstance();
                QNetworkRequest networkRequest;
                networkRequest.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
                networkRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
                QUrl requestURL = NetworkingConstants::METAVERSE_SERVER_URL();
                requestURL.setPath(POP_MARKETPLACE_API);
                networkRequest.setUrl(requestURL);

                QJsonObject request;
                request["certificate_id"] = _certificateIdFromURL;
                _verifyState = kRequestingOwner;
                QNetworkReply* networkReply = networkAccessManager.put(networkRequest, QJsonDocument(request).toJson());
                networkReply->setParent(this);
                connect(networkReply, &QNetworkReply::finished, [this, networkReply]() {
                    QString responseString = networkReply->readAll();
                    qCDebug(avatars) << "Marketplace response for avatar" << getDisplayName() << ":" << responseString;
                    QJsonDocument responseJson = QJsonDocument::fromJson(responseString.toUtf8());
                    if (networkReply->error() == QNetworkReply::NoError) {
                        QMutexLocker certifyLocker(&_avatarCertifyLock);
                        if (responseJson["status"].toString() == "success") {
                            auto jsonData = responseJson["data"];
                            // owner, owner key?
                        }
                    } else {
                        auto jsonData = responseJson["data"];
                        if (!jsonData.isUndefined() && !jsonData.toObject()["message"].isUndefined()) {
                            qCDebug(avatars) << "Certificate Id lookup failed for" << getDisplayName() << ":"
                                             << jsonData.toObject()["message"].toString();
                            _verifyState = kError;
                        }
                    }
                    networkReply->deleteLater();
                });
            }
        }
        _avatarRequest->deleteLater();
        _avatarRequest = nullptr;
    } else {
        qCDebug(avatars) << "Incorrect request for" << getDisplayName();
    }
}

bool MixerAvatar::generateFSTHash() {
    if (_avatarFSTContents.length() == 0) {
        return false;
    }
    QByteArray hashJson = canonicalJson(_avatarFSTContents);
    QCryptographicHash fstHash(QCryptographicHash::Sha256);
    fstHash.addData(hashJson);
    _certificateHash = fstHash.result();
    return true;
}

bool MixerAvatar::validateFSTHash(const QString& publicKey) {
    // Guess we should refactor this stuff into a Authorization namespace ...
    return EntityItemProperties::verifySignature(publicKey, _certificateHash,
        QByteArray::fromBase64(_certificateIdFromURL.toUtf8()));
}

QByteArray MixerAvatar::canonicalJson(const QString fstFile) {
    QStringList fstLines = fstFile.split("\n", QString::SkipEmptyParts);
    static const QString fstKeywordsReg{
        "(marketplaceID|itemDescription|itemCategories|itemArtist|itemLicenseUrl|limitedRun|itemName|"
        "filename|texdir|script|editionNumber|certificateID)"
    };
    QRegularExpression fstLineRegExp{ QString("^\\s*") + fstKeywordsReg + "\\s*=\\s*(\\S.*)$" };
    QStringListIterator fstLineIter(fstLines);

    QJsonObject certifiedItems;
    QJsonArray scriptsArray;
    QStringList scripts;
    while (fstLineIter.hasNext()) {
        auto line = fstLineIter.next();
        auto lineMatch = fstLineRegExp.match(line);
        if (lineMatch.hasMatch()) {
            QString key = lineMatch.captured(1);
            if (key == "certificateID") {
                _certificateIdFromFST = lineMatch.captured(2);
            } else if (key == "limitedRun" || key == "editionNumber") {
                double value = lineMatch.captured(2).toDouble();
                if (value != 0.0) {
                    certifiedItems[key] = QJsonValue(value);
                }
            } else if (key == "script") {
                scripts.append(lineMatch.captured(2));
            } else {
                certifiedItems[key] = QJsonValue(lineMatch.captured(2));
            }
        }
    }
    if (!scripts.empty()) {
        scripts.sort();
        certifiedItems["script"] = QJsonArray::fromStringList(scripts);
    }
    QJsonDocument jsonDocCertifiedItems(certifiedItems);
    //OK - this one works
    //return R"({"editionNumber":34,"filename":"http://mpassets.highfidelity.com/7f142fde-541a-4902-b33a-25fa89dfba21-v1/Bridger/Hifi_Toon_Male_3.fbx","itemArtist":"EgyMax","itemCategories":"Avatars","itemDescription":"This is my first avatar. I hope you like it. More will come","itemName":"Bridger","limitedRun":-1,"marketplaceID":"7f142fde-541a-4902-b33a-25fa89dfba21","texdir":"http://mpassets.highfidelity.com/7f142fde-541a-4902-b33a-25fa89dfba21-v1/Bridger/textures"})";
    return jsonDocCertifiedItems.toJson(QJsonDocument::Compact);
}
