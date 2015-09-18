//
//  WalletTransaction.cpp
//  libraries/networking/src
//
//  Created by Stephen Birarda on 2014-05-20.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QtCore/QJsonObject>

#include <UUID.h>

#include "WalletTransaction.h"


WalletTransaction::WalletTransaction() :
    _uuid(),
    _destinationUUID(),
    _amount(0),
    _isFinalized(false)
{
    
}

WalletTransaction::WalletTransaction(const QUuid& destinationUUID, qint64 amount) :
    _uuid(QUuid::createUuid()),
    _destinationUUID(destinationUUID),
    _amount(amount),
    _isFinalized(false)
{
    
}

const QString TRANSACTION_ID_KEY = "id";
const QString TRANSACTION_DESTINATION_WALLET_ID_KEY = "destination_wallet_id";
const QString TRANSACTION_AMOUNT_KEY = "amount";

const QString ROOT_OBJECT_TRANSACTION_KEY = "transaction";

QJsonDocument WalletTransaction::postJson() {
    QJsonObject rootObject;
   
    rootObject.insert(ROOT_OBJECT_TRANSACTION_KEY, toJson());
    
    return QJsonDocument(rootObject);
}

QJsonObject WalletTransaction::toJson() {
    QJsonObject transactionObject;
    
    transactionObject.insert(TRANSACTION_ID_KEY, uuidStringWithoutCurlyBraces(_uuid));
    transactionObject.insert(TRANSACTION_DESTINATION_WALLET_ID_KEY, uuidStringWithoutCurlyBraces(_destinationUUID));
    transactionObject.insert(TRANSACTION_AMOUNT_KEY, _amount);
    
    return transactionObject;
}

void WalletTransaction::loadFromJson(const QJsonObject& jsonObject) {
    // pull the destination wallet and ID of the transaction to match it
    QJsonObject transactionObject = jsonObject.value("data").toObject().value(ROOT_OBJECT_TRANSACTION_KEY).toObject();
    
    _uuid = QUuid(transactionObject.value(TRANSACTION_ID_KEY).toString());
    _destinationUUID = QUuid(transactionObject.value(TRANSACTION_DESTINATION_WALLET_ID_KEY).toString());
    _amount = transactionObject.value(TRANSACTION_AMOUNT_KEY).toInt();
}
