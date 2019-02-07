//
//  Created by Bradley Austin Davis on 2018/11/15
//  Copyright 2013-2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "OculusMobileDisplayPlugin.h"

#include <QtAndroidExtras/QAndroidJniEnvironment>
#include <glm/gtc/matrix_transform.hpp>
#include <VrApi_Types.h>
#include <VrApi_Helpers.h>

#include <AbstractViewStateInterface.h>
#include <gpu/Frame.h>
#include <gpu/Context.h>
#include <gpu/gl/GLBackend.h>
#include <ViewFrustum.h>
#include <plugins/PluginManager.h>
#include <ui-plugins/PluginContainer.h>
#include <controllers/Pose.h>
#include <display-plugins/CompositorHelper.h>
#include <gpu/Frame.h>
#include <gl/Config.h>
#include <gl/GLWidget.h>
#include <gl/Context.h>
#include <MainWindow.h>
#include <AddressManager.h>

#include <ovr/Helpers.h>
#include <VrApi.h>

using namespace ovr;

const char* OculusMobileDisplayPlugin::NAME { "Oculus Rift" };
//thread_local bool renderThread = false;
#define OCULUS_APP_ID 2331695256865113

OculusMobileDisplayPlugin::OculusMobileDisplayPlugin() {

}

OculusMobileDisplayPlugin::~OculusMobileDisplayPlugin() {
}

void OculusMobileDisplayPlugin::init() {
    Parent::init();
    initVr();

    emit deviceConnected(getName());
}

void OculusMobileDisplayPlugin::deinit() {
    shutdownVr();
    Parent::deinit();
}

bool OculusMobileDisplayPlugin::internalActivate() {
    _renderTargetSize = { 1024, 512 };
    _cullingProjection = ovr::toGlm(ovrMatrix4f_CreateProjectionFov(90.0f, 90.0f, 0.0f, 0.0f, DEFAULT_NEAR_CLIP, DEFAULT_FAR_CLIP));


    withOvrJava([&](const ovrJava* java){
        _renderTargetSize = glm::uvec2{
            vrapi_GetSystemPropertyInt(java, VRAPI_SYS_PROP_SUGGESTED_EYE_TEXTURE_WIDTH),
            vrapi_GetSystemPropertyInt(java, VRAPI_SYS_PROP_SUGGESTED_EYE_TEXTURE_HEIGHT),
        };
    });

    ovr::for_each_eye([&](ovrEye eye){
        _eyeProjections[eye] = _cullingProjection;
    });

    // This must come after the initialization, so that the values calculated
    // above are available during the customizeContext call (when not running
    // in threaded present mode)
    return Parent::internalActivate();
}

void OculusMobileDisplayPlugin::internalDeactivate() {
    Parent::internalDeactivate();
    // ovr::releaseRenderSession(_session);
}

void OculusMobileDisplayPlugin::customizeContext() {
    qWarning() << "QQQ" << __FUNCTION__ << "done";
    gl::initModuleGl();
    _mainContext = _container->getPrimaryWidget()->context();
    _mainContext->makeCurrent();
    ovr::VrHandler::setHandler(this);
    _mainContext->doneCurrent();
    _mainContext->makeCurrent();
    Parent::customizeContext();
    qWarning() << "QQQ" << __FUNCTION__ << "done";
}

void OculusMobileDisplayPlugin::uncustomizeContext() {
    ovr::VrHandler::setHandler(nullptr);
    _mainContext->doneCurrent();
    _mainContext->makeCurrent();
    Parent::uncustomizeContext();
}

QRectF OculusMobileDisplayPlugin::getPlayAreaRect() {
    QRectF result;
    VrHandler::withOvrMobile([&](ovrMobile* session){
        ovrPosef pose;
        ovrVector3f scale;
        if (ovrSuccess != vrapi_GetBoundaryOrientedBoundingBox(session, &pose, &scale)) {
            return;
        }
        // FIXME extract the center from the pose
        glm::vec2 center { 0 };
        glm::vec2 dimensions = glm::vec2(scale.x, scale.z);
        dimensions *= 2.0f;
        result = QRectF(center.x, center.y, dimensions.x, dimensions.y);
    });
    return result;
}

glm::mat4 OculusMobileDisplayPlugin::getEyeProjection(Eye eye, const glm::mat4& baseProjection) const {
    glm::mat4 result = baseProjection;
    VrHandler::withOvrMobile([&](ovrMobile* session){
        auto trackingState = vrapi_GetPredictedTracking2(session, 0.0);
        result = ovr::Fov{ trackingState.Eye[eye].ProjectionMatrix }.withZ(baseProjection);
    });
    return result;
}

glm::mat4 OculusMobileDisplayPlugin::getCullingProjection(const glm::mat4& baseProjection) const {
    glm::mat4 result = baseProjection;
    VrHandler::withOvrMobile([&](ovrMobile* session){
        auto trackingState = vrapi_GetPredictedTracking2(session, 0.0);
        ovr::Fov fovs[2];
        for (size_t i = 0; i < 2; ++i) {
            fovs[i].extract(trackingState.Eye[i].ProjectionMatrix);
        }
        fovs[0].extend(fovs[1]);
        return fovs[0].withZ(baseProjection);
    });
    return result;
}

void OculusMobileDisplayPlugin::resetSensors() {
    VrHandler::withOvrMobile([&](ovrMobile* session){
        vrapi_RecenterPose(session);
    });
    _currentRenderFrameInfo.renderPose = glm::mat4(); // identity
}

float OculusMobileDisplayPlugin::getTargetFrameRate() const {
    float result = 0.0f;
    VrHandler::withOvrJava([&](const ovrJava* java){
        result = vrapi_GetSystemPropertyFloat(java, VRAPI_SYS_PROP_DISPLAY_REFRESH_RATE);
    });
    return result;
}

bool OculusMobileDisplayPlugin::isHmdMounted() const {
    bool result = false;
    VrHandler::withOvrJava([&](const ovrJava* java){
        result = VRAPI_FALSE != vrapi_GetSystemStatusInt(java, VRAPI_SYS_STATUS_MOUNTED);
    });
    return result;
}

static void goToDevMobile() {
    auto addressManager = DependencyManager::get<AddressManager>();
    auto currentAddress = addressManager->currentAddress().toString().toStdString();
    if (std::string::npos == currentAddress.find("dev-mobile")) {
        addressManager->handleLookupString("hifi://dev-mobile/495.236,501.017,482.434/0,0.97452,0,-0.224301");
        //addressManager->handleLookupString("hifi://dev-mobile/504,498,491/0,0,0,0");
        //addressManager->handleLookupString("hifi://dev-mobile/0,-1,1");
    }
}

// Called on the render thread, establishes the rough tracking for the upcoming
bool OculusMobileDisplayPlugin::beginFrameRender(uint32_t frameIndex) {
    static QAndroidJniEnvironment* jniEnv = nullptr;
    if (nullptr == jniEnv) {
        jniEnv = new QAndroidJniEnvironment();
    }
    bool result = false;
    _currentRenderFrameInfo = FrameInfo();
    ovrTracking2 trackingState = {};
    static bool resetTrackingTransform = true;
    static glm::mat4 transformOffset;

    VrHandler::withOvrMobile([&](ovrMobile* session){
        if (resetTrackingTransform) {
            auto pose = vrapi_GetTrackingTransform( session, VRAPI_TRACKING_TRANSFORM_SYSTEM_CENTER_FLOOR_LEVEL);
            transformOffset = glm::inverse(ovr::toGlm(pose));
            vrapi_SetTrackingTransform( session, pose);
            resetTrackingTransform = false;
        }
        // Find a better way of
        _currentRenderFrameInfo.predictedDisplayTime = vrapi_GetPredictedDisplayTime(session, currentPresentIndex() + 2);
        trackingState = vrapi_GetPredictedTracking2(session, _currentRenderFrameInfo.predictedDisplayTime);
        result = true;
    });



    if (result) {
        _currentRenderFrameInfo.renderPose = transformOffset;
        withNonPresentThreadLock([&] {
            _currentRenderFrameInfo.sensorSampleTime = trackingState.HeadPose.TimeInSeconds;
            _currentRenderFrameInfo.renderPose = transformOffset * ovr::toGlm(trackingState.HeadPose.Pose);
            _currentRenderFrameInfo.presentPose = _currentRenderFrameInfo.renderPose;
            _frameInfos[frameIndex] = _currentRenderFrameInfo;
            _ipd = vrapi_GetInterpupillaryDistance(&trackingState);
            ovr::for_each_eye([&](ovrEye eye){
                _eyeProjections[eye] = ovr::toGlm(trackingState.Eye[eye].ProjectionMatrix);
                _eyeOffsets[eye] = glm::translate(mat4(), vec3{ _ipd * (eye == VRAPI_EYE_LEFT ? -0.5f : 0.5f), 0.0f, 0.0f });
            });
       });
    }

    //  static uint32_t count = 0;
    //  if ((++count % 1000) == 0) {
    //      AbstractViewStateInterface::instance()->postLambdaEvent([] {
    //          goToDevMobile();
    //      });
    //  }

    return result && Parent::beginFrameRender(frameIndex);
}

ovrTracking2 presentTracking;

void OculusMobileDisplayPlugin::updatePresentPose() {
    static QAndroidJniEnvironment* jniEnv = nullptr;
    if (nullptr == jniEnv) {
        jniEnv = new QAndroidJniEnvironment();
    }
    VrHandler::withOvrMobile([&](ovrMobile* session){
        presentTracking = beginFrame();
        _currentPresentFrameInfo.sensorSampleTime = vrapi_GetTimeInSeconds();
        _currentPresentFrameInfo.predictedDisplayTime = presentTracking.HeadPose.TimeInSeconds;
        _currentPresentFrameInfo.presentPose = ovr::toGlm(presentTracking.HeadPose.Pose);
    });
}

void OculusMobileDisplayPlugin::internalPresent() {
    VrHandler::pollTask();

    if (!vrActive()) {
        QThread::msleep(1);
        return;
    }

    auto sourceTexture = getGLBackend()->getTextureID(_compositeFramebuffer->getRenderBuffer(0));
    glm::uvec2 sourceSize{ _compositeFramebuffer->getWidth(), _compositeFramebuffer->getHeight() };
    VrHandler::presentFrame(sourceTexture, sourceSize, presentTracking);
    _presentRate.increment();
}

DisplayPluginList getDisplayPlugins() {
    static DisplayPluginList result;
    static std::once_flag once;
    std::call_once(once, [&]{
        auto plugin = std::make_shared<OculusMobileDisplayPlugin>();
        plugin->isSupported();
        result.push_back(plugin);
    });
    return result;
}

