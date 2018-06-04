//
//  OverlaysPayload.cpp
//  interface/src/ui/overlays
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <limits>
#include <typeinfo>

#include <avatar/AvatarManager.h>
#include <avatar/MyAvatar.h>
#include <LODManager.h>
#include <render/Scene.h>

#include "Image3DOverlay.h"
#include "Circle3DOverlay.h"
#include "Cube3DOverlay.h"
#include "ImageOverlay.h"
#include "Line3DOverlay.h"
#include "ModelOverlay.h"
#include "Overlays.h"
#include "Rectangle3DOverlay.h"
#include "Sphere3DOverlay.h"
#include "Grid3DOverlay.h"
#include "TextOverlay.h"
#include "Text3DOverlay.h"


namespace render {
    template <> const ItemKey payloadGetKey(const Overlay::Pointer& overlay) {
        return overlay->getKey();
    }
    template <> const Item::Bound payloadGetBound(const Overlay::Pointer& overlay) {
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

