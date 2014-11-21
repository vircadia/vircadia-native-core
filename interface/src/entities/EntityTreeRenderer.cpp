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

#include <QScriptSyntaxCheckResult>
 
#include <FBXReader.h>

#include "InterfaceConfig.h"

#include <BoxEntityItem.h>
#include <ModelEntityItem.h>
#include <MouseEvent.h>
#include <PerfStat.h>
#include <RenderArgs.h>


#include "Menu.h"
#include "NetworkAccessManager.h"
#include "EntityTreeRenderer.h"

#include "devices/OculusManager.h"

#include "RenderableBoxEntityItem.h"
#include "RenderableLightEntityItem.h"
#include "RenderableModelEntityItem.h"
#include "RenderableSphereEntityItem.h"
#include "RenderableTextEntityItem.h"


QThread* EntityTreeRenderer::getMainThread() {
    return Application::getInstance()->getEntities()->thread();
}



EntityTreeRenderer::EntityTreeRenderer(bool wantScripts) :
    OctreeRenderer(),
    _wantScripts(wantScripts),
    _entitiesScriptEngine(NULL),
    _lastMouseEventValid(false)
{
    REGISTER_ENTITY_TYPE_WITH_FACTORY(Model, RenderableModelEntityItem::factory)
    REGISTER_ENTITY_TYPE_WITH_FACTORY(Box, RenderableBoxEntityItem::factory)
    REGISTER_ENTITY_TYPE_WITH_FACTORY(Sphere, RenderableSphereEntityItem::factory)
    REGISTER_ENTITY_TYPE_WITH_FACTORY(Light, RenderableLightEntityItem::factory)
    REGISTER_ENTITY_TYPE_WITH_FACTORY(Text, RenderableTextEntityItem::factory)
    
    _currentHoverOverEntityID = EntityItemID::createInvalidEntityID(); // makes it the unknown ID
    _currentClickingOnEntityID = EntityItemID::createInvalidEntityID(); // makes it the unknown ID
}

EntityTreeRenderer::~EntityTreeRenderer() {
    // do we need to delete the _entitiesScriptEngine?? or is it deleted by default
}

void EntityTreeRenderer::clear() {
    OctreeRenderer::clear();
    _entityScripts.clear();
}

void EntityTreeRenderer::init() {
    OctreeRenderer::init();
    EntityTree* entityTree = static_cast<EntityTree*>(_tree);
    entityTree->setFBXService(this);

    if (_wantScripts) {
        _entitiesScriptEngine = new ScriptEngine(NO_SCRIPT, "Entities", 
                                        Application::getInstance()->getControllerScriptingInterface());
        Application::getInstance()->registerScriptEngineWithApplicationServices(_entitiesScriptEngine);
    }

    // make sure our "last avatar position" is something other than our current position, so that on our
    // first chance, we'll check for enter/leave entity events.    
    glm::vec3 avatarPosition = Application::getInstance()->getAvatar()->getPosition();
    _lastAvatarPosition = avatarPosition + glm::vec3(1.0f, 1.0f, 1.0f);
    
    connect(entityTree, &EntityTree::deletingEntity, this, &EntityTreeRenderer::deletingEntity);
    connect(entityTree, &EntityTree::addingEntity, this, &EntityTreeRenderer::checkAndCallPreload);
    connect(entityTree, &EntityTree::entityScriptChanging, this, &EntityTreeRenderer::entitySciptChanging);
    connect(entityTree, &EntityTree::changingEntityID, this, &EntityTreeRenderer::changingEntityID);
}

QScriptValue EntityTreeRenderer::loadEntityScript(const EntityItemID& entityItemID) {
    EntityItem* entity = static_cast<EntityTree*>(_tree)->findEntityByEntityItemID(entityItemID);
    return loadEntityScript(entity);
}


QString EntityTreeRenderer::loadScriptContents(const QString& scriptMaybeURLorText) {
    QUrl url(scriptMaybeURLorText);
    
    // If the url is not valid, this must be script text...
    if (!url.isValid()) {
        return scriptMaybeURLorText;
    }

    QString scriptContents; // assume empty
    
    // if the scheme length is one or lower, maybe they typed in a file, let's try
    const int WINDOWS_DRIVE_LETTER_SIZE = 1;
    if (url.scheme().size() <= WINDOWS_DRIVE_LETTER_SIZE) {
        url = QUrl::fromLocalFile(scriptMaybeURLorText);
    }

    // ok, let's see if it's valid... and if so, load it
    if (url.isValid()) {
        if (url.scheme() == "file") {
            QString fileName = url.toLocalFile();
            QFile scriptFile(fileName);
            if (scriptFile.open(QFile::ReadOnly | QFile::Text)) {
                qDebug() << "Loading file:" << fileName;
                QTextStream in(&scriptFile);
                scriptContents = in.readAll();
            } else {
                qDebug() << "ERROR Loading file:" << fileName;
            }
        } else {
            QNetworkAccessManager& networkAccessManager = NetworkAccessManager::getInstance();
            QNetworkReply* reply = networkAccessManager.get(QNetworkRequest(url));
            qDebug() << "Downloading script at" << url;
            QEventLoop loop;
            QObject::connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
            loop.exec();
            if (reply->error() == QNetworkReply::NoError && reply->attribute(QNetworkRequest::HttpStatusCodeAttribute) == 200) {
                scriptContents = reply->readAll();
            } else {
                qDebug() << "ERROR Loading file:" << url.toString();
            }
        }
    }
    
    return scriptContents;
}


QScriptValue EntityTreeRenderer::loadEntityScript(EntityItem* entity) {
    if (!entity) {
        return QScriptValue(); // no entity...
    }
    
    EntityItemID entityID = entity->getEntityItemID();
    if (_entityScripts.contains(entityID)) {
        EntityScriptDetails details = _entityScripts[entityID];
        
        // check to make sure our script text hasn't changed on us since we last loaded it
        if (details.scriptText == entity->getScript()) {
            return details.scriptObject; // previously loaded
        }
        
        // if we got here, then we previously loaded a script, but the entity's script value
        // has changed and so we need to reload it.
        _entityScripts.remove(entityID);
    }
    if (entity->getScript().isEmpty()) {
        return QScriptValue(); // no script
    }
    
    QString scriptContents = loadScriptContents(entity->getScript());
    
    QScriptSyntaxCheckResult syntaxCheck = QScriptEngine::checkSyntax(scriptContents);
    if (syntaxCheck.state() != QScriptSyntaxCheckResult::Valid) {
        qDebug() << "EntityTreeRenderer::loadEntityScript() entity:" << entityID;
        qDebug() << "   " << syntaxCheck.errorMessage() << ":"
                          << syntaxCheck.errorLineNumber() << syntaxCheck.errorColumnNumber();
        qDebug() << "    SCRIPT:" << entity->getScript();
        return QScriptValue(); // invalid script
    }
    
    QScriptValue entityScriptConstructor = _entitiesScriptEngine->evaluate(scriptContents);
    
    if (!entityScriptConstructor.isFunction()) {
        qDebug() << "EntityTreeRenderer::loadEntityScript() entity:" << entityID;
        qDebug() << "    NOT CONSTRUCTOR";
        qDebug() << "    SCRIPT:" << entity->getScript();
        return QScriptValue(); // invalid script
    }

    QScriptValue entityScriptObject = entityScriptConstructor.construct();
    EntityScriptDetails newDetails = { entity->getScript(), entityScriptObject };
    _entityScripts[entityID] = newDetails;

    return entityScriptObject; // newly constructed
}

QScriptValue EntityTreeRenderer::getPreviouslyLoadedEntityScript(const EntityItemID& entityItemID) {
    EntityItem* entity = static_cast<EntityTree*>(_tree)->findEntityByEntityItemID(entityItemID);
    return getPreviouslyLoadedEntityScript(entity);
}


QScriptValue EntityTreeRenderer::getPreviouslyLoadedEntityScript(EntityItem* entity) {
    if (entity) {
        EntityItemID entityID = entity->getEntityItemID();
        if (_entityScripts.contains(entityID)) {
            EntityScriptDetails details = _entityScripts[entityID];
            return details.scriptObject; // previously loaded
        }
    }
    return QScriptValue(); // no script
}
void EntityTreeRenderer::setTree(Octree* newTree) {
    OctreeRenderer::setTree(newTree);
    static_cast<EntityTree*>(_tree)->setFBXService(this);
}

void EntityTreeRenderer::update() {
    if (_tree) {
        EntityTree* tree = static_cast<EntityTree*>(_tree);
        tree->update();
        
        // check to see if the avatar has moved and if we need to handle enter/leave entity logic
        checkEnterLeaveEntities();

        // Even if we're not moving the mouse, if we started clicking on an entity and we have
        // not yet released the hold then this is still considered a holdingClickOnEntity event
        // and we want to simulate this message here as well as in mouse move
        if (_lastMouseEventValid && !_currentClickingOnEntityID.isInvalidID()) {
            emit holdingClickOnEntity(_currentClickingOnEntityID, _lastMouseEvent);
            QScriptValueList currentClickingEntityArgs = createMouseEventArgs(_currentClickingOnEntityID, _lastMouseEvent);
            QScriptValue currentClickingEntity = loadEntityScript(_currentClickingOnEntityID);
            if (currentClickingEntity.property("holdingClickOnEntity").isValid()) {
                currentClickingEntity.property("holdingClickOnEntity").call(currentClickingEntity, currentClickingEntityArgs);
            }
        }

    }
}

void EntityTreeRenderer::checkEnterLeaveEntities() {
    if (_tree) {
        _tree->lockForRead();
        glm::vec3 avatarPosition = Application::getInstance()->getAvatar()->getPosition() / (float) TREE_SCALE;
        
        if (avatarPosition != _lastAvatarPosition) {
            float radius = 1.0f / (float) TREE_SCALE; // for now, assume 1 meter radius
            QVector<const EntityItem*> foundEntities;
            QVector<EntityItemID> entitiesContainingAvatar;
            
            // find the entities near us
            static_cast<EntityTree*>(_tree)->findEntities(avatarPosition, radius, foundEntities);

            // create a list of entities that actually contain the avatar's position
            foreach(const EntityItem* entity, foundEntities) {
                if (entity->contains(avatarPosition)) {
                    entitiesContainingAvatar << entity->getEntityItemID();
                }
            }

            // for all of our previous containing entities, if they are no longer containing then send them a leave event
            foreach(const EntityItemID& entityID, _currentEntitiesInside) {
                if (!entitiesContainingAvatar.contains(entityID)) {
                    emit leaveEntity(entityID);
                    QScriptValueList entityArgs = createEntityArgs(entityID);
                    QScriptValue entityScript = loadEntityScript(entityID);
                    if (entityScript.property("leaveEntity").isValid()) {
                        entityScript.property("leaveEntity").call(entityScript, entityArgs);
                    }

                }
            }

            // for all of our new containing entities, if they weren't previously containing then send them an enter event
            foreach(const EntityItemID& entityID, entitiesContainingAvatar) {
                if (!_currentEntitiesInside.contains(entityID)) {
                    emit enterEntity(entityID);
                    QScriptValueList entityArgs = createEntityArgs(entityID);
                    QScriptValue entityScript = loadEntityScript(entityID);
                    if (entityScript.property("enterEntity").isValid()) {
                        entityScript.property("enterEntity").call(entityScript, entityArgs);
                    }
                }
            }
            _currentEntitiesInside = entitiesContainingAvatar;
            _lastAvatarPosition = avatarPosition;
        }
        _tree->unlock();
    }
}

void EntityTreeRenderer::render(RenderArgs::RenderMode renderMode, RenderArgs::RenderSide renderSide) {
    bool dontRenderAsScene = !Menu::getInstance()->isOptionChecked(MenuOption::RenderEntitiesAsScene);
    
    if (dontRenderAsScene) {
        OctreeRenderer::render(renderMode, renderSide);
    } else {
        if (_tree) {
            Model::startScene(renderSide);
            RenderArgs args = { this, _viewFrustum, getSizeScale(), getBoundaryLevelAdjust(), renderMode, renderSide,
                                                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
            _tree->lockForRead();
            _tree->recurseTreeWithOperation(renderOperation, &args);

            Model::RenderMode modelRenderMode = renderMode == RenderArgs::SHADOW_RENDER_MODE
                                                ? Model::SHADOW_RENDER_MODE : Model::DEFAULT_RENDER_MODE;

            // we must call endScene while we still have the tree locked so that no one deletes a model
            // on us while rendering the scene    
            Model::endScene(modelRenderMode, &args);
            _tree->unlock();
        
            // stats...
            _meshesConsidered = args._meshesConsidered;
            _meshesRendered = args._meshesRendered;
            _meshesOutOfView = args._meshesOutOfView;
            _meshesTooSmall = args._meshesTooSmall;

            _elementsTouched = args._elementsTouched;
            _itemsRendered = args._itemsRendered;
            _itemsOutOfView = args._itemsOutOfView;
            _itemsTooSmall = args._itemsTooSmall;

            _materialSwitches = args._materialSwitches;
            _trianglesRendered = args._trianglesRendered;
            _quadsRendered = args._quadsRendered;

            _translucentMeshPartsRendered = args._translucentMeshPartsRendered;
            _opaqueMeshPartsRendered = args._opaqueMeshPartsRendered;
        }
    }
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
    glm::vec3 elementCenter = entityTreeElement->getAACube().calcCenter() * (float) TREE_SCALE;
    float elementSize = entityTreeElement->getScale() * (float) TREE_SCALE;
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

        maxCube.scale((float) TREE_SCALE);
        minCube.scale((float) TREE_SCALE);
        entityBox.scale((float) TREE_SCALE);

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


        glm::vec3 position = entity->getPosition() * (float) TREE_SCALE;
        glm::vec3 center = entity->getCenter() * (float) TREE_SCALE;
        glm::vec3 dimensions = entity->getDimensions() * (float) TREE_SCALE;
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
            result.entity = intersectedEntity;
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

    connect(this, &EntityTreeRenderer::enterEntity, entityScriptingInterface, &EntityScriptingInterface::enterEntity);
    connect(this, &EntityTreeRenderer::leaveEntity, entityScriptingInterface, &EntityScriptingInterface::leaveEntity);
}

QScriptValueList EntityTreeRenderer::createMouseEventArgs(const EntityItemID& entityID, QMouseEvent* event, unsigned int deviceID) {
    QScriptValueList args;
    args << entityID.toScriptValue(_entitiesScriptEngine);
    args << MouseEvent(*event, deviceID).toScriptValue(_entitiesScriptEngine);
    return args;
}

QScriptValueList EntityTreeRenderer::createMouseEventArgs(const EntityItemID& entityID, const MouseEvent& mouseEvent) {
    QScriptValueList args;
    args << entityID.toScriptValue(_entitiesScriptEngine);
    args << mouseEvent.toScriptValue(_entitiesScriptEngine);
    return args;
}


QScriptValueList EntityTreeRenderer::createEntityArgs(const EntityItemID& entityID) {
    QScriptValueList args;
    args << entityID.toScriptValue(_entitiesScriptEngine);
    return args;
}

void EntityTreeRenderer::mousePressEvent(QMouseEvent* event, unsigned int deviceID) {
    PerformanceTimer perfTimer("EntityTreeRenderer::mousePressEvent");
    PickRay ray = computePickRay(event->x(), event->y());
    RayToEntityIntersectionResult rayPickResult = findRayIntersectionWorker(ray, Octree::Lock);
    if (rayPickResult.intersects) {
        //qDebug() << "mousePressEvent over entity:" << rayPickResult.entityID;
        emit mousePressOnEntity(rayPickResult.entityID, MouseEvent(*event, deviceID));

        QScriptValueList entityScriptArgs = createMouseEventArgs(rayPickResult.entityID, event, deviceID);
        QScriptValue entityScript = loadEntityScript(rayPickResult.entity);
        if (entityScript.property("mousePressOnEntity").isValid()) {
            entityScript.property("mousePressOnEntity").call(entityScript, entityScriptArgs);
        }
        
        _currentClickingOnEntityID = rayPickResult.entityID;
        emit clickDownOnEntity(_currentClickingOnEntityID, MouseEvent(*event, deviceID));
        if (entityScript.property("clickDownOnEntity").isValid()) {
            entityScript.property("clickDownOnEntity").call(entityScript, entityScriptArgs);
        }
    }
    _lastMouseEvent = MouseEvent(*event, deviceID);
    _lastMouseEventValid = true;
}

void EntityTreeRenderer::mouseReleaseEvent(QMouseEvent* event, unsigned int deviceID) {
    PerformanceTimer perfTimer("EntityTreeRenderer::mouseReleaseEvent");
    PickRay ray = computePickRay(event->x(), event->y());
    RayToEntityIntersectionResult rayPickResult = findRayIntersectionWorker(ray, Octree::Lock);
    if (rayPickResult.intersects) {
        //qDebug() << "mouseReleaseEvent over entity:" << rayPickResult.entityID;
        emit mouseReleaseOnEntity(rayPickResult.entityID, MouseEvent(*event, deviceID));

        QScriptValueList entityScriptArgs = createMouseEventArgs(rayPickResult.entityID, event, deviceID);
        QScriptValue entityScript = loadEntityScript(rayPickResult.entity);
        if (entityScript.property("mouseReleaseOnEntity").isValid()) {
            entityScript.property("mouseReleaseOnEntity").call(entityScript, entityScriptArgs);
        }
    }
    
    // Even if we're no longer intersecting with an entity, if we started clicking on it, and now
    // we're releasing the button, then this is considered a clickOn event
    if (!_currentClickingOnEntityID.isInvalidID()) {
        emit clickReleaseOnEntity(_currentClickingOnEntityID, MouseEvent(*event, deviceID));

        QScriptValueList currentClickingEntityArgs = createMouseEventArgs(_currentClickingOnEntityID, event, deviceID);
        QScriptValue currentClickingEntity = loadEntityScript(_currentClickingOnEntityID);
        if (currentClickingEntity.property("clickReleaseOnEntity").isValid()) {
            currentClickingEntity.property("clickReleaseOnEntity").call(currentClickingEntity, currentClickingEntityArgs);
        }
    }
    
    // makes it the unknown ID, we just released so we can't be clicking on anything
    _currentClickingOnEntityID = EntityItemID::createInvalidEntityID();
    _lastMouseEvent = MouseEvent(*event, deviceID);
    _lastMouseEventValid = true;
}

void EntityTreeRenderer::mouseMoveEvent(QMouseEvent* event, unsigned int deviceID) {
    PerformanceTimer perfTimer("EntityTreeRenderer::mouseMoveEvent");

    PickRay ray = computePickRay(event->x(), event->y());
    RayToEntityIntersectionResult rayPickResult = findRayIntersectionWorker(ray, Octree::TryLock);
    if (rayPickResult.intersects) {
        QScriptValueList entityScriptArgs = createMouseEventArgs(rayPickResult.entityID, event, deviceID);

        // load the entity script if needed...
        QScriptValue entityScript = loadEntityScript(rayPickResult.entity);
        if (entityScript.property("mouseMoveEvent").isValid()) {
            entityScript.property("mouseMoveEvent").call(entityScript, entityScriptArgs);
        }
    
        //qDebug() << "mouseMoveEvent over entity:" << rayPickResult.entityID;
        emit mouseMoveOnEntity(rayPickResult.entityID, MouseEvent(*event, deviceID));
        if (entityScript.property("mouseMoveOnEntity").isValid()) {
            entityScript.property("mouseMoveOnEntity").call(entityScript, entityScriptArgs);
        }
        
        // handle the hover logic...
        
        // if we were previously hovering over an entity, and this new entity is not the same as our previous entity
        // then we need to send the hover leave.
        if (!_currentHoverOverEntityID.isInvalidID() && rayPickResult.entityID != _currentHoverOverEntityID) {
            emit hoverLeaveEntity(_currentHoverOverEntityID, MouseEvent(*event, deviceID));

            QScriptValueList currentHoverEntityArgs = createMouseEventArgs(_currentHoverOverEntityID, event, deviceID);

            QScriptValue currentHoverEntity = loadEntityScript(_currentHoverOverEntityID);
            if (currentHoverEntity.property("hoverLeaveEntity").isValid()) {
                currentHoverEntity.property("hoverLeaveEntity").call(currentHoverEntity, currentHoverEntityArgs);
            }
        }

        // If the new hover entity does not match the previous hover entity then we are entering the new one
        // this is true if the _currentHoverOverEntityID is known or unknown
        if (rayPickResult.entityID != _currentHoverOverEntityID) {
            emit hoverEnterEntity(rayPickResult.entityID, MouseEvent(*event, deviceID));
            if (entityScript.property("hoverEnterEntity").isValid()) {
                entityScript.property("hoverEnterEntity").call(entityScript, entityScriptArgs);
            }
        }

        // and finally, no matter what, if we're intersecting an entity then we're definitely hovering over it, and
        // we should send our hover over event
        emit hoverOverEntity(rayPickResult.entityID, MouseEvent(*event, deviceID));
        if (entityScript.property("hoverOverEntity").isValid()) {
            entityScript.property("hoverOverEntity").call(entityScript, entityScriptArgs);
        }

        // remember what we're hovering over
        _currentHoverOverEntityID = rayPickResult.entityID;

    } else {
        // handle the hover logic...
        // if we were previously hovering over an entity, and we're no longer hovering over any entity then we need to 
        // send the hover leave for our previous entity
        if (!_currentHoverOverEntityID.isInvalidID()) {
            emit hoverLeaveEntity(_currentHoverOverEntityID, MouseEvent(*event, deviceID));

            QScriptValueList currentHoverEntityArgs = createMouseEventArgs(_currentHoverOverEntityID, event, deviceID);

            QScriptValue currentHoverEntity = loadEntityScript(_currentHoverOverEntityID);
            if (currentHoverEntity.property("hoverLeaveEntity").isValid()) {
                currentHoverEntity.property("hoverLeaveEntity").call(currentHoverEntity, currentHoverEntityArgs);
            }

            _currentHoverOverEntityID = EntityItemID::createInvalidEntityID(); // makes it the unknown ID
        }
    }
    
    // Even if we're no longer intersecting with an entity, if we started clicking on an entity and we have
    // not yet released the hold then this is still considered a holdingClickOnEntity event
    if (!_currentClickingOnEntityID.isInvalidID()) {
        emit holdingClickOnEntity(_currentClickingOnEntityID, MouseEvent(*event, deviceID));

        QScriptValueList currentClickingEntityArgs = createMouseEventArgs(_currentClickingOnEntityID, event, deviceID);

        QScriptValue currentClickingEntity = loadEntityScript(_currentClickingOnEntityID);
        if (currentClickingEntity.property("holdingClickOnEntity").isValid()) {
            currentClickingEntity.property("holdingClickOnEntity").call(currentClickingEntity, currentClickingEntityArgs);
        }
    }
    _lastMouseEvent = MouseEvent(*event, deviceID);
    _lastMouseEventValid = true;
}

void EntityTreeRenderer::deletingEntity(const EntityItemID& entityID) {

    checkAndCallUnload(entityID);
    _entityScripts.remove(entityID);
}

void EntityTreeRenderer::entitySciptChanging(const EntityItemID& entityID) {
    checkAndCallUnload(entityID);
    checkAndCallPreload(entityID);
}

void EntityTreeRenderer::checkAndCallPreload(const EntityItemID& entityID) {
    // load the entity script if needed...
    QScriptValue entityScript = loadEntityScript(entityID);
    if (entityScript.property("preload").isValid()) {
        QScriptValueList entityArgs = createEntityArgs(entityID);
        entityScript.property("preload").call(entityScript, entityArgs);
    }
}

void EntityTreeRenderer::checkAndCallUnload(const EntityItemID& entityID) {
    QScriptValue entityScript = getPreviouslyLoadedEntityScript(entityID);
    if (entityScript.property("unload").isValid()) {
        QScriptValueList entityArgs = createEntityArgs(entityID);
        entityScript.property("unload").call(entityScript, entityArgs);
    }
}


void EntityTreeRenderer::changingEntityID(const EntityItemID& oldEntityID, const EntityItemID& newEntityID) {
    if (_entityScripts.contains(oldEntityID)) {
        EntityScriptDetails details = _entityScripts[oldEntityID];
        _entityScripts.remove(oldEntityID);
        _entityScripts[newEntityID] = details;
    }
}

