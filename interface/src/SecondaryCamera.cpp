//
//  SecondaryCamera.cpp
//  interface/src
//
//  Created by Samuel Gateau, Howard Stearns, and Zach Fox on 2017-06-08.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Application.h"
#include "SecondaryCamera.h"
#include <TextureCache.h>
#include <gpu/Context.h>
#include <EntityScriptingInterface.h>
#include <glm/gtx/transform.hpp>

using RenderArgsPointer = std::shared_ptr<RenderArgs>;

void MainRenderTask::build(JobModel& task, const render::Varying& inputs, render::Varying& outputs, render::CullFunctor cullFunctor, bool isDeferred) {
    
    task.addJob<RenderShadowTask>("RenderShadowTask", cullFunctor, render::ItemKey::TAG_BITS_1, render::ItemKey::TAG_BITS_1);
    const auto items = task.addJob<RenderFetchCullSortTask>("FetchCullSort", cullFunctor, render::ItemKey::TAG_BITS_1, render::ItemKey::TAG_BITS_1);
    assert(items.canCast<RenderFetchCullSortTask::Output>());
    if (!isDeferred) {
        task.addJob<RenderForwardTask>("Forward", items);
    } else {
        task.addJob<RenderDeferredTask>("RenderDeferredTask", items);
    }
}

class SecondaryCameraJob {  // Changes renderContext for our framebuffer and view.
public:
    using Config = SecondaryCameraJobConfig;
    using JobModel = render::Job::ModelO<SecondaryCameraJob, RenderArgsPointer, Config>;
    SecondaryCameraJob() {
        _cachedArgsPointer = std::make_shared<RenderArgs>(_cachedArgs);
        _entityScriptingInterface = DependencyManager::get<EntityScriptingInterface>();
        _attachedEntityPropertyFlags += PROP_POSITION;
        _attachedEntityPropertyFlags += PROP_ROTATION;
    }

    void configure(const Config& config) {
        _attachedEntityId = config.attachedEntityId;
        _position = config.position;
        _orientation = config.orientation;
        _vFoV = config.vFoV;
        _nearClipPlaneDistance = config.nearClipPlaneDistance;
        _farClipPlaneDistance = config.farClipPlaneDistance;
        _textureWidth = config.textureWidth;
        _textureHeight = config.textureHeight;
        _mirrorProjection = config.mirrorProjection;  
    }

    void setMirrorProjection(ViewFrustum& srcViewFrustum) {
        if (_attachedEntityId.isNull()) {
            qWarning() << "ERROR: Cannot set mirror projection for SecondaryCamera without an attachedEntityId set.";
            return;
        }
       
        EntityItemProperties entityProperties = _entityScriptingInterface->getEntityProperties(_attachedEntityId, 
                                                                                               _attachedEntityPropertyFlags);
        glm::vec3 mirrorPropertiesPosition = entityProperties.getPosition();
        glm::quat mirrorPropertiesRotation = entityProperties.getRotation();
        glm::vec3 mirrorPropertiesDimensions = entityProperties.getDimensions();
        glm::vec3 halfMirrorPropertiesDimensions = 0.5f * mirrorPropertiesDimensions;

        // setup mirror from world as inverse of world from mirror transformation using inverted x and z for mirrored image
        // TODO: we are assuming here that UP is world y-axis
        glm::mat4 worldFromMirrorRotation = glm::mat4_cast(mirrorPropertiesRotation) * glm::scale(vec3(-1.0f, 1.0f, -1.0f));
        glm::mat4 worldFromMirrorTranslation = glm::translate(mirrorPropertiesPosition);
        glm::mat4 worldFromMirror = worldFromMirrorTranslation * worldFromMirrorRotation;
        glm::mat4 mirrorFromWorld = glm::inverse(worldFromMirror);

        // get mirror camera position by reflecting main camera position's z coordinate in mirror space
        glm::vec3 mainCameraPositionWorld = qApp->getCamera().getPosition();
        glm::vec3 mainCameraPositionMirror = vec3(mirrorFromWorld * vec4(mainCameraPositionWorld, 1.0f));
        glm::vec3 mirrorCameraPositionMirror = vec3(mainCameraPositionMirror.x, mainCameraPositionMirror.y, 
                                                    -mainCameraPositionMirror.z);
        glm::vec3 mirrorCameraPositionWorld = vec3(worldFromMirror * vec4(mirrorCameraPositionMirror, 1.0f));

        // set frustum position to be mirrored camera and set orientation to mirror's adjusted rotation
        glm::quat mirrorCameraOrientation = glm::quat_cast(worldFromMirrorRotation);
        srcViewFrustum.setPosition(mirrorCameraPositionWorld);
        srcViewFrustum.setOrientation(mirrorCameraOrientation);

        // build frustum using mirror space translation of mirrored camera
        float nearClip = mirrorCameraPositionMirror.z + mirrorPropertiesDimensions.z * 2.0f;
        glm::vec3 upperRight = halfMirrorPropertiesDimensions - mirrorCameraPositionMirror;
        glm::vec3 bottomLeft = -halfMirrorPropertiesDimensions - mirrorCameraPositionMirror;
        glm::mat4 frustum = glm::frustum(bottomLeft.x, upperRight.x, bottomLeft.y, upperRight.y, nearClip, _farClipPlaneDistance);
        srcViewFrustum.setProjection(frustum);
    }

    void run(const render::RenderContextPointer& renderContext, RenderArgsPointer& cachedArgs) {
        auto args = renderContext->args;
        auto textureCache = DependencyManager::get<TextureCache>();
        gpu::FramebufferPointer destFramebuffer;
        destFramebuffer = textureCache->getSpectatorCameraFramebuffer(_textureWidth, _textureHeight); // FIXME: Change the destination based on some unimplemented config var
        if (destFramebuffer) {
            _cachedArgsPointer->_blitFramebuffer = args->_blitFramebuffer;
            _cachedArgsPointer->_viewport = args->_viewport;
            _cachedArgsPointer->_displayMode = args->_displayMode;
            _cachedArgsPointer->_renderMode = args->_renderMode;
            args->_blitFramebuffer = destFramebuffer;
            args->_viewport = glm::ivec4(0, 0, destFramebuffer->getWidth(), destFramebuffer->getHeight());
            args->_displayMode = RenderArgs::MONO;
            args->_renderMode = RenderArgs::RenderMode::SECONDARY_CAMERA_RENDER_MODE;

            gpu::doInBatch(args->_context, [&](gpu::Batch& batch) {
                batch.disableContextStereo();
                batch.disableContextViewCorrection();
            });

            auto srcViewFrustum = args->getViewFrustum();
            if (_mirrorProjection) {
                setMirrorProjection(srcViewFrustum);
            } else {
                if (!_attachedEntityId.isNull()) {
                    EntityItemProperties entityProperties = _entityScriptingInterface->getEntityProperties(_attachedEntityId, 
                                                            _attachedEntityPropertyFlags);
                    srcViewFrustum.setPosition(entityProperties.getPosition());
                    srcViewFrustum.setOrientation(entityProperties.getRotation());
                } else {
                    srcViewFrustum.setPosition(_position);
                    srcViewFrustum.setOrientation(_orientation);
                }
                srcViewFrustum.setProjection(glm::perspective(glm::radians(_vFoV), 
                                            ((float)args->_viewport.z / (float)args->_viewport.w), 
                                            _nearClipPlaneDistance, _farClipPlaneDistance));
            }
            // Without calculating the bound planes, the secondary camera will use the same culling frustum as the main camera,
            // which is not what we want here.
            srcViewFrustum.calculate();
            args->pushViewFrustum(srcViewFrustum);
            cachedArgs = _cachedArgsPointer;
        }
    }

protected:
    RenderArgs _cachedArgs;
    RenderArgsPointer _cachedArgsPointer;

private:
    QUuid _attachedEntityId;
    glm::vec3 _position;
    glm::quat _orientation;
    float _vFoV;
    float _nearClipPlaneDistance;
    float _farClipPlaneDistance;
    int _textureWidth;
    int _textureHeight;
    bool _mirrorProjection;
    EntityPropertyFlags _attachedEntityPropertyFlags;
    QSharedPointer<EntityScriptingInterface> _entityScriptingInterface;
};

void SecondaryCameraJobConfig::setPosition(glm::vec3 pos) {
    if (attachedEntityId.isNull()) {
        position = pos;
        emit dirty();
    } else {
        qDebug() << "ERROR: Cannot set position of SecondaryCamera while attachedEntityId is set.";
    }
}

void SecondaryCameraJobConfig::setOrientation(glm::quat orient) {
    if (attachedEntityId.isNull()) {
        orientation = orient;
        emit dirty();
    } else {
        qDebug() << "ERROR: Cannot set orientation of SecondaryCamera while attachedEntityId is set.";
    }
}

void SecondaryCameraJobConfig::enableSecondaryCameraRenderConfigs(bool enabled) {
    qApp->getRenderEngine()->getConfiguration()->getConfig<SecondaryCameraRenderTask>()->setEnabled(enabled);
    setEnabled(enabled);
}

void SecondaryCameraJobConfig::resetSizeSpectatorCamera(int width, int height) {
    textureWidth = width;
    textureHeight = height;
    emit dirty();
}

class EndSecondaryCameraFrame {  // Restores renderContext.
public:
    using JobModel = render::Job::ModelI<EndSecondaryCameraFrame, RenderArgsPointer>;

    void run(const render::RenderContextPointer& renderContext, const RenderArgsPointer& cachedArgs) {
        auto args = renderContext->args;
        args->_blitFramebuffer = cachedArgs->_blitFramebuffer;
        args->_viewport = cachedArgs->_viewport;
        args->popViewFrustum();
        args->_displayMode = cachedArgs->_displayMode;
        args->_renderMode = cachedArgs->_renderMode;

        gpu::doInBatch(args->_context, [&](gpu::Batch& batch) {
            batch.restoreContextStereo();
            batch.restoreContextViewCorrection();
        });
    }
};

void SecondaryCameraRenderTask::build(JobModel& task, const render::Varying& inputs, render::Varying& outputs, render::CullFunctor cullFunctor, bool isDeferred) {
    const auto cachedArg = task.addJob<SecondaryCameraJob>("SecondaryCamera");
    const auto items = task.addJob<RenderFetchCullSortTask>("FetchCullSort", cullFunctor, render::ItemKey::TAG_BITS_1, render::ItemKey::TAG_BITS_1);
    assert(items.canCast<RenderFetchCullSortTask::Output>());
    if (!isDeferred) {
        task.addJob<RenderForwardTask>("Forward", items);
    } else {
        task.addJob<RenderDeferredTask>("RenderDeferredTask", items);
    }
    task.addJob<EndSecondaryCameraFrame>("EndSecondaryCamera", cachedArg);
}