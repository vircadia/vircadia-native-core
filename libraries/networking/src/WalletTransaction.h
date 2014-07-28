//
//  WalletTransaction.h
//  domain-server/src
//
//  Created by Stephen Birarda on 2014-05-20.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_WalletTransaction_h
#define hifi_WalletTransaction_h

#include <QtCore/QJsonDocument>
#include <QtCore/QObject>
#include <QtCore/QUuid>

class WalletTransaction : public QObject {
public:
    WalletTransaction();
    WalletTransaction(const QUuid& destinationUUID, qint64 amount);
    
    const QUuid& getUUID() const { return _uuid; }
    
    void setDestinationUUID(const QUuid& destinationUUID) { _destinationUUID = destinationUUID; }
    const QUuid& getDestinationUUID() const { return _destinationUUID; }
    
    qint64 getAmount() const { return _amount; }
    void setAmount(qint64 amount) { _amount = amount; }
    void incrementAmount(qint64 increment) { _amount += increment; }
    
    bool isFinalized() const { return _isFinalized; }
    void setIsFinalized(bool isFinalized) { _isFinalized = isFinalized; }
    
    QJsonDocument postJson();
    QJsonObject toJson();
    void loadFromJson(const QJsonObject& jsonObject);
protected:
    QUuid _uuid;
    QUuid _destinationUUID;
    qint64 _amount;
    bool _isFinalized;
};

#endif // hifi_WalletTransaction_h