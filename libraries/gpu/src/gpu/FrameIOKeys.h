//
//  Created by Bradley Austin Davis on 2018/10/14
//  Copyright 2013-2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once
#ifndef hifi_gpu_FrameIOKeys_h
#define hifi_gpu_FrameIOKeys_h

namespace gpu { namespace keys {

static const char* binary = "binary";
static const char* L00 = "L00";
static const char* L1m1 = "L1m1";
static const char* L10 = "L10";
static const char* L11 = "L11";
static const char* L2m2 = "L2m2";
static const char* L2m1 = "L2m1";
static const char* L20 = "L20";
static const char* L21 = "L21";
static const char* L22 = "L22";

static const char* eyeProjections = "eyeProjections";
static const char* eyeViews = "eyeViews";
static const char* alphaToCoverageEnable = "alphaToCoverageEnable";
static const char* antialisedLineEnable = "antialisedLineEnable";
static const char* attributes = "attributes";
static const char* batches = "batches";
static const char* blendFunction = "blendFunction";
static const char* borderColor = "borderColor";
static const char* bufferMask = "bufferMask";
static const char* buffers = "buffers";
static const char* capturedTextures = "capturedTextures";
static const char* channel = "channel";
static const char* colorAttachments = "colorAttachments";
static const char* colorWriteMask = "colorWriteMask";
static const char* commands = "commands";
static const char* comparisonFunction = "comparisonFunction";
static const char* cullMode = "cullMode";
static const char* data = "data";
static const char* depth = "depth";
static const char* depthBias = "depthBias";
static const char* depthBiasSlopeScale = "depthBiasSlopeScale";
static const char* depthClampEnable = "depthClampEnable";
static const char* depthStencilAttachment = "depthStencilAttachment";
static const char* depthTest = "depthTest";
static const char* drawCallInfos = "drawCallInfos";
static const char* drawcallUniform = "drawcallUniform";
static const char* drawcallUniformReset = "drawcallUniformReset";
static const char* element = "element";
static const char* fillMode = "fillMode";
static const char* filter = "filter";
static const char* formats = "formats";
static const char* frameIndex = "frameIndex";
static const char* framebuffer = "framebuffer";
static const char* framebuffers = "framebuffers";
static const char* frequency = "frequency";
static const char* frontFaceClockwise = "frontFaceClockwise";
static const char* height = "height";
static const char* id = "id";
static const char* ktxFile = "ktxFile";
static const char* layers = "layers";
static const char* maxAnisotropy = "maxAnisotropy";
static const char* maxMip = "maxMip";
static const char* minMip = "minMip";
static const char* mipOffset = "mipOffset";
static const char* mips = "mips";
static const char* multisampleEnable = "multisampleEnable";
static const char* name = "name";
static const char* namedData = "namedData";
static const char* names = "names";
static const char* objects = "objects";
static const char* offset = "offset";
static const char* pipelines = "pipelines";
static const char* pose = "pose";
static const char* profileRanges = "profileRanges";
static const char* program = "program";
static const char* programs = "programs";
static const char* projectionJitter = "projectionJitter";
static const char* queries = "queries";
static const char* sampleCount = "sampleCount";
static const char* sampleMask = "sampleMask";
static const char* sampler = "sampler";
static const char* samples = "samples";
static const char* scissorEnable = "scissorEnable";
static const char* shaders = "shaders";
static const char* size = "size";
static const char* skybox = "skybox";
static const char* slot = "slot";
static const char* source = "source";
static const char* state = "state";
static const char* stencilActivation = "stencilActivation";
static const char* stencilTestBack = "stencilTestBack";
static const char* stencilTestFront = "stencilTestFront";
static const char* stereo = "stereo";
static const char* subresource = "subresource";
static const char* swapchains = "swapchains";
static const char* texelFormat = "texelFormat";
static const char* texture = "texture";
static const char* textureTables = "textureTables";
static const char* textures = "textures";
static const char* transforms = "transforms";
static const char* type = "type";
static const char* usageType = "usageType";
static const char* view = "view";
static const char* width = "width";
static const char* wrapModeU = "wrapModeU";
static const char* wrapModeV = "wrapModeV";
static const char* wrapModeW = "wrapModeW";


static const char* backWriteMask = "backWriteMask";
static const char* frontWriteMask = "frontWriteMask";
static const char* reference = "reference";
static const char* readMask = "readMask";
static const char* failOp = "failOp";
static const char* depthFailOp = "depthFailOp";
static const char* passOp = "passOp";
static const char* enabled = "enabled";
static const char* blend = "blend";
static const char* flags = "flags";
static const char* writeMask = "writeMask";
static const char* function = "function";
static const char* sourceColor = "sourceColor";
static const char* sourceAlpha = "sourceAlpha";
static const char* destColor = "destColor";
static const char* destAlpha = "destAlpha";
static const char* opColor = "opColor";
static const char* opAlpha = "opAlpha";
static const char* enable = "enable";
static const char* contextDisable = "contextDisable";

static const char* COMMAND_NAMES[] = {
    "draw",
    "drawIndexed",
    "drawInstanced",
    "drawIndexedInstanced",
    "multiDrawIndirect",
    "multiDrawIndexedIndirect",

    "setInputFormat",
    "setInputBuffer",
    "setIndexBuffer",
    "setIndirectBuffer",

    "setModelTransform",
    "setViewTransform",
    "setProjectionTransform",
    "setProjectionJitter",
    "setViewportTransform",
    "setDepthRangeTransform",

    "setPipeline",
    "setStateBlendFactor",
    "setStateScissorRect",

    "setUniformBuffer",
    "setResourceBuffer",
    "setResourceTexture",
    "setResourceTextureTable",
    "setResourceFramebufferSwapChainTexture",

    "setFramebuffer",
    "setFramebufferSwapChain",
    "clearFramebuffer",
    "blit",
    "generateTextureMips",
    "generateTextureMipsWithPipeline",

    "advance",

    "beginQuery",
    "endQuery",
    "getQuery",

    "resetStages",

    "disableContextViewCorrection",
    "restoreContextViewCorrection",

    "disableContextStereo",
    "restoreContextStereo",

    "runLambda",

    "startNamedCall",
    "stopNamedCall",

    "glUniform1i",
    "glUniform1f",
    "glUniform2f",
    "glUniform3f",
    "glUniform4f",
    "glUniform3fv",
    "glUniform4fv",
    "glUniform4iv",
    "glUniformMatrix3fv",
    "glUniformMatrix4fv",

    "glColor4f",

    "pushProfileRange",
    "popProfileRange",
};

template<class T, size_t N>
constexpr size_t array_size(T (&)[N]) { return N; }

static_assert(array_size(COMMAND_NAMES) == Batch::Command::NUM_COMMANDS, "Command array sizes must match");
}}  // namespace gpu::keys

#endif
