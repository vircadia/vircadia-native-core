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

typedef QSharedPointer<Sound> SharedSoundPointer;

/// Scriptable interface for FBX animation loading.
class SoundCache : public ResourceCache {
    Q_OBJECT
public:
    SoundCache(QObject* parent = NULL);
    
    Q_INVOKABLE SharedSoundPointer getSound(const QUrl& url);
    
protected:
    
    virtual QSharedPointer<Resource> createResource(const QUrl& url,
                                                    const QSharedPointer<Resource>& fallback, bool delayLoad, const void* extra);
};

Q_DECLARE_METATYPE(SharedSoundPointer)

#endif // hifi_SoundCache_h