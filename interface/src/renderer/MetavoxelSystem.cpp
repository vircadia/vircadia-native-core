//
//  MetavoxelSystem.cpp
//  interface
//
//  Created by Andrzej Kapolka on 12/10/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include <QtDebug>

#include "MetavoxelSystem.h"

class DebugVisitor : public MetavoxelVisitor {
public:
    
    virtual bool visit(const QVector<AttributeValue>& attributeValues);
};

bool DebugVisitor::visit(const QVector<AttributeValue>& attributeValues) {
    qDebug() << decodeInline<float>(attributeValues.at(0).getValue()) << "\n";
    return true;
}

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
    
    DebugVisitor visitor;
    _data.visitVoxels(QVector<AttributePointer>() << blerp, visitor);
}

