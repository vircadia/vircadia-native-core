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

#include <GLMHelpers.h>

#include <gpu/Frame.h>
#include <gpu/gl/GLBackend.h>

#include <ViewFrustum.h>
#include <PathUtils.h>
#include <shared/NsightHelpers.h>
#include <controllers/Pose.h>
#include <display-plugins/CompositorHelper.h>
#include <ui-plugins/PluginContainer.h>
#include <gl/OffscreenGLCanvas.h>
#include <gl/OglplusHelpers.h>

#include "OpenVrHelpers.h"

Q_DECLARE_LOGGING_CATEGORY(displayplugins)

const QString OpenVrDisplayPlugin::NAME("OpenVR (Vive)");
const QString StandingHMDSensorMode = "Standing HMD Sensor Mode"; // this probably shouldn't be hardcoded here

PoseData _nextRenderPoseData;
PoseData _nextSimPoseData;

static std::array<vr::Hmd_Eye, 2> VR_EYES { { vr::Eye_Left, vr::Eye_Right } };
bool _openVrDisplayActive { false };
// Flip y-axis since GL UV coords are backwards.
static vr::VRTextureBounds_t OPENVR_TEXTURE_BOUNDS_LEFT{ 0, 0, 0.5f, 1 };
static vr::VRTextureBounds_t OPENVR_TEXTURE_BOUNDS_RIGHT{ 0.5f, 0, 1, 1 };



#define OPENVR_THREADED_SUBMIT 1


#if OPENVR_THREADED_SUBMIT

static QString readFile(const QString& filename) {
    QFile file(filename);
    file.open(QFile::Text | QFile::ReadOnly);
    QString result;
    result.append(QTextStream(&file).readAll());
    return result;
}

class OpenVrSubmitThread : public QThread, public Dependency {
public:
    using Mutex = std::mutex;
    using Condition = std::condition_variable;
    using Lock = std::unique_lock<Mutex>;
    friend class OpenVrDisplayPlugin;
    OffscreenGLCanvas _canvas;
    BasicFramebufferWrapperPtr _framebuffer;
    ProgramPtr _program;
    ShapeWrapperPtr _plane;
    struct ReprojectionUniforms {
        int32_t reprojectionMatrix{ -1 };
        int32_t inverseProjectionMatrix{ -1 };
        int32_t projectionMatrix{ -1 };
    } _reprojectionUniforms;


    OpenVrSubmitThread(OpenVrDisplayPlugin& plugin) : _plugin(plugin) { 
        _canvas.create(plugin._container->getPrimaryContext());
        _canvas.doneCurrent();
        _canvas.moveToThreadWithContext(this);
    }

    void updateReprojectionProgram() {
        static const QString vsFile = PathUtils::resourcesPath() + "/shaders/hmd_reproject.vert";
        static const QString fsFile = PathUtils::resourcesPath() + "/shaders/hmd_reproject.frag";
#if LIVE_SHADER_RELOAD
        static qint64 vsBuiltAge = 0;
        static qint64 fsBuiltAge = 0;
        QFileInfo vsInfo(vsFile);
        QFileInfo fsInfo(fsFile);
        auto vsAge = vsInfo.lastModified().toMSecsSinceEpoch();
        auto fsAge = fsInfo.lastModified().toMSecsSinceEpoch();
        if (!_reprojectionProgram || vsAge > vsBuiltAge || fsAge > fsBuiltAge) {
            vsBuiltAge = vsAge;
            fsBuiltAge = fsAge;
#else
        if (!_program) {
#endif
            QString vsSource = readFile(vsFile);
            QString fsSource = readFile(fsFile);
            ProgramPtr program;
            try {
                compileProgram(program, vsSource.toLocal8Bit().toStdString(), fsSource.toLocal8Bit().toStdString());
                if (program) {
                    using namespace oglplus;
                    _reprojectionUniforms.reprojectionMatrix = Uniform<glm::mat3>(*program, "reprojection").Location();
                    _reprojectionUniforms.inverseProjectionMatrix = Uniform<glm::mat4>(*program, "inverseProjections").Location();
                    _reprojectionUniforms.projectionMatrix = Uniform<glm::mat4>(*program, "projections").Location();
                    _program = program;
                }
            } catch (std::runtime_error& error) {
                qWarning() << "Error building reprojection shader " << error.what();
            }
        }
    }

    void updateSource() {
        Lock lock(_plugin._presentMutex);
        while (!_queue.empty()) {
            auto& front = _queue.front();
            auto result = glClientWaitSync(front.fence, 0, 0);
            if (GL_TIMEOUT_EXPIRED == result && GL_WAIT_FAILED == result) {
                break;
            }

            glDeleteSync(front.fence);
            front.fence = 0;
            _current = front;
            _queue.pop();
        }
    }

    void run() override {
        QThread::currentThread()->setPriority(QThread::Priority::TimeCriticalPriority);
        _canvas.makeCurrent();
        glDisable(GL_DEPTH_TEST);
        glViewport(0, 0, _plugin._renderTargetSize.x, _plugin._renderTargetSize.y);
        _framebuffer = std::make_shared<BasicFramebufferWrapper>();
        _framebuffer->Init(_plugin._renderTargetSize);
        updateReprojectionProgram();
        _plane = loadPlane(_program);
        _canvas.doneCurrent();
        while (!_quit) {
            _canvas.makeCurrent();
            updateSource();
            if (!_current.texture) {
                _canvas.doneCurrent();
                QThread::usleep(1);
                continue;
            }

            {
                auto presentRotation = glm::mat3(_nextRender.poses[0]);
                auto renderRotation = glm::mat3(_current.pose);
                auto correction = glm::inverse(renderRotation) * presentRotation;
                _framebuffer->Bound([&] {
                    glBindTexture(GL_TEXTURE_2D, _current.textureID);
                    _program->Use();
                    using namespace oglplus;
                    Texture::MinFilter(TextureTarget::_2D, TextureMinFilter::Linear);
                    Texture::MagFilter(TextureTarget::_2D, TextureMagFilter::Linear);
                    Uniform<glm::mat3>(*_program, _reprojectionUniforms.reprojectionMatrix).Set(correction);
                    //Uniform<glm::mat4>(*_reprojectionProgram, PROJECTION_MATRIX_LOCATION).Set(_eyeProjections);
                    //Uniform<glm::mat4>(*_reprojectionProgram, INVERSE_PROJECTION_MATRIX_LOCATION).Set(_eyeInverseProjections);
                    // FIXME what's the right oglplus mechanism to do this?  It's not that ^^^ ... better yet, switch to a uniform buffer
                    glUniformMatrix4fv(_reprojectionUniforms.inverseProjectionMatrix, 2, GL_FALSE, &(_plugin._eyeInverseProjections[0][0][0]));
                    glUniformMatrix4fv(_reprojectionUniforms.projectionMatrix, 2, GL_FALSE, &(_plugin._eyeProjections[0][0][0]));
                    _plane->UseInProgram(*_program);
                    _plane->Draw();
                });
                static const vr::VRTextureBounds_t leftBounds{ 0, 0, 0.5f, 1 };
                static const vr::VRTextureBounds_t rightBounds{ 0.5f, 0, 1, 1 };
                
                vr::Texture_t texture{ (void*)oglplus::GetName(_framebuffer->color), vr::API_OpenGL, vr::ColorSpace_Auto };
                vr::VRCompositor()->Submit(vr::Eye_Left, &texture, &leftBounds);
                vr::VRCompositor()->Submit(vr::Eye_Right, &texture, &rightBounds);
                PoseData nextRender, nextSim;
                nextRender.frameIndex = _plugin.presentCount();
                vr::VRCompositor()->WaitGetPoses(nextRender.vrPoses, vr::k_unMaxTrackedDeviceCount, nextSim.vrPoses, vr::k_unMaxTrackedDeviceCount);
                {
                    Lock lock(_plugin._presentMutex);
                    _presentCount++;
                    _presented.notify_one();
                    _nextRender = nextRender;
                    _nextRender.update(_plugin._sensorResetMat);
                    _nextSim = nextSim;
                    _nextSim.update(_plugin._sensorResetMat);
                }
            }
            _canvas.doneCurrent();
        }

        _canvas.makeCurrent();
        _plane.reset();
        _program.reset();
        _framebuffer.reset();
        _canvas.doneCurrent();

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

#endif

bool OpenVrDisplayPlugin::isSupported() const {
    return openVrSupported();
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
    _openVrDisplayActive = true;
    _container->setIsOptionChecked(StandingHMDSensorMode, true);

    if (!_system) {
        _system = acquireOpenVrSystem();
    }
    if (!_system) {
        qWarning() << "Failed to initialize OpenVR";
        return false;
    }

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
    //vr::VRCompositor()->ForceInterleavedReprojectionOn(true);

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

#if OPENVR_THREADED_SUBMIT
    withMainThreadContext([&] {
        _submitThread = std::make_shared<OpenVrSubmitThread>(*this);
    });
    _submitThread->setObjectName("OpenVR Submit Thread");
    _submitThread->start(QThread::TimeCriticalPriority);
#endif

    return Parent::internalActivate();
}

void OpenVrDisplayPlugin::internalDeactivate() {
    Parent::internalDeactivate();

#if OPENVR_THREADED_SUBMIT
    _submitThread->_quit = true;
    _submitThread->wait();
#endif
    
    _openVrDisplayActive = false;
    _container->setIsOptionChecked(StandingHMDSensorMode, false);
    if (_system) {
        // Invalidate poses. It's fine if someone else sets these shared values, but we're about to stop updating them, and
        // we don't want ViveControllerManager to consider old values to be valid.
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

    _compositeInfos[0].texture = _compositeFramebuffer->getRenderBuffer(0);
    for (size_t i = 0; i < COMPOSITING_BUFFER_SIZE; ++i) {
        if (0 != i) {
            _compositeInfos[i].texture = gpu::TexturePointer(gpu::Texture::create2D(gpu::Element::COLOR_RGBA_32, _renderTargetSize.x, _renderTargetSize.y, gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_POINT)));
        }
        _compositeInfos[i].textureID = getGLBackend()->getTextureID(_compositeInfos[i].texture, false);
    }
}

void OpenVrDisplayPlugin::uncustomizeContext() {
    Parent::uncustomizeContext();
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
    PROFILE_RANGE_EX(__FUNCTION__, 0xff7fff00, frameIndex)
    handleOpenVrEvents();
    if (openVrQuitRequested()) {
        QMetaObject::invokeMethod(qApp, "quit");
        return false;
    }
    _currentRenderFrameInfo = FrameInfo();

    withNonPresentThreadLock([&] {
        _currentRenderFrameInfo.renderPose = _nextSimPoseData.poses[vr::k_unTrackedDeviceIndex_Hmd];
    });

    // HACK: when interface is launched and steam vr is NOT running, openvr will return bad HMD poses for a few frames
    // To workaround this, filter out any hmd poses that are obviously bad, i.e. beneath the floor.
    if (isBadPose(&_nextSimPoseData.vrPoses[vr::k_unTrackedDeviceIndex_Hmd].mDeviceToAbsoluteTracking)) {
        qDebug() << "WARNING: ignoring bad hmd pose from openvr";

        // use the last known good HMD pose
        _nextSimPoseData.vrPoses[vr::k_unTrackedDeviceIndex_Hmd].mDeviceToAbsoluteTracking = _lastGoodHMDPose;
    } else {
        _lastGoodHMDPose = _nextSimPoseData.vrPoses[vr::k_unTrackedDeviceIndex_Hmd].mDeviceToAbsoluteTracking;
    }

    vr::TrackedDeviceIndex_t handIndices[2] { vr::k_unTrackedDeviceIndexInvalid, vr::k_unTrackedDeviceIndexInvalid };
    {
        vr::TrackedDeviceIndex_t controllerIndices[2] ;
        auto trackedCount = _system->GetSortedTrackedDeviceIndicesOfClass(vr::TrackedDeviceClass_Controller, controllerIndices, 2);
        // Find the left and right hand controllers, if they exist
        for (uint32_t i = 0; i < std::min<uint32_t>(trackedCount, 2); ++i) {
            if (_nextSimPoseData.vrPoses[i].bPoseIsValid) {
                auto role = _system->GetControllerRoleForTrackedDeviceIndex(controllerIndices[i]);
                if (vr::TrackedControllerRole_LeftHand == role) {
                    handIndices[0] = controllerIndices[i];
                } else if (vr::TrackedControllerRole_RightHand == role) {
                    handIndices[1] = controllerIndices[i];
                }
            }
        }
    }

    _currentRenderFrameInfo.renderPose = _nextSimPoseData.poses[vr::k_unTrackedDeviceIndex_Hmd];

    bool keyboardVisible = isOpenVrKeyboardShown();

    std::array<mat4, 2> handPoses;
    if (!keyboardVisible) {
        for (int i = 0; i < 2; ++i) {
            if (handIndices[i] == vr::k_unTrackedDeviceIndexInvalid) {
                continue;
            }
            auto deviceIndex = handIndices[i];
            const mat4& mat = _nextSimPoseData.poses[deviceIndex];
            const vec3& linearVelocity = _nextSimPoseData.linearVelocities[deviceIndex];
            const vec3& angularVelocity = _nextSimPoseData.angularVelocities[deviceIndex];
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
#if OPENVR_THREADED_SUBMIT
    ++_renderingIndex;
    _renderingIndex %= COMPOSITING_BUFFER_SIZE;

    auto& newComposite = _compositeInfos[_renderingIndex];
    newComposite.pose = _currentPresentFrameInfo.presentPose;
    _compositeFramebuffer->setRenderBuffer(0, newComposite.texture);
#endif

    Parent::compositeLayers();

#if OPENVR_THREADED_SUBMIT
    newComposite.fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    if (!newComposite.textureID) {
        newComposite.textureID = getGLBackend()->getTextureID(newComposite.texture, false);
    }
    withPresentThreadLock([&] {
        _submitThread->update(newComposite);
    });
#endif
}

void OpenVrDisplayPlugin::hmdPresent() {
    PROFILE_RANGE_EX(__FUNCTION__, 0xff00ff00, (uint64_t)_currentFrame->frameIndex)

#if OPENVR_THREADED_SUBMIT
    _submitThread->waitForPresent();

#else
    vr::Texture_t vrTexture{ (void*)glTexId, vr::API_OpenGL, vr::ColorSpace_Auto };
    vr::VRCompositor()->Submit(vr::Eye_Left, &vrTexture, &OPENVR_TEXTURE_BOUNDS_LEFT);
    vr::VRCompositor()->Submit(vr::Eye_Right, &vrTexture, &OPENVR_TEXTURE_BOUNDS_RIGHT);
    vr::VRCompositor()->PostPresentHandoff();
#endif
}

void OpenVrDisplayPlugin::postPreview() {
    PROFILE_RANGE_EX(__FUNCTION__, 0xff00ff00, (uint64_t)_currentFrame->frameIndex)
    PoseData nextRender, nextSim;
    nextRender.frameIndex = presentCount();
#if !OPENVR_THREADED_SUBMIT
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
    _hmdActivityLevel = vr::k_EDeviceActivityLevel_UserInteraction; // _system->GetTrackedDeviceActivityLevel(vr::k_unTrackedDeviceIndex_Hmd);
#endif
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
