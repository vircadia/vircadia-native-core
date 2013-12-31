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

/// A message containing the position of a client.
class ClientPositionMessage {
    STREAMABLE
    
public:
    
    STREAM int test;
};

DECLARE_STREAMABLE_METATYPE(ClientPositionMessage)

#endif /* defined(__interface__MetavoxelMessages__) */
