//
//  RSAKeypairGenerator.cpp
//  libraries/networking/src
//
//  Created by Stephen Birarda on 2014-10-14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <openssl/err.h>
#include <openssl/rsa.h>
#include <openssl/x509.h>

#include <qdebug.h>

#include "NetworkLogging.h"

#include "RSAKeypairGenerator.h"
#ifdef __clang__
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif

RSAKeypairGenerator::RSAKeypairGenerator(QObject* parent) :
    QObject(parent)
{
    
}

void RSAKeypairGenerator::generateKeypair() {
    
    RSA* keyPair = RSA_new();
    BIGNUM* exponent = BN_new();
    
    const unsigned long RSA_KEY_EXPONENT = 65537;
    BN_set_word(exponent, RSA_KEY_EXPONENT);
    
    // seed the random number generator before we call RSA_generate_key_ex
    srand(time(NULL));
    
    const int RSA_KEY_BITS = 2048;
    
    if (!RSA_generate_key_ex(keyPair, RSA_KEY_BITS, exponent, NULL)) {
        qCDebug(networking) << "Error generating 2048-bit RSA Keypair -" << ERR_get_error();
        
        emit errorGeneratingKeypair();
        
        // we're going to bust out of here but first we cleanup the BIGNUM
        BN_free(exponent);
        return;
    }
    
    // we don't need the BIGNUM anymore so clean that up
    BN_free(exponent);
    
    // grab the public key and private key from the file
    unsigned char* publicKeyDER = NULL;
    int publicKeyLength = i2d_RSAPublicKey(keyPair, &publicKeyDER);
    
    unsigned char* privateKeyDER = NULL;
    int privateKeyLength = i2d_RSAPrivateKey(keyPair, &privateKeyDER);
    
    if (publicKeyLength <= 0 || privateKeyLength <= 0) {
        qCDebug(networking) << "Error getting DER public or private key from RSA struct -" << ERR_get_error();
        
        emit errorGeneratingKeypair();
        
        // cleanup the RSA struct
        RSA_free(keyPair);
        
        // cleanup the public and private key DER data, if required
        if (publicKeyLength > 0) {
            OPENSSL_free(publicKeyDER);
        }
        
        if (privateKeyLength > 0) {
            OPENSSL_free(privateKeyDER);
        }
        
        return;
    }
    
    // we have the public key and private key in memory
    // we can cleanup the RSA struct before we continue on
    RSA_free(keyPair);
    
    _publicKey = QByteArray { reinterpret_cast<char*>(publicKeyDER), publicKeyLength };
    _privateKey = QByteArray { reinterpret_cast<char*>(privateKeyDER), privateKeyLength };
    
    // cleanup the publicKeyDER and publicKeyDER data
    OPENSSL_free(publicKeyDER);
    OPENSSL_free(privateKeyDER);
    
    emit generatedKeypair();
}
