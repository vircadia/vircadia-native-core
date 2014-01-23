//
//  DatagramProcessor.h
//  hifi
//
//  Created by Stephen Birarda on 1/23/2014.
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
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
