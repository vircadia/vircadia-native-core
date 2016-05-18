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
#ifndef hifi_gpu_GLBackend_h
#define hifi_gpu_GLBackend_h

// Core 41 doesn't expose the features to really separate the vertex format from the vertex buffers binding
// Core 43 does :)
#if(GPU_INPUT_PROFILE == GPU_CORE_41)
#define NO_SUPPORT_VERTEX_ATTRIB_FORMAT
#else
#define SUPPORT_VERTEX_ATTRIB_FORMAT
#endif


#include <assert.h>
#include <functional>
#include <bitset>
#include <queue>
#include <utility>
#include <list>
#include <array>

#include <gl/Config.h>

#include "Context.h"

namespace gpu {

class GLTextureTransferHelper;

class GLBackend : public Backend {

    // Context Backend static interface required
    friend class Context;
    static void init();
    static Backend* createBackend();
    static bool makeProgram(Shader& shader, const Shader::BindingSet& bindings);

    explicit GLBackend(bool syncCache);
    GLBackend();
public:
    static Context::Size getDedicatedMemory();

    virtual ~GLBackend();

    virtual void render(Batch& batch);

    // This call synchronize the Full Backend cache with the current GLState
    // THis is only intended to be used when mixing raw gl calls with the gpu api usage in order to sync
    // the gpu::Backend state with the true gl state which has probably been messed up by these ugly naked gl calls
    // Let's try to avoid to do that as much as possible!
    virtual void syncCache();

    // This is the ugly "download the pixels to sysmem for taking a snapshot"
    // Just avoid using it, it's ugly and will break performances
    virtual void downloadFramebuffer(const FramebufferPointer& srcFramebuffer, const Vec4i& region, QImage& destImage);

    static bool checkGLError(const char* name = nullptr);

    // Only checks in debug builds
    static bool checkGLErrorDebug(const char* name = nullptr);

    static void checkGLStackStable(std::function<void()> f);

    

    class GLBuffer : public GPUObject {
    public:
        const GLuint _buffer;
        const GLuint _size;
        const Stamp _stamp;

        GLBuffer(const Buffer& buffer, GLBuffer* original = nullptr);
        ~GLBuffer();

        void transfer();

    private:
        bool getNextTransferBlock(GLintptr& outOffset, GLsizeiptr& outSize, size_t& currentPage) const;
            
        // The owning texture
        const Buffer& _gpuBuffer;
    };

    static GLBuffer* syncGPUObject(const Buffer& buffer);
    static GLuint getBufferID(const Buffer& buffer);

    class GLTexture : public GPUObject {
    public:
        // The public gl texture object
        GLuint _texture{ 0 };

        const Stamp _storageStamp;
        Stamp _contentStamp { 0 };
        const GLenum _target;
        const uint16 _maxMip;
        const uint16 _minMip;
        const bool _transferrable;

        struct DownsampleSource {
            using Pointer = std::shared_ptr<DownsampleSource>;
            DownsampleSource(GLTexture& oldTexture);
            ~DownsampleSource();
            const GLuint _texture;
            const uint16 _minMip;
            const uint16 _maxMip;
        };

        DownsampleSource::Pointer _downsampleSource;

        GLTexture(bool transferrable, const gpu::Texture& gpuTexture);
        GLTexture(GLTexture& originalTexture, const gpu::Texture& gpuTexture);
        ~GLTexture();

        // Return a floating point value indicating how much of the allowed 
        // texture memory we are currently consuming.  A value of 0 indicates 
        // no texture memory usage, while a value of 1 indicates all available / allowed memory
        // is consumed.  A value above 1 indicates that there is a problem.
        static float getMemoryPressure();

        void withPreservedTexture(std::function<void()> f);

        void createTexture();
        void allocateStorage();

        GLuint size() const { return _size; }
        GLuint virtualSize() const { return _virtualSize; }

        void updateSize();

        enum SyncState {
            // The texture is currently undergoing no processing, although it's content
            // may be out of date, or it's storage may be invalid relative to the 
            // owning GPU texture
            Idle,
            // The texture has been queued for transfer to the GPU
            Pending,
            // The texture has been transferred to the GPU, but is awaiting
            // any post transfer operations that may need to occur on the 
            // primary rendering thread
            Transferred,
        };

        void setSyncState(SyncState syncState) { _syncState = syncState; }
        SyncState getSyncState() const { return _syncState; }

        // Is the storage out of date relative to the gpu texture?
        bool isInvalid() const;

        // Is the content out of date relative to the gpu texture?
        bool isOutdated() const;

        // Is the texture in a state where it can be rendered with no work?
        bool isReady() const;

        // Is this texture pushing us over the memory limit?
        bool isOverMaxMemory() const;

        // Move the image bits from the CPU to the GPU
        void transfer() const;

        // Execute any post-move operations that must occur only on the main thread
        void postTransfer();

        uint16 usedMipLevels() const { return (_maxMip - _minMip) + 1; }

        static const size_t CUBE_NUM_FACES = 6;
        static const GLenum CUBE_FACE_LAYOUT[6];
        
    private:
        friend class GLTextureTransferHelper;

        GLTexture(bool transferrable, const gpu::Texture& gpuTexture, bool init);
        // at creation the true texture is created in GL
        // it becomes public only when ready.
        GLuint _privateTexture{ 0 };

        const std::vector<GLenum>& getFaceTargets() const;

        void setSize(GLuint size);

        const GLuint _virtualSize; // theorical size as expected
        GLuint _size { 0 }; // true size as reported by the gl api

        void transferMip(uint16_t mipLevel, uint8_t face = 0) const;

        // The owning texture
        const Texture& _gpuTexture;
        std::atomic<SyncState> _syncState { SyncState::Idle };
    };
    static GLTexture* syncGPUObject(const TexturePointer& texture, bool needTransfer = true);
    static GLuint getTextureID(const TexturePointer& texture, bool sync = true);

    // very specific for now
    static void syncSampler(const Sampler& sampler, Texture::Type type, const GLTexture* object);

    class GLInputFormat : public GPUObject {
    public:
        std::string key;

        GLInputFormat();
        ~GLInputFormat();
    };
    GLInputFormat* syncGPUObject(const Stream::Format& inputFormat);

    class GLShader : public GPUObject {
    public:
        enum Version {
            Mono = 0,

            NumVersions
        };

        struct ShaderObject {
            GLuint glshader{ 0 };
            GLuint glprogram{ 0 };
            GLint transformCameraSlot{ -1 };
            GLint transformObjectSlot{ -1 };
        };

        using ShaderObjects = std::array< ShaderObject, NumVersions >;

        using UniformMapping = std::map<GLint, GLint>;
        using UniformMappingVersions = std::vector<UniformMapping>;
        
        GLShader();
        ~GLShader();

        ShaderObjects _shaderObjects;
        UniformMappingVersions _uniformMappings;

        GLuint getProgram(Version version = Mono) const {
            return _shaderObjects[version].glprogram;
        }

        GLint getUniformLocation(GLint srcLoc, Version version = Mono) {
            // THIS will be used in the future PR as we grow the number of versions
            // return _uniformMappings[version][srcLoc];
            return srcLoc;
        }

    };
    static GLShader* syncGPUObject(const Shader& shader);

    class GLState : public GPUObject {
    public:
        class Command {
        public:
            virtual void run(GLBackend* backend) = 0;
            Command() {}
            virtual ~Command() {};
        };

        template <class T> class Command1 : public Command {
        public:
            typedef void (GLBackend::*GLFunction)(T);
            void run(GLBackend* backend) { (backend->*(_func))(_param); }
            Command1(GLFunction func, T param) : _func(func), _param(param) {};
            GLFunction _func;
            T _param;
        };
        template <class T, class U> class Command2 : public Command {
        public:
            typedef void (GLBackend::*GLFunction)(T, U);
            void run(GLBackend* backend) { (backend->*(_func))(_param0, _param1); }
            Command2(GLFunction func, T param0, U param1) : _func(func), _param0(param0), _param1(param1) {};
            GLFunction _func;
            T _param0;
            U _param1;
        };

        template <class T, class U, class V> class Command3 : public Command {
        public:
            typedef void (GLBackend::*GLFunction)(T, U, V);
            void run(GLBackend* backend) { (backend->*(_func))(_param0, _param1, _param2); }
            Command3(GLFunction func, T param0, U param1, V param2) : _func(func), _param0(param0), _param1(param1), _param2(param2) {};
            GLFunction _func;
            T _param0;
            U _param1;
            V _param2;
        };

        typedef std::shared_ptr< Command > CommandPointer;
        typedef std::vector< CommandPointer > Commands;

        Commands _commands;
        Stamp _stamp;
        State::Signature _signature;

        GLState();
        ~GLState();

        // The state commands to reset to default,
        static const Commands _resetStateCommands;

        friend class GLBackend;
    };
    static GLState* syncGPUObject(const State& state);

    class GLPipeline : public GPUObject {
    public:
        GLShader* _program = 0;
        GLState* _state = 0;

        GLPipeline();
        ~GLPipeline();
    };
    static GLPipeline* syncGPUObject(const Pipeline& pipeline);


    class GLFramebuffer : public GPUObject {
    public:
        GLuint _fbo = 0;
        std::vector<GLenum> _colorBuffers;
        Stamp _depthStamp { 0 };
        std::vector<Stamp> _colorStamps;


        GLFramebuffer();
        ~GLFramebuffer();
    };
    static GLFramebuffer* syncGPUObject(const Framebuffer& framebuffer);
    static GLuint getFramebufferID(const FramebufferPointer& framebuffer);

    class GLQuery : public GPUObject {
    public:
        GLuint _qo = 0;
        GLuint64 _result = 0;

        GLQuery();
        ~GLQuery();
    };
    static GLQuery* syncGPUObject(const Query& query);
    static GLuint getQueryID(const QueryPointer& query);


    static const int MAX_NUM_ATTRIBUTES = Stream::NUM_INPUT_SLOTS;
    // The drawcall Info attribute  channel is reserved and is the upper bound for the number of availables Input buffers
    static const int MAX_NUM_INPUT_BUFFERS = Stream::DRAW_CALL_INFO;

    size_t getNumInputBuffers() const { return _input._invalidBuffers.size(); }

    // this is the maximum per shader stage on the low end apple
    // TODO make it platform dependant at init time
    static const int MAX_NUM_UNIFORM_BUFFERS = 12;
    size_t getMaxNumUniformBuffers() const { return MAX_NUM_UNIFORM_BUFFERS; }

    // this is the maximum per shader stage on the low end apple
    // TODO make it platform dependant at init time
    static const int MAX_NUM_RESOURCE_TEXTURES = 16;
    size_t getMaxNumResourceTextures() const { return MAX_NUM_RESOURCE_TEXTURES; }

    // The State setters called by the GLState::Commands when a new state is assigned
    void do_setStateFillMode(int32 mode);
    void do_setStateCullMode(int32 mode);
    void do_setStateFrontFaceClockwise(bool isClockwise);
    void do_setStateDepthClampEnable(bool enable);
    void do_setStateScissorEnable(bool enable);
    void do_setStateMultisampleEnable(bool enable);
    void do_setStateAntialiasedLineEnable(bool enable);

    void do_setStateDepthBias(Vec2 bias);
    void do_setStateDepthTest(State::DepthTest test);

    void do_setStateStencil(State::StencilActivation activation, State::StencilTest frontTest, State::StencilTest backTest);

    void do_setStateAlphaToCoverageEnable(bool enable);
    void do_setStateSampleMask(uint32 mask);

    void do_setStateBlend(State::BlendFunction blendFunction);

    void do_setStateColorWriteMask(uint32 mask);
    
protected:
    static const size_t INVALID_OFFSET = (size_t)-1;

    bool _inRenderTransferPass;

    void renderPassTransfer(Batch& batch);
    void renderPassDraw(Batch& batch);

    void setupStereoSide(int side);

    void initTextureTransferHelper();
    static void transferGPUObject(const TexturePointer& texture);

    static std::shared_ptr<GLTextureTransferHelper> _textureTransferHelper;

    // Draw Stage
    void do_draw(Batch& batch, size_t paramOffset);
    void do_drawIndexed(Batch& batch, size_t paramOffset);
    void do_drawInstanced(Batch& batch, size_t paramOffset);
    void do_drawIndexedInstanced(Batch& batch, size_t paramOffset);
    void do_multiDrawIndirect(Batch& batch, size_t paramOffset);
    void do_multiDrawIndexedIndirect(Batch& batch, size_t paramOffset);
    
    // Input Stage
    void do_setInputFormat(Batch& batch, size_t paramOffset);
    void do_setInputBuffer(Batch& batch, size_t paramOffset);
    void do_setIndexBuffer(Batch& batch, size_t paramOffset);
    void do_setIndirectBuffer(Batch& batch, size_t paramOffset);

    void initInput();
    void killInput();
    void syncInputStateCache();
    void updateInput();
    void resetInputStage();
    struct InputStageState {
        bool _invalidFormat = true;
        Stream::FormatPointer _format;
        std::string _formatKey;

        typedef std::bitset<MAX_NUM_ATTRIBUTES> ActivationCache;
        ActivationCache _attributeActivation;

        typedef std::bitset<MAX_NUM_INPUT_BUFFERS> BuffersState;
        BuffersState _invalidBuffers{ 0 };
        BuffersState _attribBindingBuffers{ 0 };

        Buffers _buffers;
        Offsets _bufferOffsets;
        Offsets _bufferStrides;
        std::vector<GLuint> _bufferVBOs;

        glm::vec4 _colorAttribute{ 0.0f };

        BufferPointer _indexBuffer;
        Offset _indexBufferOffset;
        Type _indexBufferType;
        
        BufferPointer _indirectBuffer;
        Offset _indirectBufferOffset{ 0 };
        Offset _indirectBufferStride{ 0 };

        GLuint _defaultVAO;
        
        InputStageState() :
            _invalidFormat(true),
            _format(0),
            _formatKey(),
            _attributeActivation(0),
            _buffers(_invalidBuffers.size(), BufferPointer(0)),
            _bufferOffsets(_invalidBuffers.size(), 0),
            _bufferStrides(_invalidBuffers.size(), 0),
            _bufferVBOs(_invalidBuffers.size(), 0),
            _indexBuffer(0),
            _indexBufferOffset(0),
            _indexBufferType(UINT32),
            _defaultVAO(0)
             {}
    } _input;

    // Transform Stage
    void do_setModelTransform(Batch& batch, size_t paramOffset);
    void do_setViewTransform(Batch& batch, size_t paramOffset);
    void do_setProjectionTransform(Batch& batch, size_t paramOffset);
    void do_setViewportTransform(Batch& batch, size_t paramOffset);
    void do_setDepthRangeTransform(Batch& batch, size_t paramOffset);

    void initTransform();
    void killTransform();
    // Synchronize the state cache of this Backend with the actual real state of the GL Context
    void syncTransformStateCache();
    void updateTransform(const Batch& batch);
    void resetTransformStage();

    struct TransformStageState {
        using CameraBufferElement = TransformCamera;
        using TransformCameras = std::vector<CameraBufferElement>;

        TransformCamera _camera;
        TransformCameras _cameras;

        mutable std::map<std::string, GLvoid*> _drawCallInfoOffsets;

        GLuint _objectBuffer { 0 };
        GLuint _cameraBuffer { 0 };
        GLuint _drawCallInfoBuffer { 0 };
        GLuint _objectBufferTexture { 0 };
        size_t _cameraUboSize { 0 };
        Transform _view;
        Mat4 _projection;
        Vec4i _viewport { 0, 0, 1, 1 };
        Vec2 _depthRange { 0.0f, 1.0f };
        bool _invalidView { false };
        bool _invalidProj { false };
        bool _invalidViewport { false };

        bool _enabledDrawcallInfoBuffer{ false };

        using Pair = std::pair<size_t, size_t>;
        using List = std::list<Pair>;
        List _cameraOffsets;
        mutable List::const_iterator _camerasItr;
        mutable size_t _currentCameraOffset{ INVALID_OFFSET };

        void preUpdate(size_t commandIndex, const StereoState& stereo);
        void update(size_t commandIndex, const StereoState& stereo) const;
        void bindCurrentCamera(int stereoSide) const;
        void transfer(const Batch& batch) const;
    } _transform;

    int32_t _uboAlignment{ 0 };


    // Uniform Stage
    void do_setUniformBuffer(Batch& batch, size_t paramOffset);

    void releaseUniformBuffer(uint32_t slot);
    void resetUniformStage();
    struct UniformStageState {
        Buffers _buffers;

        UniformStageState():
            _buffers(MAX_NUM_UNIFORM_BUFFERS, nullptr)
        {}
    } _uniform;
    
    // Resource Stage
    void do_setResourceTexture(Batch& batch, size_t paramOffset);

    // update resource cache and do the gl unbind call with the current gpu::Texture cached at slot s
    void releaseResourceTexture(uint32_t slot);

    void resetResourceStage();
    struct ResourceStageState {
        Textures _textures;

        int findEmptyTextureSlot() const;

        ResourceStageState():
            _textures(MAX_NUM_RESOURCE_TEXTURES, nullptr)
        {}

    } _resource;
    size_t _commandIndex{ 0 };

    // Pipeline Stage
    void do_setPipeline(Batch& batch, size_t paramOffset);
    void do_setStateBlendFactor(Batch& batch, size_t paramOffset);
    void do_setStateScissorRect(Batch& batch, size_t paramOffset);
    
    // Standard update pipeline check that the current Program and current State or good to go for a
    void updatePipeline();
    // Force to reset all the state fields indicated by the 'toBeReset" signature
    void resetPipelineState(State::Signature toBeReset);
    // Synchronize the state cache of this Backend with the actual real state of the GL Context
    void syncPipelineStateCache();
    // Grab the actual gl state into it's gpu::State equivalent. THis is used by the above call syncPipleineStateCache() 
    void getCurrentGLState(State::Data& state);
    void resetPipelineStage();

    struct PipelineStageState {

        PipelinePointer _pipeline;

        GLuint _program;
        GLShader* _programShader;
        bool _invalidProgram;

        State::Data _stateCache;
        State::Signature _stateSignatureCache;

        GLState* _state;
        bool _invalidState = false;

        PipelineStageState() :
            _pipeline(),
            _program(0),
            _programShader(nullptr),
            _invalidProgram(false),
            _stateCache(State::DEFAULT),
            _stateSignatureCache(0),
            _state(nullptr),
            _invalidState(false)
             {}
    } _pipeline;

    // Output stage
    void do_setFramebuffer(Batch& batch, size_t paramOffset);
    void do_clearFramebuffer(Batch& batch, size_t paramOffset);
    void do_blit(Batch& batch, size_t paramOffset);
    void do_generateTextureMips(Batch& batch, size_t paramOffset);

    // Synchronize the state cache of this Backend with the actual real state of the GL Context
    void syncOutputStateCache();
    void resetOutputStage();
    
    struct OutputStageState {

        FramebufferPointer _framebuffer = nullptr;
        GLuint _drawFBO = 0;
        
        OutputStageState() {}
    } _output;

    // Query section
    void do_beginQuery(Batch& batch, size_t paramOffset);
    void do_endQuery(Batch& batch, size_t paramOffset);
    void do_getQuery(Batch& batch, size_t paramOffset);

    void resetQueryStage();
    struct QueryStageState {
        
    };

    // Reset stages
    void do_resetStages(Batch& batch, size_t paramOffset);

    void do_runLambda(Batch& batch, size_t paramOffset);

    void do_startNamedCall(Batch& batch, size_t paramOffset);
    void do_stopNamedCall(Batch& batch, size_t paramOffset);

    void resetStages();

    // TODO: As long as we have gl calls explicitely issued from interface
    // code, we need to be able to record and batch these calls. THe long 
    // term strategy is to get rid of any GL calls in favor of the HIFI GPU API
    void do_glActiveBindTexture(Batch& batch, size_t paramOffset);

    void do_glUniform1i(Batch& batch, size_t paramOffset);
    void do_glUniform1f(Batch& batch, size_t paramOffset);
    void do_glUniform2f(Batch& batch, size_t paramOffset);
    void do_glUniform3f(Batch& batch, size_t paramOffset);
    void do_glUniform4f(Batch& batch, size_t paramOffset);
    void do_glUniform3fv(Batch& batch, size_t paramOffset);
    void do_glUniform4fv(Batch& batch, size_t paramOffset);
    void do_glUniform4iv(Batch& batch, size_t paramOffset);
    void do_glUniformMatrix4fv(Batch& batch, size_t paramOffset);

    void do_glColor4f(Batch& batch, size_t paramOffset);

    void do_pushProfileRange(Batch& batch, size_t paramOffset);
    void do_popProfileRange(Batch& batch, size_t paramOffset);
    
    int _currentDraw { -1 };

    typedef void (GLBackend::*CommandCall)(Batch&, size_t);
    static CommandCall _commandCalls[Batch::NUM_COMMANDS];

};

};

#endif
