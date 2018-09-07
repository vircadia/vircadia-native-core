//
//  OctreeQuery.h
//  libraries/octree/src
//
//  Created by Stephen Birarda on 4/9/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_OctreeQuery_h
#define hifi_OctreeQuery_h

#include <QtCore/QJsonObject>
#include <QtCore/QReadWriteLock>

#include <NodeData.h>
#include <shared/ConicalViewFrustum.h>

#include "OctreeConstants.h"

class OctreeQuery : public NodeData {
    Q_OBJECT

public:
    OctreeQuery(bool randomizeConnectionID = false);
    virtual ~OctreeQuery() {}

    OctreeQuery(const OctreeQuery&) = delete;
    OctreeQuery& operator=(const OctreeQuery&) = delete;

    int getBroadcastData(unsigned char* destinationBuffer);
    int parseData(ReceivedMessage& message) override;

    bool hasConicalViews() const { QMutexLocker lock(&_conicalViewsLock); return !_conicalViews.empty(); }
    void setConicalViews(ConicalViewFrustums views)
        { QMutexLocker lock(&_conicalViewsLock); _conicalViews = views; }
    void clearConicalViews() { QMutexLocker lock(&_conicalViewsLock); _conicalViews.clear(); }

    // getters/setters for JSON filter
    QJsonObject getJSONParameters() { QReadLocker locker { &_jsonParametersLock }; return _jsonParameters; }
    void setJSONParameters(const QJsonObject& jsonParameters)
        { QWriteLocker locker { &_jsonParametersLock }; _jsonParameters = jsonParameters; }
    
    // related to Octree Sending strategies
    int getMaxQueryPacketsPerSecond() const { return _maxQueryPPS; }
    float getOctreeSizeScale() const { return _octreeElementSizeScale; }
    int getBoundaryLevelAdjust() const { return _boundaryLevelAdjust; }

    void incrementConnectionID() { ++_connectionID; }

    bool hasReceivedFirstQuery() const  { return _hasReceivedFirstQuery; }

    // Want a report when the initial query is complete.
    bool wantReportInitialCompletion() const { return _reportInitialCompletion; }
    void setReportInitialCompletion(bool reportInitialCompletion) { _reportInitialCompletion = reportInitialCompletion; }

signals:
    void incomingConnectionIDChanged();

public slots:
    void setMaxQueryPacketsPerSecond(int maxQueryPPS) { _maxQueryPPS = maxQueryPPS; }
    void setOctreeSizeScale(float octreeSizeScale) { _octreeElementSizeScale = octreeSizeScale; }
    void setBoundaryLevelAdjust(int boundaryLevelAdjust) { _boundaryLevelAdjust = boundaryLevelAdjust; }

protected:
    mutable QMutex _conicalViewsLock;
    ConicalViewFrustums _conicalViews;

    // octree server sending items
    int _maxQueryPPS = DEFAULT_MAX_OCTREE_PPS;
    float _octreeElementSizeScale = DEFAULT_OCTREE_SIZE_SCALE; /// used for LOD calculations
    int _boundaryLevelAdjust = 0; /// used for LOD calculations

    uint16_t _connectionID; // query connection ID, randomized to start, increments with each new connection to server
    
    QJsonObject _jsonParameters;
    QReadWriteLock _jsonParametersLock;
    
    enum OctreeQueryFlags : uint16_t { NoFlags = 0x0, WantInitialCompletion = 0x1 };
    friend OctreeQuery::OctreeQueryFlags operator|=(OctreeQuery::OctreeQueryFlags& lhs, const int rhs);

    bool _hasReceivedFirstQuery { false };
    bool _reportInitialCompletion { false };
};

#endif // hifi_OctreeQuery_h
