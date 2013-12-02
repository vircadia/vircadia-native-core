//
//  Metavoxels.cpp
//  metavoxels
//
//  Created by Andrzej Kapolka on 12/2/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include "Bitstream.h"
#include "Metavoxel.h"

bool Metavoxel::isLeaf() const {
    for (int i = 0; i < CHILD_COUNT; i++) {
        if (_children[i]) {
            return false;
        }
    }
    return true;
}

Bitstream& operator<<(Bitstream& stream, const Metavoxel& voxel) {
    for (int i = 0; i < Metavoxel::CHILD_COUNT; i++) {
        const Metavoxel* child = voxel.getChild(i);
        if (child) {
            stream << true << *child;
            
        } else {
            stream << false;
        }
    }
    return stream;
}

Bitstream& operator>>(Bitstream& stream, Metavoxel& voxel) {
    for (int i = 0; i < Metavoxel::CHILD_COUNT; i++) {
        bool childExists;
        stream >> childExists;
        Metavoxel* child = voxel.getChild(i);
        if (childExists) {    
            if (!child) {
                voxel.setChild(i, new Metavoxel);
            }
            stream >> *child;
            
        } else if (child) {
            voxel.setChild(i, NULL);
        }
    }
    return stream;
}
