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

#ifndef hifi_DatagramProcessor_h
#define hifi_DatagramProcessor_h

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

public:
    // DEBUG

    static int skewsI[];
    static int S;

    static unsigned char typesI[];
    static int diffsI[];
    static int I;


    static quint64 prevTime;

    static unsigned char typesA[];
    static quint64 timesA[];
    static int A;

    static unsigned char typesB[];
    static quint64 timesB[];
    static int B;


    static unsigned char* currTypes, *prevTypes;
    static quint64* currTimes, *prevTimes;
    static int* currN, *prevN;
};

#endif // hifi_DatagramProcessor_h
