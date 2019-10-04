//
//  Created by Bradley Austin Davis on 2018/10/14
//  Copyright 2013-2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once
#ifndef hifi_gpu_FrameIO_h
#define hifi_gpu_FrameIO_h

#include "Forward.h"
#include "Format.h"

#include <functional>

namespace gpu {

using TextureCapturer = std::function<void(std::vector<uint8_t>&, const TexturePointer&, uint16 layer)>;
using TextureLoader = std::function<void(const std::vector<uint8_t>&, const TexturePointer&, uint16 layer)>;
void writeFrame(const std::string& filename, const FramePointer& frame, const TextureCapturer& capturer = nullptr);
FramePointer readFrame(const std::string& filename, uint32_t externalTexture, const TextureLoader& loader = nullptr);

namespace hfb {

constexpr char* EXTENSION{ ".hfb" };
constexpr uint32_t HEADER_SIZE{ sizeof(uint32_t) * 3 };
constexpr uint32_t CHUNK_HEADER_SIZE = sizeof(uint32_t) * 2;
constexpr uint32_t MAGIC{ 0x49464948 };
constexpr uint32_t VERSION{ 0x01 };
constexpr uint32_t CHUNK_TYPE_JSON{ 0x4E4F534A };
constexpr uint32_t CHUNK_TYPE_BIN{ 0x004E4942 };
constexpr uint32_t CHUNK_TYPE_PNG{ 0x00474E50 };

using Buffer = std::vector<uint8_t>;
using Buffers = std::vector<Buffer>;

struct Header {
    uint32_t magic{ 0 };
    uint32_t version{ 0 };
    uint32_t length{ 0 };
};

struct ChunkHeader {
    uint32_t length{ 0 };
    uint32_t type{ 0 };
};

struct Chunk : public ChunkHeader {
    uint32_t offset{ 0 };

    size_t end() const;
};

using Chunks = std::vector<Chunk>;

struct Descriptor {
    Header header;
    Chunks chunks;

    operator bool() const { return header.magic == MAGIC; }
    bool getChunkString(std::string& output, size_t chunk, const uint8_t* const data, size_t size);
    bool getChunkBuffer(Buffer& output, size_t chunk, const uint8_t* const data, size_t size);
    static Descriptor parse(const uint8_t* const data, size_t size);
};

void writeFrame(const std::string& filename, const std::string& json, const Buffer& binaryBuffer, const Buffers& pngBuffers);

}  // namespace hfb

}  // namespace gpu

#endif
