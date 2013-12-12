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
    
    virtual bool visit(const MetavoxelInfo& info);
};

bool DebugVisitor::visit(const MetavoxelInfo& info) {
    QRgb color = info.attributeValues.at(0).getInlineValue<QRgb>();
    qDebug("%g %g %g %g %d %d %d %d\n", info.minimum.x, info.minimum.y, info.minimum.z, info.size,
        qRed(color), qGreen(color), qBlue(color), qAlpha(color));
    return true;
}

void MetavoxelSystem::init() {
    MetavoxelPath p1;
    p1 += 0;
    p1 += 1;
    p1 += 2;
    
    AttributePointer diffuseColor = AttributeRegistry::getInstance()->getAttribute("diffuseColor");
    
    void* white = encodeInline(qRgba(0xFF, 0xFF, 0xFF, 0xFF));
    _data.setAttributeValue(p1, AttributeValue(diffuseColor, &white));
    
    DebugVisitor visitor;
    _data.visitVoxels(QVector<AttributePointer>() << diffuseColor, visitor);
}

