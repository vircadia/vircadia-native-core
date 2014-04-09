//
//  NodeData.h
//  libraries/shared/src
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

private:
    QMutex _mutex;
};

#endif
