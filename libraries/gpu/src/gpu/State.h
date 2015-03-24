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

        DEPTH_BIAS,
        DEPTH_BIAS_SLOPE_SCALE,

        FRONT_CLOCKWISE,
        DEPTH_CLIP_ENABLE,
        SCISSOR_ENABLE,
        MULTISAMPLE_ENABLE,
        ANTIALISED_LINE_ENABLE,

        DEPTH_ENABLE,
        DEPTH_WRITE_MASK,
        DEPTH_FUNC,

        STENCIL_ENABLE,
        STENCIL_READ_MASK,
        STENCIL_WRITE_MASK,
        STENCIL_TEST_FRONT,
        STENCIL_TEST_BACK,
        STENCIL_REFERENCE,

        SAMPLE_MASK,
        ALPHA_TO_COVERAGE_ENABLE,

        BLEND_ENABLE,
        BLEND_FUNCTION_COLOR,
        BLEND_FUNCTION_ALPHA,
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
        BLEND_FACTOR,
        INV_BLEND_FACTOR,
    
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

    class StencilTest {
    public:
        int8 _failOp = STENCIL_OP_KEEP;
        int8 _depthFailOp = STENCIL_OP_KEEP;
        int8 _passOp = STENCIL_OP_KEEP;
        int8 _function = ALWAYS;

        StencilTest(StencilOp failOp, StencilOp depthFailOp, StencilOp passOp, ComparisonFunction func) :
            _failOp(failOp), _depthFailOp(depthFailOp), _passOp(passOp), _function(func) {}

        int32 raw() const { return *(reinterpret_cast<const int32*>(this)); }
        StencilTest(int32 raw) { *(reinterpret_cast<int32*>(this)) = raw; }
    };

    class BlendFunction {
    public:
        int8 _source = ONE;
        int8 _operation = BLEND_OP_ADD;
        int8 _destination = ZERO;
        int8 _spare = 0;

        BlendFunction(BlendArg source, BlendOp operation, BlendArg destination) :
            _source(source), _operation(operation), _destination(destination) {}

        int32 raw() const { return *(reinterpret_cast<const int32*>(this)); }
        BlendFunction(int32 raw) { *(reinterpret_cast<int32*>(this)) = raw; }
    };

    class Value {
    public:
        union {
            uint32 _unsigned_integer;
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
    void setCullMode(CullMode cull)  { set(FILL_MODE, uint32(cull)); }
    FillMode getFillMode() const { return FillMode(get(FILL_MODE)._integer); }
    CullMode getCullMode() const { return CullMode(get(CULL_MODE)._integer); }

    void setDepthBias(float bias) { set(DEPTH_BIAS, bias); }
    void setDepthBiasSlopeScale(float scale) { set(DEPTH_BIAS_SLOPE_SCALE, scale); }
    float getDepthBias() const { return get(DEPTH_BIAS)._integer; }
    float getDepthBiasSlopeScale() const { return get(DEPTH_BIAS_SLOPE_SCALE)._float; }
 
    void setFrontClockwise(bool enable) { set(FRONT_CLOCKWISE, enable); }
    void setDepthClipEnable(bool enable) { set(DEPTH_CLIP_ENABLE, enable); }
    void setScissorEnable(bool enable) { set(SCISSOR_ENABLE, enable); }
    void setMultisampleEnable(bool enable) { set(MULTISAMPLE_ENABLE, enable); }
    void setAntialiasedLineEnable(bool enable) { set(ANTIALISED_LINE_ENABLE, enable); }
    bool getFrontClockwise() const { return get(FRONT_CLOCKWISE)._integer; }
    bool getDepthClipEnable() const { return get(DEPTH_CLIP_ENABLE)._integer; }
    bool getScissorEnable() const { return get(SCISSOR_ENABLE)._integer; }
    bool getMultisampleEnable() const { return get(MULTISAMPLE_ENABLE)._integer; }
    bool getAntialiasedLineEnable() const { return get(ANTIALISED_LINE_ENABLE)._integer; }

    void setDepthEnable(bool enable) { set(DEPTH_ENABLE, enable); }
    void setDepthWriteMask(bool enable)  { set(DEPTH_WRITE_MASK, enable); }
    void setDepthFunc(ComparisonFunction func)  { set(DEPTH_FUNC, func); }
    bool getDepthEnable() const { return get(DEPTH_ENABLE)._integer; }
    bool getDepthWriteMask() const { return get(DEPTH_WRITE_MASK)._integer; }
    ComparisonFunction getDepthFunc() const { return ComparisonFunction(get(DEPTH_FUNC)._integer); }

    void setStencilEnable(bool enable) { set(STENCIL_ENABLE, enable); }
    void setStencilReadMask(uint8 mask)  { set(STENCIL_READ_MASK, mask); }
    void setStencilWriteMask(uint8 mask)  { set(STENCIL_WRITE_MASK, mask); }
    bool getStencilEnable() const { return get(STENCIL_ENABLE)._integer; }
    uint8 getStencilReadMask() const { return get(STENCIL_READ_MASK)._unsigned_integer; }
    uint8 getStencilWriteMask() const { return get(STENCIL_WRITE_MASK)._unsigned_integer; }

    void setStencilTestFront(StencilTest test)  { set(STENCIL_TEST_FRONT, test.raw()); }
    void setStencilTestBack(StencilTest test)  { set(STENCIL_TEST_BACK, test.raw()); }
    StencilTest getStencilTestFront() const { return StencilTest(get(STENCIL_TEST_FRONT)._integer); }
    StencilTest getStencilTestBack() const { return StencilTest(get(STENCIL_TEST_BACK)._integer); }

    void setStencilReference(uint32 ref) { set(STENCIL_REFERENCE, ref); }
    uint32 getStencilReference() const { return get(STENCIL_REFERENCE)._unsigned_integer; }

    void setAlphaToCoverageEnable(bool enable) { set(ALPHA_TO_COVERAGE_ENABLE, enable); }
    bool getAlphaToCoverageEnable() const { return get(ALPHA_TO_COVERAGE_ENABLE)._integer; }

    void setSampleMask(uint32 mask) { set(SAMPLE_MASK, mask); }
    uint32 getSampleMask() const { return get(SAMPLE_MASK)._unsigned_integer; }
 
    void setBlendEnable(bool enable) { set(BLEND_ENABLE, enable); }
    bool getBlendEnable() const { return get(BLEND_ENABLE)._integer; }

    void setBlendFunctionColor(BlendFunction function) { set(BLEND_FUNCTION_COLOR, function.raw()); }
    BlendFunction getBlendFunctionColor() const { return BlendFunction(get(BLEND_FUNCTION_COLOR)._integer); }
     void setBlendFunctionAlpha(BlendFunction function) { set(BLEND_FUNCTION_ALPHA, function.raw()); }
    BlendFunction getBlendFunctionAlpha() const { return BlendFunction(get(BLEND_FUNCTION_ALPHA)._integer); }
 
    void setBlendFactor(const Vec4& factor) { set(BLEND_FACTOR_X, factor.x); set(BLEND_FACTOR_Y, factor.y); set(BLEND_FACTOR_Z, factor.z); set(BLEND_FACTOR_W, factor.w); }
    Vec4 getBlendFactor() const { return Vec4(get(BLEND_FACTOR_X)._float, get(BLEND_FACTOR_Y)._float, get(BLEND_FACTOR_Z)._float, get(BLEND_FACTOR_W)._float); }


    void setColorWriteMask(ColorMask mask) { set(COLOR_WRITE_MASK, mask); }
    ColorMask getColorWriteMask() const { return ColorMask(get(COLOR_WRITE_MASK)._integer); }

protected:
    State(const State& state);
    State& operator=(const State& state);

    FieldMap _fields;

    Stamp _stamp;

    // This shouldn't be used by anything else than the Backend class with the proper casting.
    mutable GPUObject* _gpuObject = NULL;
    void setGPUObject(GPUObject* gpuObject) const { _gpuObject = gpuObject; }
    GPUObject* getGPUObject() const { return _gpuObject; }
    friend class Backend;
};

typedef std::shared_ptr< State > StatePointer;
typedef std::vector< StatePointer > States;

};


#endif
