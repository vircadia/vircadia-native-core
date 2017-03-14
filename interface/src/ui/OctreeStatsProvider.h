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
#include <QColor>

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
    Q_PROPERTY(QString processedPackets READ processedPackets NOTIFY processedPacketsChanged)
    Q_PROPERTY(QString processedPacketsElements READ processedPacketsElements NOTIFY processedPacketsElementsChanged)
    Q_PROPERTY(QString processedPacketsEntities READ processedPacketsEntities NOTIFY processedPacketsEntitiesChanged)
    Q_PROPERTY(QString processedPacketsTiming READ processedPacketsTiming NOTIFY processedPacketsTimingChanged)
    Q_PROPERTY(QString outboundEditPackets READ outboundEditPackets NOTIFY outboundEditPacketsChanged)
    Q_PROPERTY(QString entityUpdateTime READ entityUpdateTime NOTIFY entityUpdateTimeChanged)
    Q_PROPERTY(QString entityUpdates READ entityUpdates NOTIFY entityUpdatesChanged)

    Q_PROPERTY(QStringList servers READ servers NOTIFY serversChanged)

public:
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

    QString processedPacketsElements() const {
        return m_processedPacketsElements;
    }

    QString processedPacketsEntities() const {
        return m_processedPacketsEntities;
    }

    QString processedPacketsTiming() const {
        return m_processedPacketsTiming;
    }

    QString outboundEditPackets() const {
        return m_outboundEditPackets;
    }

    QString entityUpdateTime() const {
        return m_entityUpdateTime;
    }

    QString entityUpdates() const {
        return m_entityUpdates;
    }

    QStringList servers() const {
        return m_servers;
    }

signals:

    void serversNumChanged(int serversNum);
    void serverElementsChanged(const QString &serverElements);
    void localElementsChanged(const QString &localElements);
    void sendingModeChanged(const QString &sendingMode);
    void processedPacketsChanged(const QString &processedPackets);
    void localElementsMemoryChanged(const QString &localElementsMemory);
    void processedPacketsElementsChanged(const QString &processedPacketsElements);
    void processedPacketsEntitiesChanged(const QString &processedPacketsEntities);
    void processedPacketsTimingChanged(const QString &processedPacketsTiming);
    void outboundEditPacketsChanged(const QString &outboundEditPackets);
    void entityUpdateTimeChanged(const QString &entityUpdateTime);
    void entityUpdatesChanged(const QString &entityUpdates);

    void serversChanged(const QStringList &servers);

public slots:
    void startUpdates();
    void stopUpdates();
    QColor getColor() const;

private slots:
    void updateOctreeStatsData();
protected:
    void updateOctreeServers();
    void showOctreeServersOfType(int& serverNumber, NodeType_t serverType, 
                    const char* serverTypeName, NodeToJurisdictionMap& serverJurisdictions);

private:
    NodeToOctreeSceneStats* _model;
    int _statCount;
    
    const int SAMPLES_PER_SECOND = 10;
    SimpleMovingAverage _averageUpdatesPerSecond;
    quint64 _lastWindowAt = usecTimestampNow();
    quint64 _lastKnownTrackedEdits = 0;

    quint64 _lastRefresh = 0;

    QTimer _updateTimer;
    int m_serversNum {0};
    QString m_serverElements;
    QString m_localElements;
    QString m_localElementsMemory;
    QString m_sendingMode;
    QString m_processedPackets;
    QString m_processedPacketsElements;
    QString m_processedPacketsEntities;
    QString m_processedPacketsTiming;
    QString m_outboundEditPackets;
    QString m_entityUpdateTime;
    QString m_entityUpdates;
    QStringList m_servers;
};

#endif // hifi_OctreeStatsProvider_h
