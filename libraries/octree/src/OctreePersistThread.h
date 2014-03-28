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

#include <QString>
#include <GenericThread.h>
#include "Octree.h"

/// Generalized threaded processor for handling received inbound packets.
class OctreePersistThread : public GenericThread {
    Q_OBJECT
public:
    static const int DEFAULT_PERSIST_INTERVAL = 1000 * 30; // every 30 seconds

    OctreePersistThread(Octree* tree, const QString& filename, int persistInterval = DEFAULT_PERSIST_INTERVAL);
    ~OctreePersistThread();

    bool isInitialLoadComplete() const { return _initialLoadComplete; }
    quint64 getLoadElapsedTime() const { return _loadTimeUSecs; }

signals:
    void loadCompleted();

protected:
    /// Implements generic processing behavior for this thread.
    virtual bool process();
private:
    Octree* _tree;
    QString _filename;
    int _persistInterval;
    bool _initialLoadComplete;

    quint64 _loadTimeUSecs;
    quint64 _lastCheck;
};

#endif // __Octree_server__OctreePersistThread__
