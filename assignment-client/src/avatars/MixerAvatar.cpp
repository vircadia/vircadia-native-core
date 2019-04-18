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

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/evp.h>
#include <openssl/aes.h>
#include <openssl/ecdsa.h>


void MixerAvatar::fetchAvatarFST() {
    _verifyState = kNoncertified;
    _certificateIdFromURL.clear();
    _certificateIdFromFST.clear();
    _marketplaceIdFromURL.clear();
    auto resourceManager = DependencyManager::get<ResourceManager>();
    QUrl avatarURL = getSkeletonModelURL();
    if (avatarURL.isEmpty()) {
        return;
    }

    auto avatarURLString = avatarURL.toDisplayString();
    // Match UUID + version
    static const QRegularExpression marketIdRegex{
        "^https://metaverse.highfidelity.com/api/v.+/commerce/entity_edition/([-0-9a-z]{36}).*?(certificate_id=([\\w/+%]+)).*$"
    };
    auto marketIdMatch = marketIdRegex.match(avatarURL.toDisplayString());
    if (marketIdMatch.hasMatch()) {
        QMutexLocker certifyLocker(&_avatarCertifyLock);
        _marketplaceIdFromURL = marketIdMatch.captured(1);
        if (marketIdMatch.lastCapturedIndex() == 3) {
            _certificateIdFromURL = QUrl::fromPercentEncoding(marketIdMatch.captured(3).toUtf8());
        }
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
        _verifyState = kError;
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
            _verifyState = staticVerification ? kStaticValidation : kVerificationFailed;

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
                request["certificate_id"] = _certificateIdFromFST;
                _verifyState = kRequestingOwner;
                QNetworkReply* networkReply = networkAccessManager.put(networkRequest, QJsonDocument(request).toJson());
                networkReply->setParent(this);
                connect(networkReply, &QNetworkReply::finished, [this, networkReply]() {
                    QMutexLocker certifyLocker(&_avatarCertifyLock);
                    if (networkReply->error() == QNetworkReply::NoError) {
                        _dynamicMarketResponse = networkReply->readAll();
                        _verifyState = kOwnerResponse;
                    } else {
                        auto jsonData = QJsonDocument::fromJson(networkReply->readAll())["data"];
                        if (!jsonData.isUndefined() && !jsonData.toObject()["message"].isUndefined()) {
                            qCDebug(avatars) << "Owner lookup failed for" << getDisplayName() << ":"
                                             << jsonData.toObject()["message"].toString();
                            _verifyState = kError;
                        }
                    }
                    networkReply->deleteLater();
                });
            } else {
                _verifyState = kVerificationFailed;
                qCDebug(avatars) << "Avatar" << getDisplayName() << "FAILED static certification";
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
    QByteArray::fromBase64(_certificateIdFromFST.toUtf8()));
}

QByteArray MixerAvatar::canonicalJson(const QString fstFile) {
    QStringList fstLines = fstFile.split("\n", QString::SkipEmptyParts);
    static const QString fstKeywordsReg {
        "(marketplaceID|itemDescription|itemCategories|itemArtist|itemLicenseUrl|limitedRun|itemName|"
        "filename|texdir|script|editionNumber|certificateID)"
    };
    QRegularExpression fstLineRegExp { QString("^\\s*") + fstKeywordsReg + "\\s*=\\s*(\\S.*)$" };
    QStringListIterator fstLineIter(fstLines);

    QJsonObject certifiedItems;
    QStringList scripts;
    while (fstLineIter.hasNext()) {
        auto line = fstLineIter.next();
        auto lineMatch = fstLineRegExp.match(line);
        if (lineMatch.hasMatch()) {
            QString key = lineMatch.captured(1);
            if (key == "certificateID") {
                _certificateIdFromFST = lineMatch.captured(2);
            } else if (key == "itemDescription") {
                // Item description can be multiline - intermediate lines end in <CR>
                QString itemDesc = lineMatch.captured(2);
                while (itemDesc.endsWith('\r') && fstLineIter.hasNext()) {
                    itemDesc += '\n' + fstLineIter.next();
                }
                certifiedItems[key] = QJsonValue(itemDesc);
            } else if (key == "limitedRun" || key == "editionNumber") {
                double value = lineMatch.captured(2).toDouble();
                if (value != 0.0) {
                    certifiedItems[key] = QJsonValue(value);
                }
            } else if (key == "script") {
                scripts.append(lineMatch.captured(2).trimmed());
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
    //Example working form:
    //return R"({"editionNumber":34,"filename":"http://mpassets.highfidelity.com/7f142fde-541a-4902-b33a-25fa89dfba21-v1/Bridger/Hifi_Toon_Male_3.fbx","itemArtist":"EgyMax",
    //"itemCategories":"Avatars","itemDescription":"This is my first avatar. I hope you like it. More will come","itemName":"Bridger","limitedRun":-1,
    //"marketplaceID":"7f142fde-541a-4902-b33a-25fa89dfba21","texdir":"http://mpassets.highfidelity.com/7f142fde-541a-4902-b33a-25fa89dfba21-v1/Bridger/textures"})";
    return jsonDocCertifiedItems.toJson(QJsonDocument::Compact);
}

void MixerAvatar::processCertifyEvents() {
    QMutexLocker certifyLocker(&_avatarCertifyLock);
    if (_verifyState != kOwnerResponse && _verifyState != kChallengeResponse) {
        return;
    }

    switch (_verifyState) {
    case kOwnerResponse:
    {
        QJsonDocument responseJson = QJsonDocument::fromJson(_dynamicMarketResponse.toUtf8());
        _verifyState = kChallengeClient;
        QString ownerPublicKey;
        bool ownerValid = false;
        qCDebug(avatars) << "Marketplace response for avatar" << getDisplayName() << ":" << _dynamicMarketResponse;
        if (responseJson["status"].toString() == "success") {
            QJsonValue jsonData = responseJson["data"];
            if (jsonData.isObject()) {
                auto ownerJson = jsonData["transfer_recipient_key"];
                if (ownerJson.isString()) {
                    ownerPublicKey = ownerJson.toString();
                }
                auto transferStatusJson = jsonData["transfer_status"];
                if (transferStatusJson.isArray() && transferStatusJson.toArray()[0].toString() == "confirmed") {
                    ownerValid = true;
                }
            }
            if (ownerValid && !ownerPublicKey.isEmpty()) {
                _ownerPublicKey = "-----BEGIN PUBLIC KEY-----\n"
                    + ownerPublicKey
                    + "\n-----END PUBLIC KEY-----\n";
                challengeOwner();
            } else {
                _verifyState = kError;
            }
        } else {
            qCDebug(avatars) << "Get owner status failed for " << getDisplayName() << _marketplaceIdFromURL <<
                "message:" << responseJson["message"].toString();
            _verifyState = kError;
        }
        break;
    }

    case kChallengeResponse:
    {
        int avatarIDLength;
        int signedNonceLength;
        if (_challengeResponse.length() < 8) {
            _verifyState = kError;
            break;
        }

        QDataStream responseStream(_challengeResponse);
        responseStream.setByteOrder(QDataStream::LittleEndian);
        responseStream >> avatarIDLength >> signedNonceLength;
        QByteArray avatarID(_challengeResponse.data() + 2 * sizeof(int), avatarIDLength);
        QByteArray signedNonce(_challengeResponse.data() + 2 * sizeof(int) + avatarIDLength, signedNonceLength);
        QCryptographicHash nonceHash(QCryptographicHash::Sha256);
        nonceHash.addData(_challengeNonce);

        bool challengeResult = EntityItemProperties::verifySignature(_ownerPublicKey, nonceHash.result(),
            QByteArray::fromBase64(signedNonce));
        _verifyState = challengeResult ? kVerificationSucceeded : kVerificationFailed;
        if (_verifyState == kVerificationFailed) {
            qCDebug(avatars) << "Dynamic verification FAILED for " << getDisplayName() << getSessionUUID();
        }
    }

    }  // close switch
}

void MixerAvatar::challengeOwner() {
    auto nodeList = DependencyManager::get<NodeList>();
    QByteArray avatarID = ("{" + _marketplaceIdFromURL + "}").toUtf8();
    QByteArray nonce = QUuid::createUuid().toByteArray();

    auto challengeOwnershipPacket = NLPacket::create(PacketType::ChallengeOwnership,
        2 * sizeof(int) + nonce.length() + avatarID.length(), true);
    challengeOwnershipPacket->writePrimitive(avatarID.length());
    challengeOwnershipPacket->writePrimitive(nonce.length());
    challengeOwnershipPacket->write(avatarID);
    challengeOwnershipPacket->write(nonce);

    nodeList->sendPacket(std::move(challengeOwnershipPacket), *(nodeList->nodeWithUUID(getSessionUUID())) );
    _challengeNonce = nonce;

    static constexpr int CHALLENGE_TIMEOUT_MS = 10 * 1000;  // 10 s
    _challengeTimeout.setInterval(CHALLENGE_TIMEOUT_MS);
    _challengeTimeout.connect(&_challengeTimeout, &QTimer::timeout, [this]() {
        _verifyState = kVerificationFailed;
        });
}

void MixerAvatar::handleChallengeResponse(ReceivedMessage * response) {
    QByteArray avatarID;
    QByteArray encryptedNonce;
    QMutexLocker certifyLocker(&_avatarCertifyLock);
    if (_verifyState == kChallengeClient) {
        _challengeTimeout.stop();
        _challengeResponse = response->readAll();
        _verifyState = kChallengeResponse;
    }
}
