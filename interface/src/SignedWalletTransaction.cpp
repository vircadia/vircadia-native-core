//
//  SignedWalletTransaction.cpp
//  interface/src
//
//  Created by Stephen Birarda on 2014-07-11.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QtCore/QCryptographicHash>
#include <QtCore/QDebug>
#include <QtCore/QFile>

#include "SignedWalletTransaction.h"

SignedWalletTransaction::SignedWalletTransaction(const QUuid& destinationUUID, qint64 amount,
                                                 qint64 messageTimestamp, qint64 expiryDelta) :
    WalletTransaction(destinationUUID, amount),
    _messageTimestamp(messageTimestamp),
    _expiryDelta(expiryDelta)
{
    
}

QByteArray SignedWalletTransaction::hexMessage() {
    // build the message using the components of this transaction
    
    // UUID, source UUID, destination UUID, message timestamp, expiry delta, amount
    QByteArray messageBinary;
    
    messageBinary.append(_uuid.toRfc4122());
    
    messageBinary.append(reinterpret_cast<const char*>(&_messageTimestamp), sizeof(_messageTimestamp));
    messageBinary.append(reinterpret_cast<const char*>(&_expiryDelta), sizeof(_expiryDelta));
    
    QUuid sourceUUID = QUuid::createUuid();
    qDebug() << "The faked source UUID is" << sourceUUID;
    messageBinary.append(sourceUUID.toRfc4122());
    
    messageBinary.append(_destinationUUID.toRfc4122());
    
    messageBinary.append(reinterpret_cast<const char*>(&_amount), sizeof(_amount));
    
    return messageBinary.toHex();
}

QByteArray SignedWalletTransaction::messageDigest() {
    return QCryptographicHash::hash(hexMessage(), QCryptographicHash::Sha256);
}