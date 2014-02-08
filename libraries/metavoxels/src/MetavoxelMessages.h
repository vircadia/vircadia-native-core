//
//  MetavoxelMessages.h
//  metavoxels
//
//  Created by Andrzej Kapolka on 12/31/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__MetavoxelMessages__
#define __interface__MetavoxelMessages__

#include "AttributeRegistry.h"
#include "MetavoxelUtil.h"

class MetavoxelData;

/// Requests to close the session.
class CloseSessionMessage {
    STREAMABLE
};

DECLARE_STREAMABLE_METATYPE(CloseSessionMessage)

/// Clears the mapping for a shared object.
class ClearSharedObjectMessage {
    STREAMABLE

public:
    
    STREAM int id;
};

DECLARE_STREAMABLE_METATYPE(ClearSharedObjectMessage)

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
class MetavoxelEditMessage {
    STREAMABLE

public:
    
    STREAM Box region;
    STREAM float granularity;
    STREAM OwnedAttributeValue value;
    
    void apply(MetavoxelData& data) const;
};

DECLARE_STREAMABLE_METATYPE(MetavoxelEditMessage)

#endif /* defined(__interface__MetavoxelMessages__) */
