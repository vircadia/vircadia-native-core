//
//  Created by Bradley Austin Davis on 2015/05/12
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "OpenVrDisplayPlugin.h"

#include <QtCore/QThread>
#include <QtCore/QLoggingCategory>
#include <QtCore/QFileInfo>
#include <QtCore/QDateTime>

#include <GLMHelpers.h>

#include <gl/Context.h>
#include <gl/GLShaders.h>

#include <gpu/Frame.h>
#include <gpu/gl/GLBackend.h>


#include <ViewFrustum.h>
#include <PathUtils.h>
#include <shared/NsightHelpers.h>
#include <controllers/Pose.h>
#include <display-plugins/CompositorHelper.h>
#include <ui-plugins/PluginContainer.h>
#include <gl/OffscreenGLCanvas.h>

#include "OpenVrHelpers.h"

Q_DECLARE_LOGGING_CATEGORY(displayplugins)

const char* OpenVrDisplayPlugin::NAME { "OpenVR (Vive)" };
const char* StandingHMDSensorMode { "Standing HMD Sensor Mode" }; // this probably shouldn't be hardcoded here
const char* OpenVrThreadedSubmit { "OpenVR Threaded Submit" }; // this probably shouldn't be hardcoded here

PoseData _nextRenderPoseData;
PoseData _nextSimPoseData;

#define MIN_CORES_FOR_NORMAL_RENDER 5
bool forceInterleavedReprojection = (QThread::idealThreadCount() < MIN_CORES_FOR_NORMAL_RENDER);

static std::array<vr::Hmd_Eye, 2> VR_EYES { { vr::Eye_Left, vr::Eye_Right } };
bool _openVrDisplayActive { false };
// Flip y-axis since GL UV coords are backwards.
static vr::VRTextureBounds_t OPENVR_TEXTURE_BOUNDS_LEFT{ 0, 0, 0.5f, 1 };
static vr::VRTextureBounds_t OPENVR_TEXTURE_BOUNDS_RIGHT{ 0.5f, 0, 1, 1 };

#define REPROJECTION_BINDING 1

static const char* HMD_REPROJECTION_VERT = R"SHADER(
#version 450 core

out vec3 vPosition;
out vec2 vTexCoord;

void main(void) {
    const float depth = 0.0;
    const vec4 UNIT_QUAD[4] = vec4[4](
        vec4(-1.0, -1.0, depth, 1.0),
        vec4(1.0, -1.0, depth, 1.0),
        vec4(-1.0, 1.0, depth, 1.0),
        vec4(1.0, 1.0, depth, 1.0)
    );
    vec4 pos = UNIT_QUAD[gl_VertexID];

    gl_Position = pos;
    vPosition = pos.xyz;
    vTexCoord = (pos.xy + 1.0) * 0.5;
}
)SHADER";

static const char* HMD_REPROJECTION_FRAG = R"SHADER(
#version 450 core

uniform sampler2D sampler;
layout(binding = 1, std140) uniform Reprojection
{
    mat4 projections[2];
    mat4 inverseProjections[2];
    mat4 reprojection;
};

in vec3 vPosition;
in vec2 vTexCoord;

out vec4 FragColor;

void main() {
    vec2 uv = vTexCoord;
    
    mat4 eyeInverseProjection;
    mat4 eyeProjection;
    
    float xoffset = 1.0;
    vec2 uvmin = vec2(0.0);
    vec2 uvmax = vec2(1.0);
    // determine the correct projection and inverse projection to use.
    if (vTexCoord.x < 0.5) {
        uvmax.x = 0.5;
        eyeInverseProjection = inverseProjections[0];
        eyeProjection = projections[0];
    } else {
        xoffset = -1.0;
        uvmin.x = 0.5;
        uvmax.x = 1.0;
        eyeInverseProjection = inverseProjections[1];
        eyeProjection = projections[1];
    }

    // Account for stereo in calculating the per-eye NDC coordinates
    vec4 ndcSpace = vec4(vPosition, 1.0);
    ndcSpace.x *= 2.0;
    ndcSpace.x += xoffset;
    
    // Convert from NDC to eyespace
    vec4 eyeSpace = eyeInverseProjection * ndcSpace;
    eyeSpace /= eyeSpace.w;

    // Convert to a noramlized ray 
    vec3 ray = eyeSpace.xyz;
    ray = normalize(ray);

    // Adjust the ray by the rotation
    ray = mat3(reprojection) * ray;

    // Project back on to the texture plane
    ray *= eyeSpace.z / ray.z;

    // Update the eyespace vector
    eyeSpace.xyz = ray;

    // Reproject back into NDC
    ndcSpace = eyeProjection * eyeSpace;
    ndcSpace /= ndcSpace.w;
    ndcSpace.x -= xoffset;
    ndcSpace.x /= 2.0;
    
    // Calculate the new UV coordinates
    uv = (ndcSpace.xy / 2.0) + 0.5;
    if (any(greaterThan(uv, uvmax)) || any(lessThan(uv, uvmin))) {
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
    } else {
        FragColor = texture(sampler, uv);
    }
}
)SHADER";

struct Reprojection {
    mat4 projections[2];
    mat4 inverseProjections[2];
    mat4 reprojection;
};

class OpenVrSubmitThread : public QThread, public Dependency {
public:
    using Mutex = std::mutex;
    using Condition = std::condition_variable;
    using Lock = std::unique_lock<Mutex>;
    friend class OpenVrDisplayPlugin;
    std::shared_ptr<gl::OffscreenContext> _canvas;

    OpenVrSubmitThread(OpenVrDisplayPlugin& plugin) : _plugin(plugin) { 
        setObjectName("OpenVR Submit Thread");
    }

    void updateSource() {
        _plugin.withNonPresentThreadLock([&] {
            while (!_queue.empty()) {
                auto& front = _queue.front();

                auto result = glClientWaitSync(front.fence, 0, 0);

                if (GL_TIMEOUT_EXPIRED == result || GL_WAIT_FAILED == result) {
                    break;
                } else if (GL_CONDITION_SATISFIED == result || GL_ALREADY_SIGNALED == result) {
                    glDeleteSync(front.fence);
                } else {
                    assert(false);
                }

                front.fence = 0;
                _current = front;
                _queue.pop();
            }
        });
    }

    GLuint _program { 0 };

    void updateProgram() {
        if (!_program) {
            std::string vsSource = HMD_REPROJECTION_VERT;
            std::string fsSource = HMD_REPROJECTION_FRAG;
            GLuint vertexShader { 0 }, fragmentShader { 0 };
            ::gl::compileShader(GL_VERTEX_SHADER, vsSource, "", vertexShader);
            ::gl::compileShader(GL_FRAGMENT_SHADER, fsSource, "", fragmentShader);
            _program = ::gl::compileProgram({ { vertexShader, fragmentShader } });
            glDeleteShader(vertexShader);
            glDeleteShader(fragmentShader);
            qDebug() << "Rebuild proigram";
        }
    }

#define COLOR_BUFFER_COUNT 4

    void run() override {

        GLuint _framebuffer { 0 };
        std::array<GLuint, COLOR_BUFFER_COUNT> _colors;
        size_t currentColorBuffer { 0 };
        size_t globalColorBufferCount { 0 };
        GLuint _uniformBuffer { 0 };
        GLuint _vao { 0 };
        GLuint _depth { 0 };
        Reprojection _reprojection;

        QThread::currentThread()->setPriority(QThread::Priority::TimeCriticalPriority);
        _canvas->makeCurrent();

        glCreateBuffers(1, &_uniformBuffer);
        glNamedBufferStorage(_uniformBuffer, sizeof(Reprojection), 0, GL_DYNAMIC_STORAGE_BIT);
        glCreateVertexArrays(1, &_vao);
        glBindVertexArray(_vao);


        glCreateFramebuffers(1, &_framebuffer);
        {
            glCreateRenderbuffers(1, &_depth);
            glNamedRenderbufferStorage(_depth, GL_DEPTH24_STENCIL8, _plugin._renderTargetSize.x, _plugin._renderTargetSize.y);
            glNamedFramebufferRenderbuffer(_framebuffer, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, _depth);
            glCreateTextures(GL_TEXTURE_2D, COLOR_BUFFER_COUNT, &_colors[0]);
            for (size_t i = 0; i < COLOR_BUFFER_COUNT; ++i) {
                glTextureStorage2D(_colors[i], 1, GL_RGBA8, _plugin._renderTargetSize.x, _plugin._renderTargetSize.y);
            }
        }

        glDisable(GL_DEPTH_TEST);
        glViewport(0, 0, _plugin._renderTargetSize.x, _plugin._renderTargetSize.y);
        _canvas->doneCurrent();
        while (!_quit) {
            _canvas->makeCurrent();
            updateSource();
            if (!_current.texture) {
                _canvas->doneCurrent();
                QThread::usleep(1);
                continue;
            }


            updateProgram();
            {
                auto presentRotation = glm::mat3(_nextRender.poses[0]);
                auto renderRotation = glm::mat3(_current.pose);
                for (size_t i = 0; i < 2; ++i) {
                    _reprojection.projections[i] = _plugin._eyeProjections[i];
                    _reprojection.inverseProjections[i] = _plugin._eyeInverseProjections[i];
                }
                _reprojection.reprojection = glm::inverse(renderRotation) * presentRotation;
                glNamedBufferSubData(_uniformBuffer, 0, sizeof(Reprojection), &_reprojection);
                glNamedFramebufferTexture(_framebuffer, GL_COLOR_ATTACHMENT0, _colors[currentColorBuffer], 0);
                glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _framebuffer);
                {
                    glClearColor(1, 1, 0, 1);
                    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                    glTextureParameteri(_current.textureID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                    glTextureParameteri(_current.textureID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                    glUseProgram(_program);
                    glBindBufferBase(GL_UNIFORM_BUFFER, REPROJECTION_BINDING, _uniformBuffer);
                    glBindTexture(GL_TEXTURE_2D, _current.textureID);
                    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
                }

                glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
                static const vr::VRTextureBounds_t leftBounds{ 0, 0, 0.5f, 1 };
                static const vr::VRTextureBounds_t rightBounds{ 0.5f, 0, 1, 1 };
                
                vr::Texture_t texture{ (void*)_colors[currentColorBuffer], vr::API_OpenGL, vr::ColorSpace_Auto };
                vr::VRCompositor()->Submit(vr::Eye_Left, &texture, &leftBounds);
                vr::VRCompositor()->Submit(vr::Eye_Right, &texture, &rightBounds);
                _plugin._presentRate.increment();
                PoseData nextRender, nextSim;
                nextRender.frameIndex = _plugin.presentCount();
                vr::VRCompositor()->WaitGetPoses(nextRender.vrPoses, vr::k_unMaxTrackedDeviceCount, nextSim.vrPoses, vr::k_unMaxTrackedDeviceCount);

                // Copy invalid poses in nextSim from nextRender
                for (uint32_t i = 0; i < vr::k_unMaxTrackedDeviceCount; ++i) {
                    if (!nextSim.vrPoses[i].bPoseIsValid) {
                        nextSim.vrPoses[i] = nextRender.vrPoses[i];
                    }
                }

                mat4 sensorResetMat;
                _plugin.withNonPresentThreadLock([&] {
                    sensorResetMat = _plugin._sensorResetMat;
                });

                nextRender.update(sensorResetMat);
                nextSim.update(sensorResetMat);
                _plugin.withNonPresentThreadLock([&] {
                    _nextRender = nextRender;
                    _nextSim = nextSim;
                    ++_presentCount;
                    _presented.notify_one();
                });

                ++globalColorBufferCount;
                currentColorBuffer = globalColorBufferCount % COLOR_BUFFER_COUNT;
            }
            _canvas->doneCurrent();
        }

        _canvas->makeCurrent();
        glDeleteBuffers(1, &_uniformBuffer);
        glDeleteFramebuffers(1, &_framebuffer);
        CHECK_GL_ERROR();
        glDeleteTextures(4, &_colors[0]);
        glDeleteProgram(_program);
        glBindVertexArray(0);
        glDeleteVertexArrays(1, &_vao);
        _canvas->doneCurrent();
    }

    void update(const CompositeInfo& newCompositeInfo) {
        _queue.push(newCompositeInfo);
    }

    void waitForPresent() {
        auto lastCount = _presentCount.load();
        Lock lock(_plugin._presentMutex);
        _presented.wait(lock, [&]()->bool {
            return _presentCount.load() > lastCount;
        });
        _nextSimPoseData = _nextSim;
        _nextRenderPoseData = _nextRender;
    }

    CompositeInfo _current;
    CompositeInfo::Queue _queue;

    PoseData _nextRender, _nextSim;
    bool _quit { false };
    GLuint _currentTexture { 0 };
    std::atomic<uint32_t> _presentCount { 0 };
    Condition _presented;
    OpenVrDisplayPlugin& _plugin;
};

bool OpenVrDisplayPlugin::isSupported() const {
    return openVrSupported();
}

float OpenVrDisplayPlugin::getTargetFrameRate() const {
    if (forceInterleavedReprojection && !_asyncReprojectionActive) {
        return TARGET_RATE_OpenVr / 2.0f;
    }
    return TARGET_RATE_OpenVr;
}

void OpenVrDisplayPlugin::init() {
    Plugin::init();

    _lastGoodHMDPose.m[0][0] = 1.0f;
    _lastGoodHMDPose.m[0][1] = 0.0f;
    _lastGoodHMDPose.m[0][2] = 0.0f;
    _lastGoodHMDPose.m[0][3] = 0.0f;
    _lastGoodHMDPose.m[1][0] = 0.0f;
    _lastGoodHMDPose.m[1][1] = 1.0f;
    _lastGoodHMDPose.m[1][2] = 0.0f;
    _lastGoodHMDPose.m[1][3] = 1.8f;
    _lastGoodHMDPose.m[2][0] = 0.0f;
    _lastGoodHMDPose.m[2][1] = 0.0f;
    _lastGoodHMDPose.m[2][2] = 1.0f;
    _lastGoodHMDPose.m[2][3] = 0.0f;

    emit deviceConnected(getName());
}

bool OpenVrDisplayPlugin::internalActivate() {
    if (!_system) {
        _system = acquireOpenVrSystem();
    }
    if (!_system) {
        qWarning() << "Failed to initialize OpenVR";
        return false;
    }

    // If OpenVR isn't running, then the compositor won't be accessible
    // FIXME find a way to launch the compositor?
    if (!vr::VRCompositor()) {
        qWarning() << "Failed to acquire OpenVR compositor";
        releaseOpenVrSystem();
        _system = nullptr;
        return false;
    }

    vr::Compositor_FrameTiming timing;
    memset(&timing, 0, sizeof(timing));
    timing.m_nSize = sizeof(vr::Compositor_FrameTiming);
    vr::VRCompositor()->GetFrameTiming(&timing);
    auto usingOpenVRForOculus = oculusViaOpenVR();
    _asyncReprojectionActive = (timing.m_nReprojectionFlags & VRCompositor_ReprojectionAsync) || usingOpenVRForOculus;

    _threadedSubmit = !_asyncReprojectionActive;
    if (usingOpenVRForOculus) {
        qDebug() << "Oculus active via OpenVR:  " << usingOpenVRForOculus;
    }
    qDebug() << "OpenVR Async Reprojection active:  " << _asyncReprojectionActive;
    qDebug() << "OpenVR Threaded submit enabled:  " << _threadedSubmit;

    _openVrDisplayActive = true;
    _container->setIsOptionChecked(StandingHMDSensorMode, true);

    _system->GetRecommendedRenderTargetSize(&_renderTargetSize.x, &_renderTargetSize.y);
    // Recommended render target size is per-eye, so double the X size for 
    // left + right eyes
    _renderTargetSize.x *= 2;

    withNonPresentThreadLock([&] {
        openvr_for_each_eye([&](vr::Hmd_Eye eye) {
            _eyeOffsets[eye] = toGlm(_system->GetEyeToHeadTransform(eye));
            _eyeProjections[eye] = toGlm(_system->GetProjectionMatrix(eye, DEFAULT_NEAR_CLIP, DEFAULT_FAR_CLIP, vr::API_OpenGL));
        });
        // FIXME Calculate the proper combined projection by using GetProjectionRaw values from both eyes
        _cullingProjection = _eyeProjections[0];
    });

    // enable async time warp
    if (forceInterleavedReprojection) {
        vr::VRCompositor()->ForceInterleavedReprojectionOn(true);
    }
    

    // set up default sensor space such that the UI overlay will align with the front of the room.
    auto chaperone = vr::VRChaperone();
    if (chaperone) {
        float const UI_RADIUS = 1.0f;
        float const UI_HEIGHT = 1.6f;
        float const UI_Z_OFFSET = 0.5;

        float xSize, zSize;
        chaperone->GetPlayAreaSize(&xSize, &zSize);
        glm::vec3 uiPos(0.0f, UI_HEIGHT, UI_RADIUS - (0.5f * zSize) - UI_Z_OFFSET);
        _sensorResetMat = glm::inverse(createMatFromQuatAndPos(glm::quat(), uiPos));
    } else {
        #if DEV_BUILD
            qDebug() << "OpenVR: error could not get chaperone pointer";
        #endif
    }

    if (_threadedSubmit) {
        _submitThread = std::make_shared<OpenVrSubmitThread>(*this);
        if (!_submitCanvas) {
            withMainThreadContext([&] {
                _submitCanvas = std::make_shared<gl::OffscreenContext>();
                _submitCanvas->create();
                _submitCanvas->doneCurrent();
            });
        }
    }

    return Parent::internalActivate();
}

void OpenVrDisplayPlugin::internalDeactivate() {
    Parent::internalDeactivate();

    _openVrDisplayActive = false;
    _container->setIsOptionChecked(StandingHMDSensorMode, false);
    if (_system) {
        // TODO: Invalidate poses. It's fine if someone else sets these shared values, but we're about to stop updating them, and
        // we don't want ViveControllerManager to consider old values to be valid.
        _container->makeRenderingContextCurrent();
        releaseOpenVrSystem();
        _system = nullptr;
    }
}

void OpenVrDisplayPlugin::customizeContext() {
    // Display plugins in DLLs must initialize glew locally
    static std::once_flag once;
    std::call_once(once, [] {
        glewExperimental = true;
        GLenum err = glewInit();
        glGetError(); // clear the potential error from glewExperimental
    });

    Parent::customizeContext();

    if (_threadedSubmit) {
        _compositeInfos[0].texture = _compositeFramebuffer->getRenderBuffer(0);
        for (size_t i = 0; i < COMPOSITING_BUFFER_SIZE; ++i) {
            if (0 != i) {
                _compositeInfos[i].texture = gpu::TexturePointer(gpu::Texture::create2D(gpu::Element::COLOR_RGBA_32, _renderTargetSize.x, _renderTargetSize.y, gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_POINT)));
            }
            _compositeInfos[i].textureID = getGLBackend()->getTextureID(_compositeInfos[i].texture, false);
        }
        _submitThread->_canvas = _submitCanvas;
        _submitThread->start(QThread::HighPriority);
    }
}

void OpenVrDisplayPlugin::uncustomizeContext() {
    Parent::uncustomizeContext();

    if (_threadedSubmit) {
        _submitThread->_quit = true;
        _submitThread->wait();
        _submitThread.reset();
    }
}

void OpenVrDisplayPlugin::resetSensors() {
    glm::mat4 m;
    withNonPresentThreadLock([&] {
        m = toGlm(_nextSimPoseData.vrPoses[0].mDeviceToAbsoluteTracking);
    });
    _sensorResetMat = glm::inverse(cancelOutRollAndPitch(m));
}

static bool isBadPose(vr::HmdMatrix34_t* mat) {
    if (mat->m[1][3] < -0.2f) {
        return true;
    }
    return false;
}

bool OpenVrDisplayPlugin::beginFrameRender(uint32_t frameIndex) {
    PROFILE_RANGE_EX(render, __FUNCTION__, 0xff7fff00, frameIndex)
    handleOpenVrEvents();
    if (openVrQuitRequested()) {
        QMetaObject::invokeMethod(qApp, "quit");
        return false;
    }
    _currentRenderFrameInfo = FrameInfo();

    PoseData nextSimPoseData;
    withNonPresentThreadLock([&] {
        nextSimPoseData = _nextSimPoseData;
    });

    // HACK: when interface is launched and steam vr is NOT running, openvr will return bad HMD poses for a few frames
    // To workaround this, filter out any hmd poses that are obviously bad, i.e. beneath the floor.
    if (isBadPose(&nextSimPoseData.vrPoses[vr::k_unTrackedDeviceIndex_Hmd].mDeviceToAbsoluteTracking)) {
        qDebug() << "WARNING: ignoring bad hmd pose from openvr";

        // use the last known good HMD pose
        nextSimPoseData.vrPoses[vr::k_unTrackedDeviceIndex_Hmd].mDeviceToAbsoluteTracking = _lastGoodHMDPose;
    } else {
        _lastGoodHMDPose = nextSimPoseData.vrPoses[vr::k_unTrackedDeviceIndex_Hmd].mDeviceToAbsoluteTracking;
    }

    vr::TrackedDeviceIndex_t handIndices[2] { vr::k_unTrackedDeviceIndexInvalid, vr::k_unTrackedDeviceIndexInvalid };
    {
        vr::TrackedDeviceIndex_t controllerIndices[2] ;
        auto trackedCount = _system->GetSortedTrackedDeviceIndicesOfClass(vr::TrackedDeviceClass_Controller, controllerIndices, 2);
        // Find the left and right hand controllers, if they exist
        for (uint32_t i = 0; i < std::min<uint32_t>(trackedCount, 2); ++i) {
            if (nextSimPoseData.vrPoses[i].bPoseIsValid) {
                auto role = _system->GetControllerRoleForTrackedDeviceIndex(controllerIndices[i]);
                if (vr::TrackedControllerRole_LeftHand == role) {
                    handIndices[0] = controllerIndices[i];
                } else if (vr::TrackedControllerRole_RightHand == role) {
                    handIndices[1] = controllerIndices[i];
                }
            }
        }
    }

    _currentRenderFrameInfo.renderPose = nextSimPoseData.poses[vr::k_unTrackedDeviceIndex_Hmd];
    bool keyboardVisible = isOpenVrKeyboardShown();

    std::array<mat4, 2> handPoses;
    if (!keyboardVisible) {
        for (int i = 0; i < 2; ++i) {
            if (handIndices[i] == vr::k_unTrackedDeviceIndexInvalid) {
                continue;
            }
            auto deviceIndex = handIndices[i];
            const mat4& mat = nextSimPoseData.poses[deviceIndex];
            const vec3& linearVelocity = nextSimPoseData.linearVelocities[deviceIndex];
            const vec3& angularVelocity = nextSimPoseData.angularVelocities[deviceIndex];
            auto correctedPose = openVrControllerPoseToHandPose(i == 0, mat, linearVelocity, angularVelocity);
            static const glm::quat HAND_TO_LASER_ROTATION = glm::rotation(Vectors::UNIT_Z, Vectors::UNIT_NEG_Y);
            handPoses[i] = glm::translate(glm::mat4(), correctedPose.translation) * glm::mat4_cast(correctedPose.rotation * HAND_TO_LASER_ROTATION);
        }
    }

    withNonPresentThreadLock([&] {
        _uiModelTransform = DependencyManager::get<CompositorHelper>()->getModelTransform();
        // Make controller poses available to the presentation thread
        _handPoses = handPoses;
        _frameInfos[frameIndex] = _currentRenderFrameInfo;
    });
    return Parent::beginFrameRender(frameIndex);
}

void OpenVrDisplayPlugin::compositeLayers() {
    if (_threadedSubmit) {
        ++_renderingIndex;
        _renderingIndex %= COMPOSITING_BUFFER_SIZE;

        auto& newComposite = _compositeInfos[_renderingIndex];
        newComposite.pose = _currentPresentFrameInfo.presentPose;
        _compositeFramebuffer->setRenderBuffer(0, newComposite.texture);
    }

    Parent::compositeLayers();

    if (_threadedSubmit) {
        auto& newComposite = _compositeInfos[_renderingIndex];
        newComposite.fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
        // https://www.opengl.org/registry/specs/ARB/sync.txt:
        // > The simple flushing behavior defined by
        // > SYNC_FLUSH_COMMANDS_BIT will not help when waiting for a fence
        // > command issued in another context's command stream to complete.
        // > Applications which block on a fence sync object must take
        // > additional steps to assure that the context from which the
        // > corresponding fence command was issued has flushed that command
        // > to the graphics pipeline.
        glFlush();

        if (!newComposite.textureID) {
            newComposite.textureID = getGLBackend()->getTextureID(newComposite.texture, false);
        }
        withPresentThreadLock([&] {
            _submitThread->update(newComposite);
        });
    }
}

void OpenVrDisplayPlugin::hmdPresent() {
    PROFILE_RANGE_EX(render, __FUNCTION__, 0xff00ff00, (uint64_t)_currentFrame->frameIndex)

    if (_threadedSubmit) {
        _submitThread->waitForPresent();
    } else {
        GLuint glTexId = getGLBackend()->getTextureID(_compositeFramebuffer->getRenderBuffer(0), false);
        vr::Texture_t vrTexture { (void*)glTexId, vr::API_OpenGL, vr::ColorSpace_Auto };
        vr::VRCompositor()->Submit(vr::Eye_Left, &vrTexture, &OPENVR_TEXTURE_BOUNDS_LEFT);
        vr::VRCompositor()->Submit(vr::Eye_Right, &vrTexture, &OPENVR_TEXTURE_BOUNDS_RIGHT);
        vr::VRCompositor()->PostPresentHandoff();
        _presentRate.increment();
    }

    vr::Compositor_FrameTiming frameTiming;
    memset(&frameTiming, 0, sizeof(vr::Compositor_FrameTiming));
    frameTiming.m_nSize = sizeof(vr::Compositor_FrameTiming);
    vr::VRCompositor()->GetFrameTiming(&frameTiming);
    _stutterRate.increment(frameTiming.m_nNumDroppedFrames);
}

void OpenVrDisplayPlugin::postPreview() {
    PROFILE_RANGE_EX(render, __FUNCTION__, 0xff00ff00, (uint64_t)_currentFrame->frameIndex)
    PoseData nextRender, nextSim;
    nextRender.frameIndex = presentCount();

    _hmdActivityLevel = _system->GetTrackedDeviceActivityLevel(vr::k_unTrackedDeviceIndex_Hmd);

    if (!_threadedSubmit) {
        vr::VRCompositor()->WaitGetPoses(nextRender.vrPoses, vr::k_unMaxTrackedDeviceCount, nextSim.vrPoses, vr::k_unMaxTrackedDeviceCount);

        glm::mat4 resetMat;
        withPresentThreadLock([&] {
            resetMat = _sensorResetMat;
        });
        nextRender.update(resetMat);
        nextSim.update(resetMat);
        withPresentThreadLock([&] {
            _nextSimPoseData = nextSim;
        });
        _nextRenderPoseData = nextRender;

    }
}

bool OpenVrDisplayPlugin::isHmdMounted() const {
    return _hmdActivityLevel == vr::k_EDeviceActivityLevel_UserInteraction;
}

void OpenVrDisplayPlugin::updatePresentPose() {
    _currentPresentFrameInfo.presentPose = _nextRenderPoseData.poses[vr::k_unTrackedDeviceIndex_Hmd];
}

bool OpenVrDisplayPlugin::suppressKeyboard() { 
    if (isOpenVrKeyboardShown()) {
        return false;
    }
    if (!_keyboardSupressionCount.fetch_add(1)) {
        disableOpenVrKeyboard();
    }
    return true;
}

void OpenVrDisplayPlugin::unsuppressKeyboard() {
    if (_keyboardSupressionCount == 0) {
        qWarning() << "Attempted to unsuppress a keyboard that was not suppressed";
        return;
    }
    if (1 == _keyboardSupressionCount.fetch_sub(1)) {
        enableOpenVrKeyboard(_container);
    }
}

bool OpenVrDisplayPlugin::isKeyboardVisible() {
    return isOpenVrKeyboardShown(); 
}

int OpenVrDisplayPlugin::getRequiredThreadCount() const { 
    return Parent::getRequiredThreadCount() + (_threadedSubmit ? 1 : 0);
}
