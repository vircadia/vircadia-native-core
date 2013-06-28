//
//  GeometryCache.h
//  interface
//
//  Created by Andrzej Kapolka on 6/21/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__GeometryCache__
#define __interface__GeometryCache__

#include <QHash>

#include "InterfaceConfig.h"

class GeometryCache {
public:
    
    ~GeometryCache();
    
    void renderHemisphere(int slices, int stacks);

private:
    
    typedef QPair<int, int> SlicesStacks;
    typedef QPair<GLuint, GLuint> VerticesIndices;
    
    QHash<SlicesStacks, VerticesIndices> _hemisphereVBOs;
};

#endif /* defined(__interface__GeometryCache__) */
