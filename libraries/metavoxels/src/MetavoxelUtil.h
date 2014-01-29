//
//  MetavoxelUtil.h
//  metavoxels
//
//  Created by Andrzej Kapolka on 12/30/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__MetavoxelUtil__
#define __interface__MetavoxelUtil__

#include <QUuid>

#include "Bitstream.h"

class QByteArray;

class HifiSockAddr;

/// Reads and returns the session ID from a datagram.
/// \param[out] headerPlusIDSize the size of the header (including the session ID) within the data
/// \return the session ID, or a null ID if invalid (in which case a warning will be logged)
QUuid readSessionID(const QByteArray& data, const HifiSockAddr& sender, int& headerPlusIDSize);

/// A streamable axis-aligned bounding box.
class Box {
    STREAMABLE

public:
    
    STREAM glm::vec3 minimum;
    STREAM glm::vec3 maximum;
    
    bool contains(const Box& other) const;
};

DECLARE_STREAMABLE_METATYPE(Box)

#endif /* defined(__interface__MetavoxelUtil__) */
