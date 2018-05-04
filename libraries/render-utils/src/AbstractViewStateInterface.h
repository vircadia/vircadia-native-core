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

/// Interface provided by Application to other objects that need access to the current view state details
class AbstractViewStateInterface {
public:
    /// copies the current view frustum for rendering the view state
    virtual void copyCurrentViewFrustum(ViewFrustum& viewOut) const = 0;

    virtual void copyViewFrustum(ViewFrustum& viewOut) const = 0;
    virtual void copySecondaryViewFrustum(ViewFrustum& viewOut) const = 0;
    virtual bool hasSecondaryViewFrustum() const = 0;

    virtual QThread* getMainThread() = 0;

    virtual PickRay computePickRay(float x, float y) const = 0;

    virtual glm::vec3 getAvatarPosition() const = 0;

    // Unfortunately, having this here is a bad idea.  Lots of objects connect to 
    // the aboutToQuit signal, and it's impossible to know the order in which 
    // the receivers will be called, so this might return false negatives
    //virtual bool isAboutToQuit() const = 0;

    // Queue code to execute on the main thread. 
    // If called from the main thread, the lambda will execute synchronously
    virtual void postLambdaEvent(const std::function<void()>& f) = 0;
    // Synchronously execute code on the main thread.  This function will
    // not return until the code is executed, regardles of which thread it 
    // is called from
    virtual void sendLambdaEvent(const std::function<void()>& f) = 0;

    virtual qreal getDevicePixelRatio() = 0;

    virtual render::ScenePointer getMain3DScene() = 0;
    virtual render::EnginePointer getRenderEngine() = 0;

    virtual void pushPostUpdateLambda(void* key, const std::function<void()>& func) = 0;

    virtual bool isHMDMode() const = 0;

    // FIXME - we shouldn't assume that there's a single instance of an AbstractViewStateInterface
    static AbstractViewStateInterface* instance();
    static void setInstance(AbstractViewStateInterface* instance);
};

#endif  // hifi_AbstractViewStateInterface_h
