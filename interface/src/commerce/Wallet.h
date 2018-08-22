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
#include <Node.h>
#include <ReceivedMessage.h>
#include "scripting/WalletScriptingInterface.h"

#include <QPixmap>

class Wallet : public QObject, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY

public:
    Wallet();
    ~Wallet();
    // These are currently blocking calls, although they might take a moment.
    bool generateKeyPair();
    QStringList listPublicKeys();
    QString signWithKey(const QByteArray& text, const QString& key);
    void chooseSecurityImage(const QString& imageFile);
    bool getSecurityImage();
    QString getKeyFilePath();
    bool copyKeyFileFrom(const QString& pathname);

    void setSalt(const QByteArray& salt) { _salt = salt; }
    QByteArray getSalt() { return _salt; }
    void setIv(const QByteArray& iv) { _iv = iv; }
    QByteArray getIv() { return _iv; }
    void setCKey(const QByteArray& ckey) { _ckey = ckey; }
    QByteArray getCKey() { return _ckey; }

    bool setPassphrase(const QString& passphrase);
    QString* getPassphrase() { return _passphrase; }
    bool getPassphraseIsCached() { return !(_passphrase->isEmpty()); }
    bool walletIsAuthenticatedWithPassphrase();
    bool changePassphrase(const QString& newPassphrase);
    void setSoftReset() { _isOverridingServer = true; }
    bool wasSoftReset() { bool was = _isOverridingServer; _isOverridingServer = false; return was; }
    void clear();

    void getWalletStatus();
    enum WalletStatus {
        WALLET_STATUS_NOT_LOGGED_IN = 0,
        WALLET_STATUS_NOT_SET_UP,
        WALLET_STATUS_PREEXISTING,
        WALLET_STATUS_CONFLICTING,
        WALLET_STATUS_NOT_AUTHENTICATED,
        WALLET_STATUS_READY
    };

signals:
    void securityImageResult(bool exists);
    void keyFilePathIfExistsResult(const QString& path);

    void walletStatusResult(uint walletStatus);

private slots:
    void handleChallengeOwnershipPacket(QSharedPointer<ReceivedMessage> packet, SharedNodePointer sendingNode);

private:
    friend class Ledger;
    QStringList _publicKeys{};
    QPixmap* _securityImage { nullptr };
    QByteArray _salt;
    QByteArray _iv;
    QByteArray _ckey;
    QString* _passphrase { nullptr };
    bool _isOverridingServer { false };

    bool writeWallet(const QString& newPassphrase = QString(""));
    void updateImageProvider();
    bool writeSecurityImage(const QPixmap* pixmap, const QString& outputFilePath);
    bool readSecurityImage(const QString& inputFilePath, unsigned char** outputBufferPtr, int* outputBufferLen);
    bool writeBackupInstructions();

    bool setWallet(const QByteArray& wallet);
    QByteArray getWallet();

    void account();
};

#endif // hifi_Wallet_h
