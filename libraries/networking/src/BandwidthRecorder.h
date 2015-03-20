//
//  BandwidthRecorder.h
//
//  Created by Seth Alves on 2015-1-30
//  Copyright 2015 High Fidelity, Inc.
//
//  Based on code by Tobias Schwinger
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#ifndef hifi_BandwidthRecorder_h
#define hifi_BandwidthRecorder_h

#include <QObject>
#include <QElapsedTimer>
#include "DependencyManager.h"
#include "SimpleMovingAverage.h"


class BandwidthRecorder : public QObject, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY

public:
    BandwidthRecorder();
    ~BandwidthRecorder();

    // keep track of data rate in two directions as well as units and style to use during display
    class Channel {
    public:
        Channel();
        float getAverageInputPacketsPerSecond();
        float getAverageOutputPacketsPerSecond();
        float getAverageInputKilobitsPerSecond();
        float getAverageOutputKilobitsPerSecond();

        void updateInputAverage(const float sample);
        void updateOutputAverage(const float sample);

    private:
        SimpleMovingAverage _input = SimpleMovingAverage();
        SimpleMovingAverage _output = SimpleMovingAverage();
    };

    float getAverageInputPacketsPerSecond(const quint8 channelType);
    float getAverageOutputPacketsPerSecond(const quint8 channelType);
    float getAverageInputKilobitsPerSecond(const quint8 channelType);
    float getAverageOutputKilobitsPerSecond(const quint8 channelType);

    float getTotalAverageInputPacketsPerSecond();
    float getTotalAverageOutputPacketsPerSecond();
    float getTotalAverageInputKilobitsPerSecond();
    float getTotalAverageOutputKilobitsPerSecond();

    float getCachedTotalAverageInputPacketsPerSecond();
    float getCachedTotalAverageOutputPacketsPerSecond();
    float getCachedTotalAverageInputKilobitsPerSecond();
    float getCachedTotalAverageOutputKilobitsPerSecond();


private:
    // one for each possible Node type
    static const unsigned int CHANNEL_COUNT = 256;
    Channel* _channels[CHANNEL_COUNT];


public slots:
    void updateInboundData(const quint8 channelType, const int bytes);
    void updateOutboundData(const quint8 channelType, const int bytes);
};

#endif
