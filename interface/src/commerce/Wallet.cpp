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
#include "Application.h"
#include "ui/ImageProvider.h"
#include "scripting/HMDScriptingInterface.h"

#include <PathUtils.h>
#include <OffscreenUi.h>
#include <AccountManager.h>

#include <QFile>
#include <QCryptographicHash>
#include <QQmlContext>
#include <QBuffer>

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/rsa.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/evp.h>
#include <openssl/aes.h>

// I know, right?  But per https://www.openssl.org/docs/faq.html
// this avoids OPENSSL_Uplink(00007FF847238000,08): no OPENSSL_Applink
// at runtime.
#ifdef Q_OS_WIN
#include <openssl/applink.c>
#endif

static const char* KEY_FILE = "hifikey";
static const char* IMAGE_HEADER = "-----BEGIN SECURITY IMAGE-----\n";
static const char* IMAGE_FOOTER = "-----END SECURITY IMAGE-----\n";

void initialize() {
    static bool initialized = false;
    if (!initialized) {
        SSL_load_error_strings();
        SSL_library_init();
        OpenSSL_add_all_algorithms();
        initialized = true;
    }
}

QString keyFilePath() {
    auto accountManager = DependencyManager::get<AccountManager>();
    return PathUtils::getAppDataFilePath(QString("%1.%2").arg(accountManager->getAccountInfo().getUsername(), KEY_FILE));
}

// use the cached _passphrase if it exists, otherwise we need to prompt
int passwordCallback(char* password, int maxPasswordSize, int rwFlag, void* u) {
    // just return a hardcoded pwd for now
    auto wallet = DependencyManager::get<Wallet>();
    auto passphrase = wallet->getPassphrase();
    if (passphrase && !passphrase->isEmpty()) {
        QString saltedPassphrase(*passphrase);
        saltedPassphrase.append(wallet->getSalt());
        strcpy(password, saltedPassphrase.toUtf8().constData());
        return static_cast<int>(passphrase->size());
    } else {
        // this shouldn't happen - so lets log it to tell us we have
        // a problem with the flow...
        qCCritical(commerce) << "no cached passphrase while decrypting!";
        return 0;
    }
}

RSA* readKeys(const char* filename) {
    FILE* fp;
    RSA* key = NULL;
    if ((fp = fopen(filename, "rt"))) {
        // file opened successfully
        qCDebug(commerce) << "opened key file" << filename;
        if ((key = PEM_read_RSAPublicKey(fp, NULL, NULL, NULL))) {
            // now read private key

            qCDebug(commerce) << "read public key";

            if ((key = PEM_read_RSAPrivateKey(fp, &key, passwordCallback, NULL))) {
                qCDebug(commerce) << "read private key";
                fclose(fp);
                return key;
            }
            qCDebug(commerce) << "failed to read private key";
        } else {
            qCDebug(commerce) << "failed to read public key";
        }
        fclose(fp);
    } else {
        qCDebug(commerce) << "failed to open key file" << filename;
    }
    return key;
}

bool writeKeys(const char* filename, RSA* keys) {
    FILE* fp;
    bool retval = false;
    if ((fp = fopen(filename, "wt"))) {
        if (!PEM_write_RSAPublicKey(fp, keys)) {
            fclose(fp);
            qCDebug(commerce) << "failed to write public key";
            QFile(QString(filename)).remove();
            return retval;
        }

        if (!PEM_write_RSAPrivateKey(fp, keys, EVP_des_ede3_cbc(), NULL, 0, passwordCallback, NULL)) {
            fclose(fp);
            qCDebug(commerce) << "failed to write private key";
            QFile(QString(filename)).remove();
            return retval;
        }

        retval = true;
        qCDebug(commerce) << "wrote keys successfully";
        fclose(fp);
    } else {
        qCDebug(commerce) << "failed to open key file" << filename;
    }
    return retval;
}

// copied (without emits for various signals) from libraries/networking/src/RSAKeypairGenerator.cpp.
// We will have a different implementation in practice, but this gives us a start for now
//
// TODO: we don't really use the private keys returned - we can see how this evolves, but probably
// we should just return a list of public keys?
// or perhaps return the RSA* instead?
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


    if (!writeKeys(keyFilePath().toStdString().c_str(), keyPair)) {
        qCDebug(commerce) << "couldn't save keys!";
        return retval;
    }

    RSA_free(keyPair);

    // prepare the return values.  TODO: Fix this - we probably don't really even want the
    // private key at all (better to read it when we need it?).  Or maybe we do, when we have
    // multiple keys?
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
    if ((fp = fopen(filename, "r"))) {
        // file opened successfully
        qCDebug(commerce) << "opened key file" << filename;
        if ((key = PEM_read_RSAPublicKey(fp, NULL, NULL, NULL))) {
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
    if ((fp = fopen(filename, "r"))) {
        // file opened successfully
        qCDebug(commerce) << "opened key file" << filename;
        if ((key = PEM_read_RSAPrivateKey(fp, &key, passwordCallback, NULL))) {
            qCDebug(commerce) << "parsed private key file successfully";

        } else {
            qCDebug(commerce) << "couldn't parse" << filename;
            // if the passphrase is wrong, then let's not cache it
            DependencyManager::get<Wallet>()->setPassphrase("");
        }
        fclose(fp);
    } else {
        qCDebug(commerce) << "couldn't open" << filename;
    }
    return key;
}

// QT's QByteArray will convert to Base64 without any embedded newlines.  This just
// writes it with embedded newlines, which is more readable.
void outputBase64WithNewlines(QFile& file, const QByteArray& b64Array) {
    for (int i = 0; i < b64Array.size(); i += 64) {
        file.write(b64Array.mid(i, 64));
        file.write("\n");
    }
}

void initializeAESKeys(unsigned char* ivec, unsigned char* ckey, const QByteArray& salt) {
    // use the ones in the wallet
    auto wallet = DependencyManager::get<Wallet>();
    memcpy(ivec, wallet->getIv(), 16);
    memcpy(ckey, wallet->getCKey(), 32);
}

Wallet::~Wallet() {
    if (_securityImage) {
        delete _securityImage;
    }
}

void Wallet::setPassphrase(const QString& passphrase) {
    if (_passphrase) {
        delete _passphrase;
    }
    _passphrase = new QString(passphrase);

    _publicKeys.clear();
}

bool Wallet::writeSecurityImage(const QPixmap* pixmap, const QString& outputFilePath) {
    // aes requires a couple 128-bit keys (ckey and ivec).  For now, I'll just
    // use the md5 of the salt as the ckey (md5 is 128-bit), and ivec will be
    // a constant.  We can review this later - there are ways to generate keys
    // from a password that may be better.
    unsigned char ivec[16];
    unsigned char ckey[32];

    initializeAESKeys(ivec, ckey, _salt);

    int tempSize, outSize;
    QByteArray inputFileBuffer;
    QBuffer buffer(&inputFileBuffer);
    buffer.open(QIODevice::WriteOnly);

    // another spot where we are assuming only jpgs
    pixmap->save(&buffer, "jpg");

    // reserve enough capacity for encrypted bytes
    unsigned char* outputFileBuffer = new unsigned char[inputFileBuffer.size() + AES_BLOCK_SIZE];

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();

    // TODO: add error handling!!!
    if (!EVP_EncryptInit_ex(ctx, EVP_des_ede3_cbc(), NULL, ckey, ivec)) {
        qCDebug(commerce) << "encrypt init failure";
        delete[] outputFileBuffer;
        return false;
    }
    if (!EVP_EncryptUpdate(ctx, outputFileBuffer, &tempSize, (unsigned char*)inputFileBuffer.data(), inputFileBuffer.size())) {
        qCDebug(commerce) << "encrypt update failure";
        delete[] outputFileBuffer;
        return false;
    }
    outSize = tempSize;
    if (!EVP_EncryptFinal_ex(ctx, outputFileBuffer + outSize, &tempSize)) {
        qCDebug(commerce) << "encrypt final failure";
        delete[] outputFileBuffer;
        return false;
    }

    outSize += tempSize;
    EVP_CIPHER_CTX_free(ctx);
    qCDebug(commerce) << "encrypted buffer size" << outSize;
    QByteArray output((const char*)outputFileBuffer, outSize);

    // now APPEND to the file,
    QByteArray b64output = output.toBase64();
    QFile outputFile(outputFilePath);
    outputFile.open(QIODevice::Append);
    outputFile.write(IMAGE_HEADER);
    outputBase64WithNewlines(outputFile, b64output);
    outputFile.write(IMAGE_FOOTER);
    outputFile.close();

    delete[] outputFileBuffer;
    return true;
}

bool Wallet::readSecurityImage(const QString& inputFilePath, unsigned char** outputBufferPtr, int* outputBufferSize) {
    unsigned char ivec[16];
    unsigned char ckey[32];
    initializeAESKeys(ivec, ckey, _salt);

    // read encrypted file
    QFile inputFile(inputFilePath);
    if (!inputFile.exists()) {
        qCDebug(commerce) << "cannot decrypt file" << inputFilePath << "it doesn't exist";
        return false;
    }
    inputFile.open(QIODevice::ReadOnly | QIODevice::Text);
    bool foundHeader = false;
    bool foundFooter = false;

    QByteArray base64EncryptedBuffer;

    while (!inputFile.atEnd()) {
        QString line(inputFile.readLine());
        if (!foundHeader) {
            foundHeader = (line == IMAGE_HEADER);
        } else {
            foundFooter = (line == IMAGE_FOOTER);
            if (!foundFooter) {
                base64EncryptedBuffer.append(line);
            }
        }
    }
    inputFile.close();
    if (! (foundHeader && foundFooter)) {
        qCDebug(commerce) << "couldn't parse" << inputFilePath << foundHeader << foundFooter;
        return false;
    }

    // convert to bytes
    auto encryptedBuffer = QByteArray::fromBase64(base64EncryptedBuffer);

    // setup decrypted buffer
    unsigned char* outputBuffer = new unsigned char[encryptedBuffer.size()];
    int tempSize;

    // TODO: add error handling
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!EVP_DecryptInit_ex(ctx, EVP_des_ede3_cbc(), NULL, ckey, ivec)) {
        qCDebug(commerce) << "decrypt init failure";
        delete[] outputBuffer;
        return false;
    }
    if (!EVP_DecryptUpdate(ctx, outputBuffer, &tempSize, (unsigned char*)encryptedBuffer.data(), encryptedBuffer.size())) {
        qCDebug(commerce) << "decrypt update failure";
        delete[] outputBuffer;
        return false;
    }
    *outputBufferSize = tempSize;
    if (!EVP_DecryptFinal_ex(ctx, outputBuffer + tempSize, &tempSize)) {
        qCDebug(commerce) << "decrypt final failure";
        delete[] outputBuffer;
        return false;
    }
    EVP_CIPHER_CTX_free(ctx);
    *outputBufferSize += tempSize;
    *outputBufferPtr = outputBuffer;
    qCDebug(commerce) << "decrypted buffer size" << *outputBufferSize;
    return true;
}

bool Wallet::walletIsAuthenticatedWithPassphrase() {
    // try to read existing keys if they exist...

    // FIXME: initialize OpenSSL elsewhere soon
    initialize();

    // this should always be false if we don't have a passphrase
    // cached yet
    if (!_passphrase || _passphrase->isEmpty()) {
        return false;
    }
    if (_publicKeys.count() > 0) {
        // we _must_ be authenticated if the publicKeys are there
        return true;
    }

    // otherwise, we have a passphrase but no keys, so we have to check
    auto publicKey = readPublicKey(keyFilePath().toStdString().c_str());

    if (publicKey.size() > 0) {
        if (auto key = readPrivateKey(keyFilePath().toStdString().c_str())) {
            RSA_free(key);

            // be sure to add the public key so we don't do this over and over
            _publicKeys.push_back(publicKey.toBase64());
            return true;
        }
    }

    return false;
}

bool Wallet::generateKeyPair() {
    // FIXME: initialize OpenSSL elsewhere soon
    initialize();

    qCInfo(commerce) << "Generating keypair.";
    auto keyPair = generateRSAKeypair();

    // TODO: redo this soon -- need error checking and so on
    writeSecurityImage(_securityImage, keyFilePath());
    sendKeyFilePathIfExists();
    QString oldKey = _publicKeys.count() == 0 ? "" : _publicKeys.last();
    QString key = keyPair.first->toBase64();
    _publicKeys.push_back(key);
    qCDebug(commerce) << "public key:" << key;

    // It's arguable whether we want to change the receiveAt every time, but:
    // 1. It's certainly needed the first time, when createIfNeeded answers true.
    // 2. It is maximally private, and we can step back from that later if desired.
    // 3. It maximally exercises all the machinery, so we are most likely to surface issues now.
    auto ledger = DependencyManager::get<Ledger>();
    return ledger->receiveAt(key, oldKey);
}

QStringList Wallet::listPublicKeys() {
    qCInfo(commerce) << "Enumerating public keys.";
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
    if ((rsaPrivateKey = readPrivateKey(keyFilePath().toStdString().c_str()))) {
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
            return signature.toBase64();
        }
    }
    return QString();
}

void Wallet::updateImageProvider() {
    // inform the image provider.  Note it doesn't matter which one you inform, as the
    // images are statics
    auto engine = DependencyManager::get<OffscreenUi>()->getSurfaceContext()->engine();
    auto imageProvider = reinterpret_cast<ImageProvider*>(engine->imageProvider(ImageProvider::PROVIDER_NAME));
    imageProvider->setSecurityImage(_securityImage);
}

void Wallet::chooseSecurityImage(const QString& filename) {

    if (_securityImage) {
        delete _securityImage;
    }
    QString path = qApp->applicationDirPath();
    path.append("/resources/qml/hifi/commerce/wallet/");
    path.append(filename);

    // now create a new security image pixmap
    _securityImage = new QPixmap();

    qCDebug(commerce) << "loading data for pixmap from" << path;
    _securityImage->load(path);

    // update the image now
    updateImageProvider();

    // we could be choosing the _inital_ security image.  If so, there
    // will be no hifikey file yet.  If that is the case, we are done.  If
    // there _is_ a keyfile, we need to update it (similar to changing the
    // passphrase, we need to do so into a temp file and move it).
    if (!QFile(keyFilePath()).exists()) {
        emit securityImageResult(true);
        return;
    }

    bool success = writeWallet();
    emit securityImageResult(success);
}

void Wallet::getSecurityImage() {
    unsigned char* data;
    int dataLen;

    // if already decrypted, don't do it again
    if (_securityImage) {
        emit securityImageResult(true);
        return;
    }

    bool success = false;
    // decrypt and return.  Don't bother if we have no file to decrypt, or
    // no salt set yet.
    QFileInfo fileInfo(keyFilePath());
    if (fileInfo.exists() && _salt.size() > 0 && readSecurityImage(keyFilePath(), &data, &dataLen)) {
        // create the pixmap
        _securityImage = new QPixmap();
        _securityImage->loadFromData(data, dataLen, "jpg");
        qCDebug(commerce) << "created pixmap from encrypted file";

        updateImageProvider();

        delete[] data;
        success = true;
    }
    emit securityImageResult(success);
}
void Wallet::sendKeyFilePathIfExists() {
    QString filePath(keyFilePath());
    QFileInfo fileInfo(filePath);
    if (fileInfo.exists()) {
        emit keyFilePathIfExistsResult(filePath);
    } else {
        emit keyFilePathIfExistsResult("");
    }
}

void Wallet::reset() {
    _publicKeys.clear();

    delete _securityImage;
    _securityImage = nullptr;

    // tell the provider we got nothing
    updateImageProvider();
    _passphrase->clear();


    QFile keyFile(keyFilePath());
    keyFile.remove();
}
bool Wallet::writeWallet(const QString& newPassphrase) {
    RSA* keys = readKeys(keyFilePath().toStdString().c_str());
    if (keys) {
        // we read successfully, so now write to a new temp file
        QString tempFileName = QString("%1.%2").arg(keyFilePath(), QString("temp"));
        QString oldPassphrase = *_passphrase;
        if (!newPassphrase.isEmpty()) {
            setPassphrase(newPassphrase);
        }
        if (writeKeys(tempFileName.toStdString().c_str(), keys)) {
            if (writeSecurityImage(_securityImage, tempFileName)) {
                // ok, now move the temp file to the correct spot
                QFile(QString(keyFilePath())).remove();
                QFile(tempFileName).rename(QString(keyFilePath()));
                qCDebug(commerce) << "wallet written successfully";
                return true;
            } else {
                qCDebug(commerce) << "couldn't write security image to temp wallet";
            }
        } else {
            qCDebug(commerce) << "couldn't write keys to temp wallet";
        }
        // if we are here, we failed, so cleanup
        QFile(tempFileName).remove();
        if (!newPassphrase.isEmpty()) {
            setPassphrase(oldPassphrase);
        }

    } else {
        qCDebug(commerce) << "couldn't read wallet - bad passphrase?";
        // TODO: review this, but it seems best to reset the passphrase
        // since we couldn't decrypt the existing wallet (or is doesn't
        // exist perhaps).
        setPassphrase("");
    }
    return false;
}

bool Wallet::changePassphrase(const QString& newPassphrase) {
    qCDebug(commerce) << "changing passphrase";
    return writeWallet(newPassphrase);
}
