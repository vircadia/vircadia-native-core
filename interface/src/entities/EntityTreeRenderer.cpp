//
//  EntityTreeRenderer.cpp
//  interface/src
//
//  Created by Brad Hefta-Gaub on 12/6/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <glm/gtx/quaternion.hpp>

#include <FBXReader.h>

#include "InterfaceConfig.h"

#include <BoxEntityItem.h>
#include <ModelEntityItem.h>
#include <PerfStat.h>


#include "Menu.h"
#include "EntityTreeRenderer.h"

#include "RenderableBoxEntityItem.h"
#include "RenderableModelEntityItem.h"
#include "RenderableSphereEntityItem.h"


QThread* EntityTreeRenderer::getMainThread() {
    return Application::getInstance()->getEntities()->thread();
}



EntityTreeRenderer::EntityTreeRenderer() :
    OctreeRenderer() {
    REGISTER_ENTITY_TYPE_WITH_FACTORY(Model, RenderableModelEntityItem::factory)
    REGISTER_ENTITY_TYPE_WITH_FACTORY(Box, RenderableBoxEntityItem::factory)
    REGISTER_ENTITY_TYPE_WITH_FACTORY(Sphere, RenderableSphereEntityItem::factory)
}

EntityTreeRenderer::~EntityTreeRenderer() {
}

void EntityTreeRenderer::clear() {
    OctreeRenderer::clear();
}

void EntityTreeRenderer::init() {
    OctreeRenderer::init();
    static_cast<EntityTree*>(_tree)->setFBXService(this);
}

void EntityTreeRenderer::setTree(Octree* newTree) {
    OctreeRenderer::setTree(newTree);
    static_cast<EntityTree*>(_tree)->setFBXService(this);
}

void EntityTreeRenderer::update() {
    if (_tree) {
        EntityTree* tree = static_cast<EntityTree*>(_tree);
        tree->update();
    }
}

void EntityTreeRenderer::render(RenderMode renderMode) {
    OctreeRenderer::render(renderMode);
}

const FBXGeometry* EntityTreeRenderer::getGeometryForEntity(const EntityItem* entityItem) {
    const FBXGeometry* result = NULL;
    
    if (entityItem->getType() == EntityTypes::Model) {
        const RenderableModelEntityItem* constModelEntityItem = dynamic_cast<const RenderableModelEntityItem*>(entityItem);
        RenderableModelEntityItem* modelEntityItem = const_cast<RenderableModelEntityItem*>(constModelEntityItem);
        assert(modelEntityItem); // we need this!!!
        Model* model = modelEntityItem->getModel();
        if (model) {
            result = &model->getGeometry()->getFBXGeometry();
        }
    }
    return result;
}

const Model* EntityTreeRenderer::getModelForEntityItem(const EntityItem* entityItem) {
    const Model* result = NULL;
    if (entityItem->getType() == EntityTypes::Model) {
        const RenderableModelEntityItem* constModelEntityItem = dynamic_cast<const RenderableModelEntityItem*>(entityItem);
        RenderableModelEntityItem* modelEntityItem = const_cast<RenderableModelEntityItem*>(constModelEntityItem);
        assert(modelEntityItem); // we need this!!!
    
        result = modelEntityItem->getModel();
    }
    return result;
}

void EntityTreeRenderer::renderElement(OctreeElement* element, RenderArgs* args) {
    //PerformanceTimer perfTimer("renderElement");
    args->_elementsTouched++;
    // actually render it here...
    // we need to iterate the actual entityItems of the element
    EntityTreeElement* entityTreeElement = static_cast<EntityTreeElement*>(element);

    QList<EntityItem*>& entityItems = entityTreeElement->getEntities();
    

    uint16_t numberOfEntities = entityItems.size();

    bool isShadowMode = args->_renderMode == OctreeRenderer::SHADOW_RENDER_MODE;
    bool displayElementProxy = Menu::getInstance()->isOptionChecked(MenuOption::DisplayModelElementProxy);
    bool displayElementChildProxies = Menu::getInstance()->isOptionChecked(MenuOption::DisplayModelElementChildProxies);


    if (!isShadowMode && displayElementProxy && numberOfEntities > 0) {
        glm::vec3 elementCenter = entityTreeElement->getAACube().calcCenter() * (float)TREE_SCALE;
        float elementSize = entityTreeElement->getScale() * (float)TREE_SCALE;
        glColor3f(1.0f, 0.0f, 0.0f);
        glPushMatrix();
            glTranslatef(elementCenter.x, elementCenter.y, elementCenter.z);
            glutWireCube(elementSize);
        glPopMatrix();

        if (displayElementChildProxies) {
            // draw the children
            float halfSize = elementSize / 2.0f;
            float quarterSize = elementSize / 4.0f;
            glColor3f(1.0f, 1.0f, 0.0f);
            glPushMatrix();
                glTranslatef(elementCenter.x - quarterSize, elementCenter.y - quarterSize, elementCenter.z - quarterSize);
                glutWireCube(halfSize);
            glPopMatrix();

            glColor3f(1.0f, 0.0f, 1.0f);
            glPushMatrix();
                glTranslatef(elementCenter.x + quarterSize, elementCenter.y - quarterSize, elementCenter.z - quarterSize);
                glutWireCube(halfSize);
            glPopMatrix();

            glColor3f(0.0f, 1.0f, 0.0f);
            glPushMatrix();
                glTranslatef(elementCenter.x - quarterSize, elementCenter.y + quarterSize, elementCenter.z - quarterSize);
                glutWireCube(halfSize);
            glPopMatrix();

            glColor3f(0.0f, 0.0f, 1.0f);
            glPushMatrix();
                glTranslatef(elementCenter.x - quarterSize, elementCenter.y - quarterSize, elementCenter.z + quarterSize);
                glutWireCube(halfSize);
            glPopMatrix();

            glColor3f(1.0f, 1.0f, 1.0f);
            glPushMatrix();
                glTranslatef(elementCenter.x + quarterSize, elementCenter.y + quarterSize, elementCenter.z + quarterSize);
                glutWireCube(halfSize);
            glPopMatrix();

            glColor3f(0.0f, 0.5f, 0.5f);
            glPushMatrix();
                glTranslatef(elementCenter.x - quarterSize, elementCenter.y + quarterSize, elementCenter.z + quarterSize);
                glutWireCube(halfSize);
            glPopMatrix();

            glColor3f(0.5f, 0.0f, 0.0f);
            glPushMatrix();
                glTranslatef(elementCenter.x + quarterSize, elementCenter.y - quarterSize, elementCenter.z + quarterSize);
                glutWireCube(halfSize);
            glPopMatrix();

            glColor3f(0.0f, 0.5f, 0.0f);
            glPushMatrix();
                glTranslatef(elementCenter.x + quarterSize, elementCenter.y + quarterSize, elementCenter.z - quarterSize);
                glutWireCube(halfSize);
            glPopMatrix();
        }

    }

    for (uint16_t i = 0; i < numberOfEntities; i++) {
        EntityItem* entityItem = entityItems[i];
        bool wantDebug = false;
        
        if (wantDebug) {
            bool isBestFit = entityTreeElement->bestFitEntityBounds(entityItem);
            qDebug() << "EntityTreeRenderer::renderElement() "
                     << "entityTreeElement=" << entityTreeElement 
                     << "entityItems[" << i << "]=" << entityItem
                     << "ID=" << entityItem->getEntityItemID()
                     << "isBestFit=" << isBestFit;
        }
        
        // render entityItem aspoints
        AACube entityCube = entityItem->getAACube();
        entityCube.scale(TREE_SCALE);
        
        // TODO: some entity types (like lights) might want to be rendered even
        // when they are outside of the view frustum...
        if (args->_viewFrustum->cubeInFrustum(entityCube) != ViewFrustum::OUTSIDE) {
            Glower* glower = NULL;
            if (entityItem->getGlowLevel() > 0.0f) {
                glower = new Glower(entityItem->getGlowLevel());
            }
            entityItem->render(args);
            if (glower) {
                delete glower;
            }

        } else {
            args->_itemsOutOfView++;
        }
    }
}

float EntityTreeRenderer::getSizeScale() const { 
    return Menu::getInstance()->getVoxelSizeScale();
}

int EntityTreeRenderer::getBoundaryLevelAdjust() const { 
    return Menu::getInstance()->getBoundaryLevelAdjust();
}


void EntityTreeRenderer::processEraseMessage(const QByteArray& dataByteArray, const SharedNodePointer& sourceNode) {
    static_cast<EntityTree*>(_tree)->processEraseMessage(dataByteArray, sourceNode);
}





