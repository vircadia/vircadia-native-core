//
//  RSAKeypairGenerator.h
//  libraries/networking/src
//
//  Created by Stephen Birarda on 2014-10-14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_RSAKeypairGenerator_h
#define hifi_RSAKeypairGenerator_h

#include <QtCore/QObject>
#include <QtCore/QRunnable>
#include <QtCore/QUuid>

class RSAKeypairGenerator : public QObject, public QRunnable {
    Q_OBJECT
public:
    RSAKeypairGenerator(QObject* parent = nullptr);

    virtual void run() override;
    void generateKeypair();

signals:
    void errorGeneratingKeypair();
    void generatedKeypair(QByteArray publicKey, QByteArray privateKey);

private:
    QUuid _domainID;
    QByteArray _publicKey;
    QByteArray _privateKey;
};

#endif // hifi_RSAKeypairGenerator_h
