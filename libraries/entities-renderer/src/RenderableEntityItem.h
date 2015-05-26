//
//  RenderableEntityItem.h
//  interface/src
//
//  Created by Brad Hefta-Gaub on 12/6/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_RenderableEntityItem_h
#define hifi_RenderableEntityItem_h

#include <render/Scene.h>
#include <EntityItem.h>

class RenderableEntityItem {
public:
    typedef render::Payload<RenderableEntityItem> Payload;
    typedef std::shared_ptr<render::Item::PayloadInterface> PayloadPointer;
    //typedef Payload::DataPointer Pointer;
    typedef EntityItemPointer Pointer;
        
};

#endif // hifi_RenderableEntityItem_h
