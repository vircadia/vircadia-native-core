//
//  RenderableParticleEffectEntityItem.h
//  interface/src/entities
//
//  Created by Jason Rickwald on 3/2/15.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_RenderableParticleEffectEntityItem_h
#define hifi_RenderableParticleEffectEntityItem_h

#include <ParticleEffectEntityItem.h>
#include <TextureCache.h>
#include "RenderableEntityItem.h"

class RenderableParticleEffectEntityItem : public ParticleEffectEntityItem  {
    friend class ParticlePayloadData;
public:
    static EntityItemPointer factory(const EntityItemID& entityID, const EntityItemProperties& properties);
    RenderableParticleEffectEntityItem(const EntityItemID& entityItemID);

    virtual void update(const quint64& now) override;

    void updateRenderItem();

    virtual bool addToScene(EntityItemPointer self, render::ScenePointer scene, render::PendingChanges& pendingChanges) override;
    virtual void removeFromScene(EntityItemPointer self, render::ScenePointer scene, render::PendingChanges& pendingChanges) override;

protected:
    virtual void locationChanged(bool tellPhysics = true) override { EntityItem::locationChanged(tellPhysics); notifyBoundChanged(); }
    virtual void dimensionsChanged() override { EntityItem::dimensionsChanged(); notifyBoundChanged(); }

    void notifyBoundChanged();

    void createPipelines();
    
    render::ScenePointer _scene;
    render::ItemID _renderItemId{ render::Item::INVALID_ITEM_ID };
    
    NetworkTexturePointer _texture;
    gpu::PipelinePointer _untexturedPipeline;
    gpu::PipelinePointer _texturedPipeline;
};


#endif // hifi_RenderableParticleEffectEntityItem_h
