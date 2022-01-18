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

#include <ModelEntityItem.h>

#include <QString>
#include <QStringList>

#include <ModelEntityItem.h>
#include <AnimationCache.h>
#include <Model.h>
#include <model-networking/ModelCache.h>
#include <MetaModelPayload.h>

#include "RenderableEntityItem.h"

class Model;
class EntityTreeRenderer;

namespace render { namespace entities {
class ModelEntityRenderer;
} }

// #define MODEL_ENTITY_USE_FADE_EFFECT
class ModelEntityWrapper : public ModelEntityItem {
    using Parent = ModelEntityItem;
    friend class render::entities::ModelEntityRenderer;

protected:
    ModelEntityWrapper(const EntityItemID& entityItemID) : Parent(entityItemID) {}
    void setModel(const ModelPointer& model);
    ModelPointer getModel() const;

    bool _needsInitialSimulation { true };
private:
    ModelPointer _model;
};

class RenderableModelEntityItem : public ModelEntityWrapper {
    Q_OBJECT

    friend class render::entities::ModelEntityRenderer;
    using Parent = ModelEntityWrapper;
public:
    static EntityItemPointer factory(const EntityItemID& entityID, const EntityItemProperties& properties);

    RenderableModelEntityItem(const EntityItemID& entityItemID, bool dimensionsInitialized);
    virtual ~RenderableModelEntityItem();

    virtual void setUnscaledDimensions(const glm::vec3& value) override;

    virtual EntityItemProperties getProperties(const EntityPropertyFlags& desiredProperties, bool allowEmptyDesiredProperties) const override;
    void updateModelBounds();

    glm::vec3 getPivot() const override;
    virtual bool supportsDetailedIntersection() const override;
    virtual bool findDetailedRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
                        const glm::vec3& viewFrustumPos, OctreeElementPointer& element, float& distance,
                        BoxFace& face, glm::vec3& surfaceNormal,
                        QVariantMap& extraInfo, bool precisionPicking) const override;
    virtual bool findDetailedParabolaIntersection(const glm::vec3& origin, const glm::vec3& velocity,
                        const glm::vec3& acceleration, const glm::vec3& viewFrustumPos, OctreeElementPointer& element,
                        float& parabolicDistance, BoxFace& face, glm::vec3& surfaceNormal,
                        QVariantMap& extraInfo, bool precisionPicking) const override;

    virtual void setShapeType(ShapeType type) override;
    virtual void setCompoundShapeURL(const QString& url) override;
    virtual void setModelURL(const QString& url) override;

    virtual bool isReadyToComputeShape() const override;
    virtual void computeShapeInfo(ShapeInfo& shapeInfo) override;
    bool unableToLoadCollisionShape();

    virtual bool contains(const glm::vec3& point) const override;
    void stopModelOverrideIfNoParent();

    virtual bool shouldBePhysical() const override;
    void simulateRelayedJoints();
    bool getJointMapCompleted();
    void setJointMap(std::vector<int> jointMap);
    int avatarJointIndex(int modelJointIndex);
    void setOverrideTransform(const Transform& transform, const glm::vec3& offset);

    // these are in the frame of this object (model space)
    virtual glm::quat getAbsoluteJointRotationInObjectFrame(int index) const override;
    virtual glm::vec3 getAbsoluteJointTranslationInObjectFrame(int index) const override;
    virtual int getJointParent(int index) const override;
    virtual bool setAbsoluteJointRotationInObjectFrame(int index, const glm::quat& rotation) override;
    virtual bool setAbsoluteJointTranslationInObjectFrame(int index, const glm::vec3& translation) override;

    virtual glm::quat getLocalJointRotation(int index) const override;
    virtual glm::vec3 getLocalJointTranslation(int index) const override;
    virtual bool setLocalJointRotation(int index, const glm::quat& rotation) override;
    virtual bool setLocalJointTranslation(int index, const glm::vec3& translation) override;

    virtual void setJointRotations(const QVector<glm::quat>& rotations) override;
    virtual void setJointRotationsSet(const QVector<bool>& rotationsSet) override;
    virtual void setJointTranslations(const QVector<glm::vec3>& translations) override;
    virtual void setJointTranslationsSet(const QVector<bool>& translationsSet) override;

    virtual void locationChanged(bool tellPhysics = true, bool tellChildren = true) override;

    virtual int getJointIndex(const QString& name) const override;
    virtual QStringList getJointNames() const override;

private:
    bool needsUpdateModelBounds() const;
    void autoResizeJointArrays();
    void copyAnimationJointDataToModel();
    bool readyToAnimate() const;
    void fetchCollisionGeometryResource();

    QString getCollisionShapeURL() const;

    GeometryResource::Pointer _collisionGeometryResource;
    std::vector<int> _jointMap;
    QVariantMap _originalTextures;
    bool _jointMapCompleted { false };
    bool _originalTexturesRead { false };
    bool _dimensionsInitialized { true };
    bool _needsJointSimulation { false };
    bool _needsToRescaleModel { false };
};

namespace render { namespace entities { 

class ModelEntityRenderer : public TypedEntityRenderer<RenderableModelEntityItem>, public MetaModelPayload {
    using Parent = TypedEntityRenderer<RenderableModelEntityItem>;
    friend class EntityRenderer;
    Q_OBJECT

public:
    ModelEntityRenderer(const EntityItemPointer& entity);
    virtual scriptable::ScriptableModelBase getScriptableModel() override;
    virtual bool canReplaceModelMeshPart(int meshIndex, int partIndex) override;
    virtual bool replaceScriptableModelMeshPart(scriptable::ScriptableModelBasePointer model, int meshIndex, int partIndex) override;

    void addMaterial(graphics::MaterialLayer material, const std::string& parentMaterialName) override;
    void removeMaterial(graphics::MaterialPointer material, const std::string& parentMaterialName) override;

    // FIXME: model mesh parts should fade individually
    bool isFading() const override { return false; }

protected:
    virtual void removeFromScene(const ScenePointer& scene, Transaction& transaction) override;
    virtual void onRemoveFromSceneTyped(const TypedEntityPointer& entity) override;

    void setKey(bool didVisualGeometryRequestSucceed, const ModelPointer& model);
    virtual ItemKey getKey() override;
    virtual uint32_t metaFetchMetaSubItems(ItemIDs& subItems) const override;
    virtual void handleBlendedVertices(int blendshapeNumber, const QVector<BlendshapeOffset>& blendshapeOffsets,
                                       const QVector<int>& blendedMeshSizes, const render::ItemIDs& subItemIDs) override;

    virtual bool needsRenderUpdateFromTypedEntity(const TypedEntityPointer& entity) const override;
    virtual void doRenderUpdateSynchronousTyped(const ScenePointer& scene, Transaction& transaction, const TypedEntityPointer& entity) override;
    virtual void doRenderUpdateAsynchronousTyped(const TypedEntityPointer& entity) override;
    virtual void doRender(RenderArgs* args) override;

    void setIsVisibleInSecondaryCamera(bool value) override;
    void setRenderLayer(RenderLayer value) override;
    void setCullWithParent(bool value) override;

private:
    void animate(const TypedEntityPointer& entity, const ModelPointer& model);
    void mapJoints(const TypedEntityPointer& entity, const ModelPointer& model);

    // Transparency is handled in ModelMeshPartPayload
    virtual bool isTransparent() const override { return false; }

    bool _hasModel { false };
    ModelPointer _model;
    QString _textures;
    bool _texturesLoaded { false };
    int _lastKnownCurrentFrame { -1 };
#ifdef MODEL_ENTITY_USE_FADE_EFFECT
    bool _hasTransitioned{ false };
#endif

    QUrl _parsedModelURL;
    bool _jointMappingCompleted { false };
    QVector<int> _jointMapping; // domain is index into model-joints, range is index into animation-joints
    AnimationPointer _animation;
    QString _animationURL;
    uint64_t _lastAnimated { 0 };

    render::ItemKey _itemKey { render::ItemKey::Builder().withTypeMeta() };

    bool _didLastVisualGeometryRequestSucceed { true };

    void processMaterials();
    bool _allProceduralMaterialsLoaded { false };

    static void metaBlendshapeOperator(render::ItemID renderItemID, int blendshapeNumber, const QVector<BlendshapeOffset>& blendshapeOffsets,
                                       const QVector<int>& blendedMeshSizes, const render::ItemIDs& subItemIDs);
};

} } // namespace 

#endif // hifi_RenderableModelEntityItem_h
