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

#include <QtCore/QDebug>

#include "DataServerAccountInfo.h"

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

DataServerAccountInfo::DataServerAccountInfo(const DataServerAccountInfo& otherInfo) {
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

        qDebug() << "Username changed to" << username;
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

void DataServerAccountInfo::setBalanceFromJSON(const QJsonObject& jsonObject) {
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
