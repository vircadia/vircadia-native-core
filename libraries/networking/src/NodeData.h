//
//  NodeData.h
//  libraries/networking/src
//
//  Created by Stephen Birarda on 2/19/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_NodeData_h
#define hifi_NodeData_h

#include <QtCore/QMutex>
#include <QtCore/QObject>

class Node;

class NodeData : public QObject {
    Q_OBJECT
public:
    NodeData();
    virtual ~NodeData() = 0;
    virtual int parseData(const QByteArray& packet) = 0;
    
    QMutex& getMutex() { return _mutex; }

    void setCanAdjustLocks(bool canAdjustLocks) { _canAdjustLocks = canAdjustLocks; }
    bool getCanAdjustLocks() { return _canAdjustLocks; }

    void setCanRez(bool canRez) { _canRez = canRez; }
    bool getCanRez() { return _canRez; }

private:
    QMutex _mutex;

protected:
    bool _canAdjustLocks = false; /// will this node be allowed to adjust locks on entities?
    bool _canRez = false; /// will this node be allowed to rez in new entities?
};

#endif // hifi_NodeData_h
