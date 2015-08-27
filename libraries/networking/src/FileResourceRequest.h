//
//  FileResourceRequest.h
//  libraries/networking/src
//
//  Created by Ryan Huffman on 2015/07/23
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_FileResourceRequest_h
#define hifi_FileResourceRequest_h

#include <QUrl>

#include "ResourceRequest.h"

class FileResourceRequest : public ResourceRequest {
    Q_OBJECT
public:
    FileResourceRequest(QObject* parent, const QUrl& url) : ResourceRequest(parent, url) { }

protected:
    virtual void doSend() override;
};

#endif
