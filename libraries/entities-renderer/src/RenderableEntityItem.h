//
//  RenderableEntityItem.h
//  interface/src
//
//  Created by Brad Hefta-Gaub on 12/6/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_RenderableEntityItem_h
#define hifi_RenderableEntityItem_h

#include <render/Scene.h>
#include <EntityItem.h>
#include <Sound.h>
#include "AbstractViewStateInterface.h"
#include "EntitiesRendererLogging.h"

class EntityTreeRenderer;

namespace render { namespace entities {

// Base class for all renderable entities
class EntityRenderer : public QObject, public std::enable_shared_from_this<EntityRenderer>, public PayloadProxyInterface, protected ReadWriteLockable {
    Q_OBJECT

    using Pointer = std::shared_ptr<EntityRenderer>;

public:
    static void initEntityRenderers();
    static Pointer addToScene(EntityTreeRenderer& renderer, const EntityItemPointer& entity, const ScenePointer& scene);

    // Allow classes to override this to interact with the user
    virtual bool wantsHandControllerPointerEvents() const { return false; }
    virtual bool wantsKeyboardFocus() const { return false; }
    virtual void setProxyWindow(QWindow* proxyWindow) {}
    virtual QObject* getEventHandler() { return nullptr; }
    const EntityItemPointer& getEntity() { return _entity; }
    const ItemID& getRenderItemID() const { return _renderItemID; }

    const SharedSoundPointer& getCollisionSound() { return _collisionSound; }
    void setCollisionSound(const SharedSoundPointer& sound) { _collisionSound = sound; }

    // Handlers for rendering events... executed on the main thread, only called by EntityTreeRenderer, 
    // cannot be overridden or accessed by subclasses
    virtual void updateInScene(const ScenePointer& scene, Transaction& transaction) final;
    virtual bool addToScene(const ScenePointer& scene, Transaction& transaction) final;
    virtual void removeFromScene(const ScenePointer& scene, Transaction& transaction);

protected:
    virtual bool needsRenderUpdateFromEntity() const final { return needsRenderUpdateFromEntity(_entity); }
    virtual void onAddToScene(const EntityItemPointer& entity);
    virtual void onRemoveFromScene(const EntityItemPointer& entity);

protected:
    EntityRenderer(const EntityItemPointer& entity);
    ~EntityRenderer();

    // Implementing the PayloadProxyInterface methods
    virtual ItemKey getKey() override;
    virtual ShapeKey getShapeKey() override { return ShapeKey::Builder::ownPipeline(); }
    virtual Item::Bound getBound() override;
    virtual void render(RenderArgs* args) override final;
    virtual uint32_t metaFetchMetaSubItems(ItemIDs& subItems) override;

    // Returns true if the item in question needs to have updateInScene called because of internal rendering state changes
    virtual bool needsRenderUpdate() const;

    // Returns true if the item in question needs to have updateInScene called because of changes in the entity
    virtual bool needsRenderUpdateFromEntity(const EntityItemPointer& entity) const;

    // Will be called on the main thread from updateInScene.  This can be used to fetch things like 
    // network textures or model geometry from resource caches
    virtual void doRenderUpdateSynchronous(const ScenePointer& scene, Transaction& transaction, const EntityItemPointer& entity) {  }

    // Will be called by the lambda posted to the scene in updateInScene.  
    // This function will execute on the rendering thread, so you cannot use network caches to fetch
    // data in this method if using multi-threaded rendering
    virtual void doRenderUpdateAsynchronous(const EntityItemPointer& entity);

    // Called by the `render` method after `needsRenderUpdate`
    virtual void doRender(RenderArgs* args) = 0;

    bool isFading() const { return _isFading; }
    virtual bool isTransparent() const { return _isFading ? Interpolate::calculateFadeRatio(_fadeStartTime) < 1.0f : false; }
    inline bool isValidRenderItem() const { return _renderItemID != Item::INVALID_ITEM_ID; }
    
    template <typename F, typename T>
    T withReadLockResult(const std::function<T()>& f) {
        T result;
        withReadLock([&] {
            result = f();
        });
        return result;
    }


signals:
    void requestRenderUpdate();

protected:
    template<typename T>
    std::shared_ptr<T> asTypedEntity() { return std::static_pointer_cast<T>(_entity); }
        
    static void makeStatusGetters(const EntityItemPointer& entity, Item::Status::Getters& statusGetters);
    static std::function<bool()> _entitiesShouldFadeFunction;

    SharedSoundPointer _collisionSound;
    QUuid _changeHandlerId;
    ItemID _renderItemID{ Item::INVALID_ITEM_ID };
    quint64 _fadeStartTime{ usecTimestampNow() };
    bool _isFading{ _entitiesShouldFadeFunction() };
    bool _prevIsTransparent { false };
    Transform _modelTransform;
    Item::Bound _bound;
    bool _visible { false };
    bool _moving { false };
    // Only touched on the rendering thread
    bool _renderUpdateQueued{ false };


private:
    // The rendering code only gets access to the entity in very specific circumstances
    // i.e. to see if the rendering code needs to update because of a change in state of the 
    // entity.  This forces all the rendering code itself to be independent of the entity
    const EntityItemPointer _entity;
};

template <typename T>
class TypedEntityRenderer : public EntityRenderer {
    using Parent = EntityRenderer;

public:
    TypedEntityRenderer(const EntityItemPointer& entity) : Parent(entity), _typedEntity(asTypedEntity<T>()) {}

protected:
    using TypedEntityPointer = std::shared_ptr<T>;

    virtual void onAddToScene(const EntityItemPointer& entity) override final {
        Parent::onAddToScene(entity);
        onAddToSceneTyped(_typedEntity);
    }

    virtual void onRemoveFromScene(const EntityItemPointer& entity) override final {
        Parent::onRemoveFromScene(entity);
        onRemoveFromSceneTyped(_typedEntity);
    }

    using Parent::needsRenderUpdateFromEntity;
    // Returns true if the item in question needs to have updateInScene called because of changes in the entity
    virtual bool needsRenderUpdateFromEntity(const EntityItemPointer& entity) const override final {
        if (Parent::needsRenderUpdateFromEntity(entity)) {
            return true;
        }
        return needsRenderUpdateFromTypedEntity(_typedEntity);
    }

    virtual void doRenderUpdateSynchronous(const ScenePointer& scene, Transaction& transaction, const EntityItemPointer& entity) override final {
        Parent::doRenderUpdateSynchronous(scene, transaction, entity);
        doRenderUpdateSynchronousTyped(scene, transaction, _typedEntity);
    }

    virtual void doRenderUpdateAsynchronous(const EntityItemPointer& entity) override final {
        Parent::doRenderUpdateAsynchronous(entity);
        doRenderUpdateAsynchronousTyped(_typedEntity);
    }

    virtual bool needsRenderUpdateFromTypedEntity(const TypedEntityPointer& entity) const { return false; }
    virtual void doRenderUpdateSynchronousTyped(const ScenePointer& scene, Transaction& transaction, const TypedEntityPointer& entity) { }
    virtual void doRenderUpdateAsynchronousTyped(const TypedEntityPointer& entity) { }
    virtual void onAddToSceneTyped(const TypedEntityPointer& entity) { }
    virtual void onRemoveFromSceneTyped(const TypedEntityPointer& entity) { }

private:
    const TypedEntityPointer _typedEntity;
};

} } // namespace render::entities

#endif // hifi_RenderableEntityItem_h
