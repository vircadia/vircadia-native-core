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
#include "ResourceRequestObserver.h"

#include <DependencyManager.h>
#include <StatTracker.h>

#include <QtCore/QDateTime>
#include <QtCore/QThread>

QString ResourceRequest::toHttpDateString(uint64_t msecsSinceEpoch) {
    return QDateTime::fromMSecsSinceEpoch(msecsSinceEpoch)
        .toString("ddd, dd MMM yyyy hh:mm:ss 'GMT'")
        .toLatin1();
}

void ResourceRequest::send() {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "send", Qt::QueuedConnection);
        return;
    }

    if (_isObservable) {
        DependencyManager::get<ResourceRequestObserver>()->update(_url, _callerId, _extra + " => ResourceRequest::send");
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
        case InvalidURL: return "Invalid URL";
        case NotFound: return "Not Found";
        case RedirectFail: return "Redirect Fail";
        default: return "Unspecified Error";
    }
}

void ResourceRequest::recordBytesDownloadedInStats(const QString& statName, int64_t bytesReceived) {
    auto dBytes = bytesReceived - _lastRecordedBytesDownloaded;
    if (dBytes > 0) {
        _lastRecordedBytesDownloaded = bytesReceived;
        DependencyManager::get<StatTracker>()->updateStat(statName, dBytes);
    }
}
