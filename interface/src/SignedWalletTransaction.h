//
//  SignedWalletTransaction.h
//  interfac/src
//
//  Created by Stephen Birarda on 2014-07-11.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_SignedWalletTransaction_h
#define hifi_SignedWalletTransaction_h

#include <WalletTransaction.h>

const int NUM_BYTES_SIGNED_TRANSACTION_MESSAGE = 72;

class SignedWalletTransaction : public WalletTransaction {
    Q_OBJECT
public:
    SignedWalletTransaction(const QUuid& destinationUUID, qint64 amount, qint64 messageTimestamp, qint64 expiryDelta);
    
    QByteArray binaryMessage();
    QByteArray hexMessage();
    QByteArray messageDigest();
    QByteArray signedMessageDigest();
    
private:
    qint64 _messageTimestamp;
    qint64 _expiryDelta;
};

#endif // hifi_SignedWalletTransaction_h