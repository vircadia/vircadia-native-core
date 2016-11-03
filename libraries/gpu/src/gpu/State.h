//
//  State
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
#include <vector>
#include <unordered_map>
#include <bitset>

// Why a macro and not a fancy template you will ask me ?
// Because some of the fields are bool packed tightly in the State::Cache class
// and it s just not good anymore for template T& variable manipulation...
#define SET_FIELD(field, defaultValue, value, dest) {\
    dest = value;\
    if (value == defaultValue) {\
        _signature.reset(field);\
    } else {\
        _signature.set(field);\
    }\
    _stamp++;\
}\


namespace gpu {

class GPUObject;

class State {
public:
    State();
    virtual ~State();

    Stamp getStamp() const { return _stamp; }

    typedef ::gpu::ComparisonFunction ComparisonFunction;

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
        uint8 _writeMask = true;
        uint8 _enabled = false;
        uint8 _spare = 0;
    public:
        DepthTest(bool enabled = false, bool writeMask = true, ComparisonFunction func = LESS) :
            _function(func), _writeMask(writeMask), _enabled(enabled) {}

        bool isEnabled() const { return _enabled != 0; }
        ComparisonFunction getFunction() const { return ComparisonFunction(_function); }
        uint8 getWriteMask() const { return _writeMask; }

        int32 getRaw() const { return *(reinterpret_cast<const int32*>(this)); }
        DepthTest(int32 raw) { *(reinterpret_cast<int32*>(this)) = raw; }
        bool operator== (const DepthTest& right) const { return getRaw() == right.getRaw(); }
        bool operator!= (const DepthTest& right) const { return getRaw() != right.getRaw(); }
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
        int8 _reference = 0;
        uint8 _readMask = 0xff;
     public:

        StencilTest(int8 reference = 0, uint8 readMask =0xFF, ComparisonFunction func = ALWAYS, StencilOp failOp = STENCIL_OP_KEEP, StencilOp depthFailOp = STENCIL_OP_KEEP, StencilOp passOp = STENCIL_OP_KEEP) :
             _functionAndOperations(func | (failOp << FAIL_OP_OFFSET) | (depthFailOp << DEPTH_FAIL_OP_OFFSET) | (passOp << PASS_OP_OFFSET)),
            _reference(reference), _readMask(readMask)
            {}

        ComparisonFunction getFunction() const { return ComparisonFunction(_functionAndOperations & FUNC_MASK); }
        StencilOp getFailOp() const { return StencilOp((_functionAndOperations & FAIL_OP_MASK) >> FAIL_OP_OFFSET); }
        StencilOp getDepthFailOp() const { return StencilOp((_functionAndOperations & DEPTH_FAIL_OP_MASK) >> DEPTH_FAIL_OP_OFFSET); }
        StencilOp getPassOp() const { return StencilOp((_functionAndOperations & PASS_OP_MASK) >> PASS_OP_OFFSET); }

        int8 getReference() const { return _reference; }
        uint8 getReadMask() const { return _readMask; }

        int32 getRaw() const { return *(reinterpret_cast<const int32*>(this)); }
        StencilTest(int32 raw) { *(reinterpret_cast<int32*>(this)) = raw; }
        bool operator== (const StencilTest& right) const { return getRaw() == right.getRaw(); }
        bool operator!= (const StencilTest& right) const { return getRaw() != right.getRaw(); }
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
        bool operator== (const StencilActivation& right) const { return getRaw() == right.getRaw(); }
        bool operator!= (const StencilActivation& right) const { return getRaw() != right.getRaw(); }
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
        bool operator== (const BlendFunction& right) const { return getRaw() == right.getRaw(); }
        bool operator!= (const BlendFunction& right) const { return getRaw() != right.getRaw(); }
    };

    // The Data class is the full explicit description of the State class fields value.
    // Useful for having one const static called Default for reference or for the gpu::Backend to keep track of the current value
    class Data {
    public:
        float depthBias = 0.0f;
        float depthBiasSlopeScale = 0.0f;

        DepthTest depthTest = DepthTest(false, true, LESS);

        StencilActivation stencilActivation = StencilActivation(false);
        StencilTest stencilTestFront = StencilTest(0, 0xff, ALWAYS, STENCIL_OP_KEEP, STENCIL_OP_KEEP, STENCIL_OP_KEEP);
        StencilTest stencilTestBack = StencilTest(0, 0xff, ALWAYS, STENCIL_OP_KEEP, STENCIL_OP_KEEP, STENCIL_OP_KEEP);

        uint32 sampleMask = 0xFFFFFFFF;

        BlendFunction blendFunction = BlendFunction(false);

        uint8 fillMode = FILL_FACE;
        uint8 cullMode = CULL_NONE;

        uint8 colorWriteMask = WRITE_ALL;

        bool frontFaceClockwise : 1;
        bool depthClampEnable : 1;
        bool scissorEnable : 1;
        bool multisampleEnable : 1;
        bool antialisedLineEnable : 1;
        bool alphaToCoverageEnable : 1;

        Data() :
            frontFaceClockwise(false),
            depthClampEnable(false),
            scissorEnable(false),
            multisampleEnable(false),
            antialisedLineEnable(true),
            alphaToCoverageEnable(false)
        {}
    };

    // The unique default values for all the fields
    static const Data DEFAULT;
    void setFillMode(FillMode fill) { SET_FIELD(FILL_MODE, DEFAULT.fillMode, fill, _values.fillMode); }
    FillMode getFillMode() const { return FillMode(_values.fillMode); }

    void setCullMode(CullMode cull)  { SET_FIELD(CULL_MODE, DEFAULT.cullMode, cull, _values.cullMode); }
    CullMode getCullMode() const { return CullMode(_values.cullMode); }

    void setFrontFaceClockwise(bool isClockwise) { SET_FIELD(FRONT_FACE_CLOCKWISE, DEFAULT.frontFaceClockwise, isClockwise, _values.frontFaceClockwise); }
    bool isFrontFaceClockwise() const { return _values.frontFaceClockwise; }

    void setDepthClampEnable(bool enable) { SET_FIELD(DEPTH_CLAMP_ENABLE, DEFAULT.depthClampEnable, enable, _values.depthClampEnable); }
    bool isDepthClampEnable() const { return _values.depthClampEnable; }

    void setScissorEnable(bool enable) { SET_FIELD(SCISSOR_ENABLE, DEFAULT.scissorEnable, enable, _values.scissorEnable); }
    bool isScissorEnable() const { return _values.scissorEnable; }

    void setMultisampleEnable(bool enable) { SET_FIELD(MULTISAMPLE_ENABLE, DEFAULT.multisampleEnable, enable, _values.multisampleEnable); }
    bool isMultisampleEnable() const { return _values.multisampleEnable; }

    void setAntialiasedLineEnable(bool enable) { SET_FIELD(ANTIALISED_LINE_ENABLE, DEFAULT.antialisedLineEnable, enable, _values.antialisedLineEnable); }
    bool isAntialiasedLineEnable() const { return _values.antialisedLineEnable; }

    // Depth Bias
    void setDepthBias(float bias) { SET_FIELD(DEPTH_BIAS, DEFAULT.depthBias, bias, _values.depthBias); }
    float getDepthBias() const { return _values.depthBias; }

    void setDepthBiasSlopeScale(float scale) { SET_FIELD(DEPTH_BIAS_SLOPE_SCALE, DEFAULT.depthBiasSlopeScale, scale, _values.depthBiasSlopeScale); }
    float getDepthBiasSlopeScale() const { return _values.depthBiasSlopeScale; }

    // Depth Test
    void setDepthTest(DepthTest depthTest) { SET_FIELD(DEPTH_TEST, DEFAULT.depthTest, depthTest, _values.depthTest); }
    void setDepthTest(bool enable, bool writeMask, ComparisonFunction func) { setDepthTest(DepthTest(enable, writeMask, func)); }
    DepthTest getDepthTest() const { return _values.depthTest; }

    bool isDepthTestEnabled() const { return getDepthTest().isEnabled(); }
    uint8 getDepthTestWriteMask() const { return getDepthTest().getWriteMask(); }
    ComparisonFunction getDepthTestFunc() const { return getDepthTest().getFunction(); }

    // Stencil test
    void setStencilTest(bool enabled, uint8 frontWriteMask, StencilTest frontTest, uint8 backWriteMask, StencilTest backTest) {
        SET_FIELD(STENCIL_ACTIVATION, DEFAULT.stencilActivation, StencilActivation(enabled, frontWriteMask, backWriteMask), _values.stencilActivation);
        SET_FIELD(STENCIL_TEST_FRONT, DEFAULT.stencilTestFront, frontTest, _values.stencilTestFront);
        SET_FIELD(STENCIL_TEST_BACK, DEFAULT.stencilTestBack, backTest, _values.stencilTestBack); }
    void setStencilTest(bool enabled, uint8 frontWriteMask, StencilTest frontTest) {
        setStencilTest(enabled, frontWriteMask, frontTest, frontWriteMask, frontTest); }

    StencilActivation getStencilActivation() const { return _values.stencilActivation; }
    StencilTest getStencilTestFront() const { return _values.stencilTestFront; }
    StencilTest getStencilTestBack() const { return _values.stencilTestBack; }

    bool isStencilEnabled() const { return getStencilActivation().isEnabled(); }
    uint8 getStencilWriteMaskFront() const { return getStencilActivation().getWriteMaskFront(); }
    uint8 getStencilWriteMaskBack() const { return getStencilActivation().getWriteMaskBack(); }

    // Alpha to coverage
    void setAlphaToCoverageEnable(bool enable) { SET_FIELD(ALPHA_TO_COVERAGE_ENABLE, DEFAULT.alphaToCoverageEnable, enable, _values.alphaToCoverageEnable); }
    bool isAlphaToCoverageEnabled() const { return _values.alphaToCoverageEnable; }

    // Sample mask
    void setSampleMask(uint32 mask) { SET_FIELD(SAMPLE_MASK, DEFAULT.sampleMask, mask, _values.sampleMask); }
    uint32 getSampleMask() const { return _values.sampleMask; }

    // Blend Function
    void setBlendFunction(BlendFunction function) { SET_FIELD(BLEND_FUNCTION, DEFAULT.blendFunction, function, _values.blendFunction); }
    BlendFunction getBlendFunction() const { return _values.blendFunction; }

    void setBlendFunction(bool enabled, BlendArg sourceColor, BlendOp operationColor, BlendArg destinationColor, BlendArg sourceAlpha, BlendOp operationAlpha, BlendArg destinationAlpha) {
        setBlendFunction(BlendFunction(enabled, sourceColor, operationColor, destinationColor, sourceAlpha, operationAlpha, destinationAlpha)); }
    void setBlendFunction(bool enabled, BlendArg source, BlendOp operation, BlendArg destination) {
        setBlendFunction(BlendFunction(enabled, source, operation, destination)); }

    bool isBlendEnabled() const { return getBlendFunction().isEnabled(); }

    // Color write mask
    void setColorWriteMask(uint8 mask) { SET_FIELD(COLOR_WRITE_MASK, DEFAULT.colorWriteMask, mask, _values.colorWriteMask); }
    void setColorWriteMask(bool red, bool green, bool blue, bool alpha) { uint32 value = ((WRITE_RED * red) | (WRITE_GREEN * green) | (WRITE_BLUE * blue) | (WRITE_ALPHA * alpha)); SET_FIELD(COLOR_WRITE_MASK, DEFAULT.colorWriteMask, value, _values.colorWriteMask); }
    uint8 getColorWriteMask() const { return _values.colorWriteMask; }

    // All the possible fields
    // NOTE: If you change this, you must update GLBackend::GLState::_resetStateCommands
    enum Field {
        FILL_MODE,
        CULL_MODE,
        FRONT_FACE_CLOCKWISE,
        DEPTH_CLAMP_ENABLE,
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

        COLOR_WRITE_MASK,

        NUM_FIELDS, // not a valid field, just the count
    };

    // The signature of the state tells which fields of the state are not default
    // this way during rendering the Backend can compare it's current state and try to minimize the job to do
    typedef std::bitset<NUM_FIELDS> Signature;

    Signature getSignature() const { return _signature; }

    static Signature evalSignature(const Data& state);

    // For convenience, create a State from the values directly
    State(const Data& values);
    const Data& getValues() const { return _values; }

    const GPUObjectPointer gpuObject {};

protected:
    State(const State& state);
    State& operator=(const State& state);

    Data _values;
    Signature _signature{0};
    Stamp _stamp{0};
};

typedef std::shared_ptr< State > StatePointer;
typedef std::vector< StatePointer > States;

};

#endif
