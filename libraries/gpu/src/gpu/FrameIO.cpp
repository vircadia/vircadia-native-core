//
//  Created by Bradley Austin Davis on 2019/10/03
//  Copyright 2013-2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "FrameIO.h"
#include <shared/Storage.h>

using namespace gpu::hfb;

static bool skip(const uint8_t*& ptr, size_t& remaining, uint32_t size) {
    if (remaining < size) {
        return false;
    }
    ptr += size;
    remaining -= size;
    return true;
}

template <typename T>
static bool read(const uint8_t*& ptr, size_t& remaining, T& output) {
    uint32_t readSize = (uint32_t)sizeof(T);
    if (remaining < readSize) {
        return false;
    }
    memcpy(&output, ptr, readSize);
    return skip(ptr, remaining, readSize);
}

Descriptor Descriptor::parse(const uint8_t* const data, size_t size) {
    const auto* ptr = data;
    auto remaining = size;
    Descriptor result;
    if (!read(ptr, remaining, result.header)) {
        return {};
    }
    if (result.header.length != size) {
        return {};
    }

    while (remaining != 0) {
        result.chunks.emplace_back();
        auto& chunk = result.chunks.back();
        ChunkHeader& chunkHeader = chunk;
        if (!read(ptr, remaining, chunkHeader)) {
            return {};
        }
        chunk.offset = ptr - data;
        if (!skip(ptr, remaining, chunk.length)) {
            return {};
        }
    }
    return result;
}

size_t Chunk::end() const {
    size_t result = offset;
    result += length;
    return result;
}


bool Descriptor::getChunkString(std::string& output, size_t chunkIndex, const uint8_t* const data, size_t size) {
    if (chunkIndex >= chunks.size()) {
        return false;
    }
    const auto& chunk = chunks[chunkIndex];
    if (chunk.end() > size) {
        return false;
    }
    output = std::string{ (const char*)(data + chunk.offset), chunk.length };
    return true;
}

bool Descriptor::getChunkBuffer(Buffer& output, size_t chunkIndex, const uint8_t* const data, size_t size) {
    if (chunkIndex >= chunks.size()) {
        return false;
    }
    const auto& chunk = chunks[chunkIndex];
    if (chunk.end() > size) {
        return false;
    }
    output.resize(chunk.length);
    memcpy(output.data(), data + chunk.offset, chunk.length);
    return true;
}

static void writeUint(uint8_t*& dest, uint32_t value) {
    memcpy(dest, &value, sizeof(uint32_t));
    dest += sizeof(uint32_t);
}

template <typename T>
static void writeChunk(uint8_t*& dest, uint32_t chunkType, const T& chunkData) {
    uint32_t chunkSize = static_cast<uint32_t>(chunkData.size());
    writeUint(dest, chunkSize);
    writeUint(dest, chunkType);
    memcpy(dest, chunkData.data(), chunkSize);
    dest += chunkSize;
}

void gpu::hfb::writeFrame(const std::string& filename,
                          const std::string& json,
                          const Buffer& binaryBuffer,
                          const Buffers& pngBuffers) {
    uint32_t strLen = (uint32_t)json.size();
    uint32_t size = gpu::hfb::HEADER_SIZE + gpu::hfb::CHUNK_HEADER_SIZE + strLen;
    size += gpu::hfb::CHUNK_HEADER_SIZE + (uint32_t)binaryBuffer.size();
    for (const auto& pngBuffer : pngBuffers) {
        size += gpu::hfb::CHUNK_HEADER_SIZE + (uint32_t)pngBuffer.size();
    }

    auto outputConst = storage::FileStorage::create(filename.c_str(), size, nullptr);
    auto output = std::const_pointer_cast<storage::Storage>(outputConst);
    auto ptr = output->mutableData();
    writeUint(ptr, gpu::hfb::MAGIC);
    writeUint(ptr, gpu::hfb::VERSION);
    writeUint(ptr, size);
    writeChunk(ptr, gpu::hfb::CHUNK_TYPE_JSON, json);
    writeChunk(ptr, gpu::hfb::CHUNK_TYPE_BIN, binaryBuffer);
    for (const auto& png : pngBuffers) {
        writeChunk(ptr, gpu::hfb::CHUNK_TYPE_PNG, png);
    }
    assert((ptr - output->data()) == size);
}
