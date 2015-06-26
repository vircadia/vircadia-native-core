//
//  RenderableLineEntityItem.h
//  libraries/entities-renderer/src/
//
//  Created by Seth Alves on 5/11/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_RenderableQuadEntityItem_h
#define hifi_RenderableQuadEntityItem_h

#include <gpu/Batch.h>
#include <QuadEntityItem.h>
#include "RenderableDebugableEntityItem.h"
#include "RenderableEntityItem.h"
#include <GeometryCache.h>

class RenderableQuadEntityItem : public QuadEntityItem {
public:
    static EntityItemPointer factory(const EntityItemID& entityID, const EntityItemProperties& properties);
    static void createPipeline();
    RenderableQuadEntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties);
    
    virtual void render(RenderArgs* args);
    
    SIMPLE_RENDERABLE();
    
    static gpu::PipelinePointer _pipeline;
    
protected:
    void updateGeometry();
    gpu::Stream::FormatPointer _format;
    gpu::BufferPointer _verticesBuffer;
    unsigned int _numVertices;
};


#endif // hifi_RenderableQuadEntityItem_h
