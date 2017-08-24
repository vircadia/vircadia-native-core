//
//  Wallet.h
//  interface/src/commerce
//
//  API for secure keypair management
//
//  Created by Howard Stearns on 8/4/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Wallet_h
#define hifi_Wallet_h

#include <DependencyManager.h>

#include <QPixmap>

class Wallet : public QObject, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY

public:
    // These are currently blocking calls, although they might take a moment.
    bool createIfNeeded();
    bool generateKeyPair();
    QStringList listPublicKeys();
    QString signWithKey(const QByteArray& text, const QString& key);
    void chooseSecurityImage(const QString& imageFile);
    void getSecurityImage();
    void getKeyFilePath();

    void setSalt(const QByteArray& salt) { _salt = salt; }
    QByteArray getSalt() { return _salt; }

    void setPassphrase(const QString& passphrase);
    QString* getPassphrase() { return _passphrase; }

signals:
    void securityImageResult(bool exists) ;
    void keyFilePathResult(const QString& path);

protected:
    enum SecurityImage {
        NONE = 0,
        Cat,
        Car,
        Dog,
        Stars,
        Plane,
        Gingerbread
    };

private:
    QStringList _publicKeys{};
    QPixmap* _securityImage { nullptr };
    QByteArray _salt {"iamsalt!"};
    QString* _passphrase { new QString("pwd") };

    bool encryptFile(const QString& inputFilePath, const QString& outputFilePath);
    bool decryptFile(const QString& inputFilePath, unsigned char** outputBufferPtr, int* outputBufferLen);
};

#endif // hifi_Wallet_h
