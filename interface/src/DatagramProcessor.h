//
//  DatagramProcessor.h
//  interface/src
//
//  Created by Stephen Birarda on 1/23/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef __hifi__DatagramProcessor__
#define __hifi__DatagramProcessor__

#include <QtCore/QObject>

class DatagramProcessor : public QObject {
    Q_OBJECT
public:
    DatagramProcessor(QObject* parent = 0);
    
    int getPacketCount() const { return _packetCount; }
    int getByteCount() const { return _byteCount; }
    
    void resetCounters() { _packetCount = 0; _byteCount = 0; }
public slots:
    void processDatagrams();
    
private:
    int _packetCount;
    int _byteCount;
};

#endif /* defined(__hifi__DatagramProcessor__) */
