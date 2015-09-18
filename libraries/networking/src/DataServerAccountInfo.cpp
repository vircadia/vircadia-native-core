//
//  DataServerAccountInfo.cpp
//  libraries/networking/src
//
//  Created by Stephen Birarda on 2/18/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <openssl/rsa.h>
#include <openssl/x509.h>

#include <qjsondocument.h>
#include <QtCore/QDebug>
#include <QtCore/QDataStream>

#include <UUID.h>

#include "NetworkLogging.h"
#include "DataServerAccountInfo.h"

#ifdef __clang__
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif

DataServerAccountInfo::DataServerAccountInfo() :
    _accessToken(),
    _username(),
    _xmppPassword(),
    _discourseApiKey(),
    _walletID(),
    _balance(0),
    _hasBalance(false),
    _privateKey()
{

}

DataServerAccountInfo::DataServerAccountInfo(const DataServerAccountInfo& otherInfo) : QObject() {
    _accessToken = otherInfo._accessToken;
    _username = otherInfo._username;
    _xmppPassword = otherInfo._xmppPassword;
    _discourseApiKey = otherInfo._discourseApiKey;
    _walletID = otherInfo._walletID;
    _balance = otherInfo._balance;
    _hasBalance = otherInfo._hasBalance;
    _privateKey = otherInfo._privateKey;
}

DataServerAccountInfo& DataServerAccountInfo::operator=(const DataServerAccountInfo& otherInfo) {
    DataServerAccountInfo temp(otherInfo);
    swap(temp);
    return *this;
}

void DataServerAccountInfo::swap(DataServerAccountInfo& otherInfo) {
    using std::swap;

    swap(_accessToken, otherInfo._accessToken);
    swap(_username, otherInfo._username);
    swap(_xmppPassword, otherInfo._xmppPassword);
    swap(_discourseApiKey, otherInfo._discourseApiKey);
    swap(_walletID, otherInfo._walletID);
    swap(_balance, otherInfo._balance);
    swap(_hasBalance, otherInfo._hasBalance);
    swap(_privateKey, otherInfo._privateKey);
}

void DataServerAccountInfo::setAccessTokenFromJSON(const QJsonObject& jsonObject) {
    _accessToken = OAuthAccessToken(jsonObject);
}

void DataServerAccountInfo::setUsername(const QString& username) {
    if (_username != username) {
        _username = username;
        
        qCDebug(networking) << "Username changed to" << username;
    }
}

void DataServerAccountInfo::setXMPPPassword(const QString& xmppPassword) {
     if (_xmppPassword != xmppPassword) {
         _xmppPassword = xmppPassword;
     }
}

void DataServerAccountInfo::setDiscourseApiKey(const QString& discourseApiKey) {
    if (_discourseApiKey != discourseApiKey) {
        _discourseApiKey = discourseApiKey;
    }
}

void DataServerAccountInfo::setWalletID(const QUuid& walletID) {
    if (_walletID != walletID) {
        _walletID = walletID;
    }
}

void DataServerAccountInfo::setBalance(qint64 balance) {
    if (!_hasBalance || _balance != balance) {
        _balance = balance;
        _hasBalance = true;

        emit balanceChanged(_balance);
    }
}

void DataServerAccountInfo::setBalanceFromJSON(QNetworkReply& requestReply) {
    QJsonObject jsonObject = QJsonDocument::fromJson(requestReply.readAll()).object();
    if (jsonObject["status"].toString() == "success") {
        qint64 balanceInSatoshis = jsonObject["data"].toObject()["wallet"].toObject()["balance"].toDouble();
        setBalance(balanceInSatoshis);
    }
}

bool DataServerAccountInfo::hasProfile() const {
    return _username.length() > 0;
}

void DataServerAccountInfo::setProfileInfoFromJSON(const QJsonObject& jsonObject) {
    QJsonObject user = jsonObject["data"].toObject()["user"].toObject();
    setUsername(user["username"].toString());
    setXMPPPassword(user["xmpp_password"].toString());
    setDiscourseApiKey(user["discourse_api_key"].toString());
    setWalletID(QUuid(user["wallet_id"].toString()));
}

QByteArray DataServerAccountInfo::getUsernameSignature(const QUuid& connectionToken) {
    
        if (!_privateKey.isEmpty()) {
            const char* privateKeyData = _privateKey.constData();
            RSA* rsaPrivateKey = d2i_RSAPrivateKey(NULL,
                                                   reinterpret_cast<const unsigned char**>(&privateKeyData),
                                                   _privateKey.size());
            if (rsaPrivateKey) {
                QByteArray lowercaseUsername = _username.toLower().toUtf8();
                QByteArray usernameWithToken = QCryptographicHash::hash(lowercaseUsername.append(connectionToken.toRfc4122()),
                                                                        QCryptographicHash::Sha256);
                
                QByteArray usernameSignature(RSA_size(rsaPrivateKey), 0);
                unsigned int usernameSignatureSize = 0;
                
                int encryptReturn = RSA_sign(NID_sha256,
                                             reinterpret_cast<const unsigned char*>(usernameWithToken.constData()),
                                             usernameWithToken.size(),
                                             reinterpret_cast<unsigned char*>(usernameSignature.data()),
                                             &usernameSignatureSize,
                                             rsaPrivateKey);
                
                // free the private key RSA struct now that we are done with it
                RSA_free(rsaPrivateKey);

                if (encryptReturn == -1) {
                    qCDebug(networking) << "Error encrypting username signature.";
                    qCDebug(networking) << "Will re-attempt on next domain-server check in.";
                } else {
                    qDebug(networking) << "Returning username" << _username << "signed with connection UUID" << uuidStringWithoutCurlyBraces(connectionToken);
                    return usernameSignature;
                }
                
            } else {
                qCDebug(networking) << "Could not create RSA struct from QByteArray private key.";
                qCDebug(networking) << "Will re-attempt on next domain-server check in.";
            }
        }
    return QByteArray();
}

void DataServerAccountInfo::setPrivateKey(const QByteArray& privateKey) {
    _privateKey = privateKey;
    
}

QDataStream& operator<<(QDataStream &out, const DataServerAccountInfo& info) {
    out << info._accessToken << info._username << info._xmppPassword << info._discourseApiKey
        << info._walletID << info._privateKey;
    return out;
}

QDataStream& operator>>(QDataStream &in, DataServerAccountInfo& info) {
    in >> info._accessToken >> info._username >> info._xmppPassword >> info._discourseApiKey
        >> info._walletID >> info._privateKey;
    return in;
}
