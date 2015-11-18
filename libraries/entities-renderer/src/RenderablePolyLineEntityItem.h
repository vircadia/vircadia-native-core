//
//  RenderablePolyLineEntityItem.h
//  libraries/entities-renderer/src/
//
//  Created by Eric Levin on 6/22/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_RenderablePolyLineEntityItem_h
#define hifi_RenderablePolyLineEntityItem_h

#include <gpu/Batch.h>
#include <PolyLineEntityItem.h>
#include "RenderableEntityItem.h"
#include <GeometryCache.h>
#include <QReadWriteLock>


class RenderablePolyLineEntityItem : public PolyLineEntityItem {
public:
    static EntityItemPointer factory(const EntityItemID& entityID, const EntityItemProperties& properties);
    static void createPipeline();
    RenderablePolyLineEntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties);
    
    virtual void render(RenderArgs* args);
    
    SIMPLE_RENDERABLE();
    
    static gpu::PipelinePointer _pipeline;
    static gpu::Stream::FormatPointer _format;
    static gpu::TexturePointer _texture;
    static int32_t PAINTSTROKE_GPU_SLOT;
    
protected:
    void updateGeometry();
    gpu::BufferPointer _verticesBuffer;
    unsigned int _numVertices;

};


#endif // hifi_RenderablePolyLineEntityItem_h
