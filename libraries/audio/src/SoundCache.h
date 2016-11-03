//
//  SoundCache.h
//  libraries/audio/src
//
//  Created by Stephen Birarda on 2014-11-13.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_SoundCache_h
#define hifi_SoundCache_h

#include <ResourceCache.h>

#include "Sound.h"

/// Scriptable interface for sound loading.
class SoundCache : public ResourceCache, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY

public:
    Q_INVOKABLE SharedSoundPointer getSound(const QUrl& url);

protected:
    virtual QSharedPointer<Resource> createResource(const QUrl& url, const QSharedPointer<Resource>& fallback,
        const void* extra) override;
private:
    SoundCache(QObject* parent = NULL);
};

#endif // hifi_SoundCache_h
