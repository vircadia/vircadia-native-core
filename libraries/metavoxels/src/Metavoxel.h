//
//  Metavoxel.h
//  metavoxels
//
//  Created by Andrzej Kapolka on 12/2/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__Metavoxel__
#define __interface__Metavoxel__

#include <QScopedPointer>

class Bitstream;

/// A single node in a metavoxel tree.
class Metavoxel {
public:

    static const int CHILD_COUNT = 8;

    /// Sets the child at the specified index.  Note that this object will assume ownership if non-null.
    void setChild(int index, Metavoxel* child) { _children[index].reset(child); }
    
    const Metavoxel* getChild(int index) const { return _children[index].data(); }
    Metavoxel* getChild(int index) { return _children[index].data(); }
    
    bool isLeaf() const;

private:

    QScopedPointer<Metavoxel> _children[CHILD_COUNT];    
};

Bitstream& operator<<(Bitstream& stream, const Metavoxel& voxel);
Bitstream& operator>>(Bitstream& stream, Metavoxel& voxel);

#endif /* defined(__interface__Metavoxel__) */
