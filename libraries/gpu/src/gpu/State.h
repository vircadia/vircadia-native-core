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
#include <sstream>
#include <vector>
#include <unordered_map>
#include <bitset>
#include <QString>

// Why a macro and not a fancy template you will ask me ?
// Because some of the fields are bool packed tightly in the State::Cache class
// and it s just not good anymore for template T& variable manipulation...
#define SET_FIELD(FIELD, PATH, value) \
    {                                 \
        _values.PATH = value;         \
        if (value == DEFAULT.PATH) {  \
            _signature.reset(FIELD);  \
        } else {                      \
            _signature.set(FIELD);    \
        }                             \
        _stamp++;                     \
    }

namespace gpu {

class GPUObject;

class State {
public:
    State();
    virtual ~State();

    Stamp getStamp() const { return _stamp; }

    typedef ::gpu::ComparisonFunction ComparisonFunction;

    enum FillMode
    {
        FILL_POINT = 0,
        FILL_LINE,
        FILL_FACE,

        NUM_FILL_MODES,
    };

    enum CullMode
    {
        CULL_NONE = 0,
        CULL_FRONT,
        CULL_BACK,

        NUM_CULL_MODES,
    };

    enum StencilOp
    {
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

    enum BlendArg
    {
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

    enum BlendOp
    {
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
        WRITE_ALL = (WRITE_RED | WRITE_GREEN | WRITE_BLUE | WRITE_ALPHA),
    };

    class DepthTest {
    public:
        uint8 writeMask{ true };
        uint8 enabled{ false };
        ComparisonFunction function{ LESS };

    public:
        DepthTest(bool enabled = false, bool writeMask = true, ComparisonFunction func = LESS) :
            writeMask(writeMask), enabled(enabled), function(func) {}

        bool isEnabled() const { return enabled != 0; }
        ComparisonFunction getFunction() const { return function; }
        uint8 getWriteMask() const { return writeMask; }

        bool operator==(const DepthTest& right) const {
            return writeMask == right.writeMask && enabled == right.enabled && function == right.function;
        }

        bool operator!=(const DepthTest& right) const {
            return writeMask != right.writeMask || enabled != right.enabled || function != right.function;
        }

        operator QString() const {
            return QString("{ writeMask = %1, enabled = %2, function = %3 }").arg(writeMask).arg(enabled).arg(function);

        }

    };

    struct StencilTest {
        ComparisonFunction function : 4;
        StencilOp failOp : 4;
        StencilOp depthFailOp : 4;
        StencilOp passOp : 4;
        int8 reference{ 0 };
        uint8 readMask{ 0xff };

    public:
        StencilTest(int8 reference = 0,
                    uint8 readMask = 0xFF,
                    ComparisonFunction func = ALWAYS,
                    StencilOp failOp = STENCIL_OP_KEEP,
                    StencilOp depthFailOp = STENCIL_OP_KEEP,
                    StencilOp passOp = STENCIL_OP_KEEP) :
            function(func),
            failOp(failOp), depthFailOp(depthFailOp), passOp(passOp), reference(reference), readMask(readMask) {}

        ComparisonFunction getFunction() const { return function; }
        StencilOp getFailOp() const { return failOp; }
        StencilOp getDepthFailOp() const { return depthFailOp; }
        StencilOp getPassOp() const { return passOp; }

        int8 getReference() const { return reference; }
        uint8 getReadMask() const { return readMask; }

        int32 getRaw() const { return *(reinterpret_cast<const int32*>(this)); }
        StencilTest(int32 raw) { *(reinterpret_cast<int32*>(this)) = raw; }
        bool operator==(const StencilTest& right) const { return getRaw() == right.getRaw(); }
        bool operator!=(const StencilTest& right) const { return getRaw() != right.getRaw(); }
    };
    static_assert(sizeof(StencilTest) == sizeof(uint32_t), "StencilTest size check");

    StencilTest stencilTestFront;

    struct StencilActivation {
        uint8 frontWriteMask = 0xFF;
        uint8 backWriteMask = 0xFF;
        bool enabled : 1;
        uint8 _spare1 : 7;
        uint8 _spare2{ 0 };

    public:
        StencilActivation(bool enabled = false, uint8 frontWriteMask = 0xFF, uint8 backWriteMask = 0xFF) :
            frontWriteMask(frontWriteMask), backWriteMask(backWriteMask), enabled(enabled), _spare1{ 0 } {}

        bool isEnabled() const { return enabled; }
        uint8 getWriteMaskFront() const { return frontWriteMask; }
        uint8 getWriteMaskBack() const { return backWriteMask; }

        int32 getRaw() const { return *(reinterpret_cast<const int32*>(this)); }
        StencilActivation(int32 raw) { *(reinterpret_cast<int32*>(this)) = raw; }
        bool operator==(const StencilActivation& right) const { return getRaw() == right.getRaw(); }
        bool operator!=(const StencilActivation& right) const { return getRaw() != right.getRaw(); }
    };

    static_assert(sizeof(StencilActivation) == sizeof(uint32_t), "StencilActivation size check");

    struct BlendFunction {
        // Using uint8 here will make the structure as a whole not align to 32 bits
        uint16 enabled : 8;
        BlendArg sourceColor : 4;
        BlendArg sourceAlpha : 4;
        BlendArg destColor : 4;
        BlendArg destAlpha : 4;
        BlendOp opColor : 4;
        BlendOp opAlpha : 4;

    public:
        BlendFunction(bool enabled,
                      BlendArg sourceColor,
                      BlendOp operationColor,
                      BlendArg destinationColor,
                      BlendArg sourceAlpha,
                      BlendOp operationAlpha,
                      BlendArg destinationAlpha) :
            enabled(enabled),
            sourceColor(sourceColor), sourceAlpha(sourceAlpha), 
            destColor(destinationColor), destAlpha(destinationAlpha),
            opColor(operationColor), opAlpha(operationAlpha) {}

        BlendFunction(bool enabled = false, BlendArg source = ONE, BlendOp operation = BLEND_OP_ADD, BlendArg destination = ZERO) :
            BlendFunction(enabled, source, operation, destination, source, operation, destination) {}

        bool isEnabled() const { return (enabled != 0); }

        BlendArg getSourceColor() const { return sourceColor; }
        BlendArg getDestinationColor() const { return destColor; }
        BlendOp getOperationColor() const { return opColor; }

        BlendArg getSourceAlpha() const { return sourceAlpha; }
        BlendArg getDestinationAlpha() const { return destAlpha; }
        BlendOp getOperationAlpha() const { return opAlpha; }

        int32 getRaw() const { return *(reinterpret_cast<const int32*>(this)); }
        BlendFunction(int32 raw) { *(reinterpret_cast<int32*>(this)) = raw; }
        bool operator==(const BlendFunction& right) const { return getRaw() == right.getRaw(); }
        bool operator!=(const BlendFunction& right) const { return getRaw() != right.getRaw(); }
    };

    static_assert(sizeof(BlendFunction) == sizeof(uint32_t), "BlendFunction size check");

    struct Flags {
        Flags() :
            frontFaceClockwise(false), depthClampEnable(false), scissorEnable(false), multisampleEnable(true),
            antialisedLineEnable(true), alphaToCoverageEnable(false), _spare1(0) {}
        bool frontFaceClockwise : 1;
        bool depthClampEnable : 1;
        bool scissorEnable : 1;
        bool multisampleEnable : 1;
        bool antialisedLineEnable : 1;
        bool alphaToCoverageEnable : 1;
        uint8 _spare1 : 2;

        bool operator==(const Flags& right) const { return *(uint8*)this == *(uint8*)&right; }
        bool operator!=(const Flags& right) const { return *(uint8*)this != *(uint8*)&right; }
    };

    static_assert(sizeof(Flags) == sizeof(uint8), "Flags size check");

    // The Data class is the full explicit description of the State class fields value.
    // Useful for having one const static called Default for reference or for the gpu::Backend to keep track of the current value
    class Data {
    public:
        float depthBias = 0.0f;
        float depthBiasSlopeScale = 0.0f;

        DepthTest depthTest;
        StencilActivation stencilActivation;
        StencilTest stencilTestFront;
        StencilTest stencilTestBack;
        uint32 sampleMask = 0xFFFFFFFF;
        BlendFunction blendFunction;
        FillMode fillMode{ FILL_FACE };
        CullMode cullMode{ CULL_NONE };
        ColorMask colorWriteMask{ WRITE_ALL };

        Flags flags;
    };

    // The unique default values for all the fields
    static const Data DEFAULT;
    void setFillMode(FillMode fill) { SET_FIELD(FILL_MODE, fillMode, fill); }
    FillMode getFillMode() const { return _values.fillMode; }

    void setCullMode(CullMode cull) { SET_FIELD(CULL_MODE, cullMode, cull); }
    CullMode getCullMode() const { return _values.cullMode; }

    const Flags& getFlags() const { return _values.flags; }

    void setFrontFaceClockwise(bool isClockwise) { SET_FIELD(FRONT_FACE_CLOCKWISE, flags.frontFaceClockwise, isClockwise); }
    bool isFrontFaceClockwise() const { return _values.flags.frontFaceClockwise; }

    void setDepthClampEnable(bool enable) { SET_FIELD(DEPTH_CLAMP_ENABLE, flags.depthClampEnable, enable); }
    bool isDepthClampEnable() const { return _values.flags.depthClampEnable; }

    void setScissorEnable(bool enable) { SET_FIELD(SCISSOR_ENABLE, flags.scissorEnable, enable); }
    bool isScissorEnable() const { return _values.flags.scissorEnable; }

    void setMultisampleEnable(bool enable) { SET_FIELD(MULTISAMPLE_ENABLE, flags.multisampleEnable, enable); }
    bool isMultisampleEnable() const { return _values.flags.multisampleEnable; }

    void setAntialiasedLineEnable(bool enable) { SET_FIELD(ANTIALISED_LINE_ENABLE, flags.antialisedLineEnable, enable); }
    bool isAntialiasedLineEnable() const { return _values.flags.antialisedLineEnable; }

    // Depth Bias
    void setDepthBias(float bias) { SET_FIELD(DEPTH_BIAS, depthBias, bias); }
    float getDepthBias() const { return _values.depthBias; }

    void setDepthBiasSlopeScale(float scale) { SET_FIELD(DEPTH_BIAS_SLOPE_SCALE, depthBiasSlopeScale, scale); }
    float getDepthBiasSlopeScale() const { return _values.depthBiasSlopeScale; }

    // Depth Test
    void setDepthTest(DepthTest newDepthTest) { SET_FIELD(DEPTH_TEST, depthTest, newDepthTest); }
    void setDepthTest(bool enable, bool writeMask, ComparisonFunction func) {
        setDepthTest(DepthTest(enable, writeMask, func));
    }
    DepthTest getDepthTest() const { return _values.depthTest; }

    bool isDepthTestEnabled() const { return getDepthTest().isEnabled(); }
    uint8 getDepthTestWriteMask() const { return getDepthTest().getWriteMask(); }
    ComparisonFunction getDepthTestFunc() const { return getDepthTest().getFunction(); }

    // Stencil test
    void setStencilTest(bool enabled, uint8 frontWriteMask, StencilTest frontTest, uint8 backWriteMask, StencilTest backTest) {
        SET_FIELD(STENCIL_ACTIVATION, stencilActivation, StencilActivation(enabled, frontWriteMask, backWriteMask));
        SET_FIELD(STENCIL_TEST_FRONT, stencilTestFront, frontTest);
        SET_FIELD(STENCIL_TEST_BACK, stencilTestBack, backTest);
    }

    void setStencilTest(bool enabled, uint8 frontWriteMask, StencilTest frontTest) {
        setStencilTest(enabled, frontWriteMask, frontTest, frontWriteMask, frontTest);
    }

    StencilActivation getStencilActivation() const { return _values.stencilActivation; }
    StencilTest getStencilTestFront() const { return _values.stencilTestFront; }
    StencilTest getStencilTestBack() const { return _values.stencilTestBack; }

    bool isStencilEnabled() const { return getStencilActivation().isEnabled(); }
    uint8 getStencilWriteMaskFront() const { return getStencilActivation().getWriteMaskFront(); }
    uint8 getStencilWriteMaskBack() const { return getStencilActivation().getWriteMaskBack(); }

    // Alpha to coverage
    void setAlphaToCoverageEnable(bool enable) { SET_FIELD(ALPHA_TO_COVERAGE_ENABLE, flags.alphaToCoverageEnable, enable); }
    bool isAlphaToCoverageEnabled() const { return _values.flags.alphaToCoverageEnable; }

    // Sample mask
    void setSampleMask(uint32 mask) { SET_FIELD(SAMPLE_MASK, sampleMask, mask); }
    uint32 getSampleMask() const { return _values.sampleMask; }

    // Blend Function
    void setBlendFunction(BlendFunction function) { SET_FIELD(BLEND_FUNCTION, blendFunction, function); }
    const BlendFunction& getBlendFunction() const { return _values.blendFunction; }

    void setBlendFunction(bool enabled,
                          BlendArg sourceColor,
                          BlendOp operationColor,
                          BlendArg destinationColor,
                          BlendArg sourceAlpha,
                          BlendOp operationAlpha,
                          BlendArg destinationAlpha) {
        setBlendFunction(BlendFunction(enabled, sourceColor, operationColor, destinationColor, sourceAlpha, operationAlpha,
                                       destinationAlpha));
    }
    void setBlendFunction(bool enabled, BlendArg source, BlendOp operation, BlendArg destination) {
        setBlendFunction(BlendFunction(enabled, source, operation, destination));
    }

    bool isBlendEnabled() const { return getBlendFunction().isEnabled(); }

    // Color write mask
    void setColorWriteMask(ColorMask mask) { SET_FIELD(COLOR_WRITE_MASK, colorWriteMask, mask); }
    void setColorWriteMask(bool red, bool green, bool blue, bool alpha) {
        ColorMask value = (ColorMask)((WRITE_RED * red) | (WRITE_GREEN * green) | (WRITE_BLUE * blue) | (WRITE_ALPHA * alpha));
        SET_FIELD(COLOR_WRITE_MASK, colorWriteMask, value);
    }
    ColorMask getColorWriteMask() const { return _values.colorWriteMask; }

    // All the possible fields
    // NOTE: If you change this, you must update GLBackend::GLState::_resetStateCommands
    enum Field
    {
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

        NUM_FIELDS,  // not a valid field, just the count
    };

    // The signature of the state tells which fields of the state are not default
    // this way during rendering the Backend can compare it's current state and try to minimize the job to do
    typedef std::bitset<NUM_FIELDS> Signature;

    Signature getSignature() const { return _signature; }

    static Signature evalSignature(const Data& state);

    // For convenience, create a State from the values directly
    State(const Data& values);
    const Data& getValues() const { return _values; }

    const GPUObjectPointer gpuObject{};

protected:
    State(const State& state);
    State& operator=(const State& state);

    Data _values;
    Signature _signature{ 0 };
    Stamp _stamp{ 0 };
};

typedef std::shared_ptr<State> StatePointer;
typedef std::vector<StatePointer> States;

};  // namespace gpu

#endif
