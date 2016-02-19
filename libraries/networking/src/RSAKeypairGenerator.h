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
#include <QtCore/QUuid>

class RSAKeypairGenerator : public QObject {
    Q_OBJECT
public:
    RSAKeypairGenerator(QObject* parent = 0);

    void setDomainID(const QUuid& domainID) { _domainID = domainID; }
    const QUuid& getDomainID() const { return _domainID; }
    
public slots:
    void generateKeypair();

signals:
    void errorGeneratingKeypair();
    void generatedKeypair(const QByteArray& publicKey, const QByteArray& privateKey);

private:
    QUuid _domainID;
};

#endif // hifi_RSAKeypairGenerator_h
