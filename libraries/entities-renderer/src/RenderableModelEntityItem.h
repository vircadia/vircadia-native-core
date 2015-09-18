//
//  RenderableModelEntityItem.h
//  interface/src/entities
//
//  Created by Brad Hefta-Gaub on 8/6/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_RenderableModelEntityItem_h
#define hifi_RenderableModelEntityItem_h

#include <QString>
#include <QStringList>

#include <ModelEntityItem.h>
#include "RenderableDebugableEntityItem.h"

class Model;
class EntityTreeRenderer;

class RenderableModelEntityItem : public ModelEntityItem {
public:
    static EntityItemPointer factory(const EntityItemID& entityID, const EntityItemProperties& properties);

    RenderableModelEntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties) :
        ModelEntityItem(entityItemID, properties),
        _model(NULL),
        _needsInitialSimulation(true),
        _needsModelReload(true),
        _myRenderer(NULL),
        _originalTexturesRead(false) { }

    virtual ~RenderableModelEntityItem();

    virtual EntityItemProperties getProperties(EntityPropertyFlags desiredProperties = EntityPropertyFlags()) const;
    virtual bool setProperties(const EntityItemProperties& properties);
    virtual int readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead, 
                                                ReadBitstreamToTreeParams& args,
                                                EntityPropertyFlags& propertyFlags, bool overwriteLocalData);
                                                
    virtual void somethingChangedNotification() { 
        // FIX ME: this is overly aggressive. We only really need to simulate() if something about
        // the world space transform has changed and/or if some animation is occurring.
        _needsInitialSimulation = true;  
    }

    virtual bool readyToAddToScene(RenderArgs* renderArgs = nullptr);
    virtual bool addToScene(EntityItemPointer self, std::shared_ptr<render::Scene> scene, render::PendingChanges& pendingChanges);
    virtual void removeFromScene(EntityItemPointer self, std::shared_ptr<render::Scene> scene, render::PendingChanges& pendingChanges);


    virtual void render(RenderArgs* args);
    virtual bool supportsDetailedRayIntersection() const { return true; }
    virtual bool findDetailedRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
                         bool& keepSearching, OctreeElementPointer& element, float& distance, BoxFace& face, 
                         void** intersectedObject, bool precisionPicking) const;

    Model* getModel(EntityTreeRenderer* renderer);

    bool needsToCallUpdate() const;

    virtual void setCompoundShapeURL(const QString& url);

    bool isReadyToComputeShape();
    void computeShapeInfo(ShapeInfo& info);
    
    virtual bool contains(const glm::vec3& point) const;

private:
    void remapTextures();
    
    Model* _model;
    bool _needsInitialSimulation;
    bool _needsModelReload;
    EntityTreeRenderer* _myRenderer;
    QString _currentTextures;
    QStringList _originalTextures;
    bool _originalTexturesRead;
    QVector<QVector<glm::vec3>> _points;
    
    render::ItemID _myMetaItem;
};

#endif // hifi_RenderableModelEntityItem_h
