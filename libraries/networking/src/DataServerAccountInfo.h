//
//  DataServerAccountInfo.h
//  libraries/networking/src
//
//  Created by Stephen Birarda on 2/21/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_DataServerAccountInfo_h
#define hifi_DataServerAccountInfo_h

#include <QtCore/QObject>
#include <QtCore/QUuid>

#include "OAuthAccessToken.h"

const float SATOSHIS_PER_CREDIT = 100000000.0f;

class DataServerAccountInfo : public QObject {
    Q_OBJECT
public:
    DataServerAccountInfo();
    DataServerAccountInfo(const DataServerAccountInfo& otherInfo);
    DataServerAccountInfo& operator=(const DataServerAccountInfo& otherInfo);

    const OAuthAccessToken& getAccessToken() const { return _accessToken; }
    void setAccessToken(const OAuthAccessToken& accessToken) { _accessToken = accessToken; }
    void setAccessTokenFromJSON(const QJsonObject& jsonObject);

    const QString& getUsername() const { return _username; }
    void setUsername(const QString& username);

    const QString& getXMPPPassword() const { return _xmppPassword; }
    void setXMPPPassword(const QString& xmppPassword);

    const QString& getDiscourseApiKey() const { return _discourseApiKey; }
    void setDiscourseApiKey(const QString& discourseApiKey);
    
    const QUuid& getWalletID() const { return _walletID; }
    void setWalletID(const QUuid& walletID);
    
    const QByteArray& getUsernameSignature();
    void setPrivateKey(const QByteArray& privateKey);

    qint64 getBalance() const { return _balance; }
    float getBalanceInSatoshis() const { return _balance / SATOSHIS_PER_CREDIT; }
    void setBalance(qint64 balance);
    bool hasBalance() const { return _hasBalance; }
    void setHasBalance(bool hasBalance) { _hasBalance = hasBalance; }
    Q_INVOKABLE void setBalanceFromJSON(const QJsonObject& jsonObject);

    bool hasProfile() const;

    void setProfileInfoFromJSON(const QJsonObject& jsonObject);

    friend QDataStream& operator<<(QDataStream &out, const DataServerAccountInfo& info);
    friend QDataStream& operator>>(QDataStream &in, DataServerAccountInfo& info);
signals:
    qint64 balanceChanged(qint64 newBalance);
private:
    void swap(DataServerAccountInfo& otherInfo);

    OAuthAccessToken _accessToken;
    QString _username;
    QString _xmppPassword;
    QString _discourseApiKey;
    QUuid _walletID;
    qint64 _balance;
    bool _hasBalance;
    QByteArray _privateKey;
    QByteArray _usernameSignature;
};

#endif // hifi_DataServerAccountInfo_h
