//
//  ResoruceTests.cpp
//
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#include "FileCacheTests.h"

#include <FileCache.h>

QTEST_GUILESS_MAIN(FileCacheTests)

using namespace cache;

// Limit the file size to 10 MB
static const size_t MAX_UNUSED_SIZE { 1024 * 1024 * 10 };
static const QByteArray TEST_DATA { 1024 * 1024, '0' };

static std::string getFileKey(int i) {
    return QString(QByteArray { 1, (char)i }.toHex()).toStdString();
}

class TestFile : public File {
    using Parent = File;
public:
    TestFile(Metadata&& metadata, const std::string& filepath)
        : Parent(std::move(metadata), filepath) {
    }
};

class TestFileCache : public FileCache {
    using Parent = FileCache;

public:
    TestFileCache(const std::string& dirname, const std::string& ext, QObject* parent = nullptr) : Parent(dirname, ext, nullptr) {
        initialize();
    }

    std::unique_ptr<File> createFile(Metadata&& metadata, const std::string& filepath) override {
        qCInfo(file_cache) << "Wrote KTX" << metadata.key.c_str();
        return std::unique_ptr<File>(new TestFile(std::move(metadata), filepath));
    }
};

using CachePointer = std::shared_ptr<TestFileCache>;

// The FileCache relies on deleteLater to clear unused files, but QTest classes don't run a conventional event loop
// so we need to call this function to force any pending deletes to occur in the File destructor
static void forceDeletes() {
    while (QCoreApplication::hasPendingEvents()) {
        QCoreApplication::sendPostedEvents();
        QCoreApplication::sendPostedEvents(0, QEvent::DeferredDelete);
        QCoreApplication::processEvents();
    }
}

size_t FileCacheTests::getCacheDirectorySize() const {
    size_t result = 0;
    QDir dir(_testDir.path());
    for (const auto& file : dir.entryList({ "*.tmp" })) {
        result += QFileInfo(dir.absoluteFilePath(file)).size();
    }
    return result;
}

CachePointer makeFileCache(QString& location) {
    auto result = std::make_shared<TestFileCache>(location.toStdString(), "tmp");
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
            auto file = cache->writeFile(TEST_DATA.data(), TestFileCache::Metadata(key, TEST_DATA.size()));
            inUseFiles.push_back(file);
            forceDeletes();
            QThread::msleep(10);
        }
        QCOMPARE(cache->getNumCachedFiles(), (size_t)0);
        QCOMPARE(cache->getNumTotalFiles(), (size_t)100);
        // Release the in-use files
        inUseFiles.clear();
        // Cache state is updated, but the directory state is unchanged, 
        // because the file deletes are triggered by an event loop
        QCOMPARE(cache->getNumCachedFiles(), (size_t)10);
        QCOMPARE(cache->getNumTotalFiles(), (size_t)10);
        QVERIFY(getCacheDirectorySize() >  MAX_UNUSED_SIZE);
        forceDeletes();
        QCOMPARE(cache->getNumCachedFiles(), (size_t)10);
        QCOMPARE(cache->getNumTotalFiles(), (size_t)10);
        QVERIFY(getCacheDirectorySize() <= MAX_UNUSED_SIZE);
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
    QVERIFY(getFreeSpace() < targetFreeSpace);
    forceDeletes();
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
    QVERIFY(getCacheDirectorySize() > 0);
    forceDeletes();
    QCOMPARE(getCacheDirectorySize(), (size_t)0);
}


void FileCacheTests::cleanupTestCase() {
}

