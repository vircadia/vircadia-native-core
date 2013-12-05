//
//  OctreePersistThread.h
//  Octree-server
//
//  Created by Brad Hefta-Gaub on 8/21/13
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//  Threaded or non-threaded Octree persistence
//

#ifndef __Octree_server__OctreePersistThread__
#define __Octree_server__OctreePersistThread__

#include <GenericThread.h>
#include <Octree.h>

/// Generalized threaded processor for handling received inbound packets. 
class OctreePersistThread : public virtual GenericThread {
public:
    static const int DEFAULT_PERSIST_INTERVAL = 1000 * 30; // every 30 seconds

    OctreePersistThread(Octree* tree, const char* filename, int persistInterval = DEFAULT_PERSIST_INTERVAL);
    
    bool isInitialLoadComplete() const { return _initialLoadComplete; }

    time_t* getLoadCompleted() { return &_loadCompleted; }
    uint64_t getLoadElapsedTime() const { return _loadTimeUSecs; }

protected:
    /// Implements generic processing behavior for this thread.
    virtual bool process();
private:
    Octree* _tree;
    const char* _filename;
    int _persistInterval;
    bool _initialLoadComplete;

    time_t _loadCompleted;
    uint64_t _loadTimeUSecs;
    uint64_t _lastCheck;
};

#endif // __Octree_server__OctreePersistThread__
