//
//  Created by Bradley Austin Davis on 2016/02/17
//  Copyright 2013-2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "StorageTests.h"

#include <memory>

QTEST_MAIN(StorageTests)

using namespace storage;

StorageTests::StorageTests() {
    for (size_t i = 0; i < _testData.size(); ++i) {
        _testData[i] = (uint8_t)rand();
    }
    _testFile = QDir::tempPath() + "/" + QUuid::createUuid().toString();
}

StorageTests::~StorageTests() {
    QFileInfo fileInfo(_testFile);
    if (fileInfo.exists()) {
        QFile(_testFile).remove();
    }
}


void StorageTests::testConversion() {
    {
        QFileInfo fileInfo(_testFile);
        QCOMPARE(fileInfo.exists(), false);
    }
    StoragePointer storagePointer = std::unique_ptr<MemoryStorage>(new MemoryStorage(_testData.size(), _testData.data()));
    QCOMPARE(storagePointer->size(), _testData.size());
    QCOMPARE(memcmp(_testData.data(), storagePointer->data(), _testData.size()), 0);
    // Convert to a file
    storagePointer = storagePointer->toFileStorage(_testFile);
    {
        QFileInfo fileInfo(_testFile);
        QCOMPARE(fileInfo.exists(), true);
        QCOMPARE(fileInfo.size(), (qint64)_testData.size());
    }
    QCOMPARE(storagePointer->size(), _testData.size());
    QCOMPARE(memcmp(_testData.data(), storagePointer->data(), _testData.size()), 0);

    // Convert to memory
    storagePointer = storagePointer->toMemoryStorage();
    QCOMPARE(storagePointer->size(), _testData.size());
    QCOMPARE(memcmp(_testData.data(), storagePointer->data(), _testData.size()), 0);
    {
        // ensure the file is unaffected
        QFileInfo fileInfo(_testFile);
        QCOMPARE(fileInfo.exists(), true);
        QCOMPARE(fileInfo.size(), (qint64)_testData.size());
    }

    // truncate the data as a new memory object
    auto newSize = _testData.size() / 2;
    storagePointer = std::unique_ptr<Storage>(new MemoryStorage(newSize, storagePointer->data()));
    QCOMPARE(storagePointer->size(), newSize);
    QCOMPARE(memcmp(_testData.data(), storagePointer->data(), newSize), 0);

    // Convert back to file
    storagePointer = storagePointer->toFileStorage(_testFile);
    QCOMPARE(storagePointer->size(), newSize);
    QCOMPARE(memcmp(_testData.data(), storagePointer->data(), newSize), 0);
    {
        // ensure the file is truncated
        QFileInfo fileInfo(_testFile);
        QCOMPARE(fileInfo.exists(), true);
        QCOMPARE(fileInfo.size(), (qint64)newSize);
    }
}
