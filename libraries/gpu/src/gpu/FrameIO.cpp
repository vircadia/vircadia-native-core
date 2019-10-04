//
//  Created by Bradley Austin Davis on 2019/10/03
//  Copyright 2013-2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "FrameIO.h"
#include <shared/Storage.h>
#include <stdexcept>

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

Descriptor::Descriptor(const StoragePointer& storage) : storage(storage) {
    const auto* const start = storage->data();
    const auto* ptr = storage->data();
    auto remaining = storage->size();

    try {
        // Can't parse files more than 4GB
        if (remaining > UINT32_MAX) {
            throw std::runtime_error("File too large");
        }

        if (!read(ptr, remaining, header)) {
            throw std::runtime_error("Couldn't read binary header");
        }

        if (header.length != storage->size()) {
            throw std::runtime_error("Header/Actual size mismatch");
        }

        while (remaining != 0) {
            chunks.emplace_back();
            auto& chunk = chunks.back();
            ChunkHeader& chunkHeader = chunk;
            if (!read(ptr, remaining, chunkHeader)) {
                throw std::runtime_error("Coulnd't read chunk header");
            }
            chunk.offset = (uint32_t)(ptr - start);
            if (chunk.end() > storage->size()) {
                throw std::runtime_error("Chunk too large for file");
            }
            if (!skip(ptr, remaining, chunk.length)) {
                throw std::runtime_error("Skip chunk data failed");
            }
        }
    } catch (const std::runtime_error&) {
        // LOG somnething
        header.magic = 0;
    }
}

size_t Chunk::end() const {
    size_t result = offset;
    result += length;
    return result;
}

StoragePointer Descriptor::getChunk(uint32_t chunkIndex) const {
    if (chunkIndex >= chunks.size()) {
        return {};
    }
    const auto& chunk = chunks[chunkIndex];
    if (chunk.end() > storage->size()) {
        return {};
    }
    return storage->createView(chunk.length, chunk.offset);
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
                          const StorageBuilders& ktxBuilders) {
    uint32_t strLen = (uint32_t)json.size();
    uint32_t size = gpu::hfb::HEADER_SIZE + gpu::hfb::CHUNK_HEADER_SIZE + strLen;
    size += gpu::hfb::CHUNK_HEADER_SIZE + (uint32_t)binaryBuffer.size();
    for (const auto& builder : ktxBuilders) {
        auto storage = builder();
        if (storage) {
            size += gpu::hfb::CHUNK_HEADER_SIZE + (uint32_t)storage->size();
        }
    }

    auto outputConst = storage::FileStorage::create(filename.c_str(), size, nullptr);
    auto output = std::const_pointer_cast<storage::Storage>(outputConst);
    auto ptr = output->mutableData();
    writeUint(ptr, gpu::hfb::MAGIC);
    writeUint(ptr, gpu::hfb::VERSION);
    writeUint(ptr, size);
    writeChunk(ptr, gpu::hfb::CHUNK_TYPE_JSON, json);
    writeChunk(ptr, gpu::hfb::CHUNK_TYPE_BIN, binaryBuffer);
    for (const auto& builder : ktxBuilders) {
        static StoragePointer EMPTY_STORAGE{ std::make_shared<storage::MemoryStorage>(0, nullptr) };
        auto storage = builder();
        if (!storage) {
            storage = EMPTY_STORAGE;
        }
        writeChunk(ptr, gpu::hfb::CHUNK_TYPE_KTX, *storage);
    }
    assert((ptr - output->data()) == size);
}
