//
//  Texture_ktx.cpp
//  libraries/gpu/src/gpu
//
//  Created by Sam Gateau on 2/16/2017.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#include "Texture.h"
#include <qdebug.h>

#include <ktx/KTX.h>
using namespace gpu;

using PixelsPointer = Texture::PixelsPointer;
using KtxStorage = Texture::KtxStorage;

struct GPUKTXPayload {
    Sampler::Desc _samplerDesc;
    Texture::Usage _usage;
    TextureUsageType _usageType;


    static std::string KEY;
    static bool isGPUKTX(const ktx::KeyValue& val) {
        return (val._key.compare(KEY) == 0);
    }

    static bool findInKeyValues(const ktx::KeyValues& keyValues, GPUKTXPayload& payload) {
        auto found = std::find_if(keyValues.begin(), keyValues.end(), isGPUKTX);
        if (found != keyValues.end()) {
            if ((*found)._value.size() == sizeof(GPUKTXPayload)) {
                memcpy(&payload, (*found)._value.data(), sizeof(GPUKTXPayload));
                return true;
            }
        }
        return false;
    }
};

const std::string gpu::SOURCE_HASH_KEY { "hifi.sourceHash" };
std::string GPUKTXPayload::KEY{ "hifi.gpu" };

KtxStorage::KtxStorage(const std::string& filename) : _filename(filename) {
    {
        // We are doing a lot of work here just to get descriptor data
        ktx::StoragePointer storage{ new storage::FileStorage(_filename.c_str()) };
        auto ktxPointer = ktx::KTX::create(storage);
        _ktxDescriptor.reset(new ktx::KTXDescriptor(ktxPointer->toDescriptor()));
        if (_ktxDescriptor->images.size() < _ktxDescriptor->header.numberOfMipmapLevels) {
            qWarning() << "Bad images found in ktx";
        }
        auto& keyValues = _ktxDescriptor->keyValues;
        auto found = std::find_if(keyValues.begin(), keyValues.end(), [](const ktx::KeyValue& val) -> bool {
            return val._key.compare(ktx::HIFI_MIN_POPULATED_MIP_KEY) == 0;
        });
        if (found != keyValues.end()) {
            _minMipLevelAvailable = found->_value[0];
        } else {
            // Assume all mip levels are available
            _minMipLevelAvailable = 0;
        }
    }

    // now that we know the ktx, let's get the header info to configure this Texture::Storage:
    Format mipFormat = Format::COLOR_BGRA_32;
    Format texelFormat = Format::COLOR_SRGBA_32;
    if (Texture::evalTextureFormat(_ktxDescriptor->header, mipFormat, texelFormat)) {
        _format = mipFormat;
    }
}

std::shared_ptr<storage::FileStorage> KtxStorage::maybeOpenFile() {
    std::shared_ptr<storage::FileStorage> file = _cacheFile.lock();
    if (file) {
        return file;
    }

    {
        std::lock_guard<std::mutex> lock{ _cacheFileCreateMutex };

        file = _cacheFile.lock();
        if (file) {
            return file;
        }

        file = std::make_shared<storage::FileStorage>(_filename.c_str());
        _cacheFile = file;
    }

    return file;
}

PixelsPointer KtxStorage::getMipFace(uint16 level, uint8 face) const {
    storage::StoragePointer result;
    auto faceOffset = _ktxDescriptor->getMipFaceTexelsOffset(level, face);
    auto faceSize = _ktxDescriptor->getMipFaceTexelsSize(level, face);
    if (faceSize != 0 && faceOffset != 0) {
        result = std::make_shared<storage::FileStorage>(_filename.c_str())->createView(faceSize, faceOffset)->toMemoryStorage();
    }
    return result;
}

Size KtxStorage::getMipFaceSize(uint16 level, uint8 face) const {
    return _ktxDescriptor->getMipFaceTexelsSize(level, face);
}


bool KtxStorage::isMipAvailable(uint16 level, uint8 face) const {
    return level >= _minMipLevelAvailable;
}

uint16 KtxStorage::minAvailableMipLevel() const {
    return _minMipLevelAvailable;
}

void KtxStorage::assignMipData(uint16 level, const storage::StoragePointer& storage) {
    if (level != _minMipLevelAvailable - 1) {
        qWarning() << "Invalid level to be stored, expected: " << (_minMipLevelAvailable - 1) << ", got: " << level << " " << _filename.c_str();
        return;
    }

    if (level >= _ktxDescriptor->images.size()) {
        throw std::runtime_error("Invalid level");
    }

    if (storage->size() != _ktxDescriptor->images[level]._imageSize) {
        qWarning() << "Invalid image size: " << storage->size() << ", expected: " << _ktxDescriptor->images[level]._imageSize
            << ", level: " << level << ", filename: " << QString::fromStdString(_filename);
        return;
    }

    auto file = maybeOpenFile();

    auto data = file->mutableData();
    data += ktx::KTX_HEADER_SIZE + _ktxDescriptor->header.bytesOfKeyValueData + _ktxDescriptor->images[level]._imageOffset;
    data += 4;

    auto offset = _ktxDescriptor->getValueOffsetForKey(ktx::HIFI_MIN_POPULATED_MIP_KEY);
    {
        std::lock_guard<std::mutex> lock { _cacheFileWriteMutex };

        if (level != _minMipLevelAvailable - 1) {
            qWarning() << "Invalid level to be stored";
            return;
        }

        memcpy(data, storage->data(), _ktxDescriptor->images[level]._imageSize);
        _minMipLevelAvailable = level;
        if (offset > 0) {
            memcpy(file->mutableData() + ktx::KTX_HEADER_SIZE + offset, (void*)&_minMipLevelAvailable, 1);
        }
    }
}

void KtxStorage::assignMipFaceData(uint16 level, uint8 face, const storage::StoragePointer& storage) {
    throw std::runtime_error("Invalid call");
}

void Texture::setKtxBacking(const std::string& filename) {
    // Check the KTX file for validity before using it as backing storage
    {
        ktx::StoragePointer storage { new storage::FileStorage(filename.c_str()) };
        auto ktxPointer = ktx::KTX::create(storage);
        if (!ktxPointer) {
            return;
        }
    }

    auto newBacking = std::unique_ptr<Storage>(new KtxStorage(filename));
    setStorage(newBacking);
}


ktx::KTXUniquePointer Texture::serialize(const Texture& texture) {
    ktx::Header header;

    // From texture format to ktx format description
    auto texelFormat = texture.getTexelFormat();
    auto mipFormat = texture.getStoredMipFormat();

    if (!Texture::evalKTXFormat(mipFormat, texelFormat, header)) {
        return nullptr;
    }
 
    // Set Dimensions
    uint32_t numFaces = 1;
    switch (texture.getType()) {
    case TEX_1D: {
            if (texture.isArray()) {
                header.set1DArray(texture.getWidth(), texture.getNumSlices());
            } else {
                header.set1D(texture.getWidth());
            }
            break;
        }
    case TEX_2D: {
            if (texture.isArray()) {
                header.set2DArray(texture.getWidth(), texture.getHeight(), texture.getNumSlices());
            } else {
                header.set2D(texture.getWidth(), texture.getHeight());
            }
            break;
        }
    case TEX_3D: {
            if (texture.isArray()) {
                header.set3DArray(texture.getWidth(), texture.getHeight(), texture.getDepth(), texture.getNumSlices());
            } else {
                header.set3D(texture.getWidth(), texture.getHeight(), texture.getDepth());
            }
            break;
        }
    case TEX_CUBE: {
            if (texture.isArray()) {
                header.setCubeArray(texture.getWidth(), texture.getHeight(), texture.getNumSlices());
            } else {
                header.setCube(texture.getWidth(), texture.getHeight());
            }
            numFaces = Texture::CUBE_FACE_COUNT;
            break;
        }
    default:
        return nullptr;
    }

    // Number level of mips coming
    header.numberOfMipmapLevels = texture.getNumMips();

    ktx::Images images;
    uint32_t imageOffset = 0;
    for (uint32_t level = 0; level < header.numberOfMipmapLevels; level++) {
        auto mip = texture.accessStoredMipFace(level);
        if (mip) {
            if (numFaces == 1) {
                images.emplace_back(ktx::Image(imageOffset, (uint32_t)mip->getSize(), 0, mip->readData()));
            } else {
                ktx::Image::FaceBytes cubeFaces(Texture::CUBE_FACE_COUNT);
                cubeFaces[0] = mip->readData();
                for (uint32_t face = 1; face < Texture::CUBE_FACE_COUNT; face++) {
                    cubeFaces[face] = texture.accessStoredMipFace(level, face)->readData();
                }
                images.emplace_back(ktx::Image(imageOffset, (uint32_t)mip->getSize(), 0, cubeFaces));
            }
            imageOffset += static_cast<uint32_t>(mip->getSize()) + 4;
        }
    }

    GPUKTXPayload keyval;
    keyval._samplerDesc = texture.getSampler().getDesc();
    keyval._usage = texture.getUsage();
    keyval._usageType = texture.getUsageType();
    ktx::KeyValues keyValues;
    keyValues.emplace_back(ktx::KeyValue(GPUKTXPayload::KEY, sizeof(GPUKTXPayload), (ktx::Byte*) &keyval));

    auto hash = texture.sourceHash();
    if (!hash.empty()) {
        keyValues.emplace_back(ktx::KeyValue(SOURCE_HASH_KEY, static_cast<uint32>(hash.size()), (ktx::Byte*) hash.c_str()));
    }

    auto ktxBuffer = ktx::KTX::create(header, images, keyValues);
#if 0
    auto expectedMipCount = texture.evalNumMips();
    assert(expectedMipCount == ktxBuffer->_images.size());
    assert(expectedMipCount == header.numberOfMipmapLevels);

    assert(0 == memcmp(&header, ktxBuffer->getHeader(), sizeof(ktx::Header)));
    assert(ktxBuffer->_images.size() == images.size());
    auto start = ktxBuffer->_storage->data();
    for (size_t i = 0; i < images.size(); ++i) {
        auto expected = images[i];
        auto actual = ktxBuffer->_images[i];
        assert(expected._padding == actual._padding);
        assert(expected._numFaces == actual._numFaces);
        assert(expected._imageSize == actual._imageSize);
        assert(expected._faceSize == actual._faceSize);
        assert(actual._faceBytes.size() == actual._numFaces);
        for (uint32_t face = 0; face < expected._numFaces; ++face) {
            auto expectedFace = expected._faceBytes[face];
            auto actualFace = actual._faceBytes[face];
            auto offset = actualFace - start;
            assert(offset % 4 == 0);
            assert(expectedFace != actualFace);
            assert(0 == memcmp(expectedFace, actualFace, expected._faceSize));
        }
    }
#endif
    return ktxBuffer;
}

TexturePointer Texture::unserialize(const std::string& ktxfile, TextureUsageType usageType, Usage usage, const Sampler::Desc& sampler) {
    std::unique_ptr<ktx::KTX> ktxPointer = ktx::KTX::create(ktx::StoragePointer { new storage::FileStorage(ktxfile.c_str()) });
    if (!ktxPointer) {
        return nullptr;
    }

    ktx::KTXDescriptor descriptor { ktxPointer->toDescriptor() };
    return unserialize(ktxfile, ktxPointer->toDescriptor(), usageType, usage, sampler);
}

TexturePointer Texture::unserialize(const std::string& ktxfile, const ktx::KTXDescriptor& descriptor, TextureUsageType usageType, Usage usage, const Sampler::Desc& sampler) {
    const auto& header = descriptor.header;

    Format mipFormat = Format::COLOR_BGRA_32;
    Format texelFormat = Format::COLOR_SRGBA_32;

    if (!Texture::evalTextureFormat(header, mipFormat, texelFormat)) {
        return nullptr;
    }

    // Find Texture Type based on dimensions
    Type type = TEX_1D;
    if (header.pixelWidth == 0) {
        return nullptr;
    } else if (header.pixelHeight == 0) {
        type = TEX_1D;
    } else if (header.pixelDepth == 0) {
        if (header.numberOfFaces == ktx::NUM_CUBEMAPFACES) {
            type = TEX_CUBE;
        } else {
            type = TEX_2D;
        }
    } else {
        type = TEX_3D;
    }

    
    // If found, use the 
    GPUKTXPayload gpuktxKeyValue;
    bool isGPUKTXPayload = GPUKTXPayload::findInKeyValues(descriptor.keyValues, gpuktxKeyValue);

    auto tex = Texture::create( (isGPUKTXPayload ? gpuktxKeyValue._usageType : usageType),
                                type,
                                texelFormat,
                                header.getPixelWidth(),
                                header.getPixelHeight(),
                                header.getPixelDepth(),
                                1, // num Samples
                                header.getNumberOfSlices(),
                                header.getNumberOfLevels(),
                                (isGPUKTXPayload ? gpuktxKeyValue._samplerDesc : sampler));

    tex->setUsage((isGPUKTXPayload ? gpuktxKeyValue._usage : usage));

    // Assing the mips availables
    tex->setStoredMipFormat(mipFormat);
    tex->setKtxBacking(ktxfile);
    return tex;
}

bool Texture::evalKTXFormat(const Element& mipFormat, const Element& texelFormat, ktx::Header& header) {
    if (texelFormat == Format::COLOR_RGBA_32 && mipFormat == Format::COLOR_BGRA_32) {
        header.setUncompressed(ktx::GLType::UNSIGNED_BYTE, 1, ktx::GLFormat::BGRA, ktx::GLInternalFormat_Uncompressed::RGBA8, ktx::GLBaseInternalFormat::RGBA);
    } else if (texelFormat == Format::COLOR_RGBA_32 && mipFormat == Format::COLOR_RGBA_32) {
        header.setUncompressed(ktx::GLType::UNSIGNED_BYTE, 1, ktx::GLFormat::RGBA, ktx::GLInternalFormat_Uncompressed::RGBA8, ktx::GLBaseInternalFormat::RGBA);
    } else if (texelFormat == Format::COLOR_SRGBA_32 && mipFormat == Format::COLOR_SBGRA_32) {
        header.setUncompressed(ktx::GLType::UNSIGNED_BYTE, 1, ktx::GLFormat::BGRA, ktx::GLInternalFormat_Uncompressed::SRGB8_ALPHA8, ktx::GLBaseInternalFormat::RGBA);
    } else if (texelFormat == Format::COLOR_SRGBA_32 && mipFormat == Format::COLOR_SRGBA_32) {
        header.setUncompressed(ktx::GLType::UNSIGNED_BYTE, 1, ktx::GLFormat::RGBA, ktx::GLInternalFormat_Uncompressed::SRGB8_ALPHA8, ktx::GLBaseInternalFormat::RGBA);
    } else if (texelFormat == Format::COLOR_R_8 && mipFormat == Format::COLOR_R_8) {
        header.setUncompressed(ktx::GLType::UNSIGNED_BYTE, 1, ktx::GLFormat::RED, ktx::GLInternalFormat_Uncompressed::R8, ktx::GLBaseInternalFormat::RED);
    } else if (texelFormat == Format::COLOR_COMPRESSED_SRGB && mipFormat == Format::COLOR_COMPRESSED_SRGB) {
        header.setCompressed(ktx::GLInternalFormat_Compressed::COMPRESSED_SRGB_S3TC_DXT1_EXT, ktx::GLBaseInternalFormat::RGB);
    } else if (texelFormat == Format::COLOR_COMPRESSED_SRGBA_MASK && mipFormat == Format::COLOR_COMPRESSED_SRGBA_MASK) {
        header.setCompressed(ktx::GLInternalFormat_Compressed::COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT, ktx::GLBaseInternalFormat::RGBA);
    } else if (texelFormat == Format::COLOR_COMPRESSED_SRGBA && mipFormat == Format::COLOR_COMPRESSED_SRGBA) {
        header.setCompressed(ktx::GLInternalFormat_Compressed::COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT, ktx::GLBaseInternalFormat::RGBA);
    } else if (texelFormat == Format::COLOR_COMPRESSED_RED && mipFormat == Format::COLOR_COMPRESSED_RED) {
        header.setCompressed(ktx::GLInternalFormat_Compressed::COMPRESSED_RED_RGTC1, ktx::GLBaseInternalFormat::RED);
    } else if (texelFormat == Format::COLOR_COMPRESSED_XY && mipFormat == Format::COLOR_COMPRESSED_XY) {
        header.setCompressed(ktx::GLInternalFormat_Compressed::COMPRESSED_RG_RGTC2, ktx::GLBaseInternalFormat::RG);
    } else {
        return false;
    }

    return true;
}

bool Texture::evalTextureFormat(const ktx::Header& header, Element& mipFormat, Element& texelFormat) {
    if (header.getGLFormat() == ktx::GLFormat::BGRA && header.getGLType() == ktx::GLType::UNSIGNED_BYTE && header.getTypeSize() == 1) {
        if (header.getGLInternaFormat_Uncompressed() == ktx::GLInternalFormat_Uncompressed::RGBA8) {
            mipFormat = Format::COLOR_BGRA_32;
            texelFormat = Format::COLOR_RGBA_32;
        } else if (header.getGLInternaFormat_Uncompressed() == ktx::GLInternalFormat_Uncompressed::SRGB8_ALPHA8) {
            mipFormat = Format::COLOR_SBGRA_32;
            texelFormat = Format::COLOR_SRGBA_32;
        } else {
            return false;
        }
    } else if (header.getGLFormat() == ktx::GLFormat::RGBA && header.getGLType() == ktx::GLType::UNSIGNED_BYTE && header.getTypeSize() == 1) {
        if (header.getGLInternaFormat_Uncompressed() == ktx::GLInternalFormat_Uncompressed::RGBA8) {
            mipFormat = Format::COLOR_RGBA_32;
            texelFormat = Format::COLOR_RGBA_32;
        } else if (header.getGLInternaFormat_Uncompressed() == ktx::GLInternalFormat_Uncompressed::SRGB8_ALPHA8) {
            mipFormat = Format::COLOR_SRGBA_32;
            texelFormat = Format::COLOR_SRGBA_32;
        } else {
            return false;
        }
    } else if (header.getGLFormat() == ktx::GLFormat::RED && header.getGLType() == ktx::GLType::UNSIGNED_BYTE && header.getTypeSize() == 1) {
        mipFormat = Format::COLOR_R_8;
        if (header.getGLInternaFormat_Uncompressed() == ktx::GLInternalFormat_Uncompressed::R8) {
            texelFormat = Format::COLOR_R_8;
        } else {
            return false;
        }
    } else if (header.getGLFormat() == ktx::GLFormat::COMPRESSED_FORMAT && header.getGLType() == ktx::GLType::COMPRESSED_TYPE) {
        if (header.getGLInternaFormat_Compressed() == ktx::GLInternalFormat_Compressed::COMPRESSED_SRGB_S3TC_DXT1_EXT) {
            mipFormat = Format::COLOR_COMPRESSED_SRGB;
            texelFormat = Format::COLOR_COMPRESSED_SRGB;
        } else if (header.getGLInternaFormat_Compressed() == ktx::GLInternalFormat_Compressed::COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT) {
            mipFormat = Format::COLOR_COMPRESSED_SRGBA_MASK;
            texelFormat = Format::COLOR_COMPRESSED_SRGBA_MASK;
        } else if (header.getGLInternaFormat_Compressed() == ktx::GLInternalFormat_Compressed::COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT) {
            mipFormat = Format::COLOR_COMPRESSED_SRGBA;
            texelFormat = Format::COLOR_COMPRESSED_SRGBA;
        } else if (header.getGLInternaFormat_Compressed() == ktx::GLInternalFormat_Compressed::COMPRESSED_RED_RGTC1) {
            mipFormat = Format::COLOR_COMPRESSED_RED;
            texelFormat = Format::COLOR_COMPRESSED_RED;
        } else if (header.getGLInternaFormat_Compressed() == ktx::GLInternalFormat_Compressed::COMPRESSED_RG_RGTC2) {
            mipFormat = Format::COLOR_COMPRESSED_XY;
            texelFormat = Format::COLOR_COMPRESSED_XY;
        } else {
            return false;
        }
    } else {
        return false;
    }
    return true;
}
