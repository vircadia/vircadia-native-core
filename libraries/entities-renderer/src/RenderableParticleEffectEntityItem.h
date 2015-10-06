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
friend class ParticlePayload;
public:
    static EntityItemPointer factory(const EntityItemID& entityID, const EntityItemProperties& properties);
    RenderableParticleEffectEntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties);

    virtual void update(const quint64& now) override;

    void updateRenderItem();

    virtual bool addToScene(EntityItemPointer self, render::ScenePointer scene, render::PendingChanges& pendingChanges) override;
    virtual void removeFromScene(EntityItemPointer self, render::ScenePointer scene, render::PendingChanges& pendingChanges) override;

protected:
    render::ItemID _renderItemId;

    struct Vertex {
        Vertex(glm::vec3 xyzIn, glm::vec2 uvIn, uint32_t rgbaIn) : xyz(xyzIn), uv(uvIn), rgba(rgbaIn) {}
        glm::vec3 xyz;
        glm::vec2 uv;
        uint32_t rgba;
    };

    static void createPipelines();

    std::vector<Vertex> _vertices;
    static gpu::PipelinePointer _untexturedPipeline;
    static gpu::PipelinePointer _texturedPipeline;

    render::ScenePointer _scene;
    NetworkTexturePointer _texture;
};


#endif // hifi_RenderableParticleEffectEntityItem_h
