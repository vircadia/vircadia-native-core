//
//  HTTPResourceRequest.h
//  libraries/networking/src
//
//  Created by Ryan Huffman on 2015/07/23
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_HTTPResourceRequest_h
#define hifi_HTTPResourceRequest_h

#include <QNetworkReply>
#include <QUrl>
#include <QTimer>

#include "ResourceRequest.h"

class HTTPResourceRequest : public ResourceRequest {
    Q_OBJECT
public:
    HTTPResourceRequest(const QUrl& url) : ResourceRequest(url) { }
    ~HTTPResourceRequest();

protected:
    virtual void doSend() override;

private slots:
    void onTimeout();
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void onRequestFinished();

private:
    void setupTimer();
    void cleanupTimer();

    QTimer* _sendTimer { nullptr };
    QNetworkReply* _reply { nullptr };
};

#endif
