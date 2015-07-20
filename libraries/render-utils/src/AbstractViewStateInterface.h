//
//  AbstractViewStateInterface.h
//  interface/src/renderer
//
//  Created by Brad Hefta-Gaub on 12/16/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AbstractViewStateInterface_h
#define hifi_AbstractViewStateInterface_h

#include <glm/glm.hpp>
#include <functional>

#include <render/Scene.h>
#include <render/Engine.h>

#include <QtGlobal>

class Transform;
class QThread;
class ViewFrustum;
class PickRay;
class EnvironmentData;

/// Interface provided by Application to other objects that need access to the current view state details
class AbstractViewStateInterface {
public:
    
    /// Returns the shadow distances for the current view state
    virtual const glm::vec3& getShadowDistances() const = 0;

    /// Computes the off-axis frustum parameters for the view frustum, taking mirroring into account.
    virtual void computeOffAxisFrustum(float& left, float& right, float& bottom, float& top, float& nearVal,
        float& farVal, glm::vec4& nearClipPlane, glm::vec4& farClipPlane) const = 0;

    /// gets the current view frustum for rendering the view state
    virtual ViewFrustum* getCurrentViewFrustum() = 0;

    /// overrides environment data
    virtual void overrideEnvironmentData(const EnvironmentData& newData) = 0;
    virtual void endOverrideEnvironmentData() = 0;

    /// gets the shadow view frustum for rendering the view state
    virtual ViewFrustum* getShadowViewFrustum() = 0;

    virtual bool getShadowsEnabled() = 0;
    virtual bool getCascadeShadowsEnabled() = 0;

    virtual QThread* getMainThread() = 0;

    virtual bool shouldRenderMesh(float largestDimension, float distanceToCamera) = 0;
    virtual float getSizeScale() const = 0;
    virtual int getBoundaryLevelAdjust() const = 0;
    virtual PickRay computePickRay(float x, float y) const = 0;

    virtual const glm::vec3& getAvatarPosition() const = 0;

    virtual void postLambdaEvent(std::function<void()> f) = 0;
    virtual qreal getDevicePixelRatio() = 0;

    virtual render::ScenePointer getMain3DScene() = 0;
    virtual render::EnginePointer getRenderEngine() = 0;

    // FIXME - we shouldn't assume that there's a single instance of an AbstractViewStateInterface
    static AbstractViewStateInterface* instance();
    static void setInstance(AbstractViewStateInterface* instance);
};


#endif // hifi_AbstractViewStateInterface_h
