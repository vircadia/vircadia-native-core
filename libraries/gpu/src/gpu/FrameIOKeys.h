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


constexpr char* const binary = "binary";
constexpr char* const L00 = "L00";
constexpr char* const L1m1 = "L1m1";
constexpr char* const L10 = "L10";
constexpr char* const L11 = "L11";
constexpr char* const L2m2 = "L2m2";
constexpr char* const L2m1 = "L2m1";
constexpr char* const L20 = "L20";
constexpr char* const L21 = "L21";
constexpr char* const L22 = "L22";

constexpr char* const eyeProjections = "eyeProjections";
constexpr char* const eyeViews = "eyeViews";
constexpr char* const alphaToCoverageEnable = "alphaToCoverageEnable";
constexpr char* const antialisedLineEnable = "antialisedLineEnable";
constexpr char* const attributes = "attributes";
constexpr char* const batches = "batches";
constexpr char* const blendFunction = "blendFunction";
constexpr char* const borderColor = "borderColor";
constexpr char* const bufferMask = "bufferMask";
constexpr char* const buffers = "buffers";
constexpr char* const capturedTextures = "capturedTextures";
constexpr char* const channel = "channel";
constexpr char* const chunk = "chunk";
constexpr char* const colorAttachments = "colorAttachments";
constexpr char* const colorWriteMask = "colorWriteMask";
constexpr char* const commands = "commands";
constexpr char* const comparisonFunction = "comparisonFunction";
constexpr char* const cullMode = "cullMode";
constexpr char* const data = "data";
constexpr char* const depth = "depth";
constexpr char* const depthBias = "depthBias";
constexpr char* const depthBiasSlopeScale = "depthBiasSlopeScale";
constexpr char* const depthClampEnable = "depthClampEnable";
constexpr char* const depthStencilAttachment = "depthStencilAttachment";
constexpr char* const depthTest = "depthTest";
constexpr char* const drawCallInfos = "drawCallInfos";
constexpr char* const drawcallUniform = "drawcallUniform";
constexpr char* const drawcallUniformReset = "drawcallUniformReset";
constexpr char* const element = "element";
constexpr char* const fillMode = "fillMode";
constexpr char* const filter = "filter";
constexpr char* const formats = "formats";
constexpr char* const frameIndex = "frameIndex";
constexpr char* const framebuffer = "framebuffer";
constexpr char* const framebuffers = "framebuffers";
constexpr char* const frequency = "frequency";
constexpr char* const frontFaceClockwise = "frontFaceClockwise";
constexpr char* const height = "height";
constexpr char* const id = "id";
constexpr char* const ktxFile = "ktxFile";
constexpr char* const layers = "layers";
constexpr char* const maxAnisotropy = "maxAnisotropy";
constexpr char* const maxMip = "maxMip";
constexpr char* const minMip = "minMip";
constexpr char* const mipOffset = "mipOffset";
constexpr char* const mips = "mips";
constexpr char* const multisampleEnable = "multisampleEnable";
constexpr char* const name = "name";
constexpr char* const namedData = "namedData";
constexpr char* const names = "names";
constexpr char* const objects = "objects";
constexpr char* const offset = "offset";
constexpr char* const pipelines = "pipelines";
constexpr char* const pose = "pose";
constexpr char* const profileRanges = "profileRanges";
constexpr char* const program = "program";
constexpr char* const programs = "programs";
constexpr char* const projectionJitter = "projectionJitter";
constexpr char* const queries = "queries";
constexpr char* const sampleCount = "sampleCount";
constexpr char* const sampleMask = "sampleMask";
constexpr char* const sampler = "sampler";
constexpr char* const samples = "samples";
constexpr char* const scissorEnable = "scissorEnable";
constexpr char* const shaders = "shaders";
constexpr char* const size = "size";
constexpr char* const skybox = "skybox";
constexpr char* const slot = "slot";
constexpr char* const source = "source";
constexpr char* const state = "state";
constexpr char* const stencilActivation = "stencilActivation";
constexpr char* const stencilTestBack = "stencilTestBack";
constexpr char* const stencilTestFront = "stencilTestFront";
constexpr char* const stereo = "stereo";
constexpr char* const subresource = "subresource";
constexpr char* const swapchains = "swapchains";
constexpr char* const texelFormat = "texelFormat";
constexpr char* const texture = "texture";
constexpr char* const textureTables = "textureTables";
constexpr char* const textures = "textures";
constexpr char* const transforms = "transforms";
constexpr char* const type = "type";
constexpr char* const usageType = "usageType";
constexpr char* const view = "view";
constexpr char* const width = "width";
constexpr char* const wrapModeU = "wrapModeU";
constexpr char* const wrapModeV = "wrapModeV";
constexpr char* const wrapModeW = "wrapModeW";


constexpr char* const backWriteMask = "backWriteMask";
constexpr char* const frontWriteMask = "frontWriteMask";
constexpr char* const reference = "reference";
constexpr char* const readMask = "readMask";
constexpr char* const failOp = "failOp";
constexpr char* const depthFailOp = "depthFailOp";
constexpr char* const passOp = "passOp";
constexpr char* const enabled = "enabled";
constexpr char* const blend = "blend";
constexpr char* const flags = "flags";
constexpr char* const writeMask = "writeMask";
constexpr char* const function = "function";
constexpr char* const sourceColor = "sourceColor";
constexpr char* const sourceAlpha = "sourceAlpha";
constexpr char* const destColor = "destColor";
constexpr char* const destAlpha = "destAlpha";
constexpr char* const opColor = "opColor";
constexpr char* const opAlpha = "opAlpha";
constexpr char* const enable = "enable";
constexpr char* const contextDisable = "contextDisable";

constexpr char* const COMMAND_NAMES[] = {
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
