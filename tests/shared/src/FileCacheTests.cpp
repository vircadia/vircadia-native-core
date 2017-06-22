//
//  ResoruceTests.cpp
//
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#include "FileCacheTests.h"

#include <shared/FileCache.h>

QTEST_GUILESS_MAIN(FileCacheTests)

using namespace cache;

// Limit the file size to 10 MB
static const size_t MAX_UNUSED_SIZE { 1024 * 1024 * 10 };
static const QByteArray TEST_DATA { 1024 * 1024, '0' };

static std::string getFileKey(int i) {
    return QString(QByteArray { 1, (char)i }.toHex()).toStdString();
}

size_t FileCacheTests::getCacheDirectorySize() const {
    size_t result = 0;
    QDir dir(_testDir.path());
    for (const auto& file : dir.entryList({ "*.tmp" })) {
        result += QFileInfo(dir.absoluteFilePath(file)).size();
    }
    return result;
}

FileCachePointer makeFileCache(QString& location) {
    auto result = std::make_shared<FileCache>(location.toStdString(), "tmp");
    result->initialize();
    result->setMaxSize(MAX_UNUSED_SIZE);
    return result;
}

void FileCacheTests::initTestCase() {
}

void FileCacheTests::testUnusedFiles() {
    auto cache = makeFileCache(_testDir.path());
    std::list<FilePointer> inUseFiles;

    {
        for (int i = 0; i < 100; ++i) {
            std::string key = getFileKey(i);
            auto file = cache->writeFile(TEST_DATA.data(), FileCache::Metadata(key, TEST_DATA.size()));
            QVERIFY(file->_locked);
            inUseFiles.push_back(file);
            
            QThread::msleep(10);
        }
        QCOMPARE(cache->getNumCachedFiles(), (size_t)0);
        QCOMPARE(cache->getNumTotalFiles(), (size_t)100);
        // Release the in-use files
        inUseFiles.clear();
        QCOMPARE(cache->getNumCachedFiles(), (size_t)10);
        QCOMPARE(cache->getNumTotalFiles(), (size_t)10);
        QVERIFY(getCacheDirectorySize() <= MAX_UNUSED_SIZE);
    }

    // Check behavior when destroying the cache BEFORE releasing the files
    cache = makeFileCache(_testDir.path());
    {
        auto directorySize = getCacheDirectorySize();

        // Test files 90 to 99 are present
        for (int i = 90; i < 100; ++i) {
            std::string key = getFileKey(i);
            auto file = cache->getFile(key);
            QVERIFY(file.get());
            inUseFiles.push_back(file);
        }
        cache.reset();
        inUseFiles.clear();
        QCOMPARE(getCacheDirectorySize(), directorySize);
    }

    // Reset the cache
    cache = makeFileCache(_testDir.path());
    {
        // Test files 0 to 89 are missing, because the LRU algorithm deleted them when we released the files
        for (int i = 0; i < 90; ++i) {
            std::string key = getFileKey(i);
            auto file = cache->getFile(key);
            QVERIFY(!file.get());
        }

        // Test files 90 to 99 are present
        for (int i = 90; i < 100; ++i) {
            std::string key = getFileKey(i);
            auto file = cache->getFile(key);
            QVERIFY(file.get());
            inUseFiles.push_back(file);

            if (i == 94) {
                // Each access touches the file, so we need to sleep here to ensure that the the last 5 files 
                // have later times for cache ejection priority, otherwise the test runs too fast to reliably
                // differentiate 
                QThread::msleep(1000);
            }
        }
        
        QCOMPARE(cache->getNumCachedFiles(), (size_t)0);
        QCOMPARE(cache->getNumTotalFiles(), (size_t)10);
        inUseFiles.clear();
        QCOMPARE(cache->getNumCachedFiles(), (size_t)10);
        QCOMPARE(cache->getNumTotalFiles(), (size_t)10);
    }
}

size_t FileCacheTests::getFreeSpace() const {
    return QStorageInfo(_testDir.path()).bytesFree();
}

// FIXME if something else is changing the amount of free space on the target drive concurrently with this test 
// running, then it may fail
void FileCacheTests::testFreeSpacePreservation() {
    QCOMPARE(getCacheDirectorySize(), MAX_UNUSED_SIZE);
    // Set the target free space to slightly above whatever the current free space is...
    size_t targetFreeSpace = getFreeSpace() + MAX_UNUSED_SIZE / 2;

    // Reset the cache
    auto cache = makeFileCache(_testDir.path());
    // Setting the min free space causes it to eject the oldest files that cause the cache to exceed the minimum space
    cache->setMinFreeSize(targetFreeSpace);
    QCOMPARE(cache->getNumCachedFiles(), (size_t)5);
    QCOMPARE(cache->getNumTotalFiles(), (size_t)5);
    QVERIFY(getFreeSpace() >= targetFreeSpace);
    for (int i = 0; i < 95; ++i) {
        std::string key = getFileKey(i);
        auto file = cache->getFile(key);
        QVERIFY(!file.get());
    }
    for (int i = 95; i < 100; ++i) {
        std::string key = getFileKey(i);
        auto file = cache->getFile(key);
        QVERIFY(file.get());
    }
}

void FileCacheTests::testWipe() {
    // Reset the cache
    auto cache = makeFileCache(_testDir.path());
    QCOMPARE(cache->getNumCachedFiles(), (size_t)5);
    QCOMPARE(cache->getNumTotalFiles(), (size_t)5);
    cache->wipe();
    QCOMPARE(cache->getNumCachedFiles(), (size_t)0);
    QCOMPARE(cache->getNumTotalFiles(), (size_t)0);
    QCOMPARE(getCacheDirectorySize(), (size_t)0);
}


void FileCacheTests::cleanupTestCase() {
}

