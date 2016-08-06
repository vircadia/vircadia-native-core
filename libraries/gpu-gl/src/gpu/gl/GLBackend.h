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
#include <bitset>
#include <queue>
#include <utility>
#include <list>
#include <array>

#include <QtCore/QLoggingCategory>

#include <gl/Config.h>

#include <gpu/Forward.h>
#include <gpu/Context.h>

#include "GLShared.h"

namespace gpu { namespace gl {

class GLBackend : public Backend {
    // Context Backend static interface required
    friend class gpu::Context;
    static void init();
    static Backend* createBackend();

protected:
    explicit GLBackend(bool syncCache);
    GLBackend();
public:
    static bool makeProgram(Shader& shader, const Shader::BindingSet& slotBindings = Shader::BindingSet());

    ~GLBackend();

    void setCameraCorrection(const Mat4& correction);
    void render(Batch& batch) final;

    // This call synchronize the Full Backend cache with the current GLState
    // THis is only intended to be used when mixing raw gl calls with the gpu api usage in order to sync
    // the gpu::Backend state with the true gl state which has probably been messed up by these ugly naked gl calls
    // Let's try to avoid to do that as much as possible!
    void syncCache() final;

    // This is the ugly "download the pixels to sysmem for taking a snapshot"
    // Just avoid using it, it's ugly and will break performances
    virtual void downloadFramebuffer(const FramebufferPointer& srcFramebuffer, const Vec4i& region, QImage& destImage) final;


    static const int MAX_NUM_ATTRIBUTES = Stream::NUM_INPUT_SLOTS;
    static const int MAX_NUM_INPUT_BUFFERS = 16;

    size_t getNumInputBuffers() const { return _input._invalidBuffers.size(); }

    // this is the maximum per shader stage on the low end apple
    // TODO make it platform dependant at init time
    static const int MAX_NUM_UNIFORM_BUFFERS = 12;
    size_t getMaxNumUniformBuffers() const { return MAX_NUM_UNIFORM_BUFFERS; }

    // this is the maximum per shader stage on the low end apple
    // TODO make it platform dependant at init time
    static const int MAX_NUM_RESOURCE_TEXTURES = 16;
    size_t getMaxNumResourceTextures() const { return MAX_NUM_RESOURCE_TEXTURES; }

    // Draw Stage
    virtual void do_draw(Batch& batch, size_t paramOffset) = 0;
    virtual void do_drawIndexed(Batch& batch, size_t paramOffset) = 0;
    virtual void do_drawInstanced(Batch& batch, size_t paramOffset) = 0;
    virtual void do_drawIndexedInstanced(Batch& batch, size_t paramOffset) = 0;
    virtual void do_multiDrawIndirect(Batch& batch, size_t paramOffset) = 0;
    virtual void do_multiDrawIndexedIndirect(Batch& batch, size_t paramOffset) = 0;

    // Input Stage
    virtual void do_setInputFormat(Batch& batch, size_t paramOffset) final;
    virtual void do_setInputBuffer(Batch& batch, size_t paramOffset) final;
    virtual void do_setIndexBuffer(Batch& batch, size_t paramOffset) final;
    virtual void do_setIndirectBuffer(Batch& batch, size_t paramOffset) final;
    virtual void do_generateTextureMips(Batch& batch, size_t paramOffset) final;

    // Transform Stage
    virtual void do_setModelTransform(Batch& batch, size_t paramOffset) final;
    virtual void do_setViewTransform(Batch& batch, size_t paramOffset) final;
    virtual void do_setProjectionTransform(Batch& batch, size_t paramOffset) final;
    virtual void do_setViewportTransform(Batch& batch, size_t paramOffset) final;
    virtual void do_setDepthRangeTransform(Batch& batch, size_t paramOffset) final;

    // Uniform Stage
    virtual void do_setUniformBuffer(Batch& batch, size_t paramOffset) final;

    // Resource Stage
    virtual void do_setResourceTexture(Batch& batch, size_t paramOffset) final;

    // Pipeline Stage
    virtual void do_setPipeline(Batch& batch, size_t paramOffset) final;

    // Output stage
    virtual void do_setFramebuffer(Batch& batch, size_t paramOffset) final;
    virtual void do_clearFramebuffer(Batch& batch, size_t paramOffset) final;
    virtual void do_blit(Batch& batch, size_t paramOffset) = 0;

    // Query section
    virtual void do_beginQuery(Batch& batch, size_t paramOffset) final;
    virtual void do_endQuery(Batch& batch, size_t paramOffset) final;
    virtual void do_getQuery(Batch& batch, size_t paramOffset) final;

    // Reset stages
    virtual void do_resetStages(Batch& batch, size_t paramOffset) final;

    virtual void do_runLambda(Batch& batch, size_t paramOffset) final;

    virtual void do_startNamedCall(Batch& batch, size_t paramOffset) final;
    virtual void do_stopNamedCall(Batch& batch, size_t paramOffset) final;

    virtual void do_pushProfileRange(Batch& batch, size_t paramOffset) final;
    virtual void do_popProfileRange(Batch& batch, size_t paramOffset) final;

    // TODO: As long as we have gl calls explicitely issued from interface
    // code, we need to be able to record and batch these calls. THe long 
    // term strategy is to get rid of any GL calls in favor of the HIFI GPU API
    virtual void do_glActiveBindTexture(Batch& batch, size_t paramOffset) final;

    virtual void do_glUniform1i(Batch& batch, size_t paramOffset) final;
    virtual void do_glUniform1f(Batch& batch, size_t paramOffset) final;
    virtual void do_glUniform2f(Batch& batch, size_t paramOffset) final;
    virtual void do_glUniform3f(Batch& batch, size_t paramOffset) final;
    virtual void do_glUniform4f(Batch& batch, size_t paramOffset) final;
    virtual void do_glUniform3fv(Batch& batch, size_t paramOffset) final;
    virtual void do_glUniform4fv(Batch& batch, size_t paramOffset) final;
    virtual void do_glUniform4iv(Batch& batch, size_t paramOffset) final;
    virtual void do_glUniformMatrix3fv(Batch& batch, size_t paramOffset) final;
    virtual void do_glUniformMatrix4fv(Batch& batch, size_t paramOffset) final;

    virtual void do_glColor4f(Batch& batch, size_t paramOffset) final;

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
    virtual void do_setStateStencil(State::StencilActivation activation, State::StencilTest frontTest, State::StencilTest backTest) final;
    virtual void do_setStateAlphaToCoverageEnable(bool enable) final;
    virtual void do_setStateSampleMask(uint32 mask) final;
    virtual void do_setStateBlend(State::BlendFunction blendFunction) final;
    virtual void do_setStateColorWriteMask(uint32 mask) final;
    virtual void do_setStateBlendFactor(Batch& batch, size_t paramOffset) final;
    virtual void do_setStateScissorRect(Batch& batch, size_t paramOffset) final;

    virtual GLuint getFramebufferID(const FramebufferPointer& framebuffer) const = 0;
    virtual GLuint getTextureID(const TexturePointer& texture, bool needTransfer = true) const = 0;
    virtual bool isTextureReady(const TexturePointer& texture) const;

    virtual void releaseBuffer(GLuint id, Size size) const;
    virtual void releaseTexture(GLuint id, Size size) const;
    virtual void releaseFramebuffer(GLuint id) const;
    virtual void releaseShader(GLuint id) const;
    virtual void releaseProgram(GLuint id) const;
    virtual void releaseQuery(GLuint id) const;
    void cleanupTrash() const;

protected:

    virtual GLFramebuffer* syncGPUObject(const Framebuffer& framebuffer) const = 0;

    virtual GLuint getBufferID(const Buffer& buffer) const = 0;
    virtual GLBuffer* syncGPUObject(const Buffer& buffer) const = 0;

    virtual GLTexture* syncGPUObject(const TexturePointer& texture, bool sync = true) const = 0;

    virtual GLuint getQueryID(const QueryPointer& query) const = 0;
    virtual GLQuery* syncGPUObject(const Query& query) const = 0;

    static const size_t INVALID_OFFSET = (size_t)-1;
    bool _inRenderTransferPass { false };
    int32_t _uboAlignment { 0 };
    int _currentDraw { -1 };

    std::list<std::string> profileRanges;
    mutable Mutex _trashMutex;
    mutable std::list<std::pair<GLuint, Size>> _buffersTrash;
    mutable std::list<std::pair<GLuint, Size>> _texturesTrash;
    mutable std::list<GLuint> _framebuffersTrash;
    mutable std::list<GLuint> _shadersTrash;
    mutable std::list<GLuint> _programsTrash;
    mutable std::list<GLuint> _queriesTrash;


    void renderPassTransfer(Batch& batch);
    void renderPassDraw(Batch& batch);
    void setupStereoSide(int side);

    virtual void initInput() final;
    virtual void killInput() final;
    virtual void syncInputStateCache() final;
    virtual void resetInputStage() final;
    virtual void updateInput();

    struct InputStageState {
        bool _invalidFormat { true };
        Stream::FormatPointer _format;

        typedef std::bitset<MAX_NUM_ATTRIBUTES> ActivationCache;
        ActivationCache _attributeActivation { 0 };

        typedef std::bitset<MAX_NUM_INPUT_BUFFERS> BuffersState;
        BuffersState _invalidBuffers { 0 };

        Buffers _buffers;
        Offsets _bufferOffsets;
        Offsets _bufferStrides;
        std::vector<GLuint> _bufferVBOs;

        glm::vec4 _colorAttribute{ 0.0f };

        BufferPointer _indexBuffer;
        Offset _indexBufferOffset { 0 };
        Type _indexBufferType { UINT32 };
        
        BufferPointer _indirectBuffer;
        Offset _indirectBufferOffset{ 0 };
        Offset _indirectBufferStride{ 0 };

        GLuint _defaultVAO { 0 };

        InputStageState() :
            _buffers(_invalidBuffers.size()),
            _bufferOffsets(_invalidBuffers.size(), 0),
            _bufferStrides(_invalidBuffers.size(), 0),
            _bufferVBOs(_invalidBuffers.size(), 0) {}
    } _input;

    virtual void initTransform() = 0;
    void killTransform();
    // Synchronize the state cache of this Backend with the actual real state of the GL Context
    void syncTransformStateCache();
    void updateTransform(const Batch& batch);
    void resetTransformStage();

    struct CameraCorrection {
        Mat4 _correction;
        Mat4 _correctionInverse;
    };

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
        bool _viewIsCamera{ false };
        bool _skybox { false };
        Transform _view;
        CameraCorrection _correction;

        Mat4 _projection;
        Vec4i _viewport { 0, 0, 1, 1 };
        Vec2 _depthRange { 0.0f, 1.0f };
        bool _invalidView { false };
        bool _invalidProj { false };
        bool _invalidViewport { false };

        using Pair = std::pair<size_t, size_t>;
        using List = std::list<Pair>;
        List _cameraOffsets;
        mutable List::const_iterator _camerasItr;
        mutable size_t _currentCameraOffset{ INVALID_OFFSET };

        void preUpdate(size_t commandIndex, const StereoState& stereo);
        void update(size_t commandIndex, const StereoState& stereo) const;
        void bindCurrentCamera(int stereoSide) const;
    } _transform;

    virtual void transferTransformState(const Batch& batch) const = 0;

    struct UniformStageState {
        std::array<BufferPointer, MAX_NUM_UNIFORM_BUFFERS> _buffers;
        //Buffers _buffers {  };
    } _uniform;

    void releaseUniformBuffer(uint32_t slot);
    void resetUniformStage();
    
    // update resource cache and do the gl unbind call with the current gpu::Texture cached at slot s
    void releaseResourceTexture(uint32_t slot);

    void resetResourceStage();

    struct ResourceStageState {
        std::array<TexturePointer, MAX_NUM_RESOURCE_TEXTURES> _textures;
        //Textures _textures { { MAX_NUM_RESOURCE_TEXTURES } };
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
        PipelinePointer _pipeline;

        GLuint _program { 0 };
        GLint _cameraCorrectionLocation { -1 };
        GLShader* _programShader { nullptr };
        bool _invalidProgram { false };

        BufferView _cameraCorrectionBuffer { gpu::BufferView(std::make_shared<gpu::Buffer>(sizeof(CameraCorrection), nullptr )) };

        State::Data _stateCache{ State::DEFAULT };
        State::Signature _stateSignatureCache { 0 };

        GLState* _state { nullptr };
        bool _invalidState { false };

        PipelineStageState() {
            _cameraCorrectionBuffer.edit<CameraCorrection>() = CameraCorrection();
            _cameraCorrectionBuffer._buffer->flush();
        }
    } _pipeline;

    // Synchronize the state cache of this Backend with the actual real state of the GL Context
    void syncOutputStateCache();
    void resetOutputStage();
    
    struct OutputStageState {
        FramebufferPointer _framebuffer { nullptr };
        GLuint _drawFBO { 0 };
    } _output;

    void resetQueryStage();
    struct QueryStageState {
        
    };

    void resetStages();

    typedef void (GLBackend::*CommandCall)(Batch&, size_t);
    static CommandCall _commandCalls[Batch::NUM_COMMANDS];
    friend class GLState;
};

} }

#endif
