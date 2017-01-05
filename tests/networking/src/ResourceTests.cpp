//
//  ResoruceTests.cpp
//
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QNetworkDiskCache>

#include "ResourceCache.h"
#include "NetworkAccessManager.h"
#include "DependencyManager.h"

#include "ResourceTests.h"

QTEST_MAIN(ResourceTests)

void ResourceTests::initTestCase() {

    auto resourceCacheSharedItems = DependencyManager::set<ResourceCacheSharedItems>();

    const qint64 MAXIMUM_CACHE_SIZE = 1024 * 1024 * 1024; // 1GB

    // set up the file cache
    //QString cachePath = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    QString cachePath = "./resourceTestCache";
    QNetworkAccessManager& networkAccessManager = NetworkAccessManager::getInstance();
    QNetworkDiskCache* cache = new QNetworkDiskCache();
    cache->setMaximumCacheSize(MAXIMUM_CACHE_SIZE);
    cache->setCacheDirectory(cachePath);
    cache->clear(); // clear the cache
    networkAccessManager.setCache(cache);
}

static QSharedPointer<Resource> resource;


void ResourceTests::downloadFirst() {
    // download the Mery fst file
    QUrl meryUrl = QUrl("http://hifi-public.s3.amazonaws.com/marketplace/contents/e21c0b95-e502-4d15-8c41-ea2fc40f1125/3585ddf674869a67d31d5964f7b52de1.fst");
    resource = QSharedPointer<Resource>::create(meryUrl);
    resource->setSelf(resource);

    const int timeout = 1000;
    QEventLoop loop;
    QTimer timer;
    timer.setInterval(timeout); // 1s, Qt::CoarseTimer acceptable
    timer.setSingleShot(true);

    connect(resource.data(), &Resource::loaded, &loop, &QEventLoop::quit);
    connect(resource.data(), &Resource::failed, &loop, &QEventLoop::quit);
    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.start();

    resource->ensureLoading();
    loop.exec();

    QVERIFY(resource->isLoaded());
}

void ResourceTests::downloadAgain() {
    // download the Mery fst file
    QUrl meryUrl = QUrl("http://hifi-public.s3.amazonaws.com/marketplace/contents/e21c0b95-e502-4d15-8c41-ea2fc40f1125/3585ddf674869a67d31d5964f7b52de1.fst");
    resource = QSharedPointer<Resource>::create(meryUrl);
    resource->setSelf(resource);

    const int timeout = 1000;
    QEventLoop loop;
    QTimer timer;
    timer.setInterval(timeout); // 1s, Qt::CoarseTimer acceptable
    timer.setSingleShot(true);

    connect(resource.data(), &Resource::loaded, &loop, &QEventLoop::quit);
    connect(resource.data(), &Resource::failed, &loop, &QEventLoop::quit);
    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.start();

    resource->ensureLoading();
    loop.exec();

    QVERIFY(resource->isLoaded());
}
