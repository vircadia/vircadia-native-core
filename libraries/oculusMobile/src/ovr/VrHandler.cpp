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
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>

#include <unistd.h>
#include <algorithm>
#include <array>

#include <VrApi.h>
#include <VrApi_Helpers.h>
#include <VrApi_Types.h>
//#include <OVR_Platform.h>


#include "GLContext.h"
#include "Helpers.h"
#include "Framebuffer.h"

static AAssetManager* ASSET_MANAGER = nullptr;

#define USE_BLIT_PRESENT 1

#if !USE_BLIT_PRESENT



static std::string getTextAsset(const char* assetPath) {
    if (!ASSET_MANAGER || !assetPath) {
        return nullptr;
    }
    AAsset* asset = AAssetManager_open(ASSET_MANAGER, assetPath, AASSET_MODE_BUFFER);
    if (!asset) {
        return {};
    }

    auto length = AAsset_getLength(asset);
    if (0 == length) {
        AAsset_close(asset);
        return {};
    }

    auto buffer = AAsset_getBuffer(asset);
    if (!buffer) {
        AAsset_close(asset);
        return {};
    }

    std::string result { static_cast<const char*>(buffer), static_cast<size_t>(length) };
    AAsset_close(asset);
    return result;
}

static std::string getShaderInfoLog(GLuint glshader) {
    std::string result;
    GLint infoLength = 0;
    glGetShaderiv(glshader, GL_INFO_LOG_LENGTH, &infoLength);
    if (infoLength > 0) {
        char* temp = new char[infoLength];
        glGetShaderInfoLog(glshader, infoLength, NULL, temp);
        result = std::string(temp);
        delete[] temp;
    }
    return result;
}

static GLuint buildShader(GLenum shaderDomain, const char* shader) {
    GLuint glshader = glCreateShader(shaderDomain);
    if (!glshader) {
        throw std::runtime_error("Bad shader");
    }

    glShaderSource(glshader, 1, &shader, NULL);
    glCompileShader(glshader);

    GLint compiled = 0;
    glGetShaderiv(glshader, GL_COMPILE_STATUS, &compiled);

    // if compilation fails
    if (!compiled) {
        std::string compileError = getShaderInfoLog(glshader);
        glDeleteShader(glshader);
        __android_log_print(ANDROID_LOG_WARN, "QQQ_OVR", "Shader compile error: %s", compileError.c_str());
        return 0;
    }

    return glshader;
}

static std::string getProgramInfoLog(GLuint glprogram) {
    std::string result;
    GLint infoLength = 0;
    glGetProgramiv(glprogram, GL_INFO_LOG_LENGTH, &infoLength);
    if (infoLength > 0) {
        char* temp = new char[infoLength];
        glGetProgramInfoLog(glprogram, infoLength, NULL, temp);
        result = std::string(temp);
        delete[] temp;
    } 
    return result;
}

static GLuint buildProgram(const char* vertex, const char* fragment) {
    // A brand new program:
    GLuint glprogram { 0 }, glvertex { 0 }, glfragment { 0 };
    
    try {
        glprogram = glCreateProgram();
        if (0 == glprogram) {
            throw std::runtime_error("Failed to create program, is GL context current?");
        }

        glvertex = buildShader(GL_VERTEX_SHADER, vertex);
        if (0 == glvertex) {
            throw std::runtime_error("Failed to create or compile vertex shader");
        }
        glAttachShader(glprogram, glvertex);

        glfragment = buildShader(GL_FRAGMENT_SHADER, fragment);
        if (0 == glfragment) {
            throw std::runtime_error("Failed to create or compile fragment shader");
        }
        glAttachShader(glprogram, glfragment);

        GLint linked { 0 };
        glLinkProgram(glprogram);
        glGetProgramiv(glprogram, GL_LINK_STATUS, &linked);

        if (!linked) {
            std::string linkErrorLog = getProgramInfoLog(glprogram);
            __android_log_print(ANDROID_LOG_WARN, "QQQ_OVR", "Program link error: %s", linkErrorLog.c_str());
            throw std::runtime_error("Failed to link program, is the interface between the fragment and vertex shaders correct?");
        }


    } catch(const std::runtime_error& error) {
        if (0 != glprogram) {
            glDeleteProgram(glprogram);
            glprogram = 0;
        }
    }

    if (0 != glvertex) {
        glDeleteShader(glvertex);
    }

    if (0 != glfragment) {
        glDeleteShader(glfragment);
    }

    if (0 == glprogram) {
        throw std::runtime_error("Failed to build program");
    }

    return glprogram;
}

#endif

using namespace ovr;

static thread_local bool isRenderThread { false };

struct VrSurface : public TaskQueue {
    using HandlerTask = ovr::VrHandler::HandlerTask;

    JavaVM* vm{nullptr};
    jobject oculusActivity{ nullptr };
    ANativeWindow* nativeWindow{ nullptr };

    ovr::VrHandler* handler{nullptr};
    ovrMobile* session{nullptr};
    bool resumed { false };
    ovr::GLContext vrglContext;
    ovr::Framebuffer eyesFbo;

#if USE_BLIT_PRESENT
    GLuint readFbo { 0 };
#else
    GLuint renderProgram { 0 };
    GLuint renderVao { 0 };
#endif
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

#if USE_BLIT_PRESENT
        glGenFramebuffers(1, &readFbo);
#else
        glGenVertexArrays(1, &renderVao);
        const char* vertex = nullptr;
        auto vertexShader = getTextAsset("shaders/present.vert");
        auto fragmentShader = getTextAsset("shaders/present.frag");
        renderProgram =  buildProgram(vertexShader.c_str(), fragmentShader.c_str());
#endif

        glm::uvec2 eyeTargetSize;
        withEnv([&](JNIEnv* env){
            ovrJava java{ vm, env, oculusActivity };
            eyeTargetSize = glm::uvec2 {
                vrapi_GetSystemPropertyInt(&java, VRAPI_SYS_PROP_SUGGESTED_EYE_TEXTURE_WIDTH) * EYE_BUFFER_SCALE,
                vrapi_GetSystemPropertyInt(&java, VRAPI_SYS_PROP_SUGGESTED_EYE_TEXTURE_HEIGHT) * EYE_BUFFER_SCALE,
            };
        });
        __android_log_print(ANDROID_LOG_WARN, "QQQ_OVR", "QQQ Eye Size %d, %d", eyeTargetSize.x, eyeTargetSize.y);
        eyesFbo.create(eyeTargetSize);
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
                    modeParms.Flags |= VRAPI_MODE_FLAG_FRONT_BUFFER_SRGB;
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
                    vrapi_SetExtraLatencyMode(session, VRAPI_EXTRA_LATENCY_MODE_ON);
                    // Generates a warning on the quest: "vrapi_SetDisplayRefreshRate: Dynamic Display Refresh Rate not supported"
                    // vrapi_SetDisplayRefreshRate(session, 72);
                });
            }
        }
    }

    void presentFrame(uint32_t sourceTexture, const glm::uvec2 &sourceSize, const ovrTracking2& tracking) {
        ovrLayerProjection2 layer = vrapi_DefaultLayerProjection2();
        layer.HeadPose = tracking.HeadPose;

        eyesFbo.bind();
        if (sourceTexture) {
            eyesFbo.invalidate();
#if USE_BLIT_PRESENT
            glBindFramebuffer(GL_READ_FRAMEBUFFER, readFbo);
            glFramebufferTexture(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, sourceTexture, 0);
            const auto &destSize = eyesFbo.size();
            ovr::for_each_eye([&](ovrEye eye) {
                auto sourceWidth = sourceSize.x / 2;
                auto sourceX = (eye == VRAPI_EYE_LEFT) ? 0 : sourceWidth;
                // Each eye blit uses a different draw buffer
                eyesFbo.drawBuffers(eye);
                glBlitFramebuffer(
                    sourceX, 0, sourceX + sourceWidth, sourceSize.y,
                    0, 0, destSize.x, destSize.y,
                    GL_COLOR_BUFFER_BIT, GL_NEAREST);
            });
            static const std::array<GLenum, 1> READ_INVALIDATE_ATTACHMENTS {{ GL_COLOR_ATTACHMENT0 }};
            glInvalidateFramebuffer(GL_READ_FRAMEBUFFER, (GLuint)READ_INVALIDATE_ATTACHMENTS.size(), READ_INVALIDATE_ATTACHMENTS.data());
            glFramebufferTexture(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, 0, 0);
#else
            eyesFbo.drawBuffers(VRAPI_EYE_COUNT);
            const auto &destSize = eyesFbo.size();
            glViewport(0, 0, destSize.x, destSize.y);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, sourceTexture);
            glBindVertexArray(renderVao);
            glUseProgram(renderProgram);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            glUseProgram(0);
            glBindVertexArray(0);
#endif
        } else {
            eyesFbo.drawBuffers(VRAPI_EYE_COUNT);
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
        }

        ovr::for_each_eye([&](ovrEye eye) {
            const auto &eyeTracking = tracking.Eye[eye];
            eyesFbo.updateLayer(eye, layer, &eyeTracking.ProjectionMatrix);
        });

        eyesFbo.advance();

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

JNIEXPORT void JNICALL Java_io_highfidelity_oculus_OculusMobileActivity_nativeOnCreate(JNIEnv* env, jobject obj, jobject assetManager) {
    __android_log_write(ANDROID_LOG_WARN, "QQQ_JNI", __FUNCTION__);
    ASSET_MANAGER = AAssetManager_fromJava(env, assetManager);
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
