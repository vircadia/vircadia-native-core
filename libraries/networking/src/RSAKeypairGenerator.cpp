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
#include <openssl/pem.h>

#include <qdebug.h>

#include "RSAKeypairGenerator.h"

void RSAKeypairGenerator::generateKeypair() {
    
    RSA* keyPair = RSA_new();
    BIGNUM* exponent = BN_new();
    
    const unsigned long RSA_KEY_EXPONENT = 65537;
    BN_set_word(exponent, RSA_KEY_EXPONENT);
    
    // seed the random number generator before we call RSA_generate_key_ex
    srand(time(NULL));
    
    const int RSA_KEY_BITS = 2048;
    
    if (!RSA_generate_key_ex(keyPair, RSA_KEY_BITS, exponent, NULL)) {
        qDebug() << "Error generating 2048-bit RSA Keypair -" << ERR_get_error();
        
        emit errorGeneratingKeypair();
        
        // we're going to bust out of here but first we cleanup the BIGNUM
        BN_free(exponent);
        return;
    }
    
    // we don't need the BIGNUM anymore so clean that up
    BN_free(exponent);
    
    // grab the public key and private key from the file
    BIO *privateKeyBIO = BIO_new(BIO_s_mem());
    int privateWrite = PEM_write_bio_RSAPrivateKey(privateKeyBIO, keyPair, NULL, NULL, 0, NULL, NULL);
    
    BIO *publicKeyBIO = BIO_new(BIO_s_mem());
    int publicWrite = PEM_write_bio_RSAPublicKey(publicKeyBIO, keyPair);
    
    if (privateWrite == 0 || publicWrite == 0) {
        // we had a error grabbing either the private or public key from the RSA
        
        // bubble up our error
        emit errorGeneratingKeypair();
        
        // cleanup the RSA struct
        RSA_free(keyPair);
        
        // cleanup the BIOs
        BIO_free(privateKeyBIO);
        BIO_free(publicKeyBIO);
        
        return;
    }
    
    // we have the public key and private key in memory
    // we can cleanup the RSA struct before we continue on
    RSA_free(keyPair);
    
    char* publicKeyData;
    int publicKeyLength = BIO_get_mem_data(publicKeyBIO, &publicKeyData);
    
    char* privateKeyData;
    int privateKeyLength = BIO_get_mem_data(privateKeyBIO, &privateKeyData);
    
    QByteArray publicKeyArray(publicKeyData, publicKeyLength);
    QByteArray privateKeyArray(privateKeyData, privateKeyLength);
    
    emit generatedKeypair(publicKeyArray, privateKeyArray);
}