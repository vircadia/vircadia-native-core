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

#include <assert.h>
#include <functional>
#include <bitset>
#include <queue>
#include <utility>
#include <list>

#include "GPUConfig.h"

#include "Context.h"

namespace gpu {

class GLBackend : public Backend {

    // Context Backend static interface required
    friend class Context;
    static void init();
    static Backend* createBackend();
    static bool makeProgram(Shader& shader, const Shader::BindingSet& bindings);

    explicit GLBackend(bool syncCache);
    GLBackend();
public:

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
        Stamp _stamp;
        GLuint _buffer;
        GLuint _size;

        GLBuffer();
        ~GLBuffer();
    };
    static GLBuffer* syncGPUObject(const Buffer& buffer);
    static GLuint getBufferID(const Buffer& buffer);

    class GLTexture : public GPUObject {
    public:
        Stamp _storageStamp;
        Stamp _contentStamp;
        GLuint _texture;
        GLenum _target;
        GLuint _size;

        GLTexture();
        ~GLTexture();
    };
    static GLTexture* syncGPUObject(const Texture& texture);
    static GLuint getTextureID(const TexturePointer& texture);

    // very specific for now
    static void syncSampler(const Sampler& sampler, Texture::Type type, GLTexture* object);

    class GLShader : public GPUObject {
    public:
        GLuint _shader;
        GLuint _program;

        GLint _transformCameraSlot = -1;
        GLint _transformObjectSlot = -1;

        GLShader();
        ~GLShader();
    };
    static GLShader* syncGPUObject(const Shader& shader);
    static GLuint getShaderID(const ShaderPointer& shader);

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
        // WARNING depending on the order of the State::Field enum
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
    static const int MAX_NUM_INPUT_BUFFERS = 16;

    uint32 getNumInputBuffers() const { return _input._invalidBuffers.size(); }

    // this is the maximum per shader stage on the low end apple
    // TODO make it platform dependant at init time
    static const int MAX_NUM_UNIFORM_BUFFERS = 12;
    uint32 getMaxNumUniformBuffers() const { return MAX_NUM_UNIFORM_BUFFERS; }

    // this is the maximum per shader stage on the low end apple
    // TODO make it platform dependant at init time
    static const int MAX_NUM_RESOURCE_TEXTURES = 16;
    uint32 getMaxNumResourceTextures() const { return MAX_NUM_RESOURCE_TEXTURES; }

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

    // Repporting stats of the context
    class Stats {
    public:
        int _ISNumFormatChanges = 0;
        int _ISNumInputBufferChanges = 0;
        int _ISNumIndexBufferChanges = 0;

        Stats() {}
        Stats(const Stats& stats) = default;
    };

    void getStats(Stats& stats) const { stats = _stats; }

protected:
    void renderPassTransfer(Batch& batch);
    void renderPassDraw(Batch& batch);

    Stats _stats;

    // Draw Stage
    void do_draw(Batch& batch, uint32 paramOffset);
    void do_drawIndexed(Batch& batch, uint32 paramOffset);
    void do_drawInstanced(Batch& batch, uint32 paramOffset);
    void do_drawIndexedInstanced(Batch& batch, uint32 paramOffset);

    // Input Stage
    void do_setInputFormat(Batch& batch, uint32 paramOffset);
    void do_setInputBuffer(Batch& batch, uint32 paramOffset);
    void do_setIndexBuffer(Batch& batch, uint32 paramOffset);

    void initInput();
    void killInput();
    void syncInputStateCache();
    void updateInput();
    void resetInputStage();
    struct InputStageState {
        bool _invalidFormat = true;
        Stream::FormatPointer _format;

        typedef std::bitset<MAX_NUM_ATTRIBUTES> ActivationCache;
        ActivationCache _attributeActivation;

        typedef std::bitset<MAX_NUM_INPUT_BUFFERS> BuffersState;
        BuffersState _invalidBuffers;

        Buffers _buffers;
        Offsets _bufferOffsets;
        Offsets _bufferStrides;
        std::vector<GLuint> _bufferVBOs;

        BufferPointer _indexBuffer;
        Offset _indexBufferOffset;
        Type _indexBufferType;

        GLuint _defaultVAO;

        InputStageState() :
            _invalidFormat(true),
            _format(0),
            _attributeActivation(0),
            _invalidBuffers(0),
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
    void do_setModelTransform(Batch& batch, uint32 paramOffset);
    void do_setViewTransform(Batch& batch, uint32 paramOffset);
    void do_setProjectionTransform(Batch& batch, uint32 paramOffset);
    void do_setViewportTransform(Batch& batch, uint32 paramOffset);

    void initTransform();
    void killTransform();
    // Synchronize the state cache of this Backend with the actual real state of the GL Context
    void syncTransformStateCache();
    void updateTransform() const;
    void resetTransformStage();

    struct TransformStageState {
        using TransformObjects = std::vector<TransformObject>;
        using TransformCameras = std::vector<TransformCamera>;

        TransformObject _object;
        TransformCamera _camera;
        TransformObjects _objects;
        TransformCameras _cameras;

        size_t _cameraUboSize{ 0 };
        size_t _objectUboSize{ 0 };
        GLuint _objectBuffer{ 0 };
        GLuint _cameraBuffer{ 0 };
        Transform _model;
        Transform _view;
        Mat4 _projection;
        Vec4i _viewport{ 0, 0, 1, 1 };
        bool _invalidModel{true};
        bool _invalidView{false};
        bool _invalidProj{false};
        bool _invalidViewport{ false };

        using Pair = std::pair<size_t, size_t>;
        using List = std::list<Pair>;
        List _cameraOffsets;
        List _objectOffsets;
        mutable List::const_iterator _objectsItr;
        mutable List::const_iterator _camerasItr;

        void preUpdate(size_t commandIndex, const StereoState& stereo);
        void update(size_t commandIndex, const StereoState& stereo) const;
        void transfer() const;
    } _transform;

    int32_t _uboAlignment{ 0 };


    // Uniform Stage
    void do_setUniformBuffer(Batch& batch, uint32 paramOffset);

    void releaseUniformBuffer(uint32_t slot);
    void resetUniformStage();
    struct UniformStageState {
        Buffers _buffers;

        UniformStageState():
            _buffers(MAX_NUM_UNIFORM_BUFFERS, nullptr)
        {}
    } _uniform;
    
    // Resource Stage
    void do_setResourceTexture(Batch& batch, uint32 paramOffset);
    
    void releaseResourceTexture(uint32_t slot);
    void resetResourceStage();
    struct ResourceStageState {
        Textures _textures;

        ResourceStageState():
            _textures(MAX_NUM_RESOURCE_TEXTURES, nullptr)
        {}

    } _resource;
    size_t _commandIndex{ 0 };

    // Pipeline Stage
    void do_setPipeline(Batch& batch, uint32 paramOffset);
    void do_setStateBlendFactor(Batch& batch, uint32 paramOffset);
    void do_setStateScissorRect(Batch& batch, uint32 paramOffset);
    
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
        bool _invalidProgram;

        State::Data _stateCache;
        State::Signature _stateSignatureCache;

        GLState* _state;
        bool _invalidState = false;

        PipelineStageState() :
            _pipeline(),
            _program(0),
            _invalidProgram(false),
            _stateCache(State::DEFAULT),
            _stateSignatureCache(0),
            _state(nullptr),
            _invalidState(false)
             {}
    } _pipeline;

    // Output stage
    void do_setFramebuffer(Batch& batch, uint32 paramOffset);
    void do_clearFramebuffer(Batch& batch, uint32 paramOffset);
    void do_blit(Batch& batch, uint32 paramOffset);

    // Synchronize the state cache of this Backend with the actual real state of the GL Context
    void syncOutputStateCache();
    void resetOutputStage();
    
    struct OutputStageState {

        FramebufferPointer _framebuffer = nullptr;
        GLuint _drawFBO = 0;
        
        OutputStageState() {}
    } _output;

    // Query section
    void do_beginQuery(Batch& batch, uint32 paramOffset);
    void do_endQuery(Batch& batch, uint32 paramOffset);
    void do_getQuery(Batch& batch, uint32 paramOffset);

    void resetQueryStage();
    struct QueryStageState {
        
    };

    // Reset stages
    void do_resetStages(Batch& batch, uint32 paramOffset);
    void resetStages();

    // TODO: As long as we have gl calls explicitely issued from interface
    // code, we need to be able to record and batch these calls. THe long 
    // term strategy is to get rid of any GL calls in favor of the HIFI GPU API
    void do_glActiveBindTexture(Batch& batch, uint32 paramOffset);

    void do_glUniform1i(Batch& batch, uint32 paramOffset);
    void do_glUniform1f(Batch& batch, uint32 paramOffset);
    void do_glUniform2f(Batch& batch, uint32 paramOffset);
    void do_glUniform3f(Batch& batch, uint32 paramOffset);
    void do_glUniform4f(Batch& batch, uint32 paramOffset);
    void do_glUniform3fv(Batch& batch, uint32 paramOffset);
    void do_glUniform4fv(Batch& batch, uint32 paramOffset);
    void do_glUniform4iv(Batch& batch, uint32 paramOffset);
    void do_glUniformMatrix4fv(Batch& batch, uint32 paramOffset);

    void do_glColor4f(Batch& batch, uint32 paramOffset);

    typedef void (GLBackend::*CommandCall)(Batch&, uint32);
    static CommandCall _commandCalls[Batch::NUM_COMMANDS];
};


};

#endif
