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

#include "MixerAvatar.h"

#include <QRegularExpression>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QCryptographicHash>
#include <QApplication>

#include <ResourceManager.h>
#include <NetworkAccessManager.h>
#include <NetworkingConstants.h>
#include <MetaverseAPI.h>
#include <EntityItem.h>
#include <EntityItemProperties.h>
#include "ClientTraitsHandler.h"
#include "AvatarLogging.h"

MixerAvatar::MixerAvatar() {
    static constexpr int CHALLENGE_TIMEOUT_MS = 10 * 1000;  // 10 s

    _challengeTimer.setSingleShot(true);
    _challengeTimer.setInterval(CHALLENGE_TIMEOUT_MS);
    _challengeTimer.callOnTimeout(this, &MixerAvatar::challengeTimeout);
    // QTimer::start is a set of overloaded functions.
    connect(this, &MixerAvatar::startChallengeTimer, &_challengeTimer, static_cast<void(QTimer::*)()>(&QTimer::start));
}

const char* MixerAvatar::stateToName(VerifyState state) {
    return QMetaEnum::fromType<VerifyState>().valueToKey(state);
}

void MixerAvatar::challengeTimeout() {
    switch (_verifyState) {
    case challengeClient:
        _verifyState = staticValidation;
        _pendingEvent = true;
        if (++_numberChallenges < NUM_CHALLENGES_BEFORE_FAIL) {
            qCDebug(avatars) << "Retrying (" << _numberChallenges << ") timed-out challenge for" << getDisplayName()
                << getSessionUUID();
        } else {
            _certifyFailed = true;
            _needsIdentityUpdate = true;
            qCWarning(avatars) << "ALERT: Dynamic verification TIMED-OUT for" << getDisplayName() << getSessionUUID();
        }
        break;

    case verificationFailed:
        qCDebug(avatars)  << "Retrying failed challenge for" << getDisplayName() << getSessionUUID();
        _verifyState = staticValidation;
        _pendingEvent = true;
        break;

    default:
        qCDebug(avatars) << "Ignoring timeout of avatar challenge";
        break;
    }
}

void MixerAvatar::fetchAvatarFST() {
    if (_verifyState >= requestingFST && _verifyState <= challengeClient) {
        qCDebug(avatars) << "WARNING: Avatar verification restarted; old state:" << stateToName(_verifyState);
    }
    _verifyState = nonCertified;

    _pendingEvent = false;

    QUrl avatarURL = _skeletonModelURL;
    if (avatarURL.isEmpty() || avatarURL.isLocalFile() || avatarURL.scheme() == "qrc") {
        // Not network FST.
        return;
    }
    _certificateIdFromURL.clear();
    _certificateIdFromFST.clear();
    _marketplaceIdFromURL.clear();
    _marketplaceIdFromFST.clear();
    auto resourceManager = DependencyManager::get<ResourceManager>();

    // Match UUID + (optionally) URL cert
    static const QRegularExpression marketIdRegex{
        "^https://.*?highfidelity\\.com/api/.*?/commerce/entity_edition/([-0-9a-z]{36})(.*?certificate_id=([\\w/+%]+)|.*).*$"
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

        _avatarRequest = fstRequest;
        _verifyState = requestingFST;
        connect(fstRequest, &ResourceRequest::finished, this, &MixerAvatar::fstRequestComplete);
        fstRequest->send();
    } else {
        qCDebug(avatars) << "Couldn't create FST request for" << avatarURL << getSessionUUID();
        _verifyState = error;
    }
    _needsIdentityUpdate = true;
}

void MixerAvatar::fstRequestComplete() {
    ResourceRequest* fstRequest = static_cast<ResourceRequest*>(QObject::sender());
    QMutexLocker certifyLocker(&_avatarCertifyLock);
    if (_verifyState == requestingFST && fstRequest == _avatarRequest) {
        auto result = fstRequest->getResult();
        if (result != ResourceRequest::Success) {
            _verifyState = error;
            qCDebug(avatars) << "FST request for" << fstRequest->getUrl() << "(user " << getSessionUUID() << ") failed:" << result;
        } else {
            _avatarFSTContents = fstRequest->getData();
            _verifyState = receivedFST;
            _pendingEvent = true;
        }
        _avatarRequest = nullptr;
    } else {
        qCDebug(avatars) << "Incorrect or outdated FST request for" << getDisplayName();
    }
    fstRequest->deleteLater();
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

bool MixerAvatar::validateFSTHash(const QString& publicKey) const {
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
                if (key == "marketplaceID") {
                    _marketplaceIdFromFST = lineMatch.captured(2);
                }
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

void MixerAvatar::ownerRequestComplete() {
    QMutexLocker certifyLocker(&_avatarCertifyLock);
    QNetworkReply* networkReply = static_cast<QNetworkReply*>(QObject::sender());

    if (_verifyState != requestingOwner) {
        qCDebug(avatars) << "WARNING: outdated avatar-owner information received in state" << stateToName(_verifyState);
    } else if (networkReply->error() == QNetworkReply::NoError) {
        _dynamicMarketResponse = networkReply->readAll();
        _verifyState = ownerResponse;
        _pendingEvent = true;
    } else {
        auto jsonData = QJsonDocument::fromJson(networkReply->readAll())["data"];
        if (!jsonData.isUndefined() && !jsonData.toObject()["message"].isUndefined()) {
            qCDebug(avatars) << "Owner lookup failed for" << getDisplayName() << "("
                << getSessionUUID() << ") :"
                << jsonData.toObject()["message"].toString();
            _verifyState = error;
            _pendingEvent = false;
        }
    }
    networkReply->deleteLater();
}

void MixerAvatar::requestCurrentOwnership() {
    // Get registered owner's public key from metaverse.
    static const QString POP_MARKETPLACE_API { "/api/v1/commerce/proof_of_purchase_status/transfer" };
    auto& networkAccessManager = NetworkAccessManager::getInstance();
    QNetworkRequest networkRequest;
    networkRequest.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
    networkRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QUrl requestURL = MetaverseAPI::getCurrentMetaverseServerURL();
    requestURL.setPath(POP_MARKETPLACE_API);
    networkRequest.setUrl(requestURL);

    QJsonObject request;
    request["certificate_id"] = _certificateIdFromFST;
    QNetworkReply* networkReply = networkAccessManager.put(networkRequest, QJsonDocument(request).toJson());
    connect(networkReply, &QNetworkReply::finished, this, &MixerAvatar::ownerRequestComplete);
}

void MixerAvatar::processCertifyEvents() {
    if (!_pendingEvent) {
        return;
    }

    QMutexLocker certifyLocker(&_avatarCertifyLock);
    switch (_verifyState) {

        case receivedFST:
        {
            generateFSTHash();
            _numberChallenges = 0;
            if (_certificateIdFromFST.length() != 0) {
                QString& marketplacePublicKey = EntityItem::_marketplacePublicKey;
                bool staticVerification = validateFSTHash(marketplacePublicKey);
                _verifyState = staticVerification ? staticValidation : verificationFailed;

                if (_verifyState == staticValidation) {
                    requestCurrentOwnership();
                    _verifyState = requestingOwner;
                } else {
                    _needsIdentityUpdate = true;
                    _pendingEvent = false;
                    qCDebug(avatars) << "Avatar" << getDisplayName() << "(" << getSessionUUID() << ") FAILED static certification";
                }
            } else {  // FST doesn't have a certificate, so noncertified rather than failed:
                _pendingEvent = false;
                _certifyFailed = false;
                _needsIdentityUpdate = true;
                _verifyState = nonCertified;
                qCDebug(avatars) << "Avatar " << getDisplayName() << "(" << getSessionUUID() << ") isn't certified";
            }
            break;
        }

        case staticValidation:
        {
            requestCurrentOwnership();
            _verifyState = requestingOwner;
            break;
        }

        case ownerResponse:
        {
            QJsonDocument responseJson = QJsonDocument::fromJson(_dynamicMarketResponse.toUtf8());
            QString ownerPublicKey;
            bool ownerValid = false;
            _pendingEvent = false;
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
                    if (ownerPublicKey.startsWith("-----BEGIN ")){
                        _ownerPublicKey = ownerPublicKey;
                    } else {
                        _ownerPublicKey = "-----BEGIN PUBLIC KEY-----\n"
                            + ownerPublicKey
                            + "\n-----END PUBLIC KEY-----\n";
                    }
                    sendOwnerChallenge();
                    _verifyState = challengeClient;
                } else {
                    _verifyState = error;
                    qCDebug(avatars) << "Get owner status - couldn't parse response for" << getSessionUUID()
                        << ":" << _dynamicMarketResponse;
                }
            } else {
                qCDebug(avatars) << "Get owner status failed for" << getDisplayName() << _marketplaceIdFromURL <<
                    "message:" << responseJson["message"].toString();
                _verifyState = error;
            }
            break;
        }

        case requestingOwner:
        {   // Qt networking done on this thread:
            QCoreApplication::processEvents();
            break;
        }

        default:
            qCDebug(avatars) << "Unexpected verify state" << stateToName(_verifyState);
            break;

    }  // close switch
}

void MixerAvatar::sendOwnerChallenge() {
    auto nodeList = DependencyManager::get<NodeList>();
    QByteArray avatarID = ("{" + _marketplaceIdFromFST + "}").toUtf8();
    if (_challengeNonce.isEmpty()) {
        _challengeNonce = QUuid::createUuid().toByteArray();
        QCryptographicHash nonceHash(QCryptographicHash::Sha256);
        nonceHash.addData(_challengeNonce);
        _challengeNonceHash = nonceHash.result();

    }

    auto challengeOwnershipPacket = NLPacket::create(PacketType::ChallengeOwnership,
        2 * sizeof(int) + _challengeNonce.length() + avatarID.length(), true);
    challengeOwnershipPacket->writePrimitive(avatarID.length());
    challengeOwnershipPacket->writePrimitive(_challengeNonce.length());
    challengeOwnershipPacket->write(avatarID);
    challengeOwnershipPacket->write(_challengeNonce);

    nodeList->sendPacket(std::move(challengeOwnershipPacket), *(nodeList->nodeWithUUID(getSessionUUID())) );
    QCryptographicHash nonceHash(QCryptographicHash::Sha256);
    nonceHash.addData(_challengeNonce);
    _challengeNonceHash = nonceHash.result();
    _pendingEvent = false;
    
    emit startChallengeTimer();
}

void MixerAvatar::processChallengeResponse(ReceivedMessage& response) {
    QByteArray avatarID;
    QMutexLocker certifyLocker(&_avatarCertifyLock);
    stopChallengeTimer();
    if (_verifyState == challengeClient) {
        QByteArray responseData = response.readAll();
        if (responseData.length() < 8) {
            _verifyState = error;
            qCWarning(avatars) << "ALERT: Avatar challenge response packet too small, length:" << responseData.length();
            return;
        }

        int avatarIDLength;
        int signedNonceLength;
        {
            QDataStream responseStream(responseData);
            responseStream.setByteOrder(QDataStream::LittleEndian);
            responseStream >> avatarIDLength >> signedNonceLength;
        }
        QByteArray avatarID(responseData.data() + 2 * sizeof(int), avatarIDLength);
        QByteArray signedNonce(responseData.data() + 2 * sizeof(int) + avatarIDLength, signedNonceLength);

        bool challengeResult = EntityItemProperties::verifySignature(_ownerPublicKey, _challengeNonceHash,
            QByteArray::fromBase64(signedNonce));
        _verifyState = challengeResult ? verificationSucceeded : verificationFailed;
        _certifyFailed = !challengeResult;
        _needsIdentityUpdate = true;
        if (_certifyFailed) {
            qCDebug(avatars) << "Dynamic verification FAILED for" << getDisplayName() << getSessionUUID();
            emit startChallengeTimer();
        } else {
            qCDebug(avatars) << "Dynamic verification SUCCEEDED for" << getDisplayName() << getSessionUUID();
            _challengeNonce.clear();
        }

    } else {
        qCDebug(avatars) << "WARNING: Unexpected avatar challenge-response in state" << stateToName(_verifyState);
    }
}

void MixerAvatar::stopChallengeTimer() {
    if (QThread::currentThread() == thread()) {
        _challengeTimer.stop();
    } else {
        QMetaObject::invokeMethod(&_challengeTimer, &QTimer::stop);
    }
}
