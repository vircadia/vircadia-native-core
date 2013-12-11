//
//  MetavoxelSystem.cpp
//  interface
//
//  Created by Andrzej Kapolka on 12/10/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include <QtDebug>

#include "MetavoxelSystem.h"

void MetavoxelSystem::init() {
    MetavoxelPath p1;
    p1 += 0;
    p1 += 1;
    p1 += 2;
    
    AttributePointer blerp = AttributeRegistry::getInstance()->getAttribute("blerp");
    
    void* foo = encodeInline(5.0f);
    _data.setAttributeValue(p1, AttributeValue(blerp, &foo));
    
    //p1 += 0;
    
    MetavoxelPath p2;
    
    AttributeValue value = _data.getAttributeValue(p2, blerp);
    
    qDebug("fliggedy bloo %g\n", decodeInline<float>(value.getValue()));
}

