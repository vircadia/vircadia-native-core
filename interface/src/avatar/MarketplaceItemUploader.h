//
//  MarketplaceItemUploader.h
//
//
//  Created by Ryan Huffman on 12/10/2018
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_MarketplaceItemUploader_h
#define hifi_MarketplaceItemUploader_h

#include <QObject>
#include <QUuid>

class QNetworkReply;

class MarketplaceItemUploader : public QObject {
    Q_OBJECT
public:
    enum class Error
    {
        None,
        ItemNotUpdateable,
        ItemDoesNotExist,
        RequestTimedOut,
        Unknown
    };
    enum class State
    {
        Ready,
        Sent
    };

    MarketplaceItemUploader(QUuid markertplaceID, std::vector<QString> filePaths);

    float progress{ 0.0f };

    Q_INVOKABLE void send();

signals:
    void uploadProgress(qint64 bytesSent, qint64 bytesTotal);
    void complete();

private:

    QNetworkReply* _reply;
    QUuid _marketplaceID;
    std::vector<QString> _filePaths;
};

#endif  // hifi_MarketplaceItemUploader_h
