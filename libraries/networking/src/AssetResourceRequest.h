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

#include "ResourceRequest.h"

class AssetResourceRequest : public ResourceRequest {
    Q_OBJECT
public:
    AssetResourceRequest(QObject* parent, const QUrl& url) : ResourceRequest(parent, url) { }

protected:
    virtual void doSend() override;

private slots:
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
};

#endif
