//
//  MarketplaceItemUploader.cpp
//
//
//  Created by Ryan Huffman on 12/10/2018
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "MarketplaceItemUploader.h"

#include <AccountManager.h>
#include <DependencyManager.h>

#include <qtimer.h>
    
MarketplaceItemUploader::MarketplaceItemUploader(QUuid marketplaceID, std::vector<QString> filePaths)
    : _filePaths(filePaths), _marketplaceID(marketplaceID) {

}

void MarketplaceItemUploader::send() {
    auto accountManager = DependencyManager::get<AccountManager>();
    auto request = accountManager->createRequest("/marketplace/item", AccountManagerAuth::Required);

    QNetworkAccessManager& networkAccessManager = NetworkAccessManager::getInstance();
    QByteArray data;

    /*
    auto reply = networkAccessManager.post(request, data);

    connect(reply, &QNetworkReply::uploadProgress, this, &MarketplaceItemUploader::uploadProgress);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        auto error = reply->error();
        if (error == QNetworkReply::NoError) {
        } else {
        }
        emit complete();
    });
    */

    QTimer* timer = new QTimer();
    timer->setInterval(1000);
    connect(timer, &QTimer::timeout, this, [this, timer]() { 
        if (progress <= 1.0f) {
            progress += 0.1;
            emit uploadProgress(progress * 100.0f, 100.0f);
        } else {
            emit complete();
            timer->stop();
        }
    });
    timer->start();
}
