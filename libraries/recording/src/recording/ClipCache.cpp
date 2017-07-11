//
//  Created by Bradley Austin Davis on 2015/11/19
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QThread>

#include <shared/QtHelpers.h>

#include "ClipCache.h"
#include "impl/PointerClip.h"
#include "Logging.h"

using namespace recording;
NetworkClipLoader::NetworkClipLoader(const QUrl& url) :
    Resource(url),
    _clip(std::make_shared<NetworkClip>(url)) {}

void NetworkClip::init(const QByteArray& clipData) {
    _clipData = clipData;
    PointerClip::init((uchar*)_clipData.data(), _clipData.size());
}

void NetworkClipLoader::downloadFinished(const QByteArray& data) {
    _clip->init(data);
    finishedLoading(true);
    emit clipLoaded();
}

ClipCache::ClipCache(QObject* parent) :
    ResourceCache(parent)
{
    
}

NetworkClipLoaderPointer ClipCache::getClipLoader(const QUrl& url) {
    if (QThread::currentThread() != thread()) {
        NetworkClipLoaderPointer result;
        BLOCKING_INVOKE_METHOD(this, "getClipLoader",
                                  Q_RETURN_ARG(NetworkClipLoaderPointer, result), Q_ARG(const QUrl&, url));
        return result;
    }

    return getResource(url).staticCast<NetworkClipLoader>();
}

QSharedPointer<Resource> ClipCache::createResource(const QUrl& url, const QSharedPointer<Resource>& fallback, const void* extra) {
    qCDebug(recordingLog) << "Loading recording at" << url;
    return QSharedPointer<Resource>(new NetworkClipLoader(url), &Resource::deleter);
}

