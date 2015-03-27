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
    State();
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
        uint8 _function = LESS;
        bool _writeMask = true;
        bool _enabled = false;
    public:
        DepthTest(bool enabled, bool writeMask, ComparisonFunction func) : 
            _function(func), _writeMask(writeMask), _enabled(enabled) {}

        bool isEnabled() const { return _enabled; }
        ComparisonFunction getFunction() const { return ComparisonFunction(_function); }
        bool getWriteMask() const { return _writeMask; }

        int32 getRaw() const { return *(reinterpret_cast<const int32*>(this)); }
        DepthTest(int32 raw) { *(reinterpret_cast<int32*>(this)) = raw; }
   };

     class StencilTest {
        static const int FUNC_MASK = 0x000f;
        static const int FAIL_OP_MASK = 0x00f0;
        static const int DEPTH_FAIL_OP_MASK = 0x0f00;
        static const int PASS_OP_MASK = 0xf000;
        static const int FAIL_OP_OFFSET = 4;
        static const int DEPTH_FAIL_OP_OFFSET = 8;
        static const int PASS_OP_OFFSET = 12;

        uint16 _functionAndOperations;
        uint8 _reference = 0;
        uint8 _readMask = 0xff;
     public:

        StencilTest(uint8 reference = 0, uint8 readMask =0xFF, ComparisonFunction func = ALWAYS, StencilOp failOp = STENCIL_OP_KEEP, StencilOp depthFailOp = STENCIL_OP_KEEP, StencilOp passOp = STENCIL_OP_KEEP) :
            _reference(reference), _readMask(readMask),
            _functionAndOperations(func | (failOp << FAIL_OP_OFFSET) | (depthFailOp << DEPTH_FAIL_OP_OFFSET) | (passOp << PASS_OP_OFFSET)) {}

        ComparisonFunction getFunction() const { return ComparisonFunction(_functionAndOperations & FUNC_MASK); }
        StencilOp getFailOp() const { return StencilOp((_functionAndOperations & FAIL_OP_MASK) >> FAIL_OP_OFFSET); }
        StencilOp getDepthFailOp() const { return StencilOp((_functionAndOperations & DEPTH_FAIL_OP_MASK) >> DEPTH_FAIL_OP_OFFSET); }
        StencilOp getPassOp() const { return StencilOp((_functionAndOperations & PASS_OP_MASK) >> PASS_OP_OFFSET); }

        uint8 getReference() const { return _reference; }
        uint8 getReadMask() const { return _readMask; }

        int32 getRaw() const { return *(reinterpret_cast<const int32*>(this)); }
        StencilTest(int32 raw) { *(reinterpret_cast<int32*>(this)) = raw; }
     };

     class StencilActivation {
        uint8 _frontWriteMask = 0xFF;
        uint8 _backWriteMask = 0xFF;
        uint16 _enabled = 0;
     public:

        StencilActivation(bool enabled, uint8 frontWriteMask = 0xFF, uint8 backWriteMask = 0xFF) :
            _frontWriteMask(frontWriteMask), _backWriteMask(backWriteMask), _enabled(enabled) {}

        bool isEnabled() const { return (_enabled != 0); }
        uint8 getWriteMaskFront() const { return _frontWriteMask; }
        uint8 getWriteMaskBack() const { return _backWriteMask; }

        int32 getRaw() const { return *(reinterpret_cast<const int32*>(this)); }
        StencilActivation(int32 raw) { *(reinterpret_cast<int32*>(this)) = raw; }
    };


    class BlendFunction {
        static const int COLOR_MASK = 0x0f;
        static const int ALPHA_MASK = 0xf0;
        static const int ALPHA_OFFSET = 4;

        uint8 _enabled;
        uint8 _source;
        uint8 _destination;
        uint8 _operation;
    public:

        BlendFunction(bool enabled,
            BlendArg sourceColor, BlendOp operationColor, BlendArg destinationColor,
            BlendArg sourceAlpha, BlendOp operationAlpha, BlendArg destinationAlpha) :
            _enabled(enabled),
            _source(sourceColor | (sourceAlpha << ALPHA_OFFSET)),
            _destination(destinationColor | (destinationAlpha << ALPHA_OFFSET)),
            _operation(operationColor | (operationAlpha << ALPHA_OFFSET)) {}

        BlendFunction(bool enabled, BlendArg source = ONE, BlendOp operation = BLEND_OP_ADD, BlendArg destination = ZERO) :
            _enabled(enabled),
            _source(source | (source << ALPHA_OFFSET)),
            _destination(destination | (destination << ALPHA_OFFSET)),
            _operation(operation | (operation << ALPHA_OFFSET)) {}

        bool isEnabled() const { return (_enabled != 0); }

        BlendArg getSourceColor() const { return BlendArg(_source & COLOR_MASK); }
        BlendArg getDestinationColor() const { return BlendArg(_destination & COLOR_MASK); }
        BlendOp getOperationColor() const { return BlendOp(_operation & COLOR_MASK); }

        BlendArg getSourceAlpha() const { return BlendArg((_source & ALPHA_MASK) >> ALPHA_OFFSET); }
        BlendArg getDestinationAlpha() const { return BlendArg((_destination & ALPHA_MASK) >> ALPHA_OFFSET); }
        BlendOp getOperationAlpha() const { return BlendOp((_operation & ALPHA_MASK) >> ALPHA_OFFSET); }

        int32 getRaw() const { return *(reinterpret_cast<const int32*>(this)); }
        BlendFunction(int32 raw) { *(reinterpret_cast<int32*>(this)) = raw; }
    };

    class Value {
    public:
        union {
            uint32 _unsigned_integer = 0;
            int32 _integer;
            float _float;
        };

        template <typename T> void uncast(T v) { _integer = v; }
        template <> void State::Value::uncast<int>(int v)  { _integer = v; }
        template <> void State::Value::uncast<float>(float v)  { _float = v; }
        template <> void State::Value::uncast<unsigned int>(unsigned int v)  { _unsigned_integer = v; }
        template <> void State::Value::uncast<DepthTest>(DepthTest v) { _integer = v.getRaw(); }
        template <> void State::Value::uncast<StencilActivation>(StencilActivation v) { _integer = v.getRaw(); }
        template <> void State::Value::uncast<StencilTest>(StencilTest v) { _integer = v.getRaw(); }
        template <> void State::Value::uncast<BlendFunction>(BlendFunction v) { _integer = v.getRaw(); }

        template <typename T> T cast() const;
        template <> int State::Value::cast<int>() const { return _integer; }
        template <> float State::Value::cast<float>() const { return _float; }
        template <> unsigned int State::Value::cast<unsigned int>() const { return _unsigned_integer; }
        template <> DepthTest State::Value::cast<DepthTest>() const { return DepthTest(_integer); }
        template <> StencilActivation State::Value::cast<StencilActivation>() const { return StencilActivation(_integer); }
        template <> StencilTest State::Value::cast<StencilTest>() const { return StencilTest(_integer); }
        template <> BlendFunction State::Value::cast<BlendFunction>() const { return BlendFunction(_integer); }
    };
    typedef std::unordered_map<Field, Value> FieldMap; 

    const FieldMap& getFields() const { return _fields; }

    template <class T> void set(Field field, T value) {
        _fields[field].uncast(value);
        _stamp++;
    }

    template <class T> T get(Field field, T defaultValue) const {
        auto found = _fields.find(field);
        if (found != _fields.end()) {
            return (*found).second.cast<T>();
        }
        return defaultValue;
    }

    void setFillMode(FillMode fill) { set(FILL_MODE, fill); }
    FillMode getFillMode() const { return get(FILL_MODE, FILL_FACE); }
 
    void setCullMode(CullMode cull)  { set(CULL_MODE, cull); }
    CullMode getCullMode() const { return get(CULL_MODE, CULL_NONE); }

    void setFrontFace(bool isClockwise) { set(FRONT_FACE, isClockwise); }
    bool isFrontFaceClockwise() const { return get(FRONT_FACE, true); }

    void setDepthClipEnable(bool enable) { set(DEPTH_CLIP_ENABLE, enable); }
    bool isDepthClipEnable() const { return get(DEPTH_CLIP_ENABLE, false); }

    void setScissorEnable(bool enable) { set(SCISSOR_ENABLE, enable); }
    bool isScissorEnable() const { return get(SCISSOR_ENABLE, false); }

    void setMultisampleEnable(bool enable) { set(MULTISAMPLE_ENABLE, enable); }
    bool isMultisampleEnable() const { return get(MULTISAMPLE_ENABLE, false); }

    void setAntialiasedLineEnable(bool enable) { set(ANTIALISED_LINE_ENABLE, enable); }
    bool isAntialiasedLineEnable() const { return get(ANTIALISED_LINE_ENABLE, true); }

    // Depth Bias
    void setDepthBias(float bias) { set(DEPTH_BIAS, bias); }
    void setDepthBiasSlopeScale(float scale) { set(DEPTH_BIAS_SLOPE_SCALE, scale); }
    float getDepthBias() const { return get(DEPTH_BIAS, 0.0f); }
    float getDepthBiasSlopeScale() const { return get(DEPTH_BIAS_SLOPE_SCALE, 1.0f); }

    // Depth Test
    void setDepthTest(DepthTest depthTest) { set(DEPTH_TEST, depthTest); }
    void setDepthTest(bool enable, bool writeMask, ComparisonFunction func) { setDepthTest(DepthTest(enable, writeMask, func)); }
    DepthTest getDepthTest() const { return get(DEPTH_TEST, DepthTest(false, true, LESS)); }

    bool isDepthTestEnabled() const { return getDepthTest().isEnabled(); }
    bool getDepthTestWriteMask() const { return getDepthTest().getWriteMask(); }
    ComparisonFunction getDepthTestFunc() const { return getDepthTest().getFunction(); }
 
    // Stencil test
    void setStencilTest(bool enabled, uint8 frontWriteMask, StencilTest frontTest, uint8 backWriteMask, StencilTest backTest) {
        set(STENCIL_ACTIVATION, StencilActivation(enabled, frontWriteMask, backWriteMask));
        set(STENCIL_TEST_FRONT, frontTest);
        set(STENCIL_TEST_BACK, backTest); }
    void setStencilTest(bool enabled, uint8 frontWriteMask, StencilTest frontTest) {
        setStencilTest(enabled, frontWriteMask, frontTest, frontWriteMask, frontTest); }

    StencilActivation getStencilActivation() const { return get(STENCIL_ACTIVATION, StencilActivation(false)); }
    bool isStencilEnabled() const { return getStencilActivation().isEnabled(); }
    uint8 getStencilWriteMaskFront() const { return getStencilActivation().getWriteMaskFront(); }
    uint8 getStencilWriteMaskBack() const { return getStencilActivation().getWriteMaskBack(); }

    StencilTest getStencilTestFront() const { return get(STENCIL_TEST_FRONT, StencilTest()); }
    StencilTest getStencilTestBack() const { return get(STENCIL_TEST_BACK, StencilTest()); }

    // Alpha to coverage
    void setAlphaToCoverageEnable(bool enable) { set(ALPHA_TO_COVERAGE_ENABLE, enable); }
    bool isAlphaToCoverageEnabled() const { return get(ALPHA_TO_COVERAGE_ENABLE, false); }

    // Sample mask
    void setSampleMask(uint32 mask) { set(SAMPLE_MASK, mask); }
    uint32 getSampleMask() const { return get<uint32>(SAMPLE_MASK, 0xFFFFFFFF); }
 
    // Blend
    void setBlendFunction(BlendFunction function) { set(BLEND_FUNCTION, function); }
    void setBlendFunction(bool enabled, BlendArg sourceColor, BlendOp operationColor, BlendArg destinationColor, BlendArg sourceAlpha, BlendOp operationAlpha, BlendArg destinationAlpha) {
         setBlendFunction(BlendFunction(enabled, sourceColor, operationColor, destinationColor, sourceAlpha, operationAlpha, destinationAlpha)); }
    void setBlendFunction(bool enabled, BlendArg source, BlendOp operation, BlendArg destination) { 
        setBlendFunction(BlendFunction(enabled, source, operation, destination)); }
    BlendFunction getBlendFunction() const { return get(BLEND_FUNCTION, BlendFunction(false)); }

    bool isBlendEnabled() const { return getBlendFunction().isEnabled(); }

    void setBlendFactor(const Vec4& factor) {
        set(BLEND_FACTOR_X, factor.x);
        set(BLEND_FACTOR_Y, factor.y);
        set(BLEND_FACTOR_Z, factor.z);
        set(BLEND_FACTOR_W, factor.w); }
    Vec4 State::getBlendFactor() const {
        return Vec4(get(BLEND_FACTOR_X, 0.0f), get(BLEND_FACTOR_Y, 0.0f), get(BLEND_FACTOR_Z, 0.0f), get(BLEND_FACTOR_W, 0.0f));
    }

    // Color write mask
    void setColorWriteMask(int32 mask) { set(COLOR_WRITE_MASK, mask); }
    uint32 getColorWriteMask() const { return get<uint32>(COLOR_WRITE_MASK, WRITE_ALL); }

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
