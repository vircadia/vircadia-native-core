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
    // Match UUID + version
    static const QRegularExpression marketIdRegex{ "^https?://mpassets.highfidelity.com/([-0-9a-z]{36,})/" };
    auto marketIdMatch = marketIdRegex.match(avatarURL.toDisplayString());
    if (marketIdMatch.hasMatch()) {
        QMutexLocker certifyLocker(&_avatarCertifyLock);
        _marketplaceIdString = marketIdMatch.captured(1);
    } else {
        _marketplaceIdString = "2119142f-0cd6-4126-b18e-06b53afcc0a9";  // XXX: plants entity, for testing
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

// TESTING
static const QString PLANT_CERTID {
    "MEYCIQDxKA62xq/G/x1aWpXyJbGjIHm6SU4ceQu2ljtFRfeu/QIhAKw2uEfLId8sqLfEoErOlvu2UV2wbP3ttrYP1hoZT0Ge"};

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
            _certificateId = PLANT_CERTID;
            bool staticVerification = validateFSTHash(marketplacePublicKey);
            if (!staticVerification) {
                _verifyState = kNoncertified;
            }

            if (!_certificateId.isEmpty()) {
                static const QString POP_MARKETPLACE_API { "/api/v1/commerce/proof_of_purchase_status/transfer" };
                auto& networkAccessManager = NetworkAccessManager::getInstance();
                QNetworkRequest networkRequest;
                networkRequest.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
                networkRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
                QUrl requestURL = NetworkingConstants::METAVERSE_SERVER_URL();
                requestURL.setPath(POP_MARKETPLACE_API);
                networkRequest.setUrl(requestURL);

                QJsonObject request;
                request["certificate_id"] = _certificateId;
                _verifyState = kRequestingOwner;
                QNetworkReply* networkReply = networkAccessManager.put(networkRequest, QJsonDocument(request).toJson());
                networkReply->setParent(this);
                connect(networkReply, &QNetworkReply::finished, [this, networkReply]() {
                    QString responseString = networkReply->readAll();
                    qCDebug(avatars) << "Marketplace response for avatar" << getDisplayName() << ":" << responseString;
                    QJsonDocument responseJson = QJsonDocument::fromJson(responseString.toUtf8());
                    if (networkReply->error() == QNetworkReply::NoError) {
                        volatile int x = 1;
                        if (responseJson["status"].toString() == "success") {
                            auto jsonData = responseJson["data"];
                            // owner, owner key?
                        }
                    } else {
                        auto jsonData = responseJson["data"];
                        if (!jsonData.isUndefined()
                            && !jsonData.toObject()["message"].isUndefined()) {
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
    QCryptographicHash fstHash(QCryptographicHash::Sha224);
    fstHash.addData(_avatarFSTContents);
    _certificateHash = fstHash.result();
    return true;
}

bool MixerAvatar::validateFSTHash(const QString& publicKey) {
    // Guess we should refactor this stuff into a Authorization namespace ...
    return EntityItemProperties::verifySignature(publicKey, _certificateHash,
        QByteArray::fromBase64(_certificateId.toUtf8()));
}
