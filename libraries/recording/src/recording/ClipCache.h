//
//  Created by Bradley Austin Davis on 2015/11/19
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once
#ifndef hifi_Recording_ClipCache_h
#define hifi_Recording_ClipCache_h

#include <ResourceCache.h>

#include "Forward.h"
#include "impl/PointerClip.h"

namespace recording {

class NetworkClip : public PointerClip {
public:
    using Pointer = std::shared_ptr<NetworkClip>;

    NetworkClip(const QUrl& url) : _url(url) {}
    virtual void init(const QByteArray& clipData);
    virtual QString getName() const override { return _url.toString(); }

private:
    QByteArray _clipData;
    QUrl _url;
};

class NetworkClipLoader : public Resource {
public:
    NetworkClipLoader(const QUrl& url);
    virtual void downloadFinished(const QByteArray& data) override;
    ClipPointer getClip() { return _clip; }
    bool completed() { return _failedToLoad || isLoaded(); }

private:
    const NetworkClip::Pointer _clip;
};

using NetworkClipLoaderPointer = QSharedPointer<NetworkClipLoader>;

class ClipCache : public ResourceCache {
public:
    static ClipCache& instance();

    NetworkClipLoaderPointer getClipLoader(const QUrl& url);

protected:
    virtual QSharedPointer<Resource> createResource(const QUrl& url, const QSharedPointer<Resource>& fallback, const void* extra) override;
};

}

#endif
