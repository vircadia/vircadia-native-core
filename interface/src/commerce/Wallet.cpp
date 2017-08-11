//
//  Wallet.cpp
//  interface/src/commerce
//
//  Created by Howard Stearns on 8/4/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "CommerceLogging.h"
#include "Ledger.h"
#include "Wallet.h"

#include <PathUtils.h>

#include <QCryptographicHash.h>

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/rsa.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/evp.h>

static const char* PUBLIC_KEY_FILE = "hifikey.pub";
static const char* PRIVATE_KEY_FILE = "hifikey";

void initialize() {
    static bool initialized = false;
    if (!initialized) {
        SSL_load_error_strings();
        SSL_library_init();
        OpenSSL_add_all_algorithms();
        initialized = true;
    }
}

QString publicKeyFilePath() {
    return PathUtils::getAppDataFilePath(PUBLIC_KEY_FILE);
}

QString privateKeyFilePath() {
    return PathUtils::getAppDataFilePath(PRIVATE_KEY_FILE);
}
// for now the callback function just returns the same string.  Later we can hook
// this to the gui (some thought required)

int passwordCallback(char* password, int maxPasswordSize, int rwFlag, void* u) {
    // just return a hardcoded pwd for now
    static const char* pwd = "pwd";
    strcpy(password, pwd);
    return strlen(pwd);
}

// BEGIN copied code - this will be removed/changed at some point soon
// copied (without emits for various signals) from libraries/networking/src/RSAKeypairGenerator.cpp.
// We will have a different implementation in practice, but this gives us a start for now
QPair<QByteArray*, QByteArray*> generateRSAKeypair() {

    RSA* keyPair = RSA_new();
    BIGNUM* exponent = BN_new();
    QPair<QByteArray*, QByteArray*> retval;

    const unsigned long RSA_KEY_EXPONENT = 65537;
    BN_set_word(exponent, RSA_KEY_EXPONENT);

    // seed the random number generator before we call RSA_generate_key_ex
    srand(time(NULL));

    const int RSA_KEY_BITS = 2048;

    if (!RSA_generate_key_ex(keyPair, RSA_KEY_BITS, exponent, NULL)) {
        qCDebug(commerce) << "Error generating 2048-bit RSA Keypair -" << ERR_get_error();

        // we're going to bust out of here but first we cleanup the BIGNUM
        BN_free(exponent);
        return retval;
    }

    // we don't need the BIGNUM anymore so clean that up
    BN_free(exponent);

    // grab the public key and private key from the file
    unsigned char* publicKeyDER = NULL;
    int publicKeyLength = i2d_RSAPublicKey(keyPair, &publicKeyDER);

    unsigned char* privateKeyDER = NULL;
    int privateKeyLength = i2d_RSAPrivateKey(keyPair, &privateKeyDER);

    if (publicKeyLength <= 0 || privateKeyLength <= 0) {
        qCDebug(commerce) << "Error getting DER public or private key from RSA struct -" << ERR_get_error();


        // cleanup the RSA struct
        RSA_free(keyPair);

        // cleanup the public and private key DER data, if required
        if (publicKeyLength > 0) {
            OPENSSL_free(publicKeyDER);
        }

        if (privateKeyLength > 0) {
            OPENSSL_free(privateKeyDER);
        }

        return retval;
    }



    // now lets persist them to files
    // TODO: figure out a scheme for multiple keys, etc...
    FILE* fp;
    if (fp = fopen(publicKeyFilePath().toStdString().c_str(), "wt")) {
        if (!PEM_write_RSAPublicKey(fp, keyPair)) {
            fclose(fp);
            qCDebug(commerce) << "failed to write public key";
            return retval;
        }
        fclose(fp);
    }

    if (fp = fopen(privateKeyFilePath().toStdString().c_str(), "wt")) {
        char pwd[] = "pwd";
        if (!PEM_write_RSAPrivateKey(fp, keyPair, EVP_des_ede3_cbc(), NULL, 0, passwordCallback, NULL)) {
            fclose(fp);
            qCDebug(commerce) << "failed to write private key";
            return retval;
        }
        fclose(fp);
    }

    RSA_free(keyPair);

    // prepare the return values
    retval.first = new QByteArray(reinterpret_cast<char*>(publicKeyDER), publicKeyLength ),
    retval.second = new QByteArray(reinterpret_cast<char*>(privateKeyDER), privateKeyLength );

    // cleanup the publicKeyDER and publicKeyDER data
    OPENSSL_free(publicKeyDER);
    OPENSSL_free(privateKeyDER);
    return retval;
}
// END copied code (which will soon change)

// the public key can just go into a byte array
QByteArray readPublicKey(const char* filename) {
    FILE* fp;
    RSA* key = NULL;
    if ( fp = fopen(filename, "r")) {
        // file opened successfully
        qCDebug(commerce) << "opened key file" << filename;
        if (key = PEM_read_RSAPublicKey(fp, NULL, NULL, NULL)) {
            // file read successfully
            unsigned char* publicKeyDER = NULL;
            int publicKeyLength = i2d_RSAPublicKey(key, &publicKeyDER);
            // TODO: check for 0 length?

            // cleanup
            RSA_free(key);
            fclose(fp);

            qCDebug(commerce) << "parsed public key file successfully";

            QByteArray retval((char*)publicKeyDER, publicKeyLength);
            OPENSSL_free(publicKeyDER);
            return retval;
        } else {
            qCDebug(commerce) << "couldn't parse" << filename;
        }
        fclose(fp);
    } else {
        qCDebug(commerce) << "couldn't open" << filename;
    }
    return QByteArray();
}

// the private key should be read/copied into heap memory.  For now, we need the RSA struct
// so I'll return that.  Note we need to RSA_free(key) later!!!
RSA* readPrivateKey(const char* filename) {
    FILE* fp;
    RSA* key = NULL;
    if ( fp = fopen(filename, "r")) {
        // file opened successfully
        qCDebug(commerce) << "opened key file" << filename;
        if (key = PEM_read_RSAPrivateKey(fp, &key, passwordCallback, NULL)) {
            // cleanup
            fclose(fp);

            qCDebug(commerce) << "parsed private key file successfully";
            return key;

        } else {
            qCDebug(commerce) << "couldn't parse" << filename;
        }
        fclose(fp);
    } else {
        qCDebug(commerce) << "couldn't open" << filename;
    }
    return false;
}

bool Wallet::createIfNeeded() {
    if (_publicKeys.count() > 0) return false;

    // FIXME: initialize OpenSSL elsewhere soon
    initialize();

    // try to read existing keys if they exist...
    auto publicKey = readPublicKey(publicKeyFilePath().toStdString().c_str());
    if (publicKey.size() > 0) {
        if (auto key = readPrivateKey(privateKeyFilePath().toStdString().c_str()) ) {
            qCDebug(commerce) << "read private key";
            RSA_free(key);
            // K -- add the public key since we have a legit private key associated with it
            _publicKeys.push_back(publicKey.toBase64());
            return false;
        }
    }
    qCInfo(commerce) << "Creating wallet.";
    return generateKeyPair();
}

bool Wallet::generateKeyPair() {
    qCInfo(commerce) << "Generating keypair.";
    auto keyPair = generateRSAKeypair();

    // TODO: do we need to pass params in here to make sure this is url-safe base64?
    _publicKeys.push_back(keyPair.first->toBase64());
    qCDebug(commerce) << "public key:" << keyPair.first->toBase64();

    auto ledger = DependencyManager::get<Ledger>();
    return ledger->receiveAt(_publicKeys.last());
}
QStringList Wallet::listPublicKeys() {
    qCInfo(commerce) << "Enumerating public keys.";
    createIfNeeded();
    return _publicKeys;
}

// for now a copy of how we sign in libraries/networking/src/DataServerAccountInfo -
// we sha256 the text, read the private key from disk (for now!), and return the signed
// sha256.  Note later with multiple keys, we may need the key parameter (or something
// similar) so I left it alone for now.  Also this will probably change when we move
// away from RSA keys anyways.  Note that since this returns a QString, we better avoid
// the horror of code pages and so on (changing the bytes) by just returning a base64
// encoded string representing the signature (suitable for http, etc...)
QString Wallet::signWithKey(const QByteArray& text, const QString& key) {
    qCInfo(commerce) << "Signing text.";
    RSA* rsaPrivateKey = NULL;
    if (rsaPrivateKey = readPrivateKey(privateKeyFilePath().toStdString().c_str())) {
        QByteArray signature(RSA_size(rsaPrivateKey), 0);
        unsigned int signatureBytes = 0;

        QByteArray hashedPlaintext = QCryptographicHash::hash(text, QCryptographicHash::Sha256);

        int encryptReturn = RSA_sign(NID_sha256,
                reinterpret_cast<const unsigned char*>(hashedPlaintext.constData()),
                hashedPlaintext.size(),
                reinterpret_cast<unsigned char*>(signature.data()),
                &signatureBytes,
                rsaPrivateKey);

        // free the private key RSA struct now that we are done with it
        RSA_free(rsaPrivateKey);

        if (encryptReturn != -1) {
            // TODO: do we need to pass options in here to make sure it is url-safe?
            return signature.toBase64();
        }
    }
    return QString();
}

