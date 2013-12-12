//
//  MetavoxelSystem.cpp
//  interface
//
//  Created by Andrzej Kapolka on 12/10/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include <QtDebug>

#include "MetavoxelSystem.h"

class PointVisitor : public MetavoxelVisitor {
public:
    
    virtual bool visit(const MetavoxelInfo& info);
};

bool PointVisitor::visit(const MetavoxelInfo& info) {
    QRgb color = info.attributeValues.at(0).getInlineValue<QRgb>();
    QRgb normal = info.attributeValues.at(1).getInlineValue<QRgb>();
    
    return true;
}

void MetavoxelSystem::init() {
    MetavoxelPath p1;
    p1 += 0;
    p1 += 1;
    p1 += 2;
    
    AttributePointer color = AttributeRegistry::getInstance()->getAttribute("color");
    
    void* white = encodeInline(qRgba(0xFF, 0xFF, 0xFF, 0xFF));
    _data.setAttributeValue(p1, AttributeValue(color, &white));
}

void MetavoxelSystem::simulate(float deltaTime) {
    QVector<AttributePointer> attributes;
    attributes << AttributeRegistry::getInstance()->getColorAttribute();
    attributes << AttributeRegistry::getInstance()->getNormalAttribute();
    PointVisitor visitor;
    _data.visitVoxels(attributes, visitor);
}

void MetavoxelSystem::render() {
    
}
