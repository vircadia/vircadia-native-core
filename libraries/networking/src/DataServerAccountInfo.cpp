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
    _balance(0),
    _hasBalance(false)
{
    
}

DataServerAccountInfo::DataServerAccountInfo(const QJsonObject& jsonObject) :
    _accessToken(jsonObject),
    _username(),
    _xmppPassword(),
    _balance(0),
    _hasBalance(false)
{
    QJsonObject userJSONObject = jsonObject["user"].toObject();
    setUsername(userJSONObject["username"].toString());
    setXMPPPassword(userJSONObject["xmpp_password"].toString());
    setDiscourseApiKey(userJSONObject["discourse_api_key"].toString());
}

DataServerAccountInfo::DataServerAccountInfo(const DataServerAccountInfo& otherInfo) {
    _accessToken = otherInfo._accessToken;
    _username = otherInfo._username;
    _xmppPassword = otherInfo._xmppPassword;
    _discourseApiKey = otherInfo._discourseApiKey;
    _balance = otherInfo._balance;
    _hasBalance = otherInfo._hasBalance;
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
    swap(_balance, otherInfo._balance);
    swap(_hasBalance, otherInfo._hasBalance);
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

void DataServerAccountInfo::setBalance(qint64 balance) {
    if (!_hasBalance || _balance != balance) {
        _balance = balance;
        _hasBalance = true;
        
        emit balanceChanged(_balance);
    }
}

void DataServerAccountInfo::setBalanceFromJSON(const QJsonObject& jsonObject) {
    if (jsonObject["status"].toString() == "success") {
        qint64 balanceInSatoshis = jsonObject["data"].toObject()["wallet"].toObject()["balance"].toInt();
        setBalance(balanceInSatoshis);
    }
}

QDataStream& operator<<(QDataStream &out, const DataServerAccountInfo& info) {
    out << info._accessToken << info._username << info._xmppPassword << info._discourseApiKey;
    return out;
}

QDataStream& operator>>(QDataStream &in, DataServerAccountInfo& info) {
    in >> info._accessToken >> info._username >> info._xmppPassword >> info._discourseApiKey;
    return in;
}
