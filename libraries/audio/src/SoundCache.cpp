//
//  SoundCache.cpp
//  libraries/audio/src
//
//  Created by Stephen Birarda on 2014-11-13.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <qthread.h>

#include "AudioLogging.h"
#include "SoundCache.h"

int soundPointerMetaTypeId = qRegisterMetaType<SharedSoundPointer>();

SoundCache::SoundCache(QObject* parent) :
    ResourceCache(parent)
{
    const qint64 SOUND_DEFAULT_UNUSED_MAX_SIZE = 50 * BYTES_PER_MEGABYTES;
    setUnusedResourceCacheSize(SOUND_DEFAULT_UNUSED_MAX_SIZE);
    setObjectName("SoundCache");
}

SharedSoundPointer SoundCache::getSound(const QUrl& url) {
    if (QThread::currentThread() != thread()) {
        SharedSoundPointer result;
        QMetaObject::invokeMethod(this, "getSound", Qt::BlockingQueuedConnection,
                                  Q_RETURN_ARG(SharedSoundPointer, result), Q_ARG(const QUrl&, url));
        return result;
    }
    return getResource(url).staticCast<Sound>();
}

QSharedPointer<Resource> SoundCache::createResource(const QUrl& url, const QSharedPointer<Resource>& fallback,
    const void* extra) {
    qCDebug(audio) << "Requesting sound at" << url.toString();
    return QSharedPointer<Resource>(new Sound(url), &Resource::deleter);
}
