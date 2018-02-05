//
//  ModelOverlay.h
//
//
//  Created by Clement on 6/30/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ModelOverlay_h
#define hifi_ModelOverlay_h

#include <Model.h>
#include <AnimationCache.h>

#include "Volume3DOverlay.h"

class ModelOverlay : public Volume3DOverlay {
    Q_OBJECT
public:
    static QString const TYPE;
    virtual QString getType() const override { return TYPE; }

    virtual QString getName() const override;

    ModelOverlay();
    ModelOverlay(const ModelOverlay* modelOverlay);

    virtual void update(float deltatime) override;
    virtual void render(RenderArgs* args) override {};

    virtual uint32_t fetchMetaSubItems(render::ItemIDs& subItems) const override;

    void clearSubRenderItemIDs();
    void setSubRenderItemIDs(const render::ItemIDs& ids);

    void setProperties(const QVariantMap& properties) override;
    QVariant getProperty(const QString& property) override;
    virtual bool findRayIntersection(const glm::vec3& origin, const glm::vec3& direction, float& distance,
                                        BoxFace& face, glm::vec3& surfaceNormal) override;
    virtual bool findRayIntersectionExtraInfo(const glm::vec3& origin, const glm::vec3& direction,
        float& distance, BoxFace& face, glm::vec3& surfaceNormal, QVariantMap& extraInfo) override;

    virtual ModelOverlay* createClone() const override;

    virtual bool addToScene(Overlay::Pointer overlay, const render::ScenePointer& scene, render::Transaction& transaction) override;
    virtual void removeFromScene(Overlay::Pointer overlay, const render::ScenePointer& scene, render::Transaction& transaction) override;

    void locationChanged(bool tellPhysics) override;

    float getLoadPriority() const { return _loadPriority; }

    bool hasAnimation() const { return !_animationURL.isEmpty(); }
    bool jointsMapped() const { return _jointMappingURL == _animationURL && _jointMappingCompleted; }

    void setVisible(bool visible) override;
    void setDrawInFront(bool drawInFront) override;
    void setDrawHUDLayer(bool drawHUDLayer) override;

protected:
    Transform evalRenderTransform() override;

    // helper to extract metadata from our Model's rigged joints
    template <typename itemType> using mapFunction = std::function<itemType(int jointIndex)>;
    template <typename vectorType, typename itemType>
        vectorType mapJoints(mapFunction<itemType> function) const;

    void animate();
    void mapAnimationJoints(const QStringList& modelJointNames);
    bool isAnimatingSomething() const {
        return !_animationURL.isEmpty() && _animationRunning && _animationFPS != 0.0f;
    }
    void copyAnimationJointDataToModel(QVector<JointData> jointsData);


private:

    ModelPointer _model;
    QVariantMap _modelTextures;
    bool _texturesLoaded { false };

    render::ItemIDs _subRenderItemIDs;

    QUrl _url;
    bool _updateModel { false };
    bool _scaleToFit { false };
    float _loadPriority { 0.0f };

    AnimationPointer _animation;

    QUrl _animationURL;
    float _animationFPS { 0.0f };
    float _animationCurrentFrame { 0.0f };
    bool _animationRunning { false };
    bool _animationLoop { false };
    float _animationFirstFrame { 0.0f };
    float _animationLastFrame { 0.0f };
    bool _animationHold { false };
    bool _animationAllowTranslation { false };
    uint64_t _lastAnimated { 0 };
    int _lastKnownCurrentFrame { -1 };

    QUrl _jointMappingURL;
    bool _jointMappingCompleted { false };
    QVector<int> _jointMapping; // domain is index into model-joints, range is index into animation-joints

    bool _visibleDirty { true };
    bool _drawInFrontDirty { false };
    bool _drawInHUDDirty { false };

};

#endif // hifi_ModelOverlay_h
