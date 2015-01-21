//
//  NetworkAccessManager.cpp
//
//
//  Created by Clement on 7/1/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QNetworkDiskCache>
#include <QStandardPaths>
#include <QThreadStorage>

#include "NetworkAccessManager.h"

const qint64 MAXIMUM_CACHE_SIZE = 10737418240;  // 10GB

QThreadStorage<QNetworkAccessManager*> networkAccessManagers;

QNetworkAccessManager& NetworkAccessManager::getInstance() {
    if (!networkAccessManagers.hasLocalData()) {
        QNetworkAccessManager* networkAccessManager = new QNetworkAccessManager();
        
        QString cachePath = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
        QNetworkDiskCache* cache = new QNetworkDiskCache();
        cache->setMaximumCacheSize(MAXIMUM_CACHE_SIZE);
        cache->setCacheDirectory(!cachePath.isEmpty() ? cachePath : "interfaceCache");
        networkAccessManager->setCache(cache);
        
        networkAccessManagers.setLocalData(networkAccessManager);
    }
    
    return *networkAccessManagers.localData();
}
