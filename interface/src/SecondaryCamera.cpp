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
#include <EntityTree.h>
#include <glm/gtx/transform.hpp>
#include <DebugDraw.h>

using RenderArgsPointer = std::shared_ptr<RenderArgs>;

void MainRenderTask::build(JobModel& task, const render::Varying& inputs, render::Varying& outputs, render::CullFunctor cullFunctor, bool isDeferred) {
    task.addJob<RenderShadowTask>("RenderShadowTask", cullFunctor);
    const auto items = task.addJob<RenderFetchCullSortTask>("FetchCullSort", cullFunctor);
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
            return;
        }

        glm::vec3 mainCamPos = qApp->getCamera().getPosition();

        EntityItemProperties entityProperties = _entityScriptingInterface->getEntityProperties(_attachedEntityId, _attachedEntityPropertyFlags);
        glm::vec3 mirrorPropsPos = entityProperties.getPosition();
        glm::quat mirrorPropsRot = entityProperties.getRotation();
        glm::vec3 mirrorPropsDim = entityProperties.getDimensions();

        // get mirrored camera's position and rotation reflected about the mirror plane
        glm::vec3 mirrorLocalPos = mirrorPropsRot * glm::vec3(0.f, 0.f, 0.f);
        glm::vec3 mirrorWorldPos = mirrorPropsPos + mirrorLocalPos;
        glm::vec3 mirrorToHeadVec = mainCamPos - mirrorWorldPos;
        glm::vec3 zLocalVecNormalized = mirrorPropsRot * Vectors::UNIT_Z;
        float distanceFromMirror = glm::dot(zLocalVecNormalized, mirrorToHeadVec);
        glm::vec3 eyePos = mainCamPos - (2.f * distanceFromMirror * zLocalVecNormalized);
        glm::quat eyeOrientation = glm::inverse(glm::lookAt(eyePos, mirrorWorldPos, mirrorPropsRot * Vectors::UP));
        srcViewFrustum.setPosition(eyePos);
        srcViewFrustum.setOrientation(eyeOrientation);

        // setup world from mirrored camera transformation matrix
        glm::mat4 worldFromEyeRot = glm::mat4_cast(eyeOrientation);
        glm::mat4 worldFromEyeTrans = glm::translate(eyePos);
        glm::mat4 worldFromEye = worldFromEyeTrans * worldFromEyeRot;

        // setup mirror from world inverse of world from mirror transformation matrices
        glm::mat4 worldFromMirrorRot = glm::mat4_cast(mirrorPropsRot) * glm::scale(vec3(-1.f, 1.f, -1.f));
        glm::mat4 worldFromMirrorTrans = glm::translate(mirrorWorldPos);
        glm::mat4 worldFromMirror = worldFromMirrorTrans * worldFromMirrorRot;
        glm::mat4 mirrorFromWorld = glm::inverse(worldFromMirror);

        glm::mat4 mirrorFromEye = mirrorFromWorld * worldFromEye;
        glm::vec4 mirrorCenterPos = vec4(mirrorFromEye[3]);
        mirrorFromEye[3] = vec4(0.f, 0.f, 0.f, 1.f);

        float n = mirrorCenterPos.z + mirrorPropsDim.z * 2.f;
        float f = _farClipPlaneDistance;
        float r = -mirrorCenterPos.x + mirrorPropsDim.x / 2.f;
        float l = -mirrorCenterPos.x - mirrorPropsDim.x / 2.f;
        float t = mirrorCenterPos.y + mirrorPropsDim.y / 2.f;
        float b = mirrorCenterPos.y - mirrorPropsDim.y / 2.f;

        glm::mat4 frustum = glm::frustum(l, r, b, t, n, f);
        glm::mat4 projection = frustum * mirrorFromEye;
        srcViewFrustum.setProjection(projection);

        glm::vec3 mirrorCenterPos3World = transformPoint(worldFromMirror, glm::vec3(mirrorCenterPos));
        qDebug() << "mirrorCenterPos3World  " << mirrorCenterPos3World.x << " , " << mirrorCenterPos3World.y << " , " << mirrorCenterPos3World.z;
        qDebug() << "mirrorWorldPos  " << mirrorWorldPos.x << " , " << mirrorWorldPos.y << " , " << mirrorWorldPos.z;
        DebugDraw::getInstance().drawRay(mirrorCenterPos3World, mirrorCenterPos3World + glm::vec3(0.f, 5.f, 0.f), glm::vec4(0.f, 1.f, 0.f, 1.f)); // green
        DebugDraw::getInstance().drawRay(mirrorWorldPos, mirrorWorldPos + glm::vec3(0.f, 5.f, 0.f), glm::vec4(1.f, 0.5f, 0.f, 1.f)); // orange
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
            if (_mirrorProjection && !_attachedEntityId.isNull()) {
                setMirrorProjection(srcViewFrustum);
            } else {
                if (!_attachedEntityId.isNull()) {
                    EntityItemProperties entityProperties = _entityScriptingInterface->getEntityProperties(_attachedEntityId, _attachedEntityPropertyFlags);
                    srcViewFrustum.setPosition(entityProperties.getPosition());
                    srcViewFrustum.setOrientation(entityProperties.getRotation());
                }
                else {
                    srcViewFrustum.setPosition(_position);
                    srcViewFrustum.setOrientation(_orientation);
                }
                srcViewFrustum.setProjection(glm::perspective(glm::radians(_vFoV), ((float)args->_viewport.z / (float)args->_viewport.w), _nearClipPlaneDistance, _farClipPlaneDistance));
            }
            // Without calculating the bound planes, the secondary camera will use the same culling frustum as the main camera,
            // which is not what we want here.
            srcViewFrustum.calculate();
            args->pushViewFrustum(srcViewFrustum);
            cachedArgs = _cachedArgsPointer;

            glm::vec3 dirNorm = glm::normalize(srcViewFrustum.getDirection());
            glm::vec3 nearPos = srcViewFrustum.getPosition() + dirNorm * srcViewFrustum.getNearClip();

            DebugDraw::getInstance().drawRay(srcViewFrustum.getPosition(), srcViewFrustum.getPosition() + dirNorm * srcViewFrustum.getNearClip(), glm::vec4(1.f, 0.f, 0.f, 1.f)); // red
            DebugDraw::getInstance().drawRay(nearPos, nearPos + srcViewFrustum.getRight(), glm::vec4(0.f, 0.f, 1.f, 1.f)); // blue
            DebugDraw::getInstance().drawRay(nearPos, nearPos + srcViewFrustum.getUp(), glm::vec4(0.5f, 0.f, 1.f, 1.f)); // purple

            srcViewFrustum.printDebugDetails();
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

void SecondaryCameraRenderTask::build(JobModel& task, const render::Varying& inputs, render::Varying& outputs, render::CullFunctor cullFunctor) {
    const auto cachedArg = task.addJob<SecondaryCameraJob>("SecondaryCamera");
    const auto items = task.addJob<RenderFetchCullSortTask>("FetchCullSort", cullFunctor);
    assert(items.canCast<RenderFetchCullSortTask::Output>());
    task.addJob<RenderDeferredTask>("RenderDeferredTask", items);
    task.addJob<EndSecondaryCameraFrame>("EndSecondaryCamera", cachedArg);
}