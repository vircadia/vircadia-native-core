//
//  OctreeStatsProvider.h
//  interface/src/ui
//
//  Created by Vlad Stelmahovsky on 3/12/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_OctreeStatsProvider_h
#define hifi_OctreeStatsProvider_h

#include <OctreeSceneStats.h>
#include <QTimer>

#include "DependencyManager.h"

#define MAX_STATS 100
#define MAX_VOXEL_SERVERS 50
#define DEFAULT_COLOR 0

class OctreeStatsProvider : public QObject, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY

    Q_PROPERTY(int serversNum READ serversNum NOTIFY serversNumChanged)
    Q_PROPERTY(QString serverElements READ serverElements NOTIFY serverElementsChanged)
    Q_PROPERTY(QString localElements READ localElements NOTIFY localElementsChanged)
    Q_PROPERTY(QString localElementsMemory READ localElementsMemory NOTIFY localElementsMemoryChanged)
    Q_PROPERTY(QString sendingMode READ sendingMode NOTIFY sendingModeChanged)
    //Incoming Entity Packets
    Q_PROPERTY(QString processedPackets READ processedPackets NOTIFY processedPacketsChanged)

//    int _entityUpdateTime;
//    int _entityUpdates;
//    int _processedPacketsElements;
//    int _processedPacketsEntities;
//    int _processedPacketsTiming;
//    int _outboundEditPackets;

public:
    // Sets up the UI
    OctreeStatsProvider(QObject* parent, NodeToOctreeSceneStats* model);
    ~OctreeStatsProvider();

    int serversNum() const;

    QString serverElements() const {
        return m_serverElements;
    }

    QString localElements() const {
        return m_localElements;
    }

    QString localElementsMemory() const {
        return m_localElementsMemory;
    }

    QString sendingMode() const {
        return m_sendingMode;
    }

    QString processedPackets() const {
        return m_processedPackets;
    }

signals:

    void serversNumChanged(int serversNum);
    void serverElementsChanged(QString serverElements);
    void localElementsChanged(QString localElements);
    void sendingModeChanged(QString sendingMode);
    void processedPacketsChanged(QString processedPackets);
    void localElementsMemoryChanged(QString localElementsMemory);

public slots:
    void moreless(const QString& link);
    void startUpdates();
    void stopUpdates();

private slots:
    void updateOctreeStatsData();
protected:

    int AddStatItem(const char* caption, unsigned colorRGBA = DEFAULT_COLOR);
    void updateOctreeServers();
    void showOctreeServersOfType(int& serverNumber, NodeType_t serverType, 
                    const char* serverTypeName, NodeToJurisdictionMap& serverJurisdictions);

private:
    NodeToOctreeSceneStats* _model;
    int _statCount;
    
    int _sendingMode;
    int _serverElements;
    int _localElements;
    int _localElementsMemory;

    int _entityUpdateTime;
    int _entityUpdates;
    int _processedPackets;
    int _processedPacketsElements;
    int _processedPacketsEntities;
    int _processedPacketsTiming;
    int _outboundEditPackets;
    
    const int SAMPLES_PER_SECOND = 10;
    SimpleMovingAverage _averageUpdatesPerSecond;
    quint64 _lastWindowAt = usecTimestampNow();
    quint64 _lastKnownTrackedEdits = 0;

    quint64 _lastRefresh = 0;

//    int _octreeServerLables[MAX_VOXEL_SERVERS];
//    int _octreeServerLabelsCount;
    QTimer _updateTimer;
    int m_serversNum {0};
    QString m_serverElements;
    QString m_localElements;
    QString m_localElementsMemory;
    QString m_sendingMode;
    QString m_processedPackets;
};

#endif // hifi_OctreeStatsProvider_h
