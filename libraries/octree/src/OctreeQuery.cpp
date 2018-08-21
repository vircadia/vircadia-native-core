//
//  OctreeQuery.cpp
//  libraries/octree/src
//
//  Created by Brad Hefta-Gaub on 10/24/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "OctreeQuery.h"

#include <random>

#include <QtCore/QJsonDocument>

#include <GLMHelpers.h>
#include <udt/PacketHeaders.h>

OctreeQuery::OctreeQuery(bool randomizeConnectionID) {
    if (randomizeConnectionID) {
        // randomize our initial octree query connection ID using random_device
        // the connection ID is 16 bits so we take a generated 32 bit value from random device and chop off the top
        std::random_device randomDevice;
        _connectionID = randomDevice();
    }
}

OctreeQuery::OctreeQueryFlags operator|=(OctreeQuery::OctreeQueryFlags& lhs, int rhs) {
    return lhs = OctreeQuery::OctreeQueryFlags(lhs | rhs);
}

int OctreeQuery::getBroadcastData(unsigned char* destinationBuffer) {
    unsigned char* bufferStart = destinationBuffer;

    // pack the connection ID so the server can detect when we start a new connection
    memcpy(destinationBuffer, &_connectionID, sizeof(_connectionID));
    destinationBuffer += sizeof(_connectionID);

    {
        QMutexLocker lock(&_conicalViewsLock);
        // Number of frustums
        uint8_t numFrustums = (uint8_t)_conicalViews.size();
        memcpy(destinationBuffer, &numFrustums, sizeof(numFrustums));
        destinationBuffer += sizeof(numFrustums);

        for (const auto& view : _conicalViews) {
            destinationBuffer += view.serialize(destinationBuffer);
        }
    }
    
    // desired Max Octree PPS
    memcpy(destinationBuffer, &_maxQueryPPS, sizeof(_maxQueryPPS));
    destinationBuffer += sizeof(_maxQueryPPS);

    // desired voxelSizeScale
    memcpy(destinationBuffer, &_octreeElementSizeScale, sizeof(_octreeElementSizeScale));
    destinationBuffer += sizeof(_octreeElementSizeScale);

    // desired boundaryLevelAdjust
    memcpy(destinationBuffer, &_boundaryLevelAdjust, sizeof(_boundaryLevelAdjust));
    destinationBuffer += sizeof(_boundaryLevelAdjust);
    
    // create a QByteArray that holds the binary representation of the JSON parameters
    QByteArray binaryParametersDocument;
    
    if (!_jsonParameters.isEmpty()) {
        binaryParametersDocument = QJsonDocument(_jsonParameters).toBinaryData();
    }
    
    // write the size of the JSON parameters
    uint16_t binaryParametersBytes = binaryParametersDocument.size();
    memcpy(destinationBuffer, &binaryParametersBytes, sizeof(binaryParametersBytes));
    destinationBuffer += sizeof(binaryParametersBytes);
    
    // pack the binary JSON parameters
    // NOTE: for now we assume that the filters that will be set are all small enough that we will not have a packet > MTU
    if (binaryParametersDocument.size() > 0) {
        memcpy(destinationBuffer, binaryParametersDocument.data(), binaryParametersBytes);
        destinationBuffer += binaryParametersBytes;
    }

    OctreeQueryFlags queryFlags { NoFlags };
    queryFlags |= (_reportInitialCompletion ? OctreeQuery::WantInitialCompletion : 0);
    memcpy(destinationBuffer, &queryFlags, sizeof(queryFlags));
    destinationBuffer += sizeof(queryFlags);

    return destinationBuffer - bufferStart;
}

// called on the other nodes - assigns it to my views of the others
int OctreeQuery::parseData(ReceivedMessage& message) {
 
    const unsigned char* startPosition = reinterpret_cast<const unsigned char*>(message.getRawMessage());
    const unsigned char* sourceBuffer = startPosition;

    // unpack the connection ID
    uint16_t newConnectionID;
    memcpy(&newConnectionID, sourceBuffer, sizeof(newConnectionID));
    sourceBuffer += sizeof(newConnectionID);

    if (!_hasReceivedFirstQuery) {
        // set our flag to indicate that we've parsed for this query at least once
        _hasReceivedFirstQuery = true;

        // set the incoming connection ID as the current
        _connectionID = newConnectionID;
    } else {
        if (newConnectionID != _connectionID) {
            // the connection ID has changed - emit our signal so the server
            // knows that the client is starting a new session
            _connectionID = newConnectionID;
            emit incomingConnectionIDChanged();
        }
    }

    // check if this query uses a view frustum
    uint8_t numFrustums = 0;
    memcpy(&numFrustums, sourceBuffer, sizeof(numFrustums));
    sourceBuffer += sizeof(numFrustums);

    {
        QMutexLocker lock(&_conicalViewsLock);
        _conicalViews.clear();
        for (int i = 0; i < numFrustums; ++i) {
            ConicalViewFrustum view;
            sourceBuffer += view.deserialize(sourceBuffer);
            _conicalViews.push_back(view);
        }
    }

    // desired Max Octree PPS
    memcpy(&_maxQueryPPS, sourceBuffer, sizeof(_maxQueryPPS));
    sourceBuffer += sizeof(_maxQueryPPS);

    // desired _octreeElementSizeScale
    memcpy(&_octreeElementSizeScale, sourceBuffer, sizeof(_octreeElementSizeScale));
    sourceBuffer += sizeof(_octreeElementSizeScale);

    // desired boundaryLevelAdjust
    memcpy(&_boundaryLevelAdjust, sourceBuffer, sizeof(_boundaryLevelAdjust));
    sourceBuffer += sizeof(_boundaryLevelAdjust);
    
    // check if we have a packed JSON filter
    uint16_t binaryParametersBytes;
    memcpy(&binaryParametersBytes, sourceBuffer, sizeof(binaryParametersBytes));
    sourceBuffer += sizeof(binaryParametersBytes);
    
    if (binaryParametersBytes > 0) {
        // unpack the binary JSON parameters
        QByteArray binaryJSONParameters { binaryParametersBytes, 0 };
        memcpy(binaryJSONParameters.data(), sourceBuffer, binaryParametersBytes);
        sourceBuffer += binaryParametersBytes;
        
        // grab the parameter object from the packed binary representation of JSON
        auto newJsonDocument = QJsonDocument::fromBinaryData(binaryJSONParameters);
        
        QWriteLocker jsonParameterLocker { &_jsonParametersLock };
        _jsonParameters = newJsonDocument.object();
    }

    OctreeQueryFlags queryFlags;
    memcpy(&queryFlags, sourceBuffer, sizeof(queryFlags));
    sourceBuffer += sizeof(queryFlags);

    _reportInitialCompletion = bool(queryFlags & OctreeQueryFlags::WantInitialCompletion);

    return sourceBuffer - startPosition;
}
