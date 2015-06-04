//
//  OverlaysPayload.cpp
//  interface/src/ui/overlays
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QScriptValueIterator>

#include <limits>
#include <typeinfo>

#include <Application.h>
#include <avatar/AvatarManager.h>
#include <LODManager.h>
#include <render/Scene.h>

#include "BillboardOverlay.h"
#include "Circle3DOverlay.h"
#include "Cube3DOverlay.h"
#include "ImageOverlay.h"
#include "Line3DOverlay.h"
#include "LocalModelsOverlay.h"
#include "ModelOverlay.h"
#include "Overlays.h"
#include "Rectangle3DOverlay.h"
#include "Sphere3DOverlay.h"
#include "Grid3DOverlay.h"
#include "TextOverlay.h"
#include "Text3DOverlay.h"


namespace render {
    template <> const ItemKey payloadGetKey(const Overlay::Pointer& overlay) {
        if (overlay->is3D() && !static_cast<Base3DOverlay*>(overlay.get())->getDrawOnHUD()) {
            if (static_cast<Base3DOverlay*>(overlay.get())->getDrawInFront()) {
                return ItemKey::Builder().withTypeShape().withNoDepthSort().build();
            } else {
                return ItemKey::Builder::opaqueShape();
            }
        } else {
            return ItemKey::Builder().withTypeShape().withViewSpace().build();
        }
    }
    template <> const Item::Bound payloadGetBound(const Overlay::Pointer& overlay) {
        if (overlay->is3D()) {
            return static_cast<Base3DOverlay*>(overlay.get())->getBounds();
        } else {
            QRect bounds = static_cast<Overlay2D*>(overlay.get())->getBounds();
            return AABox(glm::vec3(bounds.x(), bounds.y(), 0.0f), glm::vec3(bounds.width(), bounds.height(), 0.1f));
        }
    }
    template <> void payloadRender(const Overlay::Pointer& overlay, RenderArgs* args) {
        if (args) {
            args->_elementsTouched++;

            glPushMatrix();
            if (overlay->getAnchor() == Overlay::MY_AVATAR) {
                MyAvatar* avatar = DependencyManager::get<AvatarManager>()->getMyAvatar();
                glm::quat myAvatarRotation = avatar->getOrientation();
                glm::vec3 myAvatarPosition = avatar->getPosition();
                float angle = glm::degrees(glm::angle(myAvatarRotation));
                glm::vec3 axis = glm::axis(myAvatarRotation);
                float myAvatarScale = avatar->getScale();

                glTranslatef(myAvatarPosition.x, myAvatarPosition.y, myAvatarPosition.z);
                glRotatef(angle, axis.x, axis.y, axis.z);
                glScalef(myAvatarScale, myAvatarScale, myAvatarScale);
            }
            overlay->render(args);
            glPopMatrix();
        }
    }
}
