//
//  MetavoxelMessages.h
//  metavoxels
//
//  Created by Andrzej Kapolka on 12/31/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__MetavoxelMessages__
#define __interface__MetavoxelMessages__

#include "Bitstream.h"

/// A message containing the state of a client.
class ClientStateMessage {
    STREAMABLE
    
public:
    
    STREAM glm::vec3 position;
};

DECLARE_STREAMABLE_METATYPE(ClientStateMessage)

/// A message preceding metavoxel delta information.  The actual delta will follow it in the stream.
class MetavoxelDeltaMessage {
    STREAMABLE
};

DECLARE_STREAMABLE_METATYPE(MetavoxelDeltaMessage)

/// A simple streamable edit.
class MetavoxelEdit {
    STREAMABLE

public:
    
    glm::vec3 minimum;
    glm::vec3 maximum;
    float granularity;
};

DECLARE_STREAMABLE_METATYPE(MetavoxelEdit)

#endif /* defined(__interface__MetavoxelMessages__) */
