//
//  Created by Bradley Austin Davis on 2018/10/21
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "RenderThread.h"

#include <mutex>

#include <jni.h>
#include <android/log.h>

#include <QtCore/QFileInfo>
#include <QtGui/QWindow>
#include <QtGui/QImageReader>
#include <QtAndroidExtras/QAndroidJniObject>

#include <gl/QOpenGLContextWrapper.h>
#include <gpu/FrameIO.h>
#include <gpu/Texture.h>

#include <VrApi_Types.h>
#include <VrApi_Helpers.h>
#include <ovr/VrHandler.h>
#include <ovr/Helpers.h>

#include <VrApi.h>
#include <VrApi_Input.h>

#include "AndroidHelper.h"

//static jobject _activity { nullptr };

struct HandController{
    ovrInputTrackedRemoteCapabilities caps {};
    ovrInputStateTrackedRemote state {};
    ovrResult stateResult{ ovrSuccess };
    ovrTracking tracking {};
    ovrResult trackingResult{ ovrSuccess };

    void update(ovrMobile* session, double time = 0.0) {
        const auto& deviceId = caps.Header.DeviceID;
        stateResult = vrapi_GetCurrentInputState(session, deviceId, &state.Header);
        trackingResult = vrapi_GetInputTrackingState(session, deviceId, 0.0, &tracking);
    }
};

std::vector<HandController> devices;
QAndroidJniObject __interfaceActivity;

extern "C" {

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void *) {
    __android_log_write(ANDROID_LOG_WARN, "QQQ", __FUNCTION__);
    return JNI_VERSION_1_6;
}

JNIEXPORT void JNICALL
Java_io_highfidelity_oculus_OculusMobileActivity_questNativeOnCreate(JNIEnv *env, jobject obj) {
    __android_log_print(ANDROID_LOG_INFO, "QQQ", __FUNCTION__);
    __interfaceActivity = QAndroidJniObject(obj);
    QObject::connect(&AndroidHelper::instance(), &AndroidHelper::qtAppLoadComplete, []() {
        __interfaceActivity.callMethod<void>("onAppLoadedComplete", "()V");
        QObject::disconnect(&AndroidHelper::instance(), &AndroidHelper::qtAppLoadComplete, nullptr, nullptr);
    });
}

JNIEXPORT void
Java_io_highfidelity_oculus_OculusMobileActivity_questOnAppAfterLoad(JNIEnv *env, jobject obj) {
    AndroidHelper::instance().moveToThread(qApp->thread());
}

JNIEXPORT void JNICALL
Java_io_highfidelity_oculus_OculusMobileActivity_questNativeOnPause(JNIEnv *env, jobject obj) {
    AndroidHelper::instance().notifyEnterBackground();
}

JNIEXPORT void JNICALL
Java_io_highfidelity_oculus_OculusMobileActivity_questNativeOnResume(JNIEnv *env, jobject obj) {
    AndroidHelper::instance().notifyEnterForeground();
}

}


static const char* FRAME_FILE = "assets:/frames/20190121_1220.json";

static void textureLoader(const std::string& filename, const gpu::TexturePointer& texture, uint16_t layer) {
    QImage image;
    QImageReader(filename.c_str()).read(&image);
    if (layer > 0) {
        return;
    }
    texture->assignStoredMip(0, image.byteCount(), image.constBits());
}

void RenderThread::submitFrame(const gpu::FramePointer& frame) {
    std::unique_lock<std::mutex> lock(_frameLock);
    _pendingFrames.push(frame);
}

void RenderThread::move(const glm::vec3& v) {
    std::unique_lock<std::mutex> lock(_frameLock);
    _correction = glm::inverse(glm::translate(mat4(), v)) * _correction;
}

void RenderThread::initialize(QWindow* window) {
    std::unique_lock<std::mutex> lock(_frameLock);
    setObjectName("RenderThread");
    Parent::initialize();
    _window = window;
    _thread->setObjectName("RenderThread");
}

void RenderThread::setup() {
    // Wait until the context has been moved to this thread
    { std::unique_lock<std::mutex> lock(_frameLock); }

    ovr::VrHandler::initVr();
    ovr::VrHandler::setHandler(this);

    makeCurrent();

    // GPU library init
    gpu::Context::init<gpu::gl::GLBackend>();
    _gpuContext = std::make_shared<gpu::Context>();
    _backend = _gpuContext->getBackend();
    _gpuContext->beginFrame();
    _gpuContext->endFrame();

    makeCurrent();
    glGenTextures(1, &_externalTexture);
    glBindTexture(GL_TEXTURE_2D, _externalTexture);
    static const glm::u8vec4 color{ 0,1,0,0 };
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, &color);

    if (QFileInfo(FRAME_FILE).exists()) {
        auto frame = gpu::readFrame(FRAME_FILE, _externalTexture, &textureLoader);
        submitFrame(frame);
    }
}

void RenderThread::shutdown() {
    _activeFrame.reset();
    while (!_pendingFrames.empty()) {
        _gpuContext->consumeFrameUpdates(_pendingFrames.front());
        _pendingFrames.pop();
    }
    _gpuContext->shutdown();
    _gpuContext.reset();
}

void RenderThread::handleInput() {
    static std::once_flag once;
    std::call_once(once, [&]{
        withOvrMobile([&](ovrMobile* session){
            int deviceIndex = 0;
            ovrInputCapabilityHeader capsHeader;
            while (vrapi_EnumerateInputDevices(session, deviceIndex, &capsHeader) >= 0) {
                if (capsHeader.Type == ovrControllerType_TrackedRemote) {
                    HandController controller = {};
                    controller.caps.Header = capsHeader;
                    controller.state.Header.ControllerType = ovrControllerType_TrackedRemote;
                    vrapi_GetInputDeviceCapabilities( session, &controller.caps.Header);
                    devices.push_back(controller);
                }
                ++deviceIndex;
            }
        });
    });

    auto readResult = ovr::VrHandler::withOvrMobile([&](ovrMobile *session) {
        for (auto &controller : devices) {
            controller.update(session);
        }
    });

    if (readResult) {
        for (auto &controller : devices) {
            const auto &caps = controller.caps;
            if (controller.stateResult >= 0) {
                const auto &remote = controller.state;
                if (remote.Joystick.x != 0.0f || remote.Joystick.y != 0.0f) {
                    glm::vec3 translation;
                    float rotation = 0.0f;
                    if (caps.ControllerCapabilities & ovrControllerCaps_LeftHand) {
                        translation = glm::vec3{0.0f, -remote.Joystick.y, 0.0f};
                    } else {
                        translation = glm::vec3{remote.Joystick.x, 0.0f, -remote.Joystick.y};
                    }
                    float scale = 0.1f + (1.9f * remote.GripTrigger);
                    _correction = glm::translate(glm::mat4(), translation * scale) * _correction;
                }
            }
        }
    }
}

void RenderThread::renderFrame() {
    GLuint finalTexture = 0;
    glm::uvec2 finalTextureSize;
    const auto& tracking = beginFrame();
    if (_activeFrame) {
        const auto& frame = _activeFrame;
        auto& eyeProjections = frame->stereoState._eyeProjections;
        auto& eyeOffsets = frame->stereoState._eyeViews;
        // Quest
        auto frameCorrection = _correction * ovr::toGlm(tracking.HeadPose.Pose);
        _backend->setCameraCorrection(glm::inverse(frameCorrection), frame->view);
        ovr::for_each_eye([&](ovrEye eye){
            const auto& eyeInfo = tracking.Eye[eye];
            eyeProjections[eye] = ovr::toGlm(eyeInfo.ProjectionMatrix);
            eyeOffsets[eye] = ovr::toGlm(eyeInfo.ViewMatrix);
        });
        _backend->recycle();
        _backend->syncCache();
        _gpuContext->enableStereo(true);
        if (frame && !frame->batches.empty()) {
            _gpuContext->executeFrame(frame);
        }
        auto& glbackend = (gpu::gl::GLBackend&)(*_backend);
        finalTextureSize = { frame->framebuffer->getWidth(), frame->framebuffer->getHeight() };
        finalTexture = glbackend.getTextureID(frame->framebuffer->getRenderBuffer(0));
    }
    presentFrame(finalTexture, finalTextureSize, tracking);
}

bool RenderThread::process() {
    pollTask();

    if (!vrActive()) {
        QThread::msleep(1);
        return true;
    }

    std::queue<gpu::FramePointer> pendingFrames;
    {
        std::unique_lock<std::mutex> lock(_frameLock);
        pendingFrames.swap(_pendingFrames);
    }

    makeCurrent();
    while (!pendingFrames.empty()) {
        _activeFrame = pendingFrames.front();
        pendingFrames.pop();
        _gpuContext->consumeFrameUpdates(_activeFrame);
        _activeFrame->stereoState._enable = true;
    }

    handleInput();
    renderFrame();
    return true;
}
