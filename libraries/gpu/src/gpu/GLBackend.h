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
#include "GPUConfig.h"

#include "Context.h"
#include "Batch.h"
#include <bitset>


namespace gpu {

class GLBackend : public Backend {
public:

    GLBackend();
    ~GLBackend();

    void render(Batch& batch);

    static void renderBatch(Batch& batch);

    static bool checkGLError(const char* name = nullptr);

    static bool makeProgram(Shader& shader, const Shader::BindingSet& bindings = Shader::BindingSet());
    

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

        GLuint _transformCameraSlot = -1;
        GLuint _transformObjectSlot = -1;

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

        GLFramebuffer();
        ~GLFramebuffer();
    };
    static GLFramebuffer* syncGPUObject(const Framebuffer& framebuffer);
    static GLuint getFramebufferID(const FramebufferPointer& framebuffer);

    static const int MAX_NUM_ATTRIBUTES = Stream::NUM_INPUT_SLOTS;
    static const int MAX_NUM_INPUT_BUFFERS = 16;

    uint32 getNumInputBuffers() const { return _input._buffersState.size(); }


    // The State setters called by the GLState::Commands when a new state is assigned
    void do_setStateFillMode(int32 mode);
    void do_setStateCullMode(int32 mode);
    void do_setStateFrontFaceClockwise(bool isClockwise);
    void do_setStateDepthClipEnable(bool enable);
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

    // Draw Stage
    void do_draw(Batch& batch, uint32 paramOffset);
    void do_drawIndexed(Batch& batch, uint32 paramOffset);
    void do_drawInstanced(Batch& batch, uint32 paramOffset);
    void do_drawIndexedInstanced(Batch& batch, uint32 paramOffset);

    void do_clearFramebuffer(Batch& batch, uint32 paramOffset);

    // Input Stage
    void do_setInputFormat(Batch& batch, uint32 paramOffset);
    void do_setInputBuffer(Batch& batch, uint32 paramOffset);
    void do_setIndexBuffer(Batch& batch, uint32 paramOffset);

    void updateInput();
    struct InputStageState {
        bool _invalidFormat;
        Stream::FormatPointer _format;

        typedef std::bitset<MAX_NUM_INPUT_BUFFERS> BuffersState;
        BuffersState _buffersState;

        Buffers _buffers;
        Offsets _bufferOffsets;
        Offsets _bufferStrides;

        BufferPointer _indexBuffer;
        Offset _indexBufferOffset;
        Type _indexBufferType;

        typedef std::bitset<MAX_NUM_ATTRIBUTES> ActivationCache;
        ActivationCache _attributeActivation;

        InputStageState() :
            _invalidFormat(true),
            _format(0),
            _buffersState(0),
            _buffers(_buffersState.size(), BufferPointer(0)),
            _bufferOffsets(_buffersState.size(), 0),
            _bufferStrides(_buffersState.size(), 0),
            _indexBuffer(0),
            _indexBufferOffset(0),
            _indexBufferType(UINT32),
            _attributeActivation(0)
             {}
    } _input;

    // Transform Stage
    void do_setModelTransform(Batch& batch, uint32 paramOffset);
    void do_setViewTransform(Batch& batch, uint32 paramOffset);
    void do_setProjectionTransform(Batch& batch, uint32 paramOffset);

    void initTransform();
    void killTransform();
    void updateTransform();
    struct TransformStageState {
        TransformObject _transformObject;
        TransformCamera _transformCamera;
        GLuint _transformObjectBuffer;
        GLuint _transformCameraBuffer;
        Transform _model;
        Transform _view;
        Mat4 _projection;
        bool _invalidModel;
        bool _invalidView;
        bool _invalidProj;

        GLenum _lastMode;

        TransformStageState() :
            _transformObjectBuffer(0),
            _transformCameraBuffer(0),
            _model(),
            _view(),
            _projection(),
            _invalidModel(true),
            _invalidView(true),
            _invalidProj(false),
            _lastMode(GL_TEXTURE) {}
    } _transform;

    // Pipeline Stage
    void do_setPipeline(Batch& batch, uint32 paramOffset);

    void do_setStateBlendFactor(Batch& batch, uint32 paramOffset);

    void do_setUniformBuffer(Batch& batch, uint32 paramOffset);
    void do_setUniformTexture(Batch& batch, uint32 paramOffset);
 
    // Standard update pipeline check that the current Program and current State or good to go for a
    void updatePipeline();
    // Force to reset all the state fields indicated by the 'toBeReset" signature
    void resetPipelineState(State::Signature toBeReset);
    // Synchronize the state cache of this Backend with the actual real state of the GL Context
    void syncPipelineStateCache();
    // Grab the actual gl state into it's gpu::State equivalent. THis is used by the above call syncPipleineStateCache() 
    void getCurrentGLState(State::Data& state);

    struct PipelineStageState {

        PipelinePointer _pipeline;

        GLuint _program;
        bool _invalidProgram;

        State::Data _stateCache;
        State::Signature _stateSignatureCache;

        GLState* _state;
        bool _invalidState = false;
        bool _needStateSync = true;

        PipelineStageState() :
            _pipeline(),
            _program(0),
            _invalidProgram(false),
            _stateCache(State::DEFAULT),
            _stateSignatureCache(0),
            _state(nullptr),
            _invalidState(false),
            _needStateSync(true)
             {}
    } _pipeline;

    // Output stage
    void do_setFramebuffer(Batch& batch, uint32 paramOffset);

    struct OutputStageState {

        FramebufferPointer _framebuffer = nullptr;

        OutputStageState() {}
    } _output;

    // TODO: As long as we have gl calls explicitely issued from interface
    // code, we need to be able to record and batch these calls. THe long 
    // term strategy is to get rid of any GL calls in favor of the HIFI GPU API
    void do_glEnable(Batch& batch, uint32 paramOffset);
    void do_glDisable(Batch& batch, uint32 paramOffset);

    void do_glEnableClientState(Batch& batch, uint32 paramOffset);
    void do_glDisableClientState(Batch& batch, uint32 paramOffset);

    void do_glCullFace(Batch& batch, uint32 paramOffset);
    void do_glAlphaFunc(Batch& batch, uint32 paramOffset);

    void do_glDepthFunc(Batch& batch, uint32 paramOffset);
    void do_glDepthMask(Batch& batch, uint32 paramOffset);
    void do_glDepthRange(Batch& batch, uint32 paramOffset);

    void do_glBindBuffer(Batch& batch, uint32 paramOffset);

    void do_glBindTexture(Batch& batch, uint32 paramOffset);
    void do_glActiveTexture(Batch& batch, uint32 paramOffset);

    void do_glDrawBuffers(Batch& batch, uint32 paramOffset);

    void do_glUseProgram(Batch& batch, uint32 paramOffset);
    void do_glUniform1f(Batch& batch, uint32 paramOffset);
    void do_glUniform2f(Batch& batch, uint32 paramOffset);
    void do_glUniform4fv(Batch& batch, uint32 paramOffset);
    void do_glUniformMatrix4fv(Batch& batch, uint32 paramOffset);

    void do_glEnableVertexAttribArray(Batch& batch, uint32 paramOffset);
    void do_glDisableVertexAttribArray(Batch& batch, uint32 paramOffset);

    void do_glColor4f(Batch& batch, uint32 paramOffset);

    typedef void (GLBackend::*CommandCall)(Batch&, uint32);
    static CommandCall _commandCalls[Batch::NUM_COMMANDS];

};


};

#endif
