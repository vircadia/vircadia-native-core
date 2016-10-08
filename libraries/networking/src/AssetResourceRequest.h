//
//  AssetResourceRequest.h
//  libraries/networking/src
//
//  Created by Ryan Huffman on 2015/07/23
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AssetResourceRequest_h
#define hifi_AssetResourceRequest_h

#include <QUrl>

#include "AssetRequest.h"
#include "ResourceRequest.h"

class AssetResourceRequest : public ResourceRequest {
    Q_OBJECT
public:
    AssetResourceRequest(const QUrl& url) : ResourceRequest(url) { }
    virtual ~AssetResourceRequest() override;

protected:
    virtual void doSend() override;

private slots:
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void onTimeout();

private:
    void setupTimer();
    void cleanupTimer();

    bool urlIsAssetHash() const;

    void requestMappingForPath(const AssetPath& path);
    void requestHash(const AssetHash& hash);

    QTimer* _sendTimer { nullptr };

    GetMappingRequest* _assetMappingRequest { nullptr };
    AssetRequest* _assetRequest { nullptr };
};

#endif
