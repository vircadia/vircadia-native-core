//
//  Created by Bradley Austin Davis on 2016/02/17
//  Copyright 2013-2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_StorageTests_h
#define hifi_StorageTests_h

#include <QtTest/QtTest>

#include <shared/Storage.h>
#include <array>

class StorageTests : public QObject {
    Q_OBJECT

public:
    StorageTests();
    ~StorageTests();

private slots:
    void testConversion();

private:
    std::array<uint8_t, 1025> _testData;
    QString _testFile;
};

#endif // hifi_StorageTests_h
