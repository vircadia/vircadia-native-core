//
//  Created by Bradley Austin Davis on 2014/04/13.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "OculusBaseDisplayPlugin.h"

#include <ViewFrustum.h>
#include <controllers/Pose.h>
#include <display-plugins/CompositorHelper.h>
#include <gpu/Frame.h>
#include <gl/Config.h>
#include <shared/GlobalAppProperties.h>

#include "OculusHelpers.h"

using namespace hifi;

void OculusBaseDisplayPlugin::resetSensors() {
    ovr_RecenterTrackingOrigin(_session);
    _currentRenderFrameInfo.renderPose = glm::mat4(); // identity
}

bool OculusBaseDisplayPlugin::beginFrameRender(uint32_t frameIndex) {
    ovrResult getStatusResult;
    ovrSessionStatus status = ovr::getStatus(getStatusResult);
    if (!OVR_SUCCESS(getStatusResult)) {
        qCWarning(oculusLog) << "Unable to fetch Oculus session status" << ovr::getError();
        return false;
    }

    if (ovr::quitRequested(status) || ovr::displayLost(status)) {
        QMetaObject::invokeMethod(qApp, "quit");
        return false;
    }
    if (ovr::reorientRequested(status)) {
        emit resetSensorsRequested();
    }
    if (ovr::hmdMounted(status) != _hmdMounted) {
        _hmdMounted = !_hmdMounted;
        emit hmdMountedChanged();
    }
    if (ovr::isVisible(status) != _visible) {
        _visible = !_visible;
        emit hmdVisibleChanged(_visible);
    }

    _currentRenderFrameInfo = FrameInfo();
    _currentRenderFrameInfo.sensorSampleTime = ovr_GetTimeInSeconds();
    _currentRenderFrameInfo.predictedDisplayTime = ovr_GetPredictedDisplayTime(_session, frameIndex);
    auto trackingState = ovr::getTrackingState(_currentRenderFrameInfo.predictedDisplayTime, ovrTrue);
    _currentRenderFrameInfo.renderPose = ovr::toGlm(trackingState.HeadPose.ThePose);
    _currentRenderFrameInfo.presentPose = _currentRenderFrameInfo.renderPose;

    std::array<glm::mat4, 2> handPoses;
    // Make controller poses available to the presentation thread
    ovr::for_each_hand([&](ovrHandType hand) {
        static const auto REQUIRED_HAND_STATUS = ovrStatus_OrientationTracked | ovrStatus_PositionTracked;
        if (REQUIRED_HAND_STATUS != (trackingState.HandStatusFlags[hand] & REQUIRED_HAND_STATUS)) {
            return;
        }

        auto correctedPose = ovr::toControllerPose(hand, trackingState.HandPoses[hand]);
        static const glm::quat HAND_TO_LASER_ROTATION = glm::rotation(Vectors::UNIT_Z, Vectors::UNIT_NEG_Y);
        handPoses[hand] = glm::translate(glm::mat4(), correctedPose.translation) * glm::mat4_cast(correctedPose.rotation * HAND_TO_LASER_ROTATION);
    });

    withNonPresentThreadLock([&] {
        _frameInfos[frameIndex] = _currentRenderFrameInfo;
    });
    return Parent::beginFrameRender(frameIndex);
}

bool OculusBaseDisplayPlugin::isSupported() const {
    return ovr::available();
}

glm::mat4 OculusBaseDisplayPlugin::getEyeProjection(Eye eye, const glm::mat4& baseProjection) const {
    if (_session) {
        ViewFrustum baseFrustum;
        baseFrustum.setProjection(baseProjection);
        float baseNearClip = baseFrustum.getNearClip();
        float baseFarClip = baseFrustum.getFarClip();
        ovrEyeType ovrEye = (eye == Left) ? ovrEye_Left : ovrEye_Right;
        ovrFovPort fovPort = _hmdDesc.DefaultEyeFov[eye];
        ovrEyeRenderDesc& erd = ovr_GetRenderDesc(_session, ovrEye, fovPort);
        ovrMatrix4f ovrPerspectiveProjection = ovrMatrix4f_Projection(erd.Fov, baseNearClip, baseFarClip, ovrProjection_ClipRangeOpenGL);
        return ovr::toGlm(ovrPerspectiveProjection);
    } else {
        return baseProjection;
    }
}

glm::mat4 OculusBaseDisplayPlugin::getCullingProjection(const glm::mat4& baseProjection) const {
    if (_session) {
        ViewFrustum baseFrustum;
        baseFrustum.setProjection(baseProjection);
        float baseNearClip = baseFrustum.getNearClip();
        float baseFarClip = baseFrustum.getFarClip();
        auto combinedFov = _eyeFovs[0];
        combinedFov.LeftTan = combinedFov.RightTan = std::max(combinedFov.LeftTan, combinedFov.RightTan);
        return ovr::toGlm(ovrMatrix4f_Projection(combinedFov, baseNearClip, baseFarClip, ovrProjection_ClipRangeOpenGL));
    } else {
        return baseProjection;
    }
}

// DLL based display plugins MUST initialize GLEW inside the DLL code.
void OculusBaseDisplayPlugin::customizeContext() {
    gl::initModuleGl();
    Parent::customizeContext();
}

void OculusBaseDisplayPlugin::uncustomizeContext() {
    Parent::uncustomizeContext();
}

bool OculusBaseDisplayPlugin::internalActivate() {
    _session = ovr::acquireRenderSession();
    if (!_session) {
        return false;
    }

    _hmdDesc = ovr_GetHmdDesc(_session);

    glm::uvec2 eyeSizes[2];
    _viewScaleDesc.HmdSpaceToWorldScaleInMeters = 1.0f;

    _ipd = 0;
    ovr::for_each_eye([&](ovrEyeType eye) {
        _eyeFovs[eye] = _hmdDesc.DefaultEyeFov[eye];
        ovrEyeRenderDesc& erd = _eyeRenderDescs[eye] = ovr_GetRenderDesc(_session, eye, _eyeFovs[eye]);
        ovrMatrix4f ovrPerspectiveProjection =
            ovrMatrix4f_Projection(erd.Fov, DEFAULT_NEAR_CLIP, DEFAULT_FAR_CLIP, ovrProjection_ClipRangeOpenGL);
        _eyeProjections[eye] = ovr::toGlm(ovrPerspectiveProjection);
        _eyeOffsets[eye] = ovr::toGlm(erd.HmdToEyePose);
        eyeSizes[eye] = ovr::toGlm(ovr_GetFovTextureSize(_session, eye, erd.Fov, 1.0f));
        _viewScaleDesc.HmdToEyePose[eye] = erd.HmdToEyePose;
        _ipd += glm::abs(glm::length(ovr::toGlm(erd.HmdToEyePose.Position)));
    });

    auto combinedFov = _eyeFovs[0];
    combinedFov.LeftTan = combinedFov.RightTan = std::max(combinedFov.LeftTan, combinedFov.RightTan);
    _cullingProjection = ovr::toGlm(ovrMatrix4f_Projection(combinedFov, DEFAULT_NEAR_CLIP, DEFAULT_FAR_CLIP, ovrProjection_ClipRangeOpenGL));

    _renderTargetSize = uvec2(
        eyeSizes[0].x + eyeSizes[1].x,
        std::max(eyeSizes[0].y, eyeSizes[1].y));

    memset(&_sceneLayer, 0, sizeof(ovrLayerEyeFov));
    _sceneLayer.Header.Type = ovrLayerType_EyeFov;
    _sceneLayer.Header.Flags = ovrLayerFlag_TextureOriginAtBottomLeft;
    ovr::for_each_eye([&](ovrEyeType eye) {
        ovrFovPort & fov = _sceneLayer.Fov[eye] = _eyeRenderDescs[eye].Fov;
        ovrSizei & size = _sceneLayer.Viewport[eye].Size = ovr_GetFovTextureSize(_session, eye, fov, 1.0f);
        _sceneLayer.Viewport[eye].Pos = { eye == ovrEye_Left ? 0 : size.w, 0 };
    });

    // This must come after the initialization, so that the values calculated 
    // above are available during the customizeContext call (when not running
    // in threaded present mode)
    return Parent::internalActivate();
}

void OculusBaseDisplayPlugin::internalDeactivate() {
    Parent::internalDeactivate();
    ovr::releaseRenderSession(_session);
}

void OculusBaseDisplayPlugin::updatePresentPose() {
    ovrTrackingState trackingState;
    _currentPresentFrameInfo.sensorSampleTime = ovr_GetTimeInSeconds();
    _currentPresentFrameInfo.predictedDisplayTime = ovr_GetPredictedDisplayTime(_session, 0);
    trackingState = ovr::getTrackingState(_currentRenderFrameInfo.predictedDisplayTime);
    _currentPresentFrameInfo.presentPose = ovr::toGlm(trackingState.HeadPose.ThePose);
    _currentPresentFrameInfo.renderPose = _currentPresentFrameInfo.presentPose;
}

QRectF OculusBaseDisplayPlugin::getPlayAreaRect() {
    if (!_session) {
        return QRectF();
    }

    int floorPointsCount = 0;
    auto result = ovr_GetBoundaryGeometry(_session, ovrBoundary_PlayArea, nullptr, &floorPointsCount);
    if (!OVR_SUCCESS(result) || floorPointsCount != 4) {
        return QRectF();
    }

    auto floorPoints = new ovrVector3f[floorPointsCount];
    result = ovr_GetBoundaryGeometry(_session, ovrBoundary_PlayArea, floorPoints, nullptr);
    if (!OVR_SUCCESS(result)) {
        return QRectF();
    }

    auto minXZ = ovr::toGlm(floorPoints[0]);
    auto maxXZ = minXZ;
    for (int i = 1; i < floorPointsCount; i++) {
        auto point = ovr::toGlm(floorPoints[i]);
        minXZ.x = std::min(minXZ.x, point.x);
        minXZ.z = std::min(minXZ.z, point.z);
        maxXZ.x = std::max(maxXZ.x, point.x);
        maxXZ.z = std::max(maxXZ.z, point.z);
    }

    glm::vec2 center = glm::vec2((minXZ.x + maxXZ.x) / 2, (minXZ.z + maxXZ.z) / 2);
    glm::vec2 dimensions = glm::vec2(maxXZ.x - minXZ.x, maxXZ.z - minXZ.z);

    return QRectF(center.x, center.y, dimensions.x, dimensions.y);
}

QVector<glm::vec3> OculusBaseDisplayPlugin::getSensorPositions() {
    if (!_session) {
        return QVector<glm::vec3>();
    }

    QVector<glm::vec3> result;
    auto numTrackers = ovr_GetTrackerCount(_session);
    for (uint i = 0; i < numTrackers; i++) {
        auto trackerPose = ovr_GetTrackerPose(_session, i);
        if (trackerPose.TrackerFlags & ovrTracker_PoseTracked) {
            result.append(ovr::toGlm(trackerPose.Pose.Position));
        }
    }

    return result;
}

DisplayPlugin::StencilMaskMeshOperator OculusBaseDisplayPlugin::getStencilMaskMeshOperator() {
    if (_session) {
        if (!_stencilMeshesInitialized) {
            _stencilMeshesInitialized = true;
            ovr::for_each_eye([&](ovrEyeType eye) {
                ovrFovStencilDesc stencilDesc = {
                    ovrFovStencil_HiddenArea, 0, eye,
                    _eyeRenderDescs[eye].Fov, _eyeRenderDescs[eye].HmdToEyePose.Orientation
                };
                // First we get the size of the buffer we need
                ovrFovStencilMeshBuffer buffer = { 0, 0, nullptr, 0, 0, nullptr };
                ovrResult result = ovr_GetFovStencil(_session, &stencilDesc, &buffer);
                if (!OVR_SUCCESS(result)) {
                    _stencilMeshesInitialized = false;
                    return;
                }

                std::vector<ovrVector2f> ovrVertices(buffer.UsedVertexCount);
                std::vector<uint16_t> ovrIndices(buffer.UsedIndexCount);

                // Now we populate the actual buffer
                buffer = { (int)ovrVertices.size(), 0, ovrVertices.data(), (int)ovrIndices.size(), 0, ovrIndices.data() };
                result = ovr_GetFovStencil(_session, &stencilDesc, &buffer);

                if (!OVR_SUCCESS(result)) {
                    _stencilMeshesInitialized = false;
                    return;
                }

                std::vector<glm::vec3> vertices;
                vertices.reserve(ovrVertices.size());
                for (auto& ovrVertex : ovrVertices) {
                    // We need the vertices in clip space
                    vertices.emplace_back(ovrVertex.x - (1.0f - (float)eye),  2.0f * ovrVertex.y - 1.0f, 0.0f);
                }

                std::vector<uint32_t> indices;
                indices.reserve(ovrIndices.size());
                for (auto& ovrIndex : ovrIndices) {
                    indices.push_back(ovrIndex);
                }

                _stencilMeshes[eye] = graphics::Mesh::createIndexedTriangles_P3F((uint32_t)vertices.size(), (uint32_t)indices.size(), vertices.data(), indices.data());
            });
        }

        if (_stencilMeshesInitialized) {
            return [&](gpu::Batch& batch) {
                for (auto& mesh : _stencilMeshes) {
                    batch.setIndexBuffer(mesh->getIndexBuffer());
                    batch.setInputFormat((mesh->getVertexFormat()));
                    batch.setInputStream(0, mesh->getVertexStream());

                    // Draw
                    auto part = mesh->getPartBuffer().get<graphics::Mesh::Part>(0);
                    batch.drawIndexed(gpu::TRIANGLES, part._numIndices, part._startIndex);
                }
            };
        }
    }
    return nullptr;
}