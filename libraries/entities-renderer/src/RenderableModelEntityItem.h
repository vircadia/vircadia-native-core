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

class Model;
class EntityTreeRenderer;

class RenderableModelEntityItem : public ModelEntityItem {
public:
    static EntityItemPointer factory(const EntityItemID& entityID, const EntityItemProperties& properties);

    RenderableModelEntityItem(const EntityItemID& entityItemID, bool dimensionsInitialized);

    virtual ~RenderableModelEntityItem();

    virtual void setDimensions(const glm::vec3& value) override;
    virtual void setModelURL(const QString& url) override;

    virtual EntityItemProperties getProperties(EntityPropertyFlags desiredProperties = EntityPropertyFlags()) const override;
    virtual bool setProperties(const EntityItemProperties& properties) override;
    virtual int readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead,
                                                ReadBitstreamToTreeParams& args,
                                                EntityPropertyFlags& propertyFlags, bool overwriteLocalData,
                                                bool& somethingChanged) override;

    virtual void somethingChangedNotification() override {
        // FIX ME: this is overly aggressive. We only really need to simulate() if something about
        // the world space transform has changed and/or if some animation is occurring.
        _needsInitialSimulation = true;
    }

    virtual bool readyToAddToScene(RenderArgs* renderArgs = nullptr);
    virtual bool addToScene(EntityItemPointer self, std::shared_ptr<render::Scene> scene, render::PendingChanges& pendingChanges) override;
    virtual void removeFromScene(EntityItemPointer self, std::shared_ptr<render::Scene> scene, render::PendingChanges& pendingChanges) override;


    void updateModelBounds();
    virtual void render(RenderArgs* args) override;
    virtual bool supportsDetailedRayIntersection() const override { return true; }
    virtual bool findDetailedRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
                        bool& keepSearching, OctreeElementPointer& element, float& distance,
                        BoxFace& face, glm::vec3& surfaceNormal,
                        void** intersectedObject, bool precisionPicking) const override;

    Model* getModel(EntityTreeRenderer* renderer);

    virtual bool needsToCallUpdate() const override;
    virtual void update(const quint64& now) override;

    virtual void setCompoundShapeURL(const QString& url) override;

    virtual bool isReadyToComputeShape() override;
    virtual void computeShapeInfo(ShapeInfo& info) override;

    virtual bool contains(const glm::vec3& point) const override;

    // these are in the frame of this object (model space)
    virtual glm::quat getAbsoluteJointRotationInObjectFrame(int index) const override;
    virtual glm::vec3 getAbsoluteJointTranslationInObjectFrame(int index) const override;
    virtual bool setAbsoluteJointRotationInObjectFrame(int index, const glm::quat& rotation) override;
    virtual bool setAbsoluteJointTranslationInObjectFrame(int index, const glm::vec3& translation) override;

    virtual void loader() override;
    virtual void locationChanged() override;

    virtual void resizeJointArrays(int newSize = -1) override;

    virtual int getJointIndex(const QString& name) const override;
    virtual QStringList getJointNames() const override;

private:
    QVariantMap parseTexturesToMap(QString textures);
    void remapTextures();

    Model* _model = nullptr;
    bool _needsInitialSimulation = true;
    bool _needsModelReload = true;
    EntityTreeRenderer* _myRenderer = nullptr;
    QString _currentTextures;
    QStringList _originalTextures;
    QVariantMap _originalTexturesMap;
    bool _originalTexturesRead = false;
    QVector<QVector<glm::vec3>> _points;
    bool _dimensionsInitialized = true;

    render::ItemID _myMetaItem{ render::Item::INVALID_ITEM_ID };

    bool _showCollisionHull = false;

    bool getAnimationFrame();
};

#endif // hifi_RenderableModelEntityItem_h
