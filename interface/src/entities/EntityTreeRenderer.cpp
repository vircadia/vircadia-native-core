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
#include <MouseEvent.h>
#include <PerfStat.h>
#include <RenderArgs.h>


#include "Menu.h"
#include "EntityTreeRenderer.h"

#include "devices/OculusManager.h"

#include "RenderableBoxEntityItem.h"
#include "RenderableLightEntityItem.h"
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
    REGISTER_ENTITY_TYPE_WITH_FACTORY(Light, RenderableLightEntityItem::factory)
    
    _currentHoverOverEntityID = EntityItemID::createInvalidEntityID(); // makes it the unknown ID
    _currentClickingOnEntityID = EntityItemID::createInvalidEntityID(); // makes it the unknown ID
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

void EntityTreeRenderer::render(RenderArgs::RenderMode renderMode) {
    OctreeRenderer::render(renderMode);
    deleteReleasedModels(); // seems like as good as any other place to do some memory cleanup
}

const FBXGeometry* EntityTreeRenderer::getGeometryForEntity(const EntityItem* entityItem) {
    const FBXGeometry* result = NULL;
    
    if (entityItem->getType() == EntityTypes::Model) {
        const RenderableModelEntityItem* constModelEntityItem = dynamic_cast<const RenderableModelEntityItem*>(entityItem);
        RenderableModelEntityItem* modelEntityItem = const_cast<RenderableModelEntityItem*>(constModelEntityItem);
        assert(modelEntityItem); // we need this!!!
        Model* model = modelEntityItem->getModel(this);
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
    
        result = modelEntityItem->getModel(this);
    }
    return result;
}

void renderElementProxy(EntityTreeElement* entityTreeElement) {
    glm::vec3 elementCenter = entityTreeElement->getAACube().calcCenter() * (float)TREE_SCALE;
    float elementSize = entityTreeElement->getScale() * (float)TREE_SCALE;
    glColor3f(1.0f, 0.0f, 0.0f);
    glPushMatrix();
        glTranslatef(elementCenter.x, elementCenter.y, elementCenter.z);
        glutWireCube(elementSize);
    glPopMatrix();

    bool displayElementChildProxies = Menu::getInstance()->isOptionChecked(MenuOption::DisplayModelElementChildProxies);

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

void EntityTreeRenderer::renderProxies(const EntityItem* entity, RenderArgs* args) {
    bool isShadowMode = args->_renderMode == RenderArgs::SHADOW_RENDER_MODE;
    bool displayModelBounds = Menu::getInstance()->isOptionChecked(MenuOption::DisplayModelBounds);
    if (!isShadowMode && displayModelBounds) {
        PerformanceTimer perfTimer("renderProxies");

        AACube maxCube = entity->getMaximumAACube();
        AACube minCube = entity->getMinimumAACube();
        AABox entityBox = entity->getAABox();

        maxCube.scale((float)TREE_SCALE);
        minCube.scale((float)TREE_SCALE);
        entityBox.scale((float)TREE_SCALE);

        glm::vec3 maxCenter = maxCube.calcCenter();
        glm::vec3 minCenter = minCube.calcCenter();
        glm::vec3 entityBoxCenter = entityBox.calcCenter();
        glm::vec3 entityBoxScale = entityBox.getScale();

        // draw the max bounding cube
        glColor4f(1.0f, 1.0f, 0.0f, 1.0f);
        glPushMatrix();
            glTranslatef(maxCenter.x, maxCenter.y, maxCenter.z);
            glutWireCube(maxCube.getScale());
        glPopMatrix();

        // draw the min bounding cube
        glColor4f(0.0f, 1.0f, 0.0f, 1.0f);
        glPushMatrix();
            glTranslatef(minCenter.x, minCenter.y, minCenter.z);
            glutWireCube(minCube.getScale());
        glPopMatrix();
        
        // draw the entityBox bounding box
        glColor4f(0.0f, 0.0f, 1.0f, 1.0f);
        glPushMatrix();
            glTranslatef(entityBoxCenter.x, entityBoxCenter.y, entityBoxCenter.z);
            glScalef(entityBoxScale.x, entityBoxScale.y, entityBoxScale.z);
            glutWireCube(1.0f);
        glPopMatrix();


        glm::vec3 position = entity->getPosition() * (float)TREE_SCALE;
        glm::vec3 center = entity->getCenter() * (float)TREE_SCALE;
        glm::vec3 dimensions = entity->getDimensions() * (float)TREE_SCALE;
        glm::quat rotation = entity->getRotation();

        glColor4f(1.0f, 0.0f, 1.0f, 1.0f);
        glPushMatrix();
            glTranslatef(position.x, position.y, position.z);
            glm::vec3 axis = glm::axis(rotation);
            glRotatef(glm::degrees(glm::angle(rotation)), axis.x, axis.y, axis.z);
            glPushMatrix();
                glm::vec3 positionToCenter = center - position;
                glTranslatef(positionToCenter.x, positionToCenter.y, positionToCenter.z);
                glScalef(dimensions.x, dimensions.y, dimensions.z);
                glutWireCube(1.0f);
            glPopMatrix();
        glPopMatrix();
    }
}

void EntityTreeRenderer::renderElement(OctreeElement* element, RenderArgs* args) {
    bool wantDebug = false;

    args->_elementsTouched++;
    // actually render it here...
    // we need to iterate the actual entityItems of the element
    EntityTreeElement* entityTreeElement = static_cast<EntityTreeElement*>(element);

    QList<EntityItem*>& entityItems = entityTreeElement->getEntities();
    

    uint16_t numberOfEntities = entityItems.size();

    bool isShadowMode = args->_renderMode == RenderArgs::SHADOW_RENDER_MODE;
    bool displayElementProxy = Menu::getInstance()->isOptionChecked(MenuOption::DisplayModelElementProxy);



    if (!isShadowMode && displayElementProxy && numberOfEntities > 0) {
        renderElementProxy(entityTreeElement);
    }
    
    for (uint16_t i = 0; i < numberOfEntities; i++) {
        EntityItem* entityItem = entityItems[i];
        
        if (entityItem->isVisible()) {
            // render entityItem 
            AABox entityBox = entityItem->getAABox();

            entityBox.scale(TREE_SCALE);
        
            // TODO: some entity types (like lights) might want to be rendered even
            // when they are outside of the view frustum...
            float distance = args->_viewFrustum->distanceToCamera(entityBox.calcCenter());
            
            if (wantDebug) {
                qDebug() << "------- renderElement() ----------";
                qDebug() << "                 type:" << EntityTypes::getEntityTypeName(entityItem->getType());
                if (entityItem->getType() == EntityTypes::Model) {
                    ModelEntityItem* modelEntity = static_cast<ModelEntityItem*>(entityItem);
                    qDebug() << "                  url:" << modelEntity->getModelURL();
                }
                qDebug() << "            entityBox:" << entityItem->getAABox();
                qDebug() << "           dimensions:" << entityItem->getDimensionsInMeters() << "in meters";
                qDebug() << "     largestDimension:" << entityBox.getLargestDimension() << "in meters";
                qDebug() << "         shouldRender:" << Menu::getInstance()->shouldRenderMesh(entityBox.getLargestDimension(), distance);
                qDebug() << "           in frustum:" << (args->_viewFrustum->boxInFrustum(entityBox) != ViewFrustum::OUTSIDE);
            }

            bool outOfView = args->_viewFrustum->boxInFrustum(entityBox) == ViewFrustum::OUTSIDE;
            if (!outOfView) {
                bool bigEnoughToRender = Menu::getInstance()->shouldRenderMesh(entityBox.getLargestDimension(), distance);
                
                if (bigEnoughToRender) {
                    renderProxies(entityItem, args);

                    Glower* glower = NULL;
                    if (entityItem->getGlowLevel() > 0.0f) {
                        glower = new Glower(entityItem->getGlowLevel());
                    }
                    entityItem->render(args);
                    args->_itemsRendered++;
                    if (glower) {
                        delete glower;
                    }
                } else {
                    args->_itemsTooSmall++;
                }
            } else {
                args->_itemsOutOfView++;
            }
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

Model* EntityTreeRenderer::allocateModel(const QString& url) {
    Model* model = NULL;
    // Make sure we only create and delete models on the thread that owns the EntityTreeRenderer
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "allocateModel", Qt::BlockingQueuedConnection, 
                Q_RETURN_ARG(Model*, model),
                Q_ARG(const QString&, url));

        return model;
    }
    model = new Model();
    model->init();
    model->setURL(QUrl(url));
    return model;
}

Model* EntityTreeRenderer::updateModel(Model* original, const QString& newUrl) {
    Model* model = NULL;

    // The caller shouldn't call us if the URL doesn't need to change. But if they
    // do, we just return their original back to them.
    if (!original || (QUrl(newUrl) == original->getURL())) {
        return original;
    }

    // Before we do any creating or deleting, make sure we're on our renderer thread
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "updateModel", Qt::BlockingQueuedConnection, 
                Q_RETURN_ARG(Model*, model),
                Q_ARG(Model*, original),
                Q_ARG(const QString&, newUrl));

        return model;
    }

    // at this point we know we need to replace the model, and we know we're on the
    // correct thread, so we can do all our work.
    if (original) {
        delete original; // delete the old model...
    }

    // create the model and correctly initialize it with the new url
    model = new Model();
    model->init();
    model->setURL(QUrl(newUrl));
        
    return model;
}

void EntityTreeRenderer::releaseModel(Model* model) {
    // If we're not on the renderer's thread, then remember this model to be deleted later
    if (QThread::currentThread() != thread()) {
        _releasedModels << model;
    } else { // otherwise just delete it right away
        delete model;
    }
}

void EntityTreeRenderer::deleteReleasedModels() {
    if (_releasedModels.size() > 0) {
        foreach(Model* model, _releasedModels) {
            delete model;
        }
        _releasedModels.clear();
    }
}

PickRay EntityTreeRenderer::computePickRay(float x, float y) {
    float screenWidth = Application::getInstance()->getGLWidget()->width();
    float screenHeight = Application::getInstance()->getGLWidget()->height();
    PickRay result;
    if (OculusManager::isConnected()) {
        Camera* camera = Application::getInstance()->getCamera();
        result.origin = camera->getPosition();
        Application::getInstance()->getApplicationOverlay().computeOculusPickRay(x / screenWidth, y / screenHeight, result.direction);
    } else {
        ViewFrustum* viewFrustum = Application::getInstance()->getViewFrustum();
        viewFrustum->computePickRay(x / screenWidth, y / screenHeight, result.origin, result.direction);
    }
    return result;
}


RayToEntityIntersectionResult EntityTreeRenderer::findRayIntersectionWorker(const PickRay& ray, Octree::lockType lockType) {
    RayToEntityIntersectionResult result;
    if (_tree) {
        EntityTree* entityTree = static_cast<EntityTree*>(_tree);

        OctreeElement* element;
        EntityItem* intersectedEntity = NULL;
        result.intersects = entityTree->findRayIntersection(ray.origin, ray.direction, element, result.distance, result.face, 
                                                                (void**)&intersectedEntity, lockType, &result.accurate);
        if (result.intersects && intersectedEntity) {
            result.entityID = intersectedEntity->getEntityItemID();
            result.properties = intersectedEntity->getProperties();
            result.intersection = ray.origin + (ray.direction * result.distance);
        }
    }
    return result;
}

void EntityTreeRenderer::connectSignalsToSlots(EntityScriptingInterface* entityScriptingInterface) {
    connect(this, &EntityTreeRenderer::mousePressOnEntity, entityScriptingInterface, &EntityScriptingInterface::mousePressOnEntity);
    connect(this, &EntityTreeRenderer::mouseMoveOnEntity, entityScriptingInterface, &EntityScriptingInterface::mouseMoveOnEntity);
    connect(this, &EntityTreeRenderer::mouseReleaseOnEntity, entityScriptingInterface, &EntityScriptingInterface::mouseReleaseOnEntity);

    connect(this, &EntityTreeRenderer::clickDownOnEntity, entityScriptingInterface, &EntityScriptingInterface::clickDownOnEntity);
    connect(this, &EntityTreeRenderer::holdingClickOnEntity, entityScriptingInterface, &EntityScriptingInterface::holdingClickOnEntity);
    connect(this, &EntityTreeRenderer::clickReleaseOnEntity, entityScriptingInterface, &EntityScriptingInterface::clickReleaseOnEntity);

    connect(this, &EntityTreeRenderer::hoverEnterEntity, entityScriptingInterface, &EntityScriptingInterface::hoverEnterEntity);
    connect(this, &EntityTreeRenderer::hoverOverEntity, entityScriptingInterface, &EntityScriptingInterface::hoverOverEntity);
    connect(this, &EntityTreeRenderer::hoverLeaveEntity, entityScriptingInterface, &EntityScriptingInterface::hoverLeaveEntity);
}

void EntityTreeRenderer::mousePressEvent(QMouseEvent* event, unsigned int deviceID) {
    PerformanceTimer perfTimer("EntityTreeRenderer::mousePressEvent");
    PickRay ray = computePickRay(event->x(), event->y());
    RayToEntityIntersectionResult rayPickResult = findRayIntersectionWorker(ray, Octree::Lock);
    if (rayPickResult.intersects) {
        //qDebug() << "mousePressEvent over entity:" << rayPickResult.entityID;
        emit mousePressOnEntity(rayPickResult.entityID, MouseEvent(*event, deviceID));
        
        _currentClickingOnEntityID = rayPickResult.entityID;
        emit clickDownOnEntity(_currentClickingOnEntityID, MouseEvent(*event, deviceID));
    }
}

void EntityTreeRenderer::mouseReleaseEvent(QMouseEvent* event, unsigned int deviceID) {
    PerformanceTimer perfTimer("EntityTreeRenderer::mouseReleaseEvent");
    PickRay ray = computePickRay(event->x(), event->y());
    RayToEntityIntersectionResult rayPickResult = findRayIntersectionWorker(ray, Octree::Lock);
    if (rayPickResult.intersects) {
        //qDebug() << "mouseReleaseEvent over entity:" << rayPickResult.entityID;
        emit mouseReleaseOnEntity(rayPickResult.entityID, MouseEvent(*event, deviceID));
    }
    
    // Even if we're no longer intersecting with an entity, if we started clicking on it, and now
    // we're releasing the button, then this is considered a clickOn event
    if (!_currentClickingOnEntityID.isInvalidID()) {
        emit clickReleaseOnEntity(_currentClickingOnEntityID, MouseEvent(*event, deviceID));
    }
    
    // makes it the unknown ID, we just released so we can't be clicking on anything
    _currentClickingOnEntityID = EntityItemID::createInvalidEntityID();
}

void EntityTreeRenderer::mouseMoveEvent(QMouseEvent* event, unsigned int deviceID) {
    PerformanceTimer perfTimer("EntityTreeRenderer::mouseMoveEvent");
    PickRay ray = computePickRay(event->x(), event->y());
    RayToEntityIntersectionResult rayPickResult = findRayIntersectionWorker(ray, Octree::TryLock);
    if (rayPickResult.intersects) {
        //qDebug() << "mouseMoveEvent over entity:" << rayPickResult.entityID;
        emit mouseMoveOnEntity(rayPickResult.entityID, MouseEvent(*event, deviceID));
        
        // handle the hover logic...
        
        // if we were previously hovering over an entity, and this new entity is not the same as our previous entity
        // then we need to send the hover leave.
        if (!_currentHoverOverEntityID.isInvalidID() && rayPickResult.entityID != _currentHoverOverEntityID) {
            emit hoverLeaveEntity(_currentHoverOverEntityID, MouseEvent(*event, deviceID));
        }

        // If the new hover entity does not match the previous hover entity then we are entering the new one
        // this is true if the _currentHoverOverEntityID is known or unknown
        if (rayPickResult.entityID != _currentHoverOverEntityID) {
            emit hoverEnterEntity(rayPickResult.entityID, MouseEvent(*event, deviceID));
        }

        // and finally, no matter what, if we're intersecting an entity then we're definitely hovering over it, and
        // we should send our hover over event
        emit hoverOverEntity(rayPickResult.entityID, MouseEvent(*event, deviceID));

        // remember what we're hovering over
        _currentHoverOverEntityID = rayPickResult.entityID;

    } else {
        // handle the hover logic...
        // if we were previously hovering over an entity, and we're no longer hovering over any entity then we need to 
        // send the hover leave for our previous entity
        if (!_currentHoverOverEntityID.isInvalidID()) {
            emit hoverLeaveEntity(_currentHoverOverEntityID, MouseEvent(*event, deviceID));
            _currentHoverOverEntityID = EntityItemID::createInvalidEntityID(); // makes it the unknown ID
        }
    }
    
    // Even if we're no longer intersecting with an entity, if we started clicking on an entity and we have
    // not yet released the hold then this is still considered a holdingClickOnEntity event
    if (!_currentClickingOnEntityID.isInvalidID()) {
        emit holdingClickOnEntity(_currentClickingOnEntityID, MouseEvent(*event, deviceID));
    }
}


