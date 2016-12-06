//
//  ResourceRequest.cpp
//  libraries/networking/src
//
//  Created by Ryan Huffman on 2015/07/23
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ResourceRequest.h"

#include <QtCore/QThread>

ResourceRequest::ResourceRequest(const QUrl& url) : _url(url) { }

void ResourceRequest::send() {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "send", Qt::QueuedConnection);
        return;
    }
    
    Q_ASSERT(_state == NotStarted);

    _state = InProgress;
    doSend();
}

QString ResourceRequest::getResultString() const {
    switch (_result) {
        case Success: return "Success";
        case Error: return "Error";
        case Timeout: return "Timeout";
        case ServerUnavailable: return "Server Unavailable";
        case AccessDenied: return "Access Denied";
        case InvalidURL: return "Ivalid URL";
        case NotFound: return "Not Found";
        default: return "Unspecified Error";
    }
}
