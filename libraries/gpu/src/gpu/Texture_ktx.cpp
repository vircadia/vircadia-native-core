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

#include <QtCore/QByteArray>

#include <ktx/KTX.h>

#include "GPULogging.h"

using namespace gpu;

using PixelsPointer = Texture::PixelsPointer;
using KtxStorage = Texture::KtxStorage;

struct GPUKTXPayload {
    using Version = uint8;

    static const std::string KEY;
    static const Version CURRENT_VERSION { 1 };
    static const size_t PADDING { 2 };
    static const size_t SIZE { sizeof(Version) + sizeof(Sampler::Desc) + sizeof(uint32) + sizeof(TextureUsageType) + PADDING };
    static_assert(GPUKTXPayload::SIZE == 36, "Packing size may differ between platforms");
    static_assert(GPUKTXPayload::SIZE % 4 == 0, "GPUKTXPayload is not 4 bytes aligned");

    Sampler::Desc _samplerDesc;
    Texture::Usage _usage;
    TextureUsageType _usageType;

    Byte* serialize(Byte* data) const {
        *(Version*)data = CURRENT_VERSION;
        data += sizeof(Version);

        memcpy(data, &_samplerDesc, sizeof(Sampler::Desc));
        data += sizeof(Sampler::Desc);
        
        // We can't copy the bitset in Texture::Usage in a crossplateform manner
        // So serialize it manually
        *(uint32*)data = _usage._flags.to_ulong();
        data += sizeof(uint32);

        *(TextureUsageType*)data = _usageType;
        data += sizeof(TextureUsageType);

        return data + PADDING;
    }

    bool unserialize(const Byte* data, size_t size) {
        if (size != SIZE) {
            return false;
        }

        Version version = *(const Version*)data;
        if (version != CURRENT_VERSION) {
            glm::vec4 borderColor(1.0f);
            if (memcmp(&borderColor, data, sizeof(glm::vec4)) == 0) {
                memcpy(this, data, sizeof(GPUKTXPayload));
                return true;
            } else {
                return false;
            }
        }
        data += sizeof(Version);

        memcpy(&_samplerDesc, data, sizeof(Sampler::Desc));
        data += sizeof(Sampler::Desc);
        
        // We can't copy the bitset in Texture::Usage in a crossplateform manner
        // So unserialize it manually
        _usage = Texture::Usage(*(const uint32*)data);
        data += sizeof(uint32);

        _usageType = *(const TextureUsageType*)data;
        return true;
    }

    static bool isGPUKTX(const ktx::KeyValue& val) {
        return (val._key.compare(KEY) == 0);
    }

    static bool findInKeyValues(const ktx::KeyValues& keyValues, GPUKTXPayload& payload) {
        auto found = std::find_if(keyValues.begin(), keyValues.end(), isGPUKTX);
        if (found != keyValues.end()) {
            auto value = found->_value;
            return payload.unserialize(value.data(), value.size());
        }
        return false;
    }
};
const std::string GPUKTXPayload::KEY { "hifi.gpu" };


struct IrradianceKTXPayload {
    using Version = uint8;

    static const std::string KEY;
    static const Version CURRENT_VERSION{ 0 };
    static const size_t PADDING{ 3 };
    static const size_t SIZE{ sizeof(Version) +  sizeof(SphericalHarmonics) + PADDING };
    static_assert(IrradianceKTXPayload::SIZE == 148, "Packing size may differ between platforms");
    static_assert(IrradianceKTXPayload::SIZE % 4 == 0, "IrradianceKTXPayload is not 4 bytes aligned");

    SphericalHarmonics _irradianceSH;

    Byte* serialize(Byte* data) const {
        *(Version*)data = CURRENT_VERSION;
        data += sizeof(Version);

        memcpy(data, &_irradianceSH, sizeof(SphericalHarmonics));
        data += sizeof(SphericalHarmonics);

        return data + PADDING;
    }

    bool unserialize(const Byte* data, size_t size) {
        if (size != SIZE) {
            return false;
        }

        Version version = *(const Version*)data;
        if (version != CURRENT_VERSION) {
            return false;
        }
        data += sizeof(Version);

        memcpy(&_irradianceSH, data, sizeof(SphericalHarmonics));
        data += sizeof(SphericalHarmonics);

        return true;
    }

    static bool isIrradianceKTX(const ktx::KeyValue& val) {
        return (val._key.compare(KEY) == 0);
    }

    static bool findInKeyValues(const ktx::KeyValues& keyValues, IrradianceKTXPayload& payload) {
        auto found = std::find_if(keyValues.begin(), keyValues.end(), isIrradianceKTX);
        if (found != keyValues.end()) {
            auto value = found->_value;
            return payload.unserialize(value.data(), value.size());
        }
        return false;
    }
};
const std::string IrradianceKTXPayload::KEY{ "hifi.irradianceSH" };

KtxStorage::KtxStorage(const cache::FilePointer& cacheEntry) : KtxStorage(cacheEntry->getFilepath())  {
    _cacheEntry = cacheEntry;
}

KtxStorage::KtxStorage(const std::string& filename) : _filename(filename) {
    {
        // We are doing a lot of work here just to get descriptor data
        ktx::StoragePointer storage{ new storage::FileStorage(_filename.c_str()) };
        auto ktxPointer = ktx::KTX::create(storage);
        _ktxDescriptor.reset(new ktx::KTXDescriptor(ktxPointer->toDescriptor()));
        if (_ktxDescriptor->images.size() < _ktxDescriptor->header.numberOfMipmapLevels) {
            qWarning() << "Bad images found in ktx";
        }

        _offsetToMinMipKV = _ktxDescriptor->getValueOffsetForKey(ktx::HIFI_MIN_POPULATED_MIP_KEY);
        if (_offsetToMinMipKV) {
            auto data = storage->data() + ktx::KTX_HEADER_SIZE + _offsetToMinMipKV;
            _minMipLevelAvailable = *data;
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

std::shared_ptr<storage::FileStorage> KtxStorage::maybeOpenFile() const {
    // 1. Try to get the shared ptr
    // 2. If it doesn't exist, grab the mutex around its creation
    // 3. If it was created before we got the mutex, return it
    // 4. Otherwise, create it

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
    auto faceOffset = _ktxDescriptor->getMipFaceTexelsOffset(level, face);
    auto faceSize = _ktxDescriptor->getMipFaceTexelsSize(level, face);
    if (faceSize != 0 && faceOffset != 0) {
        auto file = maybeOpenFile();
        if (file) {
            auto storageView = file->createView(faceSize, faceOffset);
            if (storageView) {
                return storageView->toMemoryStorage();
            } else {
                qWarning() << "Failed to get a valid storageView for faceSize=" << faceSize << "  faceOffset=" << faceOffset << "out of valid file " << QString::fromStdString(_filename);
            }
        } else {
            qWarning() << "Failed to get a valid file out of maybeOpenFile " << QString::fromStdString(_filename);
        }
    }
    return nullptr;
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

    auto& imageDesc = _ktxDescriptor->images[level];
    if (storage->size() != imageDesc._imageSize) {
        qWarning() << "Invalid image size: " << storage->size() << ", expected: " << imageDesc._imageSize
            << ", level: " << level << ", filename: " << QString::fromStdString(_filename);
        return;
    }

    auto file = maybeOpenFile();
    if (!file) {
        qWarning() << "Failed to open file to assign mip data " << QString::fromStdString(_filename);
        return;
    }

    auto fileData = file->mutableData();
    if (!fileData) {
        qWarning() << "Failed to get mutable data for " << QString::fromStdString(_filename);
        return;
    }

    auto imageData = fileData;
    imageData += ktx::KTX_HEADER_SIZE + _ktxDescriptor->header.bytesOfKeyValueData + _ktxDescriptor->images[level]._imageOffset;
    imageData += ktx::IMAGE_SIZE_WIDTH;

    {
        std::lock_guard<std::mutex> lock { _cacheFileWriteMutex };

        if (level != _minMipLevelAvailable - 1) {
            qWarning() << "Invalid level to be stored";
            return;
        }

        memcpy(imageData, storage->data(), storage->size());
        _minMipLevelAvailable = level;
        if (_offsetToMinMipKV > 0) {
            auto minMipKeyData = fileData + ktx::KTX_HEADER_SIZE + _offsetToMinMipKV;
            memcpy(minMipKeyData, (void*)&_minMipLevelAvailable, 1);
        }
    }
}

void KtxStorage::assignMipFaceData(uint16 level, uint8 face, const storage::StoragePointer& storage) {
    throw std::runtime_error("Invalid call");
}

bool validKtx(const std::string& filename) {
    ktx::StoragePointer storage { new storage::FileStorage(filename.c_str()) };
    auto ktxPointer = ktx::KTX::create(storage);
    if (!ktxPointer) {
        return false;
    }
    return true;
}

void Texture::setKtxBacking(const std::string& filename) {
    // Check the KTX file for validity before using it as backing storage
    if (!validKtx(filename)) {
        return;
    }

    auto newBacking = std::unique_ptr<Storage>(new KtxStorage(filename));
    setStorage(newBacking);
}

void Texture::setKtxBacking(const cache::FilePointer& cacheEntry) {
    // Check the KTX file for validity before using it as backing storage
    if (!validKtx(cacheEntry->getFilepath())) {
        return;
    }

    auto newBacking = std::unique_ptr<Storage>(new KtxStorage(cacheEntry));
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
            imageOffset += static_cast<uint32_t>(mip->getSize()) + ktx::IMAGE_SIZE_WIDTH;
        }
    }

    GPUKTXPayload gpuKeyval;
    gpuKeyval._samplerDesc = texture.getSampler().getDesc();
    gpuKeyval._usage = texture.getUsage();
    gpuKeyval._usageType = texture.getUsageType();

    Byte keyvalPayload[GPUKTXPayload::SIZE];
    gpuKeyval.serialize(keyvalPayload);

    ktx::KeyValues keyValues;
    keyValues.emplace_back(GPUKTXPayload::KEY, (uint32)GPUKTXPayload::SIZE, (ktx::Byte*) &keyvalPayload);

    if (texture.getIrradiance()) {
        IrradianceKTXPayload irradianceKeyval;
        irradianceKeyval._irradianceSH = *texture.getIrradiance();

        Byte irradianceKeyvalPayload[IrradianceKTXPayload::SIZE];
        irradianceKeyval.serialize(irradianceKeyvalPayload);

        keyValues.emplace_back(IrradianceKTXPayload::KEY, (uint32)IrradianceKTXPayload::SIZE, (ktx::Byte*) &irradianceKeyvalPayload);
    }

    auto hash = texture.sourceHash();
    if (!hash.empty()) {
        // the sourceHash is an std::string in hex
        // we use QByteArray to take the hex and turn it into the smaller binary representation (16 bytes)
        auto binaryHash = QByteArray::fromHex(QByteArray::fromStdString(hash));
        keyValues.emplace_back(SOURCE_HASH_KEY, static_cast<uint32>(binaryHash.size()), (ktx::Byte*) binaryHash.data());
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

TexturePointer Texture::build(const ktx::KTXDescriptor& descriptor) {
    Format mipFormat = Format::COLOR_BGRA_32;
    Format texelFormat = Format::COLOR_SRGBA_32;
    const auto& header = descriptor.header;

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

    GPUKTXPayload gpuktxKeyValue;
    if (!GPUKTXPayload::findInKeyValues(descriptor.keyValues, gpuktxKeyValue)) {
        qCWarning(gpulogging) << "Could not find GPUKTX key values.";
        return TexturePointer();
    }

    auto texture = create(gpuktxKeyValue._usageType,
        type,
        texelFormat,
        header.getPixelWidth(),
        header.getPixelHeight(),
        header.getPixelDepth(),
        1, // num Samples
        header.getNumberOfSlices(),
        header.getNumberOfLevels(),
        gpuktxKeyValue._samplerDesc);
    texture->setUsage(gpuktxKeyValue._usage);

    // Assing the mips availables
    texture->setStoredMipFormat(mipFormat);

    IrradianceKTXPayload irradianceKtxKeyValue;
    if (IrradianceKTXPayload::findInKeyValues(descriptor.keyValues, irradianceKtxKeyValue)) {
        texture->overrideIrradiance(std::make_shared<SphericalHarmonics>(irradianceKtxKeyValue._irradianceSH));
    }

    return texture;
}



TexturePointer Texture::unserialize(const cache::FilePointer& cacheEntry) {
    std::unique_ptr<ktx::KTX> ktxPointer = ktx::KTX::create(std::make_shared<storage::FileStorage>(cacheEntry->getFilepath().c_str()));
    if (!ktxPointer) {
        return nullptr;
    }

    auto texture = build(ktxPointer->toDescriptor());
    if (texture) {
        texture->setKtxBacking(cacheEntry);
    }

    return texture;
}

TexturePointer Texture::unserialize(const std::string& ktxfile) {
    std::unique_ptr<ktx::KTX> ktxPointer = ktx::KTX::create(std::make_shared<storage::FileStorage>(ktxfile.c_str()));
    if (!ktxPointer) {
        return nullptr;
    }
    
    auto texture = build(ktxPointer->toDescriptor());
    if (texture) {
        texture->setKtxBacking(ktxfile);
    }

    return texture;
}

bool Texture::evalKTXFormat(const Element& mipFormat, const Element& texelFormat, ktx::Header& header) {
    if (texelFormat == Format::COLOR_RGBA_32 && mipFormat == Format::COLOR_BGRA_32) {
        header.setUncompressed(ktx::GLType::UNSIGNED_BYTE, 1, ktx::GLFormat::BGRA, ktx::GLInternalFormat::RGBA8, ktx::GLBaseInternalFormat::RGBA);
    } else if (texelFormat == Format::COLOR_RGBA_32 && mipFormat == Format::COLOR_RGBA_32) {
        header.setUncompressed(ktx::GLType::UNSIGNED_BYTE, 1, ktx::GLFormat::RGBA, ktx::GLInternalFormat::RGBA8, ktx::GLBaseInternalFormat::RGBA);
    } else if (texelFormat == Format::COLOR_SRGBA_32 && mipFormat == Format::COLOR_SBGRA_32) {
        header.setUncompressed(ktx::GLType::UNSIGNED_BYTE, 1, ktx::GLFormat::BGRA, ktx::GLInternalFormat::SRGB8_ALPHA8, ktx::GLBaseInternalFormat::RGBA);
    } else if (texelFormat == Format::COLOR_SRGBA_32 && mipFormat == Format::COLOR_SRGBA_32) {
        header.setUncompressed(ktx::GLType::UNSIGNED_BYTE, 1, ktx::GLFormat::RGBA, ktx::GLInternalFormat::SRGB8_ALPHA8, ktx::GLBaseInternalFormat::RGBA);
    } else if (texelFormat == Format::COLOR_R_8 && mipFormat == Format::COLOR_R_8) {
        header.setUncompressed(ktx::GLType::UNSIGNED_BYTE, 1, ktx::GLFormat::RED, ktx::GLInternalFormat::R8, ktx::GLBaseInternalFormat::RED);
    } else if (texelFormat == Format::VEC2NU8_XY && mipFormat == Format::VEC2NU8_XY) {
        header.setUncompressed(ktx::GLType::UNSIGNED_BYTE, 1, ktx::GLFormat::RG, ktx::GLInternalFormat::RG8, ktx::GLBaseInternalFormat::RG);
    } else if (texelFormat == Format::COLOR_COMPRESSED_SRGB && mipFormat == Format::COLOR_COMPRESSED_SRGB) {
        header.setCompressed(ktx::GLInternalFormat::COMPRESSED_SRGB_S3TC_DXT1_EXT, ktx::GLBaseInternalFormat::RGB);
    } else if (texelFormat == Format::COLOR_COMPRESSED_SRGBA_MASK && mipFormat == Format::COLOR_COMPRESSED_SRGBA_MASK) {
        header.setCompressed(ktx::GLInternalFormat::COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT, ktx::GLBaseInternalFormat::RGBA);
    } else if (texelFormat == Format::COLOR_COMPRESSED_SRGBA && mipFormat == Format::COLOR_COMPRESSED_SRGBA) {
        header.setCompressed(ktx::GLInternalFormat::COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT, ktx::GLBaseInternalFormat::RGBA);
    } else if (texelFormat == Format::COLOR_COMPRESSED_RED && mipFormat == Format::COLOR_COMPRESSED_RED) {
        header.setCompressed(ktx::GLInternalFormat::COMPRESSED_RED_RGTC1, ktx::GLBaseInternalFormat::RED);
    } else if (texelFormat == Format::COLOR_COMPRESSED_XY && mipFormat == Format::COLOR_COMPRESSED_XY) {
        header.setCompressed(ktx::GLInternalFormat::COMPRESSED_RG_RGTC2, ktx::GLBaseInternalFormat::RG);
    } else if (texelFormat == Format::COLOR_COMPRESSED_SRGBA_HIGH && mipFormat == Format::COLOR_COMPRESSED_SRGBA_HIGH) {
        header.setCompressed(ktx::GLInternalFormat::COMPRESSED_SRGB_ALPHA_BPTC_UNORM, ktx::GLBaseInternalFormat::RGBA);
    } else {
        return false;
    }

    return true;
}

bool Texture::evalTextureFormat(const ktx::Header& header, Element& mipFormat, Element& texelFormat) {
    if (header.getGLFormat() == ktx::GLFormat::BGRA && header.getGLType() == ktx::GLType::UNSIGNED_BYTE && header.getTypeSize() == 1) {
        if (header.getGLInternaFormat() == ktx::GLInternalFormat::RGBA8) {
            mipFormat = Format::COLOR_BGRA_32;
            texelFormat = Format::COLOR_RGBA_32;
        } else if (header.getGLInternaFormat() == ktx::GLInternalFormat::SRGB8_ALPHA8) {
            mipFormat = Format::COLOR_SBGRA_32;
            texelFormat = Format::COLOR_SRGBA_32;
        } else {
            return false;
        }
    } else if (header.getGLFormat() == ktx::GLFormat::RGBA && header.getGLType() == ktx::GLType::UNSIGNED_BYTE && header.getTypeSize() == 1) {
        if (header.getGLInternaFormat() == ktx::GLInternalFormat::RGBA8) {
            mipFormat = Format::COLOR_RGBA_32;
            texelFormat = Format::COLOR_RGBA_32;
        } else if (header.getGLInternaFormat() == ktx::GLInternalFormat::SRGB8_ALPHA8) {
            mipFormat = Format::COLOR_SRGBA_32;
            texelFormat = Format::COLOR_SRGBA_32;
        } else {
            return false;
        }
    } else if (header.getGLFormat() == ktx::GLFormat::RED && header.getGLType() == ktx::GLType::UNSIGNED_BYTE && header.getTypeSize() == 1) {
        mipFormat = Format::COLOR_R_8;
        if (header.getGLInternaFormat() == ktx::GLInternalFormat::R8) {
            texelFormat = Format::COLOR_R_8;
        } else {
            return false;
        }
    } else if (header.getGLFormat() == ktx::GLFormat::RG && header.getGLType() == ktx::GLType::UNSIGNED_BYTE && header.getTypeSize() == 1) {
        mipFormat = Format::VEC2NU8_XY;
        if (header.getGLInternaFormat() == ktx::GLInternalFormat::RG8) {
            texelFormat = Format::VEC2NU8_XY;
        } else {
            return false;
        }
    } else if (header.isCompressed()) {
        if (header.getGLInternaFormat() == ktx::GLInternalFormat::COMPRESSED_SRGB_S3TC_DXT1_EXT) {
            mipFormat = Format::COLOR_COMPRESSED_SRGB;
            texelFormat = Format::COLOR_COMPRESSED_SRGB;
        } else if (header.getGLInternaFormat() == ktx::GLInternalFormat::COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT) {
            mipFormat = Format::COLOR_COMPRESSED_SRGBA_MASK;
            texelFormat = Format::COLOR_COMPRESSED_SRGBA_MASK;
        } else if (header.getGLInternaFormat() == ktx::GLInternalFormat::COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT) {
            mipFormat = Format::COLOR_COMPRESSED_SRGBA;
            texelFormat = Format::COLOR_COMPRESSED_SRGBA;
        } else if (header.getGLInternaFormat() == ktx::GLInternalFormat::COMPRESSED_RED_RGTC1) {
            mipFormat = Format::COLOR_COMPRESSED_RED;
            texelFormat = Format::COLOR_COMPRESSED_RED;
        } else if (header.getGLInternaFormat() == ktx::GLInternalFormat::COMPRESSED_RG_RGTC2) {
            mipFormat = Format::COLOR_COMPRESSED_XY;
            texelFormat = Format::COLOR_COMPRESSED_XY;
        } else if (header.getGLInternaFormat() == ktx::GLInternalFormat::COMPRESSED_SRGB_ALPHA_BPTC_UNORM) {
            mipFormat = Format::COLOR_COMPRESSED_SRGBA_HIGH;
            texelFormat = Format::COLOR_COMPRESSED_SRGBA_HIGH;
        } else {
            return false;
        }
    } else {
        return false;
    }
    return true;
}
