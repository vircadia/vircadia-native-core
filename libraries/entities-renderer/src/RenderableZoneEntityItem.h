//
//  RenderableZoneEntityItem.h
//
//
//  Created by Clement on 4/22/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_RenderableZoneEntityItem_h
#define hifi_RenderableZoneEntityItem_h

#include <ZoneEntityItem.h>
#include <graphics/Skybox.h>
#include <graphics/Haze.h>
#include <graphics/Stage.h>
#include <LightStage.h>
#include <BackgroundStage.h>
#include <HazeStage.h>
#include <TextureCache.h>
#include "RenderableEntityItem.h"
#include <ComponentMode.h>

#if 0
#include <Model.h>
#endif
namespace render { namespace entities { 

class ZoneEntityRenderer : public TypedEntityRenderer<ZoneEntityItem> {
    using Parent = TypedEntityRenderer<ZoneEntityItem>;
    friend class EntityRenderer;

public:
    ZoneEntityRenderer(const EntityItemPointer& entity);

protected:
    virtual void onRemoveFromSceneTyped(const TypedEntityPointer& entity) override;
    virtual ItemKey getKey() override;
    virtual void doRender(RenderArgs* args) override;
    virtual void removeFromScene(const ScenePointer& scene, Transaction& transaction) override;
    virtual bool needsRenderUpdateFromTypedEntity(const TypedEntityPointer& entity) const override;
    virtual void doRenderUpdateSynchronousTyped(const ScenePointer& scene, Transaction& transaction, const TypedEntityPointer& entity) override;
    virtual void doRenderUpdateAsynchronousTyped(const TypedEntityPointer& entity) override;

private:
    void updateKeyZoneItemFromEntity(const TypedEntityPointer& entity);
    void updateKeySunFromEntity(const TypedEntityPointer& entity);
    void updateAmbientLightFromEntity(const TypedEntityPointer& entity);
    void updateHazeFromEntity(const TypedEntityPointer& entity);
    void updateKeyBackgroundFromEntity(const TypedEntityPointer& entity);
    void updateAmbientMap();
    void updateSkyboxMap();
    void setAmbientURL(const QString& ambientUrl);
    void setSkyboxURL(const QString& skyboxUrl);

    void setHazeMode(ComponentMode mode);
    void setKeyLightMode(ComponentMode mode);
    void setAmbientLightMode(ComponentMode mode);
    void setSkyboxMode(ComponentMode mode);

    void setSkyboxColor(const glm::vec3& color);
    void setProceduralUserData(const QString& userData);

    graphics::LightPointer editSunLight() { _needSunUpdate = true; return _sunLight; }
    graphics::LightPointer editAmbientLight() { _needAmbientUpdate = true; return _ambientLight; }
    graphics::SunSkyStagePointer editBackground() { _needBackgroundUpdate = true; return _background; }
    graphics::SkyboxPointer editSkybox() { return editBackground()->getSkybox(); }
    graphics::HazePointer editHaze() { _needHazeUpdate = true; return _haze; }

    bool _needsInitialSimulation{ true };
    glm::vec3 _lastPosition;
    glm::vec3 _lastDimensions;
    glm::quat _lastRotation;

    // FIXME compount shapes are currently broken
    // FIXME draw zone boundaries are currently broken (also broken in master)
#if 0
    ModelPointer _model;
    bool _lastModelActive { false };
    QString _lastShapeURL;
#endif

    LightStagePointer _stage;
    const graphics::LightPointer _sunLight{ std::make_shared<graphics::Light>() };
    const graphics::LightPointer _ambientLight{ std::make_shared<graphics::Light>() };
    const graphics::SunSkyStagePointer _background{ std::make_shared<graphics::SunSkyStage>() };
    const graphics::HazePointer _haze{ std::make_shared<graphics::Haze>() };

    ComponentMode _keyLightMode { COMPONENT_MODE_INHERIT };
    ComponentMode _ambientLightMode { COMPONENT_MODE_INHERIT };
    ComponentMode _skyboxMode { COMPONENT_MODE_INHERIT };
    ComponentMode _hazeMode { COMPONENT_MODE_INHERIT };

    indexed_container::Index _sunIndex{ LightStage::INVALID_INDEX };
    indexed_container::Index _shadowIndex{ LightStage::INVALID_INDEX };
    indexed_container::Index _ambientIndex{ LightStage::INVALID_INDEX };

    BackgroundStagePointer _backgroundStage;
    BackgroundStage::Index _backgroundIndex{ BackgroundStage::INVALID_INDEX };

    HazeStagePointer _hazeStage;
    HazeStage::Index _hazeIndex{ HazeStage::INVALID_INDEX };

    bool _needUpdate{ true };
    bool _needSunUpdate{ true };
    bool _needAmbientUpdate{ true };
    bool _needBackgroundUpdate{ true };
    bool _needHazeUpdate{ true };

    KeyLightPropertyGroup _keyLightProperties;
    AmbientLightPropertyGroup _ambientLightProperties;
    HazePropertyGroup _hazeProperties;
    SkyboxPropertyGroup _skyboxProperties;

    // More attributes used for rendering:
    QString _ambientTextureURL;
    NetworkTexturePointer _ambientTexture;
    bool _pendingAmbientTexture{ false };

    QString _skyboxTextureURL;
    NetworkTexturePointer _skyboxTexture;
    bool _pendingSkyboxTexture{ false };

    QString _proceduralUserData;
    Transform _renderTransform;
};

} } // namespace 

#if 0

class NetworkGeometry;
class KeyLightPayload;

class RenderableZoneEntityItemMeta;

class RenderableZoneEntityItem : public ZoneEntityItem, public RenderableEntityInterface  {
public:
    virtual bool contains(const glm::vec3& point) const override;
    virtual bool addToScene(const EntityItemPointer& self, const render::ScenePointer& scene, render::Transaction& transaction) override;
    virtual void removeFromScene(const EntityItemPointer& self, const render::ScenePointer& scene, render::Transaction& transaction) override;
private:
    virtual void locationChanged(bool tellPhysics = true) override { EntityItem::locationChanged(tellPhysics); notifyBoundChanged(); }
    virtual void dimensionsChanged() override { EntityItem::dimensionsChanged(); notifyBoundChanged(); }
    void notifyBoundChanged();
    void notifyChangedRenderItem();
    void sceneUpdateRenderItemFromEntity(render::Transaction& transaction);
};
#endif

#endif // hifi_RenderableZoneEntityItem_h
