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

#include <QtCore/QSharedPointer>

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
    
    /*@jsdoc
     * <p>A <code>WalletStatus</code> may have one of the following values:</p>
     * <table>
     *   <thead>
     *     <tr><th>Value</th><th>Meaning</th><th>Description</th></tr>
     *   </thead>
     *   <tbody>
     *     <tr><td><code>0</code></td><td>Not logged in</td><td>The user is not logged in.</td></tr>
     *     <tr><td><code>1</code></td><td>Not set up</td><td>The user's wallet has not been set up.</td></tr>
     *     <tr><td><code>2</code></td><td>Pre-existing</td><td>There is a wallet present on the server but not one 
     *       locally.</td></tr>
     *     <tr><td><code>3</code></td><td>Conflicting</td><td>There is a wallet present on the server plus one present locally, 
     *       and they don't match.</td></tr>
     *     <tr><td><code>4</code></td><td>Not authenticated</td><td>There is a wallet present locally but the user hasn't 
     *       logged into it.</td></tr>
     *     <tr><td><code>5</code></td><td>Ready</td><td>The wallet is ready for use.</td></tr>
     *   </tbody>
     * </table>
     * <p>Wallets used to be stored locally but now they're only stored on the server. A wallet is present in both places if 
     * your computer previously stored its information locally.</p>
     * @typedef {number} WalletScriptingInterface.WalletStatus
     */
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
    void sendChallengeOwnershipResponses();

private:
    friend class Ledger;
    QStringList _publicKeys{};
    QPixmap* _securityImage { nullptr };
    QByteArray _salt;
    QByteArray _iv;
    QByteArray _ckey;
    QString* _passphrase { nullptr };
    bool _isOverridingServer { false };
    std::vector<QSharedPointer<ReceivedMessage>> _pendingChallenges;

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
