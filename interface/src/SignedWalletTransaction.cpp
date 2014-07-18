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

#include <openssl/bio.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>

#include <AccountManager.h>

#include "Menu.h"

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
    
    messageBinary.append(AccountManager::getInstance().getAccountInfo().getWalletID().toRfc4122());
    
    messageBinary.append(_destinationUUID.toRfc4122());
    
    messageBinary.append(reinterpret_cast<const char*>(&_amount), sizeof(_amount));
    
    return messageBinary.toHex();
}

QByteArray SignedWalletTransaction::messageDigest() {
    return QCryptographicHash::hash(hexMessage(), QCryptographicHash::Sha256).toHex();
}

QByteArray SignedWalletTransaction::signedMessageDigest() {
    // pull the current private key from menu into RSA structure in memory
    QByteArray privateKeyByteArray = Menu::getInstance()->getWalletPrivateKey();
    
    BIO* privateKeyBIO = NULL;
    RSA* rsaPrivateKey = NULL;
    
    privateKeyBIO = BIO_new_mem_buf(privateKeyByteArray.data(), privateKeyByteArray.size());
    PEM_read_bio_RSAPrivateKey(privateKeyBIO, &rsaPrivateKey, NULL, NULL);
    
    QByteArray digestToEncrypt = messageDigest();
    QByteArray encryptedDigest(RSA_size(rsaPrivateKey), 0);
    
    int encryptReturn = RSA_private_encrypt(digestToEncrypt.size(),
                                            reinterpret_cast<const unsigned char*>(digestToEncrypt.constData()),
                                            reinterpret_cast<unsigned char*>(encryptedDigest.data()),
                                            rsaPrivateKey, RSA_PKCS1_PADDING);
    
    // free the two structures used
    BIO_free(privateKeyBIO);
    RSA_free(rsaPrivateKey);
    
    return (encryptReturn != -1) ? encryptedDigest : QByteArray();
}