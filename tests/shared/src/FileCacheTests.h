//
//  ResourceTests.h
//
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ResourceTests_h
#define hifi_ResourceTests_h

#include <QtTest/QtTest>
#include <QtCore/QTemporaryDir>

class FileCacheTests : public QObject {
    Q_OBJECT
private slots:
    void initTestCase();
    void testUnusedFiles();
    void testFreeSpacePreservation();
    void cleanupTestCase();
    void testWipe();

private:
    size_t getFreeSpace() const;
    size_t getCacheDirectorySize() const;
    QTemporaryDir _testDir;
};

#endif // hifi_ResourceTests_h
