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


constexpr char* binary = "binary";
constexpr char* L00 = "L00";
constexpr char* L1m1 = "L1m1";
constexpr char* L10 = "L10";
constexpr char* L11 = "L11";
constexpr char* L2m2 = "L2m2";
constexpr char* L2m1 = "L2m1";
constexpr char* L20 = "L20";
constexpr char* L21 = "L21";
constexpr char* L22 = "L22";

constexpr char* eyeProjections = "eyeProjections";
constexpr char* eyeViews = "eyeViews";
constexpr char* alphaToCoverageEnable = "alphaToCoverageEnable";
constexpr char* antialisedLineEnable = "antialisedLineEnable";
constexpr char* attributes = "attributes";
constexpr char* batches = "batches";
constexpr char* blendFunction = "blendFunction";
constexpr char* borderColor = "borderColor";
constexpr char* bufferMask = "bufferMask";
constexpr char* buffers = "buffers";
constexpr char* capturedTextures = "capturedTextures";
constexpr char* channel = "channel";
constexpr char* chunk = "chunk";
constexpr char* colorAttachments = "colorAttachments";
constexpr char* colorWriteMask = "colorWriteMask";
constexpr char* commands = "commands";
constexpr char* comparisonFunction = "comparisonFunction";
constexpr char* cullMode = "cullMode";
constexpr char* data = "data";
constexpr char* depth = "depth";
constexpr char* depthBias = "depthBias";
constexpr char* depthBiasSlopeScale = "depthBiasSlopeScale";
constexpr char* depthClampEnable = "depthClampEnable";
constexpr char* depthStencilAttachment = "depthStencilAttachment";
constexpr char* depthTest = "depthTest";
constexpr char* drawCallInfos = "drawCallInfos";
constexpr char* drawcallUniform = "drawcallUniform";
constexpr char* drawcallUniformReset = "drawcallUniformReset";
constexpr char* element = "element";
constexpr char* fillMode = "fillMode";
constexpr char* filter = "filter";
constexpr char* formats = "formats";
constexpr char* frameIndex = "frameIndex";
constexpr char* framebuffer = "framebuffer";
constexpr char* framebuffers = "framebuffers";
constexpr char* frequency = "frequency";
constexpr char* frontFaceClockwise = "frontFaceClockwise";
constexpr char* height = "height";
constexpr char* id = "id";
constexpr char* ktxFile = "ktxFile";
constexpr char* layers = "layers";
constexpr char* maxAnisotropy = "maxAnisotropy";
constexpr char* maxMip = "maxMip";
constexpr char* minMip = "minMip";
constexpr char* mipOffset = "mipOffset";
constexpr char* mips = "mips";
constexpr char* multisampleEnable = "multisampleEnable";
constexpr char* name = "name";
constexpr char* namedData = "namedData";
constexpr char* names = "names";
constexpr char* objects = "objects";
constexpr char* offset = "offset";
constexpr char* pipelines = "pipelines";
constexpr char* pose = "pose";
constexpr char* profileRanges = "profileRanges";
constexpr char* program = "program";
constexpr char* programs = "programs";
constexpr char* projectionJitter = "projectionJitter";
constexpr char* queries = "queries";
constexpr char* sampleCount = "sampleCount";
constexpr char* sampleMask = "sampleMask";
constexpr char* sampler = "sampler";
constexpr char* samples = "samples";
constexpr char* scissorEnable = "scissorEnable";
constexpr char* shaders = "shaders";
constexpr char* size = "size";
constexpr char* skybox = "skybox";
constexpr char* slot = "slot";
constexpr char* source = "source";
constexpr char* state = "state";
constexpr char* stencilActivation = "stencilActivation";
constexpr char* stencilTestBack = "stencilTestBack";
constexpr char* stencilTestFront = "stencilTestFront";
constexpr char* stereo = "stereo";
constexpr char* subresource = "subresource";
constexpr char* swapchains = "swapchains";
constexpr char* texelFormat = "texelFormat";
constexpr char* texture = "texture";
constexpr char* textureTables = "textureTables";
constexpr char* textures = "textures";
constexpr char* transforms = "transforms";
constexpr char* type = "type";
constexpr char* usageType = "usageType";
constexpr char* view = "view";
constexpr char* width = "width";
constexpr char* wrapModeU = "wrapModeU";
constexpr char* wrapModeV = "wrapModeV";
constexpr char* wrapModeW = "wrapModeW";


constexpr char* backWriteMask = "backWriteMask";
constexpr char* frontWriteMask = "frontWriteMask";
constexpr char* reference = "reference";
constexpr char* readMask = "readMask";
constexpr char* failOp = "failOp";
constexpr char* depthFailOp = "depthFailOp";
constexpr char* passOp = "passOp";
constexpr char* enabled = "enabled";
constexpr char* blend = "blend";
constexpr char* flags = "flags";
constexpr char* writeMask = "writeMask";
constexpr char* function = "function";
constexpr char* sourceColor = "sourceColor";
constexpr char* sourceAlpha = "sourceAlpha";
constexpr char* destColor = "destColor";
constexpr char* destAlpha = "destAlpha";
constexpr char* opColor = "opColor";
constexpr char* opAlpha = "opAlpha";
constexpr char* enable = "enable";
constexpr char* contextDisable = "contextDisable";

constexpr char* COMMAND_NAMES[] = {
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
