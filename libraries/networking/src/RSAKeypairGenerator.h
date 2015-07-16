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

#include <qobject.h>

class RSAKeypairGenerator : public QObject {
    Q_OBJECT
public:
    RSAKeypairGenerator(QObject* parent = 0);
public slots:
    void generateKeypair();
signals:
    void errorGeneratingKeypair();
    void generatedKeypair(const QByteArray& publicKey, const QByteArray& privateKey);
};

#endif // hifi_RSAKeypairGenerator_h