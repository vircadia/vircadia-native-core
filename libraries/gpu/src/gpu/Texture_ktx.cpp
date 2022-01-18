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
#include <glm/gtc/type_ptr.hpp>

#include <ktx/KTX.h>

#include "GPULogging.h"

using namespace gpu;

using PixelsPointer = Texture::PixelsPointer;
using KtxStorage = Texture::KtxStorage;

std::vector<std::pair<std::shared_ptr<storage::FileStorage>, std::shared_ptr<std::mutex>>> KtxStorage::_cachedKtxFiles;
std::mutex KtxStorage::_cachedKtxFilesMutex;

struct GPUKTXPayload {
    using Version = uint8;

    static const std::string KEY;
    static const Version CURRENT_VERSION { 2 };
    static const size_t PADDING { 2 };
    static const size_t SIZE { sizeof(Version) + sizeof(Sampler::Desc) + sizeof(uint32) + sizeof(TextureUsageType) + sizeof(glm::ivec2) + PADDING };
    static_assert(GPUKTXPayload::SIZE == 44, "Packing size may differ between platforms");
    static_assert(GPUKTXPayload::SIZE % 4 == 0, "GPUKTXPayload is not 4 bytes aligned");

    Sampler::Desc _samplerDesc;
    Texture::Usage _usage;
    TextureUsageType _usageType;
    glm::ivec2 _originalSize { 0, 0 };

    Byte* serialize(Byte* data) const {
        *(Version*)data = CURRENT_VERSION;
        data += sizeof(Version);

        memcpy(data, &_samplerDesc, sizeof(Sampler::Desc));
        data += sizeof(Sampler::Desc);

        // We can't copy the bitset in Texture::Usage in a crossplateform manner
        // So serialize it manually
        uint32 usageData = _usage._flags.to_ulong();
        memcpy(data, &usageData, sizeof(uint32));
        data += sizeof(uint32);

        memcpy(data, &_usageType, sizeof(TextureUsageType));
        data += sizeof(TextureUsageType);

        memcpy(data, glm::value_ptr(_originalSize), sizeof(glm::ivec2));
        data += sizeof(glm::ivec2);

        return data + PADDING;
    }

    bool unserialize(const Byte* data, size_t size) {
        Version version = *(const Version*)data;
        data += sizeof(Version);

        if (version > CURRENT_VERSION) {
            // If we try to load a version that we don't know how to parse,
            // it will render incorrectly
            return false;
        }

        memcpy(&_samplerDesc, data, sizeof(Sampler::Desc));
        data += sizeof(Sampler::Desc);

        // We can't copy the bitset in Texture::Usage in a crossplateform manner
        // So unserialize it manually
        uint32 usageData;
        memcpy(&usageData, data, sizeof(uint32));
        _usage = Texture::Usage(usageData);
        data += sizeof(uint32);

        memcpy(&_usageType, data, sizeof(TextureUsageType));
        data += sizeof(TextureUsageType);

        if (version >= 2) {
            memcpy(&_originalSize, data, sizeof(glm::ivec2));
            data += sizeof(glm::ivec2);
        }

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

KtxStorage::KtxStorage(const storage::StoragePointer& storage) : _storage(storage) {
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

    // now that we know the ktx, let's get the header info to configure this Texture::Storage:
    Format mipFormat = Format::COLOR_BGRA_32;
    Format texelFormat = Format::COLOR_SRGBA_32;
    if (Texture::evalTextureFormat(_ktxDescriptor->header, mipFormat, texelFormat)) {
        _format = mipFormat;
    }
}

KtxStorage::KtxStorage(const cache::FilePointer& cacheEntry) : KtxStorage(cacheEntry->getFilepath()) {
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

// maybeOpenFile should be called with _cacheFileMutex already held to avoid modifying the file from multiple threads
std::shared_ptr<storage::FileStorage> KtxStorage::maybeOpenFile() const {
    // Try to get the shared_ptr
    std::shared_ptr<storage::FileStorage> file = _cacheFile.lock();
    if (file) {
        return file;
    }

    // If the file isn't open, create it and save a weak_ptr to it
    file = std::make_shared<storage::FileStorage>(_filename.c_str());
    _cacheFile = file;

    {
        // Add the shared_ptr to the global list of open KTX files, to be released at the beginning of the next present thread frame
        std::lock_guard<std::mutex> lock(_cachedKtxFilesMutex);
        _cachedKtxFiles.emplace_back(file, _cacheFileMutex);
    }

    return file;
}

void KtxStorage::releaseOpenKtxFiles() {
    std::vector<std::pair<std::shared_ptr<storage::FileStorage>, std::shared_ptr<std::mutex>>> localKtxFiles;
    {
        std::lock_guard<std::mutex> lock(_cachedKtxFilesMutex);
        localKtxFiles.swap(_cachedKtxFiles);
    }
    for (auto& cacheFileAndMutex : localKtxFiles) {
        std::lock_guard<std::mutex> lock(*(cacheFileAndMutex.second));
        cacheFileAndMutex.first.reset();
    }
}

PixelsPointer KtxStorage::getMipFace(uint16 level, uint8 face) const {
    auto faceOffset = _ktxDescriptor->getMipFaceTexelsOffset(level, face);
    auto faceSize = _ktxDescriptor->getMipFaceTexelsSize(level, face);
    storage::StoragePointer storageView;
    if (faceSize != 0 && faceOffset != 0) {
        if (_storage) {
            storageView = _storage->createView(faceSize, faceOffset);
        } else {
            std::lock_guard<std::mutex> lock(*_cacheFileMutex);
            auto file = maybeOpenFile();
            if (file) {
                storageView = file->createView(faceSize, faceOffset);
            } else {
                qWarning() << "Failed to get a valid file out of maybeOpenFile " << QString::fromStdString(_filename);
            }
        }
    }
    if (!storageView) {
        qWarning() << "Failed to get a valid storageView for faceSize=" << faceSize << "  faceOffset=" << faceOffset
                    << "out of valid file " << QString::fromStdString(_filename);
    }
    return storageView->toMemoryStorage();
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
        qWarning() << "Invalid level to be stored, expected: " << (_minMipLevelAvailable - 1) << ", got: " << level;
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

    std::lock_guard<std::mutex> lock(*_cacheFileMutex);
    auto file = maybeOpenFile();
    if (!file) {
        qWarning() << "Failed to open file to assign mip data ";
        return;
    }

    auto fileData = file->mutableData();
    if (!fileData) {
        qWarning() << "Failed to get mutable data for ";
        return;
    }

    auto imageData = fileData;
    imageData += ktx::KTX_HEADER_SIZE + _ktxDescriptor->header.bytesOfKeyValueData + _ktxDescriptor->images[level]._imageOffset;
    imageData += ktx::IMAGE_SIZE_WIDTH;

    {
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

bool validKtx(const storage::StoragePointer& storage) {
    auto ktxPointer = ktx::KTX::create(storage);
    if (!ktxPointer) {
        return false;
    }
    return true;
}

bool validKtx(const std::string& filename) {
    ktx::StoragePointer storage{ new storage::FileStorage(filename.c_str()) };
    return validKtx(storage);
}

void Texture::setKtxBacking(const storage::StoragePointer& storage) {
    // Check the KTX file for validity before using it as backing storage
    if (!validKtx(storage)) {
        return;
    }

    auto newBacking = std::unique_ptr<Storage>(new KtxStorage(storage));
    setStorage(newBacking);
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


ktx::KTXUniquePointer Texture::serialize(const Texture& texture, const glm::ivec2& originalSize) {
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
    gpuKeyval._originalSize = originalSize;

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

std::pair<TexturePointer, glm::ivec2> Texture::build(const ktx::KTXDescriptor& descriptor) {
    Format mipFormat = Format::COLOR_BGRA_32;
    Format texelFormat = Format::COLOR_SRGBA_32;
    const auto& header = descriptor.header;

    if (!Texture::evalTextureFormat(header, mipFormat, texelFormat)) {
        return { nullptr, { 0, 0 } };
    }

    // Find Texture Type based on dimensions
    Type type = TEX_1D;
    if (header.pixelWidth == 0) {
        return { nullptr, { 0, 0 } };
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
        // FIXME use sensible defaults based on the texture type and format
        gpuktxKeyValue._usageType = TextureUsageType::RESOURCE;
        gpuktxKeyValue._usage = Texture::Usage::Builder().withColor().withAlpha().build();
    }

    auto samplerDesc = gpuktxKeyValue._samplerDesc;
    samplerDesc._maxMip = gpu::Sampler::MAX_MIP_LEVEL;
    auto texture = create(gpuktxKeyValue._usageType,
        type,
        texelFormat,
        header.getPixelWidth(),
        header.getPixelHeight(),
        header.getPixelDepth(),
        1, // num Samples
        header.isArray() ? header.getNumberOfSlices() : 0,
        header.getNumberOfLevels(),
        samplerDesc);
    texture->setUsage(gpuktxKeyValue._usage);

    // Assing the mips availables
    texture->setStoredMipFormat(mipFormat);

    IrradianceKTXPayload irradianceKtxKeyValue;
    if (IrradianceKTXPayload::findInKeyValues(descriptor.keyValues, irradianceKtxKeyValue)) {
        texture->overrideIrradiance(std::make_shared<SphericalHarmonics>(irradianceKtxKeyValue._irradianceSH));
    }

    return { texture, gpuktxKeyValue._originalSize };
}

std::pair<TexturePointer, glm::ivec2> Texture::unserialize(const cache::FilePointer& cacheEntry, const std::string& source) {
    std::unique_ptr<ktx::KTX> ktxPointer = ktx::KTX::create(std::make_shared<storage::FileStorage>(cacheEntry->getFilepath().c_str()));
    if (!ktxPointer) {
        return { nullptr, { 0, 0 } };
    }

    auto textureAndSize = build(ktxPointer->toDescriptor());
    if (textureAndSize.first) {
        textureAndSize.first->setKtxBacking(cacheEntry);
        if (textureAndSize.first->source().empty()) {
            textureAndSize.first->setSource(source);
        }
    }

    return { textureAndSize.first, textureAndSize.second };
}

std::pair<TexturePointer, glm::ivec2> Texture::unserialize(const std::string& ktxfile) {
    std::unique_ptr<ktx::KTX> ktxPointer = ktx::KTX::create(std::make_shared<storage::FileStorage>(ktxfile.c_str()));
    if (!ktxPointer) {
        return { nullptr, { 0, 0 } };
    }

    auto textureAndSize = build(ktxPointer->toDescriptor());
    if (textureAndSize.first) {
        textureAndSize.first->setKtxBacking(ktxfile);
        textureAndSize.first->setSource(ktxfile);
    }

    return { textureAndSize.first, textureAndSize.second };
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
    } else if (texelFormat == Format::COLOR_COMPRESSED_BCX_SRGB && mipFormat == Format::COLOR_COMPRESSED_BCX_SRGB) {
        header.setCompressed(ktx::GLInternalFormat::COMPRESSED_SRGB_S3TC_DXT1_EXT, ktx::GLBaseInternalFormat::RGB);
    } else if (texelFormat == Format::COLOR_COMPRESSED_BCX_SRGBA_MASK && mipFormat == Format::COLOR_COMPRESSED_BCX_SRGBA_MASK) {
        header.setCompressed(ktx::GLInternalFormat::COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT, ktx::GLBaseInternalFormat::RGBA);
    } else if (texelFormat == Format::COLOR_COMPRESSED_BCX_SRGBA && mipFormat == Format::COLOR_COMPRESSED_BCX_SRGBA) {
        header.setCompressed(ktx::GLInternalFormat::COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT, ktx::GLBaseInternalFormat::RGBA);
    } else if (texelFormat == Format::COLOR_COMPRESSED_BCX_RED && mipFormat == Format::COLOR_COMPRESSED_BCX_RED) {
        header.setCompressed(ktx::GLInternalFormat::COMPRESSED_RED_RGTC1, ktx::GLBaseInternalFormat::RED);
    } else if (texelFormat == Format::COLOR_COMPRESSED_BCX_XY && mipFormat == Format::COLOR_COMPRESSED_BCX_XY) {
        header.setCompressed(ktx::GLInternalFormat::COMPRESSED_RG_RGTC2, ktx::GLBaseInternalFormat::RG);
    } else if (texelFormat == Format::COLOR_COMPRESSED_BCX_SRGBA_HIGH && mipFormat == Format::COLOR_COMPRESSED_BCX_SRGBA_HIGH) {
        header.setCompressed(ktx::GLInternalFormat::COMPRESSED_SRGB_ALPHA_BPTC_UNORM, ktx::GLBaseInternalFormat::RGBA);
    } else if (texelFormat == Format::COLOR_COMPRESSED_BCX_HDR_RGB && mipFormat == Format::COLOR_COMPRESSED_BCX_HDR_RGB) {
        header.setCompressed(ktx::GLInternalFormat::COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT, ktx::GLBaseInternalFormat::RGB);
    } else if (texelFormat == Format::COLOR_COMPRESSED_ETC2_RGB && mipFormat == Format::COLOR_COMPRESSED_ETC2_RGB) {
        header.setCompressed(ktx::GLInternalFormat::COMPRESSED_RGB8_ETC2, ktx::GLBaseInternalFormat::RGB);
    } else if (texelFormat == Format::COLOR_COMPRESSED_ETC2_SRGB && mipFormat == Format::COLOR_COMPRESSED_ETC2_SRGB) {
        header.setCompressed(ktx::GLInternalFormat::COMPRESSED_SRGB8_ETC2, ktx::GLBaseInternalFormat::RGB);
    } else if (texelFormat == Format::COLOR_COMPRESSED_ETC2_RGB_PUNCHTHROUGH_ALPHA && mipFormat == Format::COLOR_COMPRESSED_ETC2_RGB_PUNCHTHROUGH_ALPHA) {
        header.setCompressed(ktx::GLInternalFormat::COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2, ktx::GLBaseInternalFormat::RGBA);
    } else if (texelFormat == Format::COLOR_COMPRESSED_ETC2_SRGB_PUNCHTHROUGH_ALPHA && mipFormat == Format::COLOR_COMPRESSED_ETC2_SRGB_PUNCHTHROUGH_ALPHA) {
        header.setCompressed(ktx::GLInternalFormat::COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2, ktx::GLBaseInternalFormat::RGBA);
    } else if (texelFormat == Format::COLOR_COMPRESSED_ETC2_RGBA && mipFormat == Format::COLOR_COMPRESSED_ETC2_RGBA) {
        header.setCompressed(ktx::GLInternalFormat::COMPRESSED_RGBA8_ETC2_EAC, ktx::GLBaseInternalFormat::RGBA);
    } else if (texelFormat == Format::COLOR_COMPRESSED_ETC2_SRGBA && mipFormat == Format::COLOR_COMPRESSED_ETC2_SRGBA) {
        header.setCompressed(ktx::GLInternalFormat::COMPRESSED_SRGB8_ALPHA8_ETC2_EAC, ktx::GLBaseInternalFormat::RGBA);
    } else if (texelFormat == Format::COLOR_COMPRESSED_EAC_RED && mipFormat == Format::COLOR_COMPRESSED_EAC_RED) {
        header.setCompressed(ktx::GLInternalFormat::COMPRESSED_R11_EAC, ktx::GLBaseInternalFormat::RED);
    } else if (texelFormat == Format::COLOR_COMPRESSED_EAC_RED_SIGNED && mipFormat == Format::COLOR_COMPRESSED_EAC_RED_SIGNED) {
        header.setCompressed(ktx::GLInternalFormat::COMPRESSED_SIGNED_R11_EAC, ktx::GLBaseInternalFormat::RED);
    } else if (texelFormat == Format::COLOR_COMPRESSED_EAC_XY && mipFormat == Format::COLOR_COMPRESSED_EAC_XY) {
        header.setCompressed(ktx::GLInternalFormat::COMPRESSED_RG11_EAC, ktx::GLBaseInternalFormat::RG);
    } else if (texelFormat == Format::COLOR_COMPRESSED_EAC_XY_SIGNED && mipFormat == Format::COLOR_COMPRESSED_EAC_XY_SIGNED) {
        header.setCompressed(ktx::GLInternalFormat::COMPRESSED_SIGNED_RG11_EAC, ktx::GLBaseInternalFormat::RG);
    } else if (texelFormat == Format::COLOR_RGB9E5 && mipFormat == Format::COLOR_RGB9E5) {
        header.setUncompressed(ktx::GLType::UNSIGNED_INT_5_9_9_9_REV, 1, ktx::GLFormat::RGB, ktx::GLInternalFormat::RGB9_E5, ktx::GLBaseInternalFormat::RGB);
    } else if (texelFormat == Format::COLOR_R11G11B10 && mipFormat == Format::COLOR_R11G11B10) {
        header.setUncompressed(ktx::GLType::UNSIGNED_INT_10F_11F_11F_REV, 1, ktx::GLFormat::RGB, ktx::GLInternalFormat::R11F_G11F_B10F, ktx::GLBaseInternalFormat::RGB);
    } else {
        return false;
    }

    return true;
}

bool Texture::getCompressedFormat(ktx::GLInternalFormat format, Element& elFormat) {
    if (format == ktx::GLInternalFormat::COMPRESSED_SRGB_S3TC_DXT1_EXT) {
        elFormat = Format::COLOR_COMPRESSED_BCX_SRGB;
    } else if (format == ktx::GLInternalFormat::COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT) {
        elFormat = Format::COLOR_COMPRESSED_BCX_SRGBA_MASK;
    } else if (format == ktx::GLInternalFormat::COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT) {
        elFormat = Format::COLOR_COMPRESSED_BCX_SRGBA;
    } else if (format == ktx::GLInternalFormat::COMPRESSED_RED_RGTC1) {
        elFormat = Format::COLOR_COMPRESSED_BCX_RED;
    } else if (format == ktx::GLInternalFormat::COMPRESSED_RG_RGTC2) {
        elFormat = Format::COLOR_COMPRESSED_BCX_XY;
    } else if (format == ktx::GLInternalFormat::COMPRESSED_SRGB_ALPHA_BPTC_UNORM) {
        elFormat = Format::COLOR_COMPRESSED_BCX_SRGBA_HIGH;
    } else if (format == ktx::GLInternalFormat::COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT) {
        elFormat = Format::COLOR_COMPRESSED_BCX_HDR_RGB;
    } else if (format == ktx::GLInternalFormat::COMPRESSED_RGB8_ETC2) {
        elFormat = Format::COLOR_COMPRESSED_ETC2_RGB;
    } else if (format == ktx::GLInternalFormat::COMPRESSED_SRGB8_ETC2) {
        elFormat = Format::COLOR_COMPRESSED_ETC2_SRGB;
    } else if (format == ktx::GLInternalFormat::COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2) {
        elFormat = Format::COLOR_COMPRESSED_ETC2_RGB_PUNCHTHROUGH_ALPHA;
    } else if (format == ktx::GLInternalFormat::COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2) {
        elFormat = Format::COLOR_COMPRESSED_ETC2_SRGB_PUNCHTHROUGH_ALPHA;
    } else if (format == ktx::GLInternalFormat::COMPRESSED_RGBA8_ETC2_EAC) {
        elFormat = Format::COLOR_COMPRESSED_ETC2_RGBA;
    } else if (format == ktx::GLInternalFormat::COMPRESSED_SRGB8_ALPHA8_ETC2_EAC) {
        elFormat = Format::COLOR_COMPRESSED_ETC2_SRGBA;
    } else if (format == ktx::GLInternalFormat::COMPRESSED_R11_EAC) {
        elFormat = Format::COLOR_COMPRESSED_EAC_RED;
    } else if (format == ktx::GLInternalFormat::COMPRESSED_SIGNED_R11_EAC) {
        elFormat = Format::COLOR_COMPRESSED_EAC_RED_SIGNED;
    } else if (format == ktx::GLInternalFormat::COMPRESSED_RG11_EAC) {
        elFormat = Format::COLOR_COMPRESSED_EAC_XY;
    } else if (format == ktx::GLInternalFormat::COMPRESSED_SIGNED_RG11_EAC) {
        elFormat = Format::COLOR_COMPRESSED_EAC_XY_SIGNED;
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
    } else if (header.getGLFormat() == ktx::GLFormat::RGB && header.getGLType() == ktx::GLType::UNSIGNED_INT_10F_11F_11F_REV) {
        mipFormat = Format::COLOR_R11G11B10;
        texelFormat = Format::COLOR_R11G11B10;
    } else if (header.getGLFormat() == ktx::GLFormat::RGB && header.getGLType() == ktx::GLType::UNSIGNED_INT_5_9_9_9_REV) {
        mipFormat = Format::COLOR_RGB9E5;
        texelFormat = Format::COLOR_RGB9E5;
    } else if (header.isCompressed()) {
        if (!getCompressedFormat(header.getGLInternaFormat(), texelFormat)) {
            return false;
        }
        mipFormat = texelFormat;
    } else {
        return false;
    }
    return true;
}
