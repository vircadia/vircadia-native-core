//
//  GLBackend.h
//  libraries/gpu/src/gpu
//
//  Created by Sam Gateau on 10/27/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_gpu_gl_GLBackend_h
#define hifi_gpu_gl_GLBackend_h

#include <assert.h>
#include <functional>
#include <memory>
#include <bitset>
#include <queue>
#include <utility>
#include <list>
#include <array>

#include <QtCore/QLoggingCategory>

#include <gl/Config.h>
#include <gl/GLShaders.h>

#include <gpu/Forward.h>
#include <gpu/Context.h>

#include "GLShared.h"

// Different versions for the stereo drawcall
// Current preferred is  "instanced" which draw the shape twice but instanced and rely on clipping plane to draw left/right side only
#if defined(USE_GLES)
#define GPU_STEREO_TECHNIQUE_DOUBLED_SIMPLE
#else
//#define GPU_STEREO_TECHNIQUE_DOUBLED_SMARTER
#define GPU_STEREO_TECHNIQUE_INSTANCED
#endif

// Let these be configured by the one define picked above
#ifdef GPU_STEREO_TECHNIQUE_DOUBLED_SIMPLE
#define GPU_STEREO_DRAWCALL_DOUBLED
#endif

#ifdef GPU_STEREO_TECHNIQUE_DOUBLED_SMARTER
#define GPU_STEREO_DRAWCALL_DOUBLED
#define GPU_STEREO_CAMERA_BUFFER
#endif

#ifdef GPU_STEREO_TECHNIQUE_INSTANCED
#define GPU_STEREO_DRAWCALL_INSTANCED
#define GPU_STEREO_CAMERA_BUFFER
#endif

//
// GL Backend pointer storage mechanism
// One of the following three defines must be defined.
// GPU_POINTER_STORAGE_SHARED

// The platonic ideal, use references to smart pointers.
// However, this produces artifacts because there are too many places in the code right now that
// create temporary values (undesirable smart pointer duplications) and then those temp variables
// get passed on and have their reference taken, and then invalidated
// GPU_POINTER_STORAGE_REF

// Raw pointer manipulation.  Seems more dangerous than the reference wrappers,
// but in practice, the danger of grabbing a reference to a temporary variable
// is causing issues
// GPU_POINTER_STORAGE_RAW

#if defined(USE_GLES)
#define GPU_POINTER_STORAGE_SHARED
#else
#define GPU_POINTER_STORAGE_RAW
#endif

namespace gpu { namespace gl {

#if defined(GPU_POINTER_STORAGE_SHARED)
template <typename T>
static inline bool compare(const std::shared_ptr<T>& a, const std::shared_ptr<T>& b) {
    return a == b;
}

template <typename T>
static inline T* acquire(const std::shared_ptr<T>& pointer) {
    return pointer.get();
}

template <typename T>
static inline void reset(std::shared_ptr<T>& pointer) {
    return pointer.reset();
}

template <typename T>
static inline bool valid(const std::shared_ptr<T>& pointer) {
    return pointer.operator bool();
}

template <typename T>
static inline void assign(std::shared_ptr<T>& pointer, const std::shared_ptr<T>& source) {
    pointer = source;
}

using BufferReference = BufferPointer;
using TextureReference = TexturePointer;
using FramebufferReference = FramebufferPointer;
using FormatReference = Stream::FormatPointer;
using PipelineReference = PipelinePointer;

#define GPU_REFERENCE_INIT_VALUE nullptr

#elif defined(GPU_POINTER_STORAGE_REF)

template <typename T>
class PointerReferenceWrapper : public std::reference_wrapper<const std::shared_ptr<T>> {
    using Parent = std::reference_wrapper<const std::shared_ptr<T>>;

public:
    using Pointer = std::shared_ptr<T>;
    PointerReferenceWrapper() : Parent(EMPTY()) {}
    PointerReferenceWrapper(const Pointer& pointer) : Parent(pointer) {}
    void clear() { *this = EMPTY(); }

private:
    static const Pointer& EMPTY() {
        static const Pointer EMPTY_VALUE;
        return EMPTY_VALUE;
    };
};

template <typename T>
static bool compare(const PointerReferenceWrapper<T>& reference, const std::shared_ptr<T>& pointer) {
    return reference.get() == pointer;
}

template <typename T>
static inline T* acquire(const PointerReferenceWrapper<T>& reference) {
    return reference.get().get();
}

template <typename T>
static void assign(PointerReferenceWrapper<T>& reference, const std::shared_ptr<T>& pointer) {
    reference = pointer;
}

template <typename T>
static bool valid(const PointerReferenceWrapper<T>& reference) {
    return reference.get().operator bool();
}

template <typename T>
static inline void reset(PointerReferenceWrapper<T>& reference) {
    return reference.clear();
}

using BufferReference = PointerReferenceWrapper<Buffer>;
using TextureReference = PointerReferenceWrapper<Texture>;
using FramebufferReference = PointerReferenceWrapper<Framebuffer>;
using FormatReference = PointerReferenceWrapper<Stream::Format>;
using PipelineReference = PointerReferenceWrapper<Pipeline>;

#define GPU_REFERENCE_INIT_VALUE

#elif defined(GPU_POINTER_STORAGE_RAW)

template <typename T>
static bool compare(const T* const& rawPointer, const std::shared_ptr<T>& pointer) {
    return rawPointer == pointer.get();
}

template <typename T>
static inline T* acquire(T*& rawPointer) {
    return rawPointer;
}

template <typename T>
static inline bool valid(const T* const& rawPointer) {
    return rawPointer;
}

template <typename T>
static inline void reset(T*& rawPointer) {
    rawPointer = nullptr;
}

template <typename T>
static inline void assign(T*& rawPointer, const std::shared_ptr<T>& pointer) {
    rawPointer = pointer.get();
}

using BufferReference = Buffer*;
using TextureReference = Texture*;
using FramebufferReference = Framebuffer*;
using FormatReference = Stream::Format*;
using PipelineReference = Pipeline*;

#define GPU_REFERENCE_INIT_VALUE nullptr

#endif

class GLBackend : public Backend, public std::enable_shared_from_this<GLBackend> {
    // Context Backend static interface required
    friend class gpu::Context;
    static void init();
    static BackendPointer createBackend();

protected:
    explicit GLBackend(bool syncCache);
    GLBackend();

public:
#if defined(USE_GLES)
    // https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glGet.xhtml
    static const GLint MIN_REQUIRED_TEXTURE_IMAGE_UNITS = 16;
    static const GLint MIN_REQUIRED_COMBINED_UNIFORM_BLOCKS = 60;
    static const GLint MIN_REQUIRED_COMBINED_TEXTURE_IMAGE_UNITS = 48;
    static const GLint MIN_REQUIRED_UNIFORM_BUFFER_BINDINGS = 72;
    static const GLint MIN_REQUIRED_UNIFORM_LOCATIONS = 1024;
#else
    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glGet.xhtml
    static const GLint MIN_REQUIRED_TEXTURE_IMAGE_UNITS = 16;
    static const GLint MIN_REQUIRED_COMBINED_UNIFORM_BLOCKS = 70;
    static const GLint MIN_REQUIRED_COMBINED_TEXTURE_IMAGE_UNITS = 48;
    static const GLint MIN_REQUIRED_UNIFORM_BUFFER_BINDINGS = 36;
    static const GLint MIN_REQUIRED_UNIFORM_LOCATIONS = 1024;
#endif

    static GLint MAX_TEXTURE_IMAGE_UNITS;
    static GLint MAX_UNIFORM_BUFFER_BINDINGS;
    static GLint MAX_COMBINED_UNIFORM_BLOCKS;
    static GLint MAX_COMBINED_TEXTURE_IMAGE_UNITS;
    static GLint MAX_UNIFORM_BLOCK_SIZE;
    static GLint UNIFORM_BUFFER_OFFSET_ALIGNMENT;

    virtual ~GLBackend();

    // Shutdown rendering and persist any required resources
    void shutdown() override;

    void setCameraCorrection(const Mat4& correction, const Mat4& prevRenderView, bool reset = false);
    void render(const Batch& batch) final override;

    // This call synchronize the Full Backend cache with the current GLState
    // THis is only intended to be used when mixing raw gl calls with the gpu api usage in order to sync
    // the gpu::Backend state with the true gl state which has probably been messed up by these ugly naked gl calls
    // Let's try to avoid to do that as much as possible!
    void syncCache() final override;

    // This is the ugly "download the pixels to sysmem for taking a snapshot"
    // Just avoid using it, it's ugly and will break performances
    virtual void downloadFramebuffer(const FramebufferPointer& srcFramebuffer,
                                     const Vec4i& region,
                                     QImage& destImage) final override;

    // this is the maximum numeber of available input buffers
    size_t getNumInputBuffers() const { return _input._invalidBuffers.size(); }

    // this is the maximum per shader stage on the low end apple
    // TODO make it platform dependant at init time
    static const int MAX_NUM_UNIFORM_BUFFERS = 14;
    size_t getMaxNumUniformBuffers() const { return MAX_NUM_UNIFORM_BUFFERS; }

    // this is the maximum per shader stage on the low end apple
    // TODO make it platform dependant at init time
    static const int MAX_NUM_RESOURCE_BUFFERS = 16;
    size_t getMaxNumResourceBuffers() const { return MAX_NUM_RESOURCE_BUFFERS; }
    static const int MAX_NUM_RESOURCE_TEXTURES = 16;
    size_t getMaxNumResourceTextures() const { return MAX_NUM_RESOURCE_TEXTURES; }

    // Texture Tables offers 2 dedicated slot (taken from the ubo slots)
    static const int MAX_NUM_RESOURCE_TABLE_TEXTURES = 2;
    size_t getMaxNumResourceTextureTables() const { return MAX_NUM_RESOURCE_TABLE_TEXTURES; }

    // Draw Stage
    virtual void do_draw(const Batch& batch, size_t paramOffset) = 0;
    virtual void do_drawIndexed(const Batch& batch, size_t paramOffset) = 0;
    virtual void do_drawInstanced(const Batch& batch, size_t paramOffset) = 0;
    virtual void do_drawIndexedInstanced(const Batch& batch, size_t paramOffset) = 0;
    virtual void do_multiDrawIndirect(const Batch& batch, size_t paramOffset) = 0;
    virtual void do_multiDrawIndexedIndirect(const Batch& batch, size_t paramOffset) = 0;

    // Input Stage
    virtual void do_setInputFormat(const Batch& batch, size_t paramOffset) final;
    virtual void do_setInputBuffer(const Batch& batch, size_t paramOffset) final;
    virtual void do_setIndexBuffer(const Batch& batch, size_t paramOffset) final;
    virtual void do_setIndirectBuffer(const Batch& batch, size_t paramOffset) final;
    virtual void do_generateTextureMips(const Batch& batch, size_t paramOffset) final;

    // Transform Stage
    virtual void do_setModelTransform(const Batch& batch, size_t paramOffset) final;
    virtual void do_setViewTransform(const Batch& batch, size_t paramOffset) final;
    virtual void do_setProjectionTransform(const Batch& batch, size_t paramOffset) final;
    virtual void do_setProjectionJitter(const Batch& batch, size_t paramOffset) final;
    virtual void do_setViewportTransform(const Batch& batch, size_t paramOffset) final;
    virtual void do_setDepthRangeTransform(const Batch& batch, size_t paramOffset) final;

    // Uniform Stage
    virtual void do_setUniformBuffer(const Batch& batch, size_t paramOffset) final;

    // Resource Stage
    virtual void do_setResourceBuffer(const Batch& batch, size_t paramOffset) final;
    virtual void do_setResourceTexture(const Batch& batch, size_t paramOffset) final;
    virtual void do_setResourceTextureTable(const Batch& batch, size_t paramOffset);
    virtual void do_setResourceFramebufferSwapChainTexture(const Batch& batch, size_t paramOffset) final;

    // Pipeline Stage
    virtual void do_setPipeline(const Batch& batch, size_t paramOffset) final;

    // Output stage
    virtual void do_setFramebuffer(const Batch& batch, size_t paramOffset) final;
    virtual void do_setFramebufferSwapChain(const Batch& batch, size_t paramOffset) final;
    virtual void do_clearFramebuffer(const Batch& batch, size_t paramOffset) final;
    virtual void do_blit(const Batch& batch, size_t paramOffset) = 0;

    virtual void do_advance(const Batch& batch, size_t paramOffset) final;

    // Query section
    virtual void do_beginQuery(const Batch& batch, size_t paramOffset) final;
    virtual void do_endQuery(const Batch& batch, size_t paramOffset) final;
    virtual void do_getQuery(const Batch& batch, size_t paramOffset) final;

    // Reset stages
    virtual void do_resetStages(const Batch& batch, size_t paramOffset) final;

    virtual void do_disableContextViewCorrection(const Batch& batch, size_t paramOffset) final;
    virtual void do_restoreContextViewCorrection(const Batch& batch, size_t paramOffset) final;

    virtual void do_disableContextStereo(const Batch& batch, size_t paramOffset) final;
    virtual void do_restoreContextStereo(const Batch& batch, size_t paramOffset) final;

    virtual void do_runLambda(const Batch& batch, size_t paramOffset) final;

    virtual void do_startNamedCall(const Batch& batch, size_t paramOffset) final;
    virtual void do_stopNamedCall(const Batch& batch, size_t paramOffset) final;

    static const int MAX_NUM_ATTRIBUTES = Stream::NUM_INPUT_SLOTS;
    // The drawcall Info attribute  channel is reserved and is the upper bound for the number of availables Input buffers
    static const int MAX_NUM_INPUT_BUFFERS = Stream::DRAW_CALL_INFO;

    virtual void do_pushProfileRange(const Batch& batch, size_t paramOffset) final;
    virtual void do_popProfileRange(const Batch& batch, size_t paramOffset) final;

    // TODO: As long as we have gl calls explicitely issued from interface
    // code, we need to be able to record and batch these calls. THe long
    // term strategy is to get rid of any GL calls in favor of the HIFI GPU API
    virtual void do_glUniform1i(const Batch& batch, size_t paramOffset) final;
    virtual void do_glUniform1f(const Batch& batch, size_t paramOffset) final;
    virtual void do_glUniform2f(const Batch& batch, size_t paramOffset) final;
    virtual void do_glUniform3f(const Batch& batch, size_t paramOffset) final;
    virtual void do_glUniform4f(const Batch& batch, size_t paramOffset) final;
    virtual void do_glUniform3fv(const Batch& batch, size_t paramOffset) final;
    virtual void do_glUniform4fv(const Batch& batch, size_t paramOffset) final;
    virtual void do_glUniform4iv(const Batch& batch, size_t paramOffset) final;
    virtual void do_glUniformMatrix3fv(const Batch& batch, size_t paramOffset) final;
    virtual void do_glUniformMatrix4fv(const Batch& batch, size_t paramOffset) final;

    virtual void do_glColor4f(const Batch& batch, size_t paramOffset) final;

    // The State setters called by the GLState::Commands when a new state is assigned
    virtual void do_setStateFillMode(int32 mode) final;
    virtual void do_setStateCullMode(int32 mode) final;
    virtual void do_setStateFrontFaceClockwise(bool isClockwise) final;
    virtual void do_setStateDepthClampEnable(bool enable) final;
    virtual void do_setStateScissorEnable(bool enable) final;
    virtual void do_setStateMultisampleEnable(bool enable) final;
    virtual void do_setStateAntialiasedLineEnable(bool enable) final;
    virtual void do_setStateDepthBias(Vec2 bias) final;
    virtual void do_setStateDepthTest(State::DepthTest test) final;
    virtual void do_setStateStencil(State::StencilActivation activation,
                                    State::StencilTest frontTest,
                                    State::StencilTest backTest) final;
    virtual void do_setStateAlphaToCoverageEnable(bool enable) final;
    virtual void do_setStateSampleMask(uint32 mask) final;
    virtual void do_setStateBlend(State::BlendFunction blendFunction) final;
    virtual void do_setStateColorWriteMask(uint32 mask) final;
    virtual void do_setStateBlendFactor(const Batch& batch, size_t paramOffset) final;
    virtual void do_setStateScissorRect(const Batch& batch, size_t paramOffset) final;

    virtual GLuint getFramebufferID(const FramebufferPointer& framebuffer) = 0;
    virtual GLuint getTextureID(const TexturePointer& texture) final;
    virtual GLuint getBufferID(const Buffer& buffer) = 0;
    virtual GLuint getBufferIDUnsynced(const Buffer& buffer) = 0;
    virtual GLuint getQueryID(const QueryPointer& query) = 0;

    virtual GLFramebuffer* syncGPUObject(const Framebuffer& framebuffer) = 0;
    virtual GLBuffer* syncGPUObject(const Buffer& buffer) = 0;
    virtual GLTexture* syncGPUObject(const TexturePointer& texture);
    virtual GLQuery* syncGPUObject(const Query& query) = 0;
    //virtual bool isTextureReady(const TexturePointer& texture);

    virtual void releaseBuffer(GLuint id, Size size) const;
    virtual void releaseExternalTexture(GLuint id, const Texture::ExternalRecycler& recycler) const;
    virtual void releaseTexture(GLuint id, Size size) const;
    virtual void releaseFramebuffer(GLuint id) const;
    virtual void releaseShader(GLuint id) const;
    virtual void releaseProgram(GLuint id) const;
    virtual void releaseQuery(GLuint id) const;
    virtual void queueLambda(const std::function<void()> lambda) const;

    bool isTextureManagementSparseEnabled() const override {
        return (_textureManagement._sparseCapable && Texture::getEnableSparseTextures());
    }

protected:
    virtual GLint getRealUniformLocation(GLint location) const;

    void recycle() const override;

    // FIXME instead of a single flag, create a features struct similar to
    // https://www.khronos.org/registry/vulkan/specs/1.0/man/html/VkPhysicalDeviceFeatures.html
    virtual bool supportsBindless() const { return false; }

    static const size_t INVALID_OFFSET = (size_t)-1;
    bool _inRenderTransferPass{ false };
    int _currentDraw{ -1 };

    std::list<std::string> profileRanges;
    mutable Mutex _trashMutex;
    mutable std::list<std::pair<GLuint, Size>> _buffersTrash;
    mutable std::list<std::pair<GLuint, Size>> _texturesTrash;
    mutable std::list<std::pair<GLuint, Texture::ExternalRecycler>> _externalTexturesTrash;
    mutable std::list<GLuint> _framebuffersTrash;
    mutable std::list<GLuint> _shadersTrash;
    mutable std::list<GLuint> _programsTrash;
    mutable std::list<GLuint> _queriesTrash;
    mutable std::list<std::function<void()>> _lambdaQueue;

    void renderPassTransfer(const Batch& batch);
    void renderPassDraw(const Batch& batch);

#ifdef GPU_STEREO_DRAWCALL_DOUBLED
    void setupStereoSide(int side);
#endif

    virtual void setResourceTexture(unsigned int slot, const TexturePointer& resourceTexture);
    virtual void setFramebuffer(const FramebufferPointer& framebuffer);
    virtual void initInput() final;
    virtual void killInput() final;
    virtual void syncInputStateCache() final;
    virtual void resetInputStage();
    virtual void updateInput() = 0;

    struct InputStageState {
        bool _invalidFormat{ true };
        bool _lastUpdateStereoState{ false };
        bool _hadColorAttribute{ true };
        FormatReference _format{ GPU_REFERENCE_INIT_VALUE };
        std::string _formatKey;

        typedef std::bitset<MAX_NUM_ATTRIBUTES> ActivationCache;
        ActivationCache _attributeActivation{ 0 };

        typedef std::bitset<MAX_NUM_INPUT_BUFFERS> BuffersState;

        BuffersState _invalidBuffers{ 0 };
        BuffersState _attribBindingBuffers{ 0 };

        std::array<BufferReference, MAX_NUM_INPUT_BUFFERS> _buffers{};
        std::array<Offset, MAX_NUM_INPUT_BUFFERS> _bufferOffsets{};
        std::array<Offset, MAX_NUM_INPUT_BUFFERS> _bufferStrides{};
        std::array<GLuint, MAX_NUM_INPUT_BUFFERS> _bufferVBOs{};

        glm::vec4 _colorAttribute{ 0.0f };

        BufferReference _indexBuffer{};
        Offset _indexBufferOffset{ 0 };
        Type _indexBufferType{ UINT32 };

        BufferReference _indirectBuffer{};
        Offset _indirectBufferOffset{ 0 };
        Offset _indirectBufferStride{ 0 };

        GLuint _defaultVAO{ 0 };
    } _input;

    virtual void initTransform() = 0;
    void killTransform();
    // Synchronize the state cache of this Backend with the actual real state of the GL Context
    void syncTransformStateCache();
    virtual void updateTransform(const Batch& batch) = 0;
    virtual void resetTransformStage();

    // Allows for correction of the camera pose to account for changes
    // between the time when a was recorded and the time(s) when it is
    // executed
    // Prev is the previous correction used at previous frame
    struct CameraCorrection {
        mat4 correction;
        mat4 correctionInverse;
        mat4 prevView;
        mat4 prevViewInverse;
    };

    struct TransformStageState {
#ifdef GPU_STEREO_CAMERA_BUFFER
        struct Cameras {
            TransformCamera _cams[2];

            Cameras(){};
            Cameras(const TransformCamera& cam) { memcpy(_cams, &cam, sizeof(TransformCamera)); };
            Cameras(const TransformCamera& camL, const TransformCamera& camR) {
                memcpy(_cams, &camL, sizeof(TransformCamera));
                memcpy(_cams + 1, &camR, sizeof(TransformCamera));
            };
        };

        using CameraBufferElement = Cameras;
#else
        using CameraBufferElement = TransformCamera;
#endif
        using TransformCameras = std::vector<CameraBufferElement>;

        TransformCamera _camera;
        TransformCameras _cameras;

        mutable std::map<std::string, GLvoid*> _drawCallInfoOffsets;

        GLuint _objectBuffer{ 0 };
        GLuint _cameraBuffer{ 0 };
        GLuint _drawCallInfoBuffer{ 0 };
        GLuint _objectBufferTexture{ 0 };
        size_t _cameraUboSize{ 0 };
        bool _viewIsCamera{ false };
        bool _skybox{ false };
        Transform _view;
        CameraCorrection _correction;
        bool _viewCorrectionEnabled{ true };

        Mat4 _projection;
        Vec4i _viewport{ 0, 0, 1, 1 };
        Vec2 _depthRange{ 0.0f, 1.0f };
        Vec2 _projectionJitter{ 0.0f, 0.0f };
        bool _invalidView{ false };
        bool _invalidProj{ false };
        bool _invalidViewport{ false };

        bool _enabledDrawcallInfoBuffer{ false };

        using Pair = std::pair<size_t, size_t>;
        using List = std::list<Pair>;
        List _cameraOffsets;
        mutable List::const_iterator _camerasItr;
        mutable size_t _currentCameraOffset{ INVALID_OFFSET };

        void preUpdate(size_t commandIndex, const StereoState& stereo, Vec2u framebufferSize);
        void update(size_t commandIndex, const StereoState& stereo) const;
        void bindCurrentCamera(int stereoSide) const;
    } _transform;

    virtual void transferTransformState(const Batch& batch) const = 0;

    struct UniformStageState {
        struct BufferState {
            BufferReference buffer{};
            GLintptr offset{ 0 };
            GLsizeiptr size{ 0 };

            BufferState& operator=(const BufferState& other) = delete;
            void reset() {
                gpu::gl::reset(buffer);
                offset = 0;
                size = 0;
            }
            bool compare(const BufferPointer& buffer, GLintptr offset, GLsizeiptr size) {
                const auto& self = *this;
                return (self.offset == offset && self.size == size && gpu::gl::compare(self.buffer, buffer));
            }
        };

        // MAX_NUM_UNIFORM_BUFFERS-1 is the max uniform index BATCHES are allowed to set, but
        // MIN_REQUIRED_UNIFORM_BUFFER_BINDINGS is used here because the backend sets some
        // internal UBOs for things like camera correction
        std::array<BufferState, MIN_REQUIRED_UNIFORM_BUFFER_BINDINGS> _buffers;
    } _uniform;

    // Helper function that provides common code
    void bindUniformBuffer(uint32_t slot, const BufferPointer& buffer, GLintptr offset = 0, GLsizeiptr size = 0);
    void releaseUniformBuffer(uint32_t slot);
    void resetUniformStage();

    // update resource cache and do the gl bind/unbind call with the current gpu::Buffer cached at slot s
    // This is using different gl object  depending on the gl version
    virtual bool bindResourceBuffer(uint32_t slot, const BufferPointer& buffer) = 0;
    virtual void releaseResourceBuffer(uint32_t slot) = 0;

    // Helper function that provides common code used by do_setResourceTexture and
    // do_setResourceTextureTable (in non-bindless mode)
    void bindResourceTexture(uint32_t slot, const TexturePointer& texture);

    // update resource cache and do the gl unbind call with the current gpu::Texture cached at slot s
    void releaseResourceTexture(uint32_t slot);

    void resetResourceStage();

    struct ResourceStageState {
        struct TextureState {
            TextureReference _texture{};
            GLenum _target;
        };
        std::array<BufferReference, MAX_NUM_RESOURCE_BUFFERS> _buffers{};
        std::array<TextureState, MAX_NUM_RESOURCE_TEXTURES> _textures{};
        int findEmptyTextureSlot() const;
    } _resource;

    size_t _commandIndex{ 0 };

    // Standard update pipeline check that the current Program and current State or good to go for a
    void updatePipeline();
    // Force to reset all the state fields indicated by the 'toBeReset" signature
    void resetPipelineState(State::Signature toBeReset);
    // Synchronize the state cache of this Backend with the actual real state of the GL Context
    void syncPipelineStateCache();
    void resetPipelineStage();

    struct PipelineStageState {
        PipelineReference _pipeline{};

        GLuint _program{ 0 };
        bool _cameraCorrection{ false };
        GLShader* _programShader{ nullptr };
        bool _invalidProgram{ false };

        BufferView _cameraCorrectionBuffer{ gpu::BufferView(std::make_shared<gpu::Buffer>(sizeof(CameraCorrection), nullptr)) };
        BufferView _cameraCorrectionBufferIdentity{ gpu::BufferView(
            std::make_shared<gpu::Buffer>(sizeof(CameraCorrection), nullptr)) };

        State::Data _stateCache{ State::DEFAULT };
        State::Signature _stateSignatureCache{ 0 };

        GLState* _state{ nullptr };
        bool _invalidState{ false };

        PipelineStageState() {
            _cameraCorrectionBuffer.edit<CameraCorrection>() = CameraCorrection();
            _cameraCorrectionBufferIdentity.edit<CameraCorrection>() = CameraCorrection();
            _cameraCorrectionBufferIdentity._buffer->flush();
        }
    } _pipeline;

    // Backend dependant compilation of the shader
    virtual void postLinkProgram(ShaderObject& programObject, const Shader& program) const;
    virtual GLShader* compileBackendProgram(const Shader& program, const Shader::CompilationHandler& handler);
    virtual GLShader* compileBackendShader(const Shader& shader, const Shader::CompilationHandler& handler);
    virtual std::string getBackendShaderHeader() const = 0;
    // For a program, this will return a string containing all the source files (without any
    // backend headers or defines).  For a vertex, fragment or geometry shader, this will
    // return the fully customized shader with all the version and backend specific
    // preprocessor directives
    // The program string returned can be used as a key for a cache of shader binaries
    // The shader strings can be reliably sent to the low level `compileShader` functions
    virtual std::string getShaderSource(const Shader& shader, int version) final;
    class ElementResource {
    public:
        gpu::Element _element;
        uint16 _resource;
        ElementResource(Element&& elem, uint16 resource) : _element(elem), _resource(resource) {}
    };
    ElementResource getFormatFromGLUniform(GLenum gltype);

    // Synchronize the state cache of this Backend with the actual real state of the GL Context
    void syncOutputStateCache();
    void resetOutputStage();

    struct OutputStageState {
        FramebufferReference _framebuffer{};
        GLuint _drawFBO{ 0 };
    } _output;

    void resetQueryStage();
    struct QueryStageState {
        uint32_t _rangeQueryDepth{ 0 };
    } _queryStage;

    void resetStages();

    // Stores cached binary versions of the shaders for quicker startup on subsequent runs
    // Note that shaders in the cache can still fail to load due to hardware or driver
    // changes that invalidate the cached binary, in which case we fall back on compiling
    // the source again
    struct ShaderBinaryCache {
        std::mutex _mutex;
        std::vector<GLint> _formats;
        std::unordered_map<std::string, ::gl::CachedShader> _binaries;
    } _shaderBinaryCache;

    virtual void initShaderBinaryCache();
    virtual void killShaderBinaryCache();

    struct TextureManagementStageState {
        bool _sparseCapable{ false };
        GLTextureTransferEnginePointer _transferEngine;
    } _textureManagement;
    virtual void initTextureManagementStage();
    virtual void killTextureManagementStage();

    typedef void (GLBackend::*CommandCall)(const Batch&, size_t);
    static CommandCall _commandCalls[Batch::NUM_COMMANDS];
    friend class GLState;
    friend class GLTexture;
    friend class GLShader;
};

}}  // namespace gpu::gl

#endif
