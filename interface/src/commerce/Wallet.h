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

class Wallet : public QObject, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY

public:
    // These are currently blocking calls, although they might take a moment.
    bool createIfNeeded();
    bool generateKeyPair();
    QStringList listPublicKeys();
    QString signWithKey(const QByteArray& text, const QString& key);
    void chooseSecurityImage(uint imageID);
    void getSecurityImage();

signals:
    void securityImageResult(uint imageID);

protected:
    // ALWAYS add SecurityImage enum values to the END of the enum.
    // They must be in the same order as the images are listed in
    //     SecurityImageSelection.qml
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
    SecurityImage _chosenSecurityImage = SecurityImage::NONE;
};

#endif // hifi_Wallet_h
