//
//  Created by Bradley Austin Davis on 2018/11/15
//  Copyright 2013-2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "VrHandler.h"

#include <android/native_window_jni.h>
#include <android/log.h>

#include <unistd.h>

#include <VrApi.h>
#include <VrApi_Helpers.h>
#include <VrApi_Types.h>
//#include <OVR_Platform.h>

#include "GLContext.h"
#include "Helpers.h"
#include "Framebuffer.h"



using namespace ovr;

static thread_local bool isRenderThread { false };

struct VrSurface : public TaskQueue {
    using HandlerTask = VrHandler::HandlerTask;

    JavaVM* vm{nullptr};
    jobject oculusActivity{ nullptr };
    ANativeWindow* nativeWindow{ nullptr };

    VrHandler* handler{nullptr};
    ovrMobile* session{nullptr};
    bool resumed { false };
    GLContext vrglContext;
    Framebuffer eyeFbos[2];
    uint32_t readFbo{0};
    std::atomic<uint32_t> presentIndex{1};
    double displayTime{0};
    // Not currently set by anything
    bool noErrorContext { false };

    static constexpr float EYE_BUFFER_SCALE = 1.0f;

    void onCreate(JNIEnv* env, jobject activity) {
        env->GetJavaVM(&vm);
        oculusActivity = env->NewGlobalRef(activity);
    }

    void setResumed(bool newResumed) {
        this->resumed = newResumed;
        submitRenderThreadTask([this](VrHandler* handler){ updateVrMode(); });
    }

    void setNativeWindow(ANativeWindow* newNativeWindow) {
        auto oldNativeWindow = nativeWindow;
        nativeWindow = newNativeWindow;
        if (oldNativeWindow) {
            ANativeWindow_release(oldNativeWindow);
        }
        submitRenderThreadTask([this](VrHandler* handler){ updateVrMode(); });
    }

    void init() {
        if (!handler) {
            return;
        }

        EGLContext currentContext = eglGetCurrentContext();
        EGLDisplay currentDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        vrglContext.create(currentDisplay, currentContext, noErrorContext);
        vrglContext.makeCurrent();

        glm::uvec2 eyeTargetSize;
        withEnv([&](JNIEnv* env){
            ovrJava java{ vm, env, oculusActivity };
            eyeTargetSize = glm::uvec2 {
                vrapi_GetSystemPropertyInt(&java, VRAPI_SYS_PROP_SUGGESTED_EYE_TEXTURE_WIDTH) * EYE_BUFFER_SCALE,
                vrapi_GetSystemPropertyInt(&java, VRAPI_SYS_PROP_SUGGESTED_EYE_TEXTURE_HEIGHT) * EYE_BUFFER_SCALE,
            };
        });
        __android_log_print(ANDROID_LOG_WARN, "QQQ_OVR", "QQQ Eye Size %d, %d", eyeTargetSize.x, eyeTargetSize.y);
        ovr::for_each_eye([&](ovrEye eye) {
            eyeFbos[eye].create(eyeTargetSize);
        });
        glGenFramebuffers(1, &readFbo);
        vrglContext.doneCurrent();
    }

    void shutdown() {
        noErrorContext = false;
        // Destroy the context?
    }

    void setHandler(VrHandler *newHandler, bool noError) {
        withLock([&] {
            isRenderThread = newHandler != nullptr;
            if (handler != newHandler) {
                shutdown();
                handler = newHandler;
                noErrorContext = noError;
                init();
                if (handler) {
                    updateVrMode();
                }
            }
        });
    }

    void submitRenderThreadTask(const HandlerTask &task) {
        withLockConditional([&](Lock &lock) {
            if (handler != nullptr) {
                submitTaskBlocking(lock, [&] {
                    task(handler);
                });
            }
        });
    }

    void withEnv(const std::function<void(JNIEnv*)>& f) {
        JNIEnv* env = nullptr;
        bool attached = false;
        vm->GetEnv((void**)&env, JNI_VERSION_1_6);
        if (!env) {
            attached = true;
            vm->AttachCurrentThread(&env, nullptr);
        }
        f(env);
        if (attached) {
            vm->DetachCurrentThread();
        }
    }

    void updateVrMode() {
        // For VR mode to be valid, the activity must be between an onResume and
        // an onPause call and must additionally have a valid native window handle
        bool vrReady = resumed && nullptr != nativeWindow;
        // If we're IN VR mode, we'll have a non-null ovrMobile pointer in session
        bool vrRunning = session != nullptr;
        if (vrReady != vrRunning) {
            if (vrRunning) {
                __android_log_write(ANDROID_LOG_WARN, "QQQ_OVR", "vrapi_LeaveVrMode");
                vrapi_SetClockLevels(session, 1, 1);
                vrapi_SetExtraLatencyMode(session, VRAPI_EXTRA_LATENCY_MODE_OFF);
                vrapi_LeaveVrMode(session);

                session = nullptr;
                oculusActivity = nullptr;
            } else {
                __android_log_write(ANDROID_LOG_WARN, "QQQ_OVR", "vrapi_EnterVrMode");
                withEnv([&](JNIEnv* env){
                    ovrJava java{ vm, env, oculusActivity };
                    ovrModeParms modeParms = vrapi_DefaultModeParms(&java);
                    modeParms.Flags |= VRAPI_MODE_FLAG_NATIVE_WINDOW;
                    if (noErrorContext) {
                        modeParms.Flags |= VRAPI_MODE_FLAG_CREATE_CONTEXT_NO_ERROR;
                    }
                    modeParms.Display = (unsigned long long) vrglContext.display;
                    modeParms.ShareContext = (unsigned long long) vrglContext.context;
                    modeParms.WindowSurface = (unsigned long long) nativeWindow;
                    session = vrapi_EnterVrMode(&modeParms);
                    vrapi_SetTrackingSpace( session, VRAPI_TRACKING_SPACE_LOCAL);
                    vrapi_SetPerfThread(session, VRAPI_PERF_THREAD_TYPE_RENDERER, gettid());
                    vrapi_SetClockLevels(session, 2, 4);
                    vrapi_SetExtraLatencyMode(session, VRAPI_EXTRA_LATENCY_MODE_DYNAMIC);
                    // Generates a warning on the quest: "vrapi_SetDisplayRefreshRate: Dynamic Display Refresh Rate not supported"
                    // vrapi_SetDisplayRefreshRate(session, 72);
                });
            }
        }
    }

    void presentFrame(uint32_t sourceTexture, const glm::uvec2 &sourceSize, const ovrTracking2& tracking) {
        ovrLayerProjection2 layer = vrapi_DefaultLayerProjection2();
        layer.HeadPose = tracking.HeadPose;
        if (sourceTexture) {
            glBindFramebuffer(GL_READ_FRAMEBUFFER, readFbo);
            glFramebufferTexture(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, sourceTexture, 0);
            GLenum framebufferStatus = glCheckFramebufferStatus(GL_READ_FRAMEBUFFER);
            if (GL_FRAMEBUFFER_COMPLETE != framebufferStatus) {
                __android_log_print(ANDROID_LOG_WARN, "QQQ_OVR", "incomplete framebuffer");
            }
        }
        GLenum invalidateAttachment = GL_COLOR_ATTACHMENT0;

        ovr::for_each_eye([&](ovrEye eye) {
            const auto &eyeTracking = tracking.Eye[eye];
            auto &eyeFbo = eyeFbos[eye];
            const auto &destSize = eyeFbo._size;
            eyeFbo.bind();
            glInvalidateFramebuffer(GL_DRAW_FRAMEBUFFER, 1, &invalidateAttachment);
            if (sourceTexture) {
                auto sourceWidth = sourceSize.x / 2;
                auto sourceX = (eye == VRAPI_EYE_LEFT) ? 0 : sourceWidth;
                glBlitFramebuffer(
                    sourceX, 0, sourceX + sourceWidth, sourceSize.y,
                    0, 0, destSize.x, destSize.y,
                    GL_COLOR_BUFFER_BIT, GL_NEAREST);
            }
            eyeFbo.updateLayer(eye, layer, &eyeTracking.ProjectionMatrix);
            eyeFbo.advance();
        });
        if (sourceTexture) {
            glInvalidateFramebuffer(GL_READ_FRAMEBUFFER, 1, &invalidateAttachment);
            glFramebufferTexture(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, 0, 0);
        }
        glFlush();

        ovrLayerHeader2 *layerHeader = &layer.Header;
        ovrSubmitFrameDescription2 frameDesc = {};
        frameDesc.SwapInterval = 2;
        frameDesc.FrameIndex = presentIndex;
        frameDesc.DisplayTime = displayTime;
        frameDesc.LayerCount = 1;
        frameDesc.Layers = &layerHeader;
        vrapi_SubmitFrame2(session, &frameDesc);
        ++presentIndex;
    }

    ovrTracking2 beginFrame() {
        displayTime = vrapi_GetPredictedDisplayTime(session, presentIndex);
        return vrapi_GetPredictedTracking2(session, displayTime);
    }
};

static VrSurface SURFACE;

bool VrHandler::vrActive() const {
    return SURFACE.session != nullptr;
}

void VrHandler::setHandler(VrHandler* handler, bool noError) {
    SURFACE.setHandler(handler, noError);
}

void VrHandler::pollTask() {
    SURFACE.pollTask();
}

void VrHandler::makeCurrent() {
    if (!SURFACE.vrglContext.makeCurrent()) {
        __android_log_write(ANDROID_LOG_WARN, "QQQ", "Failed to make GL current");
    }
}

void VrHandler::doneCurrent() {
    SURFACE.vrglContext.doneCurrent();
}

uint32_t VrHandler::currentPresentIndex() const {
    return SURFACE.presentIndex;
}

ovrTracking2 VrHandler::beginFrame() {
    return SURFACE.beginFrame();
}

void VrHandler::presentFrame(uint32_t sourceTexture, const glm::uvec2 &sourceSize, const ovrTracking2& tracking) const {
    SURFACE.presentFrame(sourceTexture, sourceSize, tracking);
}

bool VrHandler::withOvrJava(const OvrJavaTask& task) {
    SURFACE.withEnv([&](JNIEnv* env){
        ovrJava java{ SURFACE.vm, env, SURFACE.oculusActivity };
        task(&java);
    });
    return true;
}

bool VrHandler::withOvrMobile(const OvrMobileTask &task) {
    auto sessionTask = [&]()->bool{
        if (!SURFACE.session) {
            return false;
        }
        task(SURFACE.session);
        return true;
    };

    if (isRenderThread) {
        return sessionTask();
    }

    bool result = false;
    SURFACE.withLock([&]{
        result = sessionTask();
    });
    return result;
}


void VrHandler::initVr(const char* appId) {
    withOvrJava([&](const ovrJava* java){
        ovrInitParms initParms = vrapi_DefaultInitParms(java);
        initParms.GraphicsAPI = VRAPI_GRAPHICS_API_OPENGL_ES_3;
        if (vrapi_Initialize(&initParms) != VRAPI_INITIALIZE_SUCCESS) {
            __android_log_write(ANDROID_LOG_WARN, "QQQ_OVR", "Failed vrapi init");
        }
    });

  //  if (appId) {
  //      auto platformInitResult = ovr_PlatformInitializeAndroid(appId, activity.object(), env);
  //      if (ovrPlatformInitialize_Success != platformInitResult) {
  //          __android_log_write(ANDROID_LOG_WARN, "QQQ_OVR", "Failed ovr platform init");
  //      }
  //  }
}

void VrHandler::shutdownVr() {
    vrapi_Shutdown();
}

extern "C" {

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *, void *) {
    __android_log_write(ANDROID_LOG_WARN, "QQQ", "oculusMobile::JNI_OnLoad");
    return JNI_VERSION_1_6;
}

JNIEXPORT void JNICALL Java_io_highfidelity_oculus_OculusMobileActivity_nativeOnCreate(JNIEnv* env, jobject obj) {
    __android_log_write(ANDROID_LOG_WARN, "QQQ_JNI", __FUNCTION__);
    SURFACE.onCreate(env, obj);
}

JNIEXPORT void JNICALL Java_io_highfidelity_oculus_OculusMobileActivity_nativeOnResume(JNIEnv*, jclass) {
    __android_log_write(ANDROID_LOG_WARN, "QQQ_JNI", __FUNCTION__);
    SURFACE.setResumed(true);
}

JNIEXPORT void JNICALL Java_io_highfidelity_oculus_OculusMobileActivity_nativeOnPause(JNIEnv*, jclass) {
    __android_log_write(ANDROID_LOG_WARN, "QQQ_JNI", __FUNCTION__);
    SURFACE.setResumed(false);
}

JNIEXPORT void JNICALL Java_io_highfidelity_oculus_OculusMobileActivity_nativeOnSurfaceChanged(JNIEnv* env, jclass, jobject surface) {
    __android_log_write(ANDROID_LOG_WARN, "QQQ_JNI", __FUNCTION__);
    SURFACE.setNativeWindow(surface ? ANativeWindow_fromSurface( env, surface ) : nullptr);
}

} // extern "C"
