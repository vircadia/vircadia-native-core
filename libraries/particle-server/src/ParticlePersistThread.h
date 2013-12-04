//
//  ParticlePersistThread.h
//  particle-server
//
//  Created by Brad Hefta-Gaub on 12/2/13
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//  Threaded or non-threaded particle persistence
//

#ifndef __particle_server__ParticlePersistThread__
#define __particle_server__ParticlePersistThread__

#include <GenericThread.h>
#include <NetworkPacket.h>
#include <ParticleTree.h>

/// Generalized threaded processor for handling received inbound packets. 
class ParticlePersistThread : public virtual GenericThread {
public:
    static const int DEFAULT_PERSIST_INTERVAL = 1000 * 30; // every 30 seconds

    ParticlePersistThread(ParticleTree* tree, const char* filename, int persistInterval = DEFAULT_PERSIST_INTERVAL);
    
    bool isInitialLoadComplete() const { return _initialLoadComplete; }

    time_t* getLoadCompleted() { return &_loadCompleted; }
    uint64_t getLoadElapsedTime() const { return _loadTimeUSecs; }

protected:
    /// Implements generic processing behavior for this thread.
    virtual bool process();
private:
    ParticleTree* _tree;
    const char* _filename;
    int _persistInterval;
    bool _initialLoadComplete;

    time_t _loadCompleted;
    uint64_t _loadTimeUSecs;
    uint64_t _lastCheck;
};

#endif // __particle_server__ParticlePersistThread__
