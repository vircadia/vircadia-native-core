//
//  Pipeline.h
//  libraries/gpu/src/gpu
//
//  Created by Sam Gateau on 3/8/2015.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_gpu_State_h
#define hifi_gpu_State_h

#include "Format.h"

#include <memory>
#include <unordered_map>
 

namespace gpu {

class GPUObject;

class State {
public:
    State() {}
    virtual ~State();

    const Stamp getStamp() const { return _stamp; }

    // All the possible fields
    enum Field {
        FILL_MODE,
        CULL_MODE,
        FRONT_FACE,
        DEPTH_CLIP_ENABLE,
        SCISSOR_ENABLE,
        MULTISAMPLE_ENABLE,
        ANTIALISED_LINE_ENABLE,

        DEPTH_BIAS,
        DEPTH_BIAS_SLOPE_SCALE,

        DEPTH_TEST,

        STENCIL_ACTIVATION,
        STENCIL_TEST_FRONT,
        STENCIL_TEST_BACK,

        SAMPLE_MASK,
        ALPHA_TO_COVERAGE_ENABLE,

        BLEND_FUNCTION,
        BLEND_FACTOR_X,
        BLEND_FACTOR_Y,
        BLEND_FACTOR_Z,
        BLEND_FACTOR_W,

        COLOR_WRITE_MASK,
 
        NUM_FIELDS, // not a valid field, just the count
    };

    enum ComparisonFunction {
        NEVER = 0,
        LESS,
        EQUAL,
        LESS_EQUAL,
        GREATER,
        NOT_EQUAL,
        GREATER_EQUAL,
        ALWAYS,

        NUM_COMPARISON_FUNCS,
    };

    enum FillMode {
        FILL_POINT = 0,
        FILL_LINE,
        FILL_FACE,

        NUM_FILL_MODES,
    };
 
    enum CullMode {
        CULL_NONE = 0,
        CULL_FRONT,
        CULL_BACK,

        NUM_CULL_MODES,
    };

    enum StencilOp {
        STENCIL_OP_KEEP = 0,
        STENCIL_OP_ZERO,
        STENCIL_OP_REPLACE,
        STENCIL_OP_INCR_SAT,
        STENCIL_OP_DECR_SAT,
        STENCIL_OP_INVERT,
        STENCIL_OP_INCR,
        STENCIL_OP_DECR,

        NUM_STENCIL_OPS,
    };

    enum BlendArg {
        ZERO = 0,
        ONE,
        SRC_COLOR,
        INV_SRC_COLOR,
        SRC_ALPHA,
        INV_SRC_ALPHA,
        DEST_ALPHA,
        INV_DEST_ALPHA,
        DEST_COLOR,
        INV_DEST_COLOR,
        SRC_ALPHA_SAT,
        FACTOR_COLOR,
        INV_FACTOR_COLOR,
        FACTOR_ALPHA,
        INV_FACTOR_ALPHA,

        NUM_BLEND_ARGS,
    };

    enum BlendOp {
        BLEND_OP_ADD = 0,
        BLEND_OP_SUBTRACT,
        BLEND_OP_REV_SUBTRACT,
        BLEND_OP_MIN,
        BLEND_OP_MAX,

        NUM_BLEND_OPS,
    };

    enum ColorMask
    {
        WRITE_NONE = 0,
        WRITE_RED = 1,
        WRITE_GREEN = 2,
        WRITE_BLUE = 4,
        WRITE_ALPHA = 8,
        WRITE_ALL = (WRITE_RED | WRITE_GREEN | WRITE_BLUE | WRITE_ALPHA ),
    };

    class DepthTest {
    public:
        uint8 _function = LESS;
        bool _writeMask = true;
        bool _enabled = false;

        DepthTest(bool enabled, bool writeMask, ComparisonFunction func) : 
            _function(func), _writeMask(writeMask), _enabled(enabled) {}


        int32 getRaw() const { return *(reinterpret_cast<const int32*>(this)); }
        DepthTest(int32 raw) { *(reinterpret_cast<int32*>(this)) = raw; }
   };

     class StencilTest {
     public:
        uint8 _reference;
        uint8 _readMask;
        int8 _function : 4;
        int8 _failOp : 4;
        int8 _depthFailOp : 4;
        int8 _passOp : 4;
 
        StencilTest(uint8 reference = 0, uint8 readMask =0xFF, ComparisonFunction func = ALWAYS, StencilOp failOp = STENCIL_OP_KEEP, StencilOp depthFailOp = STENCIL_OP_KEEP, StencilOp passOp = STENCIL_OP_KEEP) :
            _reference(reference), _readMask(readMask), _function(func), _failOp(failOp), _depthFailOp(depthFailOp), _passOp(passOp) {}
 
        int32 getRaw() const { return *(reinterpret_cast<const int32*>(this)); }
        StencilTest(int32 raw) { *(reinterpret_cast<int32*>(this)) = raw; }
     };

     class StencilActivation {
     public:
        uint8 _frontWriteMask;
        uint8 _backWriteMask;
        int16 _enabled;
 
        StencilActivation(bool enabled, uint8 frontWriteMask = 0xFF, uint8 backWriteMask = 0xFF) :
            _frontWriteMask(frontWriteMask), _backWriteMask(backWriteMask), _enabled(enabled) {}

        int32 getRaw() const { return *(reinterpret_cast<const int32*>(this)); }
        StencilActivation(int32 raw) { *(reinterpret_cast<int32*>(this)) = raw; }
    };


    class BlendFunction {
    public:
        int8 _enabled;
        int8 _sourceColor : 4;
        int8 _sourceAlpha : 4;
        int8 _destinationColor : 4;
        int8 _destinationAlpha : 4;
        int8 _operationColor : 4;
        int8 _operationAlpha : 4;

        BlendFunction(bool enabled,
            BlendArg sourceColor, BlendOp operationColor, BlendArg destinationColor,
            BlendArg sourceAlpha, BlendOp operationAlpha, BlendArg destinationAlpha) :
            _enabled(enabled),
            _sourceColor(sourceColor), _operationColor(operationColor), _destinationColor(destinationColor),
            _sourceAlpha(sourceAlpha), _operationAlpha(operationAlpha), _destinationAlpha(destinationAlpha) {}

        BlendFunction(bool enabled, BlendArg source, BlendOp operation, BlendArg destination) :
            _enabled(enabled),
            _sourceColor(source), _operationColor(operation), _destinationColor(destination),
            _sourceAlpha(source), _operationAlpha(operation), _destinationAlpha(destination) {}

        int32 raw() const { return *(reinterpret_cast<const int32*>(this)); }
        BlendFunction(int32 raw) { *(reinterpret_cast<int32*>(this)) = raw; }
    };

    class Value {
    public:
        union {
            uint32 _unsigned_integer = 0;
            int32 _integer;
            float _float;
        };
    };
    typedef std::unordered_map<Field, Value> FieldMap; 

    const FieldMap& getFields() const { return _fields; }

    void set(Field field, bool value);
    void set(Field field, uint32 value);
    void set(Field field, int32 value);
    void set(Field field, float value);

    Value get(Field field) const;

    void setFillMode(FillMode fill) { set(FILL_MODE, uint32(fill)); }
    FillMode getFillMode() const { return FillMode(get(FILL_MODE)._integer); }
 
    void setCullMode(CullMode cull)  { set(CULL_MODE, uint32(cull)); }
    CullMode getCullMode() const { return CullMode(get(CULL_MODE)._integer); }

    void setFrontFace(bool isClockwise) { set(FRONT_FACE, isClockwise); }
    void setDepthClipEnable(bool enable) { set(DEPTH_CLIP_ENABLE, enable); }
    void setScissorEnable(bool enable) { set(SCISSOR_ENABLE, enable); }
    void setMultisampleEnable(bool enable) { set(MULTISAMPLE_ENABLE, enable); }
    void setAntialiasedLineEnable(bool enable) { set(ANTIALISED_LINE_ENABLE, enable); }

    bool isFrontFaceClockwise() const { return get(FRONT_FACE)._integer; }
    bool isDepthClipEnable() const { return get(DEPTH_CLIP_ENABLE)._integer; }
    bool isScissorEnable() const { return get(SCISSOR_ENABLE)._integer; }
    bool isMultisampleEnable() const { return get(MULTISAMPLE_ENABLE)._integer; }
    bool isAntialiasedLineEnable() const { return get(ANTIALISED_LINE_ENABLE)._integer; }

    void setDepthBias(float bias) { set(DEPTH_BIAS, bias); }
    void setDepthBiasSlopeScale(float scale) { set(DEPTH_BIAS_SLOPE_SCALE, scale); }
    float getDepthBias() const { return get(DEPTH_BIAS)._integer; }
    float getDepthBiasSlopeScale() const { return get(DEPTH_BIAS_SLOPE_SCALE)._float; }

    void setDepthTest(bool enable, bool writeMask, ComparisonFunction func) { set(DEPTH_TEST, DepthTest(enable, writeMask, func).getRaw()); }
    DepthTest getDepthTest() const { return DepthTest(get(DEPTH_TEST)._integer); }
    bool isDepthTestEnabled() const { return getDepthTest()._enabled; }
    bool getDepthTestWriteMask() const { return getDepthTest()._writeMask; }
    ComparisonFunction getDepthTestFunc() const { return ComparisonFunction(getDepthTest()._function); }
 
    void setStencilTest(bool enabled, uint8 frontWriteMask, StencilTest frontTest, uint8 backWriteMask, StencilTest backTest) {
        set(STENCIL_ACTIVATION, StencilActivation(enabled, frontWriteMask, backWriteMask).getRaw());
        set(STENCIL_TEST_FRONT, frontTest.getRaw()); set(STENCIL_TEST_BACK, backTest.getRaw());
    }
    void setStencilTest(bool enabled, uint8 frontWriteMask, StencilTest frontTest) { setStencilTest(enabled, frontWriteMask, frontTest, frontWriteMask, frontTest); }

    StencilActivation getStencilActivation() const { return StencilActivation(get(STENCIL_ACTIVATION)._integer); }
    bool isStencilEnabled() const { return getStencilActivation()._enabled != 0; }
    uint8 getStencilWriteMaskFront() const { return getStencilActivation()._frontWriteMask; }
    uint8 getStencilWriteMaskBack() const { return getStencilActivation()._backWriteMask; }
    StencilTest getStencilTestFront() const { return StencilTest(get(STENCIL_TEST_FRONT)._integer); }
    StencilTest getStencilTestBack() const { return StencilTest(get(STENCIL_TEST_BACK)._integer); }

    void setAlphaToCoverageEnable(bool enable) { set(ALPHA_TO_COVERAGE_ENABLE, enable); }
    bool isAlphaToCoverageEnabled() const { return get(ALPHA_TO_COVERAGE_ENABLE)._integer; }

    void setSampleMask(uint32 mask) { set(SAMPLE_MASK, mask); }
    uint32 getSampleMask() const { return get(SAMPLE_MASK)._unsigned_integer; }
 
    void setBlendFunction(BlendFunction function) { set(BLEND_FUNCTION, function.raw()); }
    void setBlendFunction(bool enabled, BlendArg sourceColor, BlendOp operationColor, BlendArg destinationColor,
                          BlendArg sourceAlpha, BlendOp operationAlpha, BlendArg destinationAlpha) {
        setBlendFunction(BlendFunction(enabled, sourceColor, operationColor, destinationColor, sourceAlpha, operationAlpha, destinationAlpha)); }
    void setBlendFunction(bool enabled, BlendArg source, BlendOp operation, BlendArg destination) {
        setBlendFunction(BlendFunction(enabled, source, operation, destination)); }
    bool isBlendEnabled() const { return BlendFunction(get(BLEND_FUNCTION)._integer)._enabled; }
    BlendFunction getBlendFunction() const { return BlendFunction(get(BLEND_FUNCTION)._integer); } 
    void setBlendFactor(const Vec4& factor) { set(BLEND_FACTOR_X, factor.x); set(BLEND_FACTOR_Y, factor.y); set(BLEND_FACTOR_Z, factor.z); set(BLEND_FACTOR_W, factor.w); }
    Vec4 getBlendFactor() const { return Vec4(get(BLEND_FACTOR_X)._float, get(BLEND_FACTOR_Y)._float, get(BLEND_FACTOR_Z)._float, get(BLEND_FACTOR_W)._float); }

    void setColorWriteMask(int32 mask) { set(COLOR_WRITE_MASK, mask); }
    int32 getColorWriteMask() const { return ColorMask(get(COLOR_WRITE_MASK)._integer); }

protected:
    State(const State& state);
    State& operator=(const State& state);

    FieldMap _fields;

    Stamp _stamp = 0;

    // This shouldn't be used by anything else than the Backend class with the proper casting.
    mutable GPUObject* _gpuObject = nullptr;
    void setGPUObject(GPUObject* gpuObject) const { _gpuObject = gpuObject; }
    GPUObject* getGPUObject() const { return _gpuObject; }
    friend class Backend;
};

typedef std::shared_ptr< State > StatePointer;
typedef std::vector< StatePointer > States;

};


#endif
