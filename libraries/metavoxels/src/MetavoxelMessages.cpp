//
//  MetavoxelMessages.cpp
//  metavoxels
//
//  Created by Andrzej Kapolka on 1/24/14.
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

#include "MetavoxelData.h"
#include "MetavoxelMessages.h"

class EditVisitor : public MetavoxelVisitor {
public:
    
    EditVisitor(const MetavoxelEditMessage& edit);
    
    virtual bool visit(MetavoxelInfo& info);

private:
    
    const MetavoxelEditMessage& _edit;
};

EditVisitor::EditVisitor(const MetavoxelEditMessage& edit) :
    MetavoxelVisitor(QVector<AttributePointer>(), QVector<AttributePointer>() << edit.value.getAttribute()),
    _edit(edit) {
}

bool EditVisitor::visit(MetavoxelInfo& info) {
    // find the intersection between volume and voxel
    glm::vec3 minimum = glm::max(info.minimum, _edit.region.minimum);
    glm::vec3 maximum = glm::min(info.minimum + glm::vec3(info.size, info.size, info.size), _edit.region.maximum);
    glm::vec3 size = maximum - minimum;
    if (size.x <= 0.0f || size.y <= 0.0f || size.z <= 0.0f) {
        return false; // disjoint
    }
    float volume = (size.x * size.y * size.z) / (info.size * info.size * info.size);
    if (volume >= 1.0f) {
        info.outputValues[0] = _edit.value;
        return false; // entirely contained
    }
    if (info.size <= _edit.granularity) {
        if (volume >= 0.5f) {
            info.outputValues[0] = _edit.value;
        }
        return false; // reached granularity limit; take best guess
    }
    return true; // subdivide
}

void MetavoxelEditMessage::apply(MetavoxelData& data) const {
    // expand to fit the entire edit
    while (!data.getBounds().contains(region)) {
        data.expand();
    }

    EditVisitor visitor(*this);
    data.guide(visitor);
}
