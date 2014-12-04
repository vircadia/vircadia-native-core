//
//  RenderableModelEntityItem.cpp
//  interface/src
//
//  Created by Brad Hefta-Gaub on 8/6/14.
//  Copyright 2014 High Fidelity, Inc.
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
#include "RenderableModelEntityItem.h"

EntityItem* RenderableModelEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    return new RenderableModelEntityItem(entityID, properties);
}

RenderableModelEntityItem::~RenderableModelEntityItem() {
    assert(_myRenderer || !_model); // if we have a model, we need to know our renderer
    if (_myRenderer && _model) {
        _myRenderer->releaseModel(_model);
        _model = NULL;
    }
}

bool RenderableModelEntityItem::setProperties(const EntityItemProperties& properties, bool forceCopy) {
    QString oldModelURL = getModelURL();
    bool somethingChanged = ModelEntityItem::setProperties(properties, forceCopy);
    if (somethingChanged && oldModelURL != getModelURL()) {
        _needsModelReload = true;
    }
    return somethingChanged;
}

int RenderableModelEntityItem::readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead, 
                                                ReadBitstreamToTreeParams& args,
                                                EntityPropertyFlags& propertyFlags, bool overwriteLocalData) {
    QString oldModelURL = getModelURL();
    int bytesRead = ModelEntityItem::readEntitySubclassDataFromBuffer(data, bytesLeftToRead, 
                                                                        args, propertyFlags, overwriteLocalData);
    if (oldModelURL != getModelURL()) {
        _needsModelReload = true;
    }
    return bytesRead;
}

void RenderableModelEntityItem::remapTextures() {
    if (!_model) {
        return; // nothing to do if we don't have a model
    }
    
    if (!_model->isLoadedWithTextures()) {
        return; // nothing to do if the model has not yet loaded it's default textures
    }
    
    if (!_originalTexturesRead && _model->isLoadedWithTextures()) {
        const QSharedPointer<NetworkGeometry>& networkGeometry = _model->getGeometry();
        if (networkGeometry) {
            _originalTextures = networkGeometry->getTextureNames();
            _originalTexturesRead = true;
        }
    }
    
    if (_currentTextures == _textures) {
        return; // nothing to do if our recently mapped textures match our desired textures
    }
    
    // since we're changing here, we need to run through our current texture map
    // and any textures in the recently mapped texture, that is not in our desired
    // textures, we need to "unset"
    QJsonDocument currentTexturesAsJson = QJsonDocument::fromJson(_currentTextures.toUtf8());
    QJsonObject currentTexturesAsJsonObject = currentTexturesAsJson.object();
    QVariantMap currentTextureMap = currentTexturesAsJsonObject.toVariantMap();

    QJsonDocument texturesAsJson = QJsonDocument::fromJson(_textures.toUtf8());
    QJsonObject texturesAsJsonObject = texturesAsJson.object();
    QVariantMap textureMap = texturesAsJsonObject.toVariantMap();

    foreach(const QString& key, currentTextureMap.keys()) {
        // if the desired texture map (what we're setting the textures to) doesn't
        // contain this texture, then remove it by setting the URL to null
        if (!textureMap.contains(key)) {
            QUrl noURL;
            qDebug() << "Removing texture named" << key << "by replacing it with no URL";
            _model->setTextureWithNameToURL(key, noURL);
        }
    }

    // here's where we remap any textures if needed...
    foreach(const QString& key, textureMap.keys()) {
        QUrl newTextureURL = textureMap[key].toUrl();
        qDebug() << "Updating texture named" << key << "to texture at URL" << newTextureURL;
        _model->setTextureWithNameToURL(key, newTextureURL);
    }
    
    _currentTextures = _textures;
}


void RenderableModelEntityItem::render(RenderArgs* args) {
    PerformanceTimer perfTimer("RMEIrender");
    assert(getType() == EntityTypes::Model);
    
    bool drawAsModel = hasModel();

    glm::vec3 position = getPosition() * (float)TREE_SCALE;
    float size = getSize() * (float)TREE_SCALE;
    glm::vec3 dimensions = getDimensions() * (float)TREE_SCALE;
    
    if (drawAsModel) {
        remapTextures();
        glPushMatrix();
        {
            float alpha = getLocalRenderAlpha();

            if (!_model || _needsModelReload) {
                // TODO: this getModel() appears to be about 3% of model render time. We should optimize
                PerformanceTimer perfTimer("getModel");
                EntityTreeRenderer* renderer = static_cast<EntityTreeRenderer*>(args->_renderer);
                getModel(renderer);
            }
            
            
            

            if (_model) {
                // handle animations..
                if (hasAnimation()) {
                    if (!jointsMapped()) {
                        QStringList modelJointNames = _model->getJointNames();
                        mapJoints(modelJointNames);
                    }

                    if (jointsMapped()) {
                        QVector<glm::quat> frameData = getAnimationFrame();
                        for (int i = 0; i < frameData.size(); i++) {
                            _model->setJointState(i, true, frameData[i]);
                        }
                    }
                }

                glm::quat rotation = getRotation();
                if (needsSimulation() && _model->isActive()) {
                    _model->setScaleToFit(true, dimensions);
                    _model->setSnapModelToRegistrationPoint(true, getRegistrationPoint());
                    _model->setRotation(rotation);
                    _model->setTranslation(position);
                    
                    // make sure to simulate so everything gets set up correctly for rendering
                    {
                        PerformanceTimer perfTimer("_model->simulate");
                        _model->simulate(0.0f);
                    }
                    _needsInitialSimulation = false;
                }

                // TODO: should we allow entityItems to have alpha on their models?
                Model::RenderMode modelRenderMode = args->_renderMode == RenderArgs::SHADOW_RENDER_MODE
                                                        ? Model::SHADOW_RENDER_MODE : Model::DEFAULT_RENDER_MODE;
        
                if (_model->isActive()) {
                    // TODO: this is the majority of model render time. And rendering of a cube model vs the basic Box render
                    // is significantly more expensive. Is there a way to call this that doesn't cost us as much? 
                    PerformanceTimer perfTimer("model->render");
                    bool dontRenderAsScene = Menu::getInstance()->isOptionChecked(MenuOption::DontRenderEntitiesAsScene);
                    if (dontRenderAsScene) {
                        _model->render(alpha, modelRenderMode, args);
                    } else {
                        _model->renderInScene(alpha, args);
                    }
                } else {
                    // if we couldn't get a model, then just draw a cube
                    glColor3ub(getColor()[RED_INDEX],getColor()[GREEN_INDEX],getColor()[BLUE_INDEX]);
                    glPushMatrix();
                        glTranslatef(position.x, position.y, position.z);
                        Application::getInstance()->getDeferredLightingEffect()->renderWireCube(size);
                    glPopMatrix();
                }
            } else {
                // if we couldn't get a model, then just draw a cube
                glColor3ub(getColor()[RED_INDEX],getColor()[GREEN_INDEX],getColor()[BLUE_INDEX]);
                glPushMatrix();
                    glTranslatef(position.x, position.y, position.z);
                    Application::getInstance()->getDeferredLightingEffect()->renderWireCube(size);
                glPopMatrix();
            }
        }
        glPopMatrix();
    } else {
        glColor3ub(getColor()[RED_INDEX],getColor()[GREEN_INDEX],getColor()[BLUE_INDEX]);
        glPushMatrix();
        glTranslatef(position.x, position.y, position.z);
        Application::getInstance()->getDeferredLightingEffect()->renderWireCube(size);
        glPopMatrix();
    }
}

Model* RenderableModelEntityItem::getModel(EntityTreeRenderer* renderer) {
    Model* result = NULL;

    // make sure our renderer is setup
    if (!_myRenderer) {
        _myRenderer = renderer;
    }
    assert(_myRenderer == renderer); // you should only ever render on one renderer
    
    if (QThread::currentThread() != _myRenderer->thread()) {
        return _model;
    }
    
    _needsModelReload = false; // this is the reload

    // if we have a URL, then we will want to end up returning a model...
    if (!getModelURL().isEmpty()) {
    
        // if we have a previously allocated model, but it's URL doesn't match
        // then we need to let our renderer update our model for us.
        if (_model && QUrl(getModelURL()) != _model->getURL()) {
            result = _model = _myRenderer->updateModel(_model, getModelURL());
            _needsInitialSimulation = true;
        } else if (!_model) { // if we don't yet have a model, then we want our renderer to allocate one
            result = _model = _myRenderer->allocateModel(getModelURL());
            _needsInitialSimulation = true;
        } else { // we already have the model we want...
            result = _model;
        }
    } else { // if our desired URL is empty, we may need to delete our existing model
        if (_model) {
            _myRenderer->releaseModel(_model);
            result = _model = NULL;
            _needsInitialSimulation = true;
        }
    }
    
    return result;
}

bool RenderableModelEntityItem::needsSimulation() const {
    return _needsInitialSimulation || getSimulationState() == EntityItem::Moving;
}

EntityItemProperties RenderableModelEntityItem::getProperties() const {
    EntityItemProperties properties = ModelEntityItem::getProperties(); // get the properties from our base class
    if (_originalTexturesRead) {
        properties.setTextureNames(_originalTextures);
    }
    return properties;
}

bool RenderableModelEntityItem::findDetailedRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
                         bool& keepSearching, OctreeElement*& element, float& distance, BoxFace& face, 
                         void** intersectedObject) const {
                         
    // extents is the entity relative, scaled, centered extents of the entity
    glm::mat4 rotation = glm::mat4_cast(getRotation());
    glm::mat4 translation = glm::translate(getPosition());
    glm::mat4 entityToWorldMatrix = translation * rotation;
    glm::mat4 worldToEntityMatrix = glm::inverse(entityToWorldMatrix);

    glm::vec3 entityFrameOrigin = glm::vec3(worldToEntityMatrix * glm::vec4(origin, 1.0f));
    glm::vec3 entityFrameDirection = glm::vec3(worldToEntityMatrix * glm::vec4(direction, 0.0f));
                         
    float depth = depthOfRayIntersection(entityFrameOrigin, entityFrameDirection);

    return true; // we only got here if we intersected our non-aabox                         
}


/*
void RenderableModelEntityItem::renderEntityAsBillboard() {
    TextureCache* textureCache = Application->getInstance()->getTextureCache();
    textureCache->getPrimaryFramebufferObject()->bind();

    const int BILLBOARD_SIZE = 64;
    renderRearViewMirror(QRect(0, _glWidget->getDeviceHeight() - BILLBOARD_SIZE, BILLBOARD_SIZE, BILLBOARD_SIZE), true);

    //QImage image(BILLBOARD_SIZE, BILLBOARD_SIZE, QImage::Format_ARGB32);
    //glReadPixels(0, 0, BILLBOARD_SIZE, BILLBOARD_SIZE, GL_BGRA, GL_UNSIGNED_BYTE, image.bits());

    textureCache->getPrimaryFramebufferObject()->release();

    return image;
}
*/

float RenderableModelEntityItem::depthOfRayIntersection(const glm::vec3& entityFrameOrigin, const glm::vec3& entityFrameDirection) const {
    qDebug() << "RenderableModelEntityItem::depthOfRayIntersection()....";
    
    Application::getInstance()->getTextureCache()->getPrimaryFramebufferObject()->bind();
    
    glEnable(GL_SCISSOR_TEST);
    glEnable(GL_LIGHTING); // enable?
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND); // we don't need blending
        
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();

    // * we know the direction that the ray is coming into our bounding box.
    // * we know the location on our bounding box that the ray intersected
    // * because this API is theoretically called for things that aren't on the screen, 
    //   or could be off in the distance, but with an original ray pick origin ALSO off
    //   in the distance, we don't really know the "pixel" size of the model at that
    //   place in space. In fact that concept really doesn't make sense at all... so
    //   we need to pick our own "scale" based on whatever level of precision makes
    //   sense... what makes sense? 
    // * we could say that we allow ray intersections down to some N meters (say 1cm 
    //   or 0.01 meters) in real space. The model's bounds in meters are known.
    //   
    //   
    float renderGranularity = 0.01f; // 1cm of render granularity - this could be ridiculous for large models

    qDebug() << "    renderGranularity:" << renderGranularity;

    // note: these are in tree units, not meters
    glm::vec3 dimensions = getDimensions();
    glm::vec3 registrationPoint = getRegistrationPoint();
    glm::vec3 corner = -(dimensions * registrationPoint);

    AABox entityFrameBox(corner, dimensions);
    entityFrameBox.scale((float)TREE_SCALE);

    // rotationBetween(v1, v2) -- Helper function return the rotation from the first vector onto the second
    //glm::quat viewRotation = rotationBetween(entityFrameDirection, IDENTITY_FRONT);
    //glm::quat viewRotation = rotationBetween(IDENTITY_FRONT, entityFrameDirection);
    glm::quat viewRotation = rotationBetween(glm::vec3(0.0f, 1.0f, 0.0f), IDENTITY_FRONT);
    //glm::quat viewRotation = rotationBetween(IDENTITY_FRONT, IDENTITY_FRONT);

    // I'd like to calculate the tightest bounding box around the entity for
    // the direction of the 
    glm::vec3 minima(FLT_MAX, FLT_MAX, FLT_MAX);
    glm::vec3 maxima(-FLT_MAX, -FLT_MAX, -FLT_MAX);
    const int VERTEX_COUNT = 8;
    for (int j = 0; j < VERTEX_COUNT; j++) {
        glm::vec3 vertex = entityFrameBox.getVertex((BoxVertex)j);
        qDebug() << "    vertex[" << j <<"]:" << vertex;

        glm::vec3 rotated = viewRotation * vertex;
        qDebug() << "    rotated[" << j <<"]:" << rotated;
        
        minima = glm::min(minima, rotated);
        maxima = glm::max(maxima, rotated);
    }

    qDebug() << "    minima:" << minima;
    qDebug() << "    maxima:" << maxima;
    
    int width = glm::round((maxima.x - minima.x) / renderGranularity);
    int height = glm::round((maxima.y - minima.y) / renderGranularity);

    qDebug() << "    width:" << width;
    qDebug() << "    height:" << height;
    
    glViewport(0, 0, width, height);
    glScissor(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glLoadIdentity();
    glOrtho(minima.x, maxima.x, minima.y, maxima.y, -maxima.z, -minima.z);
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glm::vec3 axis = glm::axis(viewRotation);
    glRotatef(glm::degrees(glm::angle(viewRotation)), axis.x, axis.y, axis.z);

    glm::vec3 entityFrameOriginInMeters = entityFrameOrigin * (float)TREE_SCALE;
    glm::vec3 entityFrameDirectionInMeters = entityFrameDirection * (float)TREE_SCALE;
    //glTranslatef(entityFrameOriginInMerters.x, entityFrameOriginInMerters.y, entityFrameOriginInMerters.z);
    
    Application::getInstance()->setupWorldLight();
    Application::getInstance()->updateUntranslatedViewMatrix();
    
    
    bool renderAsModel = true;
    
    if (renderAsModel) {
        const float alpha = 1.0f;

        glm::vec3 position = getPositionInMeters();
        glm::vec3 center = getCenterInMeters();
        dimensions = getDimensions() * (float)TREE_SCALE;
        glm::quat rotation = getRotation();

        const float MAX_COLOR = 255.0f;
        glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
                        
        glPushMatrix();
        {
            //glTranslatef(position.x, position.y, position.z);
            //glm::vec3 axis = glm::axis(rotation);
            //glRotatef(glm::degrees(glm::angle(rotation)), axis.x, axis.y, axis.z);
            
            
            glPushMatrix();
                glm::vec3 positionToCenter = center - position;
                //glTranslatef(positionToCenter.x, positionToCenter.y, positionToCenter.z);

                //glScalef(dimensions.x, dimensions.y, dimensions.z);
                //Application::getInstance()->getDeferredLightingEffect()->renderSolidSphere(0.5f, 15, 15);

                //_model->setRotation(rotation);
                //_model->setScaleToFit(true, glm::vec3(1.0f,1.0f,1.0f));

                //glm::vec3(0.0f,2.0f,0.0f)
                _model->setSnapModelToRegistrationPoint(true, glm::vec3(0.5f,0.5f,0.5f));
                _model->setTranslation(glm::vec3(0.0f,0.0f,0.0f));
                _model->simulate(0.0f);
                _model->render(alpha, Model::DEFAULT_RENDER_MODE);
               
                //_model->render(1.0f, Model::DEFAULT_RENDER_MODE);

                //_model->setScaleToFit(true, dimensions);
                _model->setSnapModelToRegistrationPoint(true, getRegistrationPoint());
                _model->setTranslation(position);
                _model->simulate(0.0f);

                glPushMatrix();
                    glScalef(dimensions.x, dimensions.y, dimensions.z);
                    Application::getInstance()->getDeferredLightingEffect()->renderWireSphere(0.5f, 15, 15);
                glPopMatrix();
                
                /*
                glBegin(GL_LINES);

                // low-z side - blue
                glColor4f(0.0f, 0.0f, 1.0f, 1.0f);
                glVertex3f(-0.5f, -0.5f, -0.5f);
                glVertex3f(0.5f, -0.5f, -0.5f);
                glVertex3f(-0.5f, 0.5f, -0.5f);
                glVertex3f(0.5f, 0.5f, -0.5f);
                glVertex3f(0.5f, 0.5f, -0.5f);
                glVertex3f(0.5f, -0.5f, -0.5f);
                glVertex3f(-0.5f, 0.5f, -0.5f);
                glVertex3f(-0.5f, -0.5f, -0.5f);

                // high-z side - cyan
                glColor4f(0.0f, 1.0f, 1.0f, 1.0f);
                glVertex3f(-0.5f, -0.5f, 0.5f);
                glVertex3f( 0.5f, -0.5f, 0.5f);
                glVertex3f(-0.5f,  0.5f, 0.5f);
                glVertex3f( 0.5f,  0.5f, 0.5f);
                glVertex3f( 0.5f,  0.5f, 0.5f);
                glVertex3f( 0.5f, -0.5f, 0.5f);
                glVertex3f(-0.5f,  0.5f, 0.5f);
                glVertex3f(-0.5f, -0.5f, 0.5f);

                // low-x side - yellow
                glColor4f(1.0f, 1.0f, 0.0f, 1.0f);
                glVertex3f(-0.5f, -0.5f, -0.5f);
                glVertex3f(-0.5f, -0.5f,  0.5f);

                glVertex3f(-0.5f, -0.5f,  0.5f);
                glVertex3f(-0.5f,  0.5f,  0.5f);

                glVertex3f(-0.5f,  0.5f,  0.5f);
                glVertex3f(-0.5f,  0.5f, -0.5f);

                glVertex3f(-0.5f,  0.5f, -0.5f);
                glVertex3f(-0.5f, -0.5f, -0.5f);

                // high-x side - red
                glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
                glVertex3f(0.5f, -0.5f, -0.5f);
                glVertex3f(0.5f, -0.5f,  0.5f);
                glVertex3f(0.5f, -0.5f,  0.5f);
                glVertex3f(0.5f,  0.5f,  0.5f);
                glVertex3f(0.5f,  0.5f,  0.5f);
                glVertex3f(0.5f,  0.5f, -0.5f);
                glVertex3f(0.5f,  0.5f, -0.5f);
                glVertex3f(0.5f, -0.5f, -0.5f);


                // origin and direction - green
                float distanceToHit;
                BoxFace ignoreFace;

                entityFrameBox.findRayIntersection(entityFrameOriginInMeters, entityFrameDirectionInMeters, distanceToHit, ignoreFace);
                glm::vec3 pointOfIntersection = entityFrameOriginInMeters + (entityFrameDirectionInMeters * distanceToHit);
                
qDebug() << "distanceToHit: " << distanceToHit;
qDebug() << "pointOfIntersection: " << pointOfIntersection;

                glm::vec3 pointA = pointOfIntersection + (entityFrameDirectionInMeters * -1.0f);
                glm::vec3 pointB = pointOfIntersection + (entityFrameDirectionInMeters * 1.0f);
qDebug() << "pointA: " << pointA;
qDebug() << "pointB: " << pointB;

                glColor4f(0.0f, 1.0f, 0.0f, 1.0f);
                glVertex3f(pointA.x, pointA.y, pointA.z);
                glVertex3f(pointB.x, pointB.y, pointB.z);

                glEnd();
                */
                
                
            glPopMatrix();
        }
        glPopMatrix();


    } else {
        glm::vec3 position = getPositionInMeters();
        glm::vec3 center = getCenterInMeters();
        dimensions = getDimensions() * (float)TREE_SCALE;
        glm::quat rotation = getRotation();

        glColor4f(1.0f, 0.0f, 1.0f, 1.0f);
        glLineWidth(2.0f);
                        
        glPushMatrix();
        {
            //glTranslatef(position.x, position.y, position.z);
            glm::vec3 axis = glm::axis(rotation);
            glRotatef(glm::degrees(glm::angle(rotation)), axis.x, axis.y, axis.z);
            
            
            glPushMatrix();
                glm::vec3 positionToCenter = center - position;
                glTranslatef(positionToCenter.x, positionToCenter.y, positionToCenter.z);

                glScalef(dimensions.x, dimensions.y, dimensions.z);
                //Application::getInstance()->getDeferredLightingEffect()->renderWireCube(1.0f);
                Application::getInstance()->getDeferredLightingEffect()->renderWireSphere(0.5f, 15, 15);
                
                glBegin(GL_LINES);

                // low-z side - blue
                glColor4f(0.0f, 0.0f, 1.0f, 1.0f);
                glVertex3f(-0.5f, -0.5f, -0.5f);
                glVertex3f(0.5f, -0.5f, -0.5f);
                glVertex3f(-0.5f, 0.5f, -0.5f);
                glVertex3f(0.5f, 0.5f, -0.5f);
                glVertex3f(0.5f, 0.5f, -0.5f);
                glVertex3f(0.5f, -0.5f, -0.5f);
                glVertex3f(-0.5f, 0.5f, -0.5f);
                glVertex3f(-0.5f, -0.5f, -0.5f);

                // high-z side - cyan
                glColor4f(0.0f, 1.0f, 1.0f, 1.0f);
                glVertex3f(-0.5f, -0.5f, 0.5f);
                glVertex3f( 0.5f, -0.5f, 0.5f);
                glVertex3f(-0.5f,  0.5f, 0.5f);
                glVertex3f( 0.5f,  0.5f, 0.5f);
                glVertex3f( 0.5f,  0.5f, 0.5f);
                glVertex3f( 0.5f, -0.5f, 0.5f);
                glVertex3f(-0.5f,  0.5f, 0.5f);
                glVertex3f(-0.5f, -0.5f, 0.5f);

                // low-x side - yellow
                glColor4f(1.0f, 1.0f, 0.0f, 1.0f);
                glVertex3f(-0.5f, -0.5f, -0.5f);
                glVertex3f(-0.5f, -0.5f,  0.5f);

                glVertex3f(-0.5f, -0.5f,  0.5f);
                glVertex3f(-0.5f,  0.5f,  0.5f);

                glVertex3f(-0.5f,  0.5f,  0.5f);
                glVertex3f(-0.5f,  0.5f, -0.5f);

                glVertex3f(-0.5f,  0.5f, -0.5f);
                glVertex3f(-0.5f, -0.5f, -0.5f);

                // high-x side - red
                glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
                glVertex3f(0.5f, -0.5f, -0.5f);
                glVertex3f(0.5f, -0.5f,  0.5f);
                glVertex3f(0.5f, -0.5f,  0.5f);
                glVertex3f(0.5f,  0.5f,  0.5f);
                glVertex3f(0.5f,  0.5f,  0.5f);
                glVertex3f(0.5f,  0.5f, -0.5f);
                glVertex3f(0.5f,  0.5f, -0.5f);
                glVertex3f(0.5f, -0.5f, -0.5f);


                // origin and direction - green
                float distanceToHit;
                BoxFace ignoreFace;

                entityFrameBox.findRayIntersection(entityFrameOriginInMeters, entityFrameDirectionInMeters, distanceToHit, ignoreFace);
                glm::vec3 pointOfIntersection = entityFrameOriginInMeters + (entityFrameDirectionInMeters * distanceToHit);
                
qDebug() << "distanceToHit: " << distanceToHit;
qDebug() << "pointOfIntersection: " << pointOfIntersection;

                glm::vec3 pointA = pointOfIntersection + (entityFrameDirectionInMeters * -1.0f);
                glm::vec3 pointB = pointOfIntersection + (entityFrameDirectionInMeters * 1.0f);
qDebug() << "pointA: " << pointA;
qDebug() << "pointB: " << pointB;

                glColor4f(0.0f, 1.0f, 0.0f, 1.0f);
                glVertex3f(pointA.x, pointA.y, pointA.z);
                glVertex3f(pointB.x, pointB.y, pointB.z);

                glEnd();

            glPopMatrix();
        }
        glPopMatrix();
    }
    
    QImage colorData(width, height, QImage::Format_ARGB32);
    QVector<float> depthData(width * height);
    
    glReadPixels(0, 0, width, height, GL_BGRA, GL_UNSIGNED_BYTE, colorData.bits());
    glReadPixels(0, 0, width, height, GL_DEPTH_COMPONENT, GL_FLOAT, depthData.data());

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
 
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    
    glEnable(GL_BLEND);
    glDisable(GL_SCISSOR_TEST);
    
    Application::getInstance()->getTextureCache()->getPrimaryFramebufferObject()->release();

    glViewport(0, 0, Application::getInstance()->getGLWidget()->width(), Application::getInstance()->getGLWidget()->height());
    
    QImage imageData = colorData.mirrored(false,true);

    bool saved = imageData.save("/Users/zappoman/Development/foo.bmp");
    
    qDebug() << "    saved:" << saved;

    
    return 0.0f;
}

