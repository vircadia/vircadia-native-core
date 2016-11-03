//
//  AudioInjectorManager.h
//  libraries/audio/src
//
//  Created by Stephen Birarda on 2015-11-16.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once

#ifndef hifi_AudioInjectorManager_h
#define hifi_AudioInjectorManager_h

#include <condition_variable>
#include <queue>
#include <mutex>

#include <QtCore/QPointer>
#include <QtCore/QThread>

#include <DependencyManager.h>

class AudioInjector;

class AudioInjectorManager : public QObject, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY
public:
    ~AudioInjectorManager();
private slots:
    void run();
private:
    
    using InjectorQPointer = QPointer<AudioInjector>;
    using TimeInjectorPointerPair = std::pair<uint64_t, InjectorQPointer>;
    
    struct greaterTime {
        bool operator() (const TimeInjectorPointerPair& x, const TimeInjectorPointerPair& y) const {
            return x.first > y.first;
        }
    };
    
    using InjectorQueue = std::priority_queue<TimeInjectorPointerPair,
                                              std::deque<TimeInjectorPointerPair>,
                                              greaterTime>;
    using Mutex = std::mutex;
    using Lock = std::unique_lock<Mutex>;
    
    bool threadInjector(AudioInjector* injector);
    bool restartFinishedInjector(AudioInjector* injector);
    void notifyInjectorReadyCondition() { _injectorReady.notify_one(); }
    bool wouldExceedLimits();
    
    AudioInjectorManager() {};
    AudioInjectorManager(const AudioInjectorManager&) = delete;
    AudioInjectorManager& operator=(const AudioInjectorManager&) = delete;
    
    void createThread();
    
    QThread* _thread { nullptr };
    bool _shouldStop { false };
    InjectorQueue _injectors;
    Mutex _injectorsMutex;
    std::condition_variable _injectorReady;
    
    friend class AudioInjector;
};



#endif // hifi_AudioInjectorManager_h
