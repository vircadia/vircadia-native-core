//
//  OverlaysPayload.cpp
//  interface/src/ui/overlays
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Overlay.h"

namespace render {
    template <> const ItemKey payloadGetKey(const Overlay::Pointer& overlay) {
        return overlay->getKey();
    }
    template <> const Item::Bound payloadGetBound(const Overlay::Pointer& overlay, RenderArgs* args) {
        return overlay->getBounds();
    }
    template <> void payloadRender(const Overlay::Pointer& overlay, RenderArgs* args) {
        if (args) {
            overlay->render(args);
        }
    }
    template <> const ShapeKey shapeGetShapeKey(const Overlay::Pointer& overlay) {
        return overlay->getShapeKey();
    }

    template <> uint32_t metaFetchMetaSubItems(const Overlay::Pointer& overlay, ItemIDs& subItems) {
        return overlay->fetchMetaSubItems(subItems);
    }
}

