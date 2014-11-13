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

#include "SoundCache.h"

static int soundPointerMetaTypeId = qRegisterMetaType<SharedSoundPointer>();

SoundCache& SoundCache::getInstance() {
    static SoundCache staticInstance;
    return staticInstance;
}

SoundCache::SoundCache(QObject* parent) :
    ResourceCache(parent)
{
    
}

SharedSoundPointer SoundCache::getSound(const QUrl& url) {
    if (QThread::currentThread() != thread()) {
        SharedSoundPointer result;
        QMetaObject::invokeMethod(this, "getSound", Qt::BlockingQueuedConnection,
                                  Q_RETURN_ARG(SharedSoundPointer, result), Q_ARG(const QUrl&, url));
        return result;
    }
    qDebug() << "Requesting sound at" << url.toString() << "from SoundCache";
    return getResource(url).staticCast<Sound>();
}

QSharedPointer<Resource> SoundCache::createResource(const QUrl& url, const QSharedPointer<Resource>& fallback,
                                                    	bool delayLoad, const void* extra) {
    return QSharedPointer<Resource>(new Sound(url), &Resource::allReferencesCleared);
}