//
//  WalletTransaction.cpp
//  domain-server/src
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

WalletTransaction::WalletTransaction(const QUuid& destinationUUID, double amount) :
    _uuid(QUuid::createUuid()),
    _destinationUUID(destinationUUID),
    _amount(amount),
    _isFinalized(false)
{
    
}

QJsonDocument WalletTransaction::postJson() {
    QJsonObject rootObject;
    QJsonObject transactionObject;
    
    const QString TRANSCATION_ID_KEY = "id";
    const QString TRANSACTION_DESTINATION_WALLET_ID_KEY = "destination_wallet_id";
    const QString TRANSACTION_AMOUNT_KEY = "amount";
    
    transactionObject.insert(TRANSCATION_ID_KEY, uuidStringWithoutCurlyBraces(_uuid));
    transactionObject.insert(TRANSACTION_DESTINATION_WALLET_ID_KEY, uuidStringWithoutCurlyBraces(_destinationUUID));
    transactionObject.insert(TRANSACTION_AMOUNT_KEY, _amount);
    
    const QString ROOT_OBJECT_TRANSACTION_KEY = "transaction";
    rootObject.insert(ROOT_OBJECT_TRANSACTION_KEY, transactionObject);
    
    return QJsonDocument(rootObject);
}