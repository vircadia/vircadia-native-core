//
//  Texture.cpp
//  libraries/gpu/src/gpu
//
//  Created by Sam Gateau on 1/17/2015.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#include "Texture.h"

#include <glm/gtc/constants.hpp>
#include <glm/gtx/component_wise.hpp>

#include <QtCore/QDebug>
#include <QtCore/QThread>
#include <Trace.h>

#include <ktx/KTX.h>
#include <NumericalConstants.h>

#include "GPULogging.h"
#include "Context.h"

#include "ColorUtils.h"

using namespace gpu;

int TexturePointerMetaTypeId = qRegisterMetaType<TexturePointer>();


ContextMetricCount Texture::_textureCPUCount;
ContextMetricSize Texture::_textureCPUMemSize;
std::atomic<Texture::Size> Texture::_allowedCPUMemoryUsage { 0 };


#define MIN_CORES_FOR_INCREMENTAL_TEXTURES 5
bool recommendedSparseTextures = (QThread::idealThreadCount() >= MIN_CORES_FOR_INCREMENTAL_TEXTURES);

std::atomic<bool> Texture::_enableSparseTextures { recommendedSparseTextures };

struct ReportTextureState {
    ReportTextureState() {
        qCDebug(gpulogging) << "[TEXTURE TRANSFER SUPPORT]"
            << "\n\tidealThreadCount:" << QThread::idealThreadCount()
            << "\n\tRECOMMENDED enableSparseTextures:" << recommendedSparseTextures;
    }
} report;

void Texture::setEnableSparseTextures(bool enabled) {
#ifdef Q_OS_WIN
    qCDebug(gpulogging) << "[TEXTURE TRANSFER SUPPORT] SETTING - Enable Sparse Textures and Dynamic Texture Management:" << enabled;
    _enableSparseTextures = enabled;
#else
    qCDebug(gpulogging) << "[TEXTURE TRANSFER SUPPORT] Sparse Textures and Dynamic Texture Management not supported on this platform.";
#endif
}

bool Texture::getEnableSparseTextures() { 
    return _enableSparseTextures.load(); 
}

uint32_t Texture::getTextureCPUCount() {
    return _textureCPUCount.getValue();
}

Texture::Size Texture::getTextureCPUMemSize() {
    return _textureCPUMemSize.getValue();
}


Texture::Size Texture::getAllowedGPUMemoryUsage() {
    return _allowedCPUMemoryUsage;
}


void Texture::setAllowedGPUMemoryUsage(Size size) {
    qCDebug(gpulogging) << "New MAX texture memory " << BYTES_TO_MB(size) << " MB";
    _allowedCPUMemoryUsage = size;
}

uint8 Texture::NUM_FACES_PER_TYPE[NUM_TYPES] = { 1, 1, 1, 6 };

using Storage = Texture::Storage;
using PixelsPointer = Texture::PixelsPointer;
using MemoryStorage = Texture::MemoryStorage;

void Storage::assignTexture(Texture* texture) {
    _texture = texture;
    if (_texture) {
        _type = _texture->getType();
    }
}

void MemoryStorage::reset() {
    _mips.clear();
    bumpStamp();
}

PixelsPointer MemoryStorage::getMipFace(uint16 level, uint8 face) const {
    if (level < _mips.size()) {
        assert(face < _mips[level].size());
        return _mips[level][face];
    }
    return PixelsPointer();
}


Size MemoryStorage::getMipFaceSize(uint16 level, uint8 face) const {
    PixelsPointer mipFace = getMipFace(level, face);
    if (mipFace) {
        return mipFace->getSize();
    } else {
        return 0;
    }
}

bool MemoryStorage::isMipAvailable(uint16 level, uint8 face) const {
    PixelsPointer mipFace = getMipFace(level, face);
    return (mipFace && mipFace->getSize());
}

void MemoryStorage::allocateMip(uint16 level) {
    auto faceCount = Texture::NUM_FACES_PER_TYPE[getType()];
    if (level >= _mips.size()) {
        _mips.resize(level + 1, std::vector<PixelsPointer>(faceCount));
    }

    auto& mip = _mips[level];
    if (mip.size() != faceCount) {
        mip.resize(faceCount);
    }
    bumpStamp();
}

void MemoryStorage::assignMipData(uint16 level, const storage::StoragePointer& storagePointer) {
    allocateMip(level);
    auto& mip = _mips[level];

    auto faceCount = Texture::NUM_FACES_PER_TYPE[getType()];

    // here we grabbed an array of faces
    // The bytes assigned here are supposed to contain all the faces bytes of the mip.
    // For tex1D, 2D, 3D there is only one face
    // For Cube, we expect the 6 faces in the order X+, X-, Y+, Y-, Z+, Z-
    auto sizePerFace = storagePointer->size() / faceCount;

    if (sizePerFace > 0) {
        size_t offset = 0;
        for (auto& face : mip) {
            face = storagePointer->createView(sizePerFace, offset);
            offset += sizePerFace;
        }

        bumpStamp();
    }
}


void Texture::MemoryStorage::assignMipFaceData(uint16 level, uint8 face, const storage::StoragePointer& storagePointer) {
    allocateMip(level);
    auto& mip = _mips[level];
    if (face < mip.size()) { 
        mip[face] = storagePointer;
        bumpStamp();
    }
}

TexturePointer Texture::createExternal(const ExternalRecycler& recycler, const Sampler& sampler) {
    TexturePointer tex = std::make_shared<Texture>(TextureUsageType::EXTERNAL);
    tex->_type = TEX_2D;
    tex->_texelFormat = Element::COLOR_RGBA_32;
    tex->_maxMipLevel = 0;
    tex->_sampler = sampler;
    tex->setExternalRecycler(recycler);
    return tex;
}

TexturePointer Texture::createRenderBuffer(const Element& texelFormat, uint16 width, uint16 height, uint16 numMips, const Sampler& sampler) {
    return create(TextureUsageType::RENDERBUFFER, TEX_2D, texelFormat, width, height, 1, 1, 0, numMips, sampler);
}

TexturePointer Texture::create1D(const Element& texelFormat, uint16 width, uint16 numMips, const Sampler& sampler) {
    return create(TextureUsageType::RESOURCE, TEX_1D, texelFormat, width, 1, 1, 1, 0, numMips, sampler);
}

TexturePointer Texture::create2D(const Element& texelFormat, uint16 width, uint16 height, uint16 numMips, const Sampler& sampler) {
    return create(TextureUsageType::RESOURCE, TEX_2D, texelFormat, width, height, 1, 1, 0, numMips, sampler);
}

TexturePointer Texture::createStrict(const Element& texelFormat, uint16 width, uint16 height, uint16 numMips, const Sampler& sampler) {
    return create(TextureUsageType::STRICT_RESOURCE, TEX_2D, texelFormat, width, height, 1, 1, 0, numMips, sampler);
}

TexturePointer Texture::create3D(const Element& texelFormat, uint16 width, uint16 height, uint16 depth, uint16 numMips, const Sampler& sampler) {
    return create(TextureUsageType::RESOURCE, TEX_3D, texelFormat, width, height, depth, 1, 0, numMips, sampler);
}

TexturePointer Texture::createCube(const Element& texelFormat, uint16 width, uint16 numMips, const Sampler& sampler) {
    return create(TextureUsageType::RESOURCE, TEX_CUBE, texelFormat, width, width, 1, 1, 0, numMips, sampler);
}

TexturePointer Texture::create(TextureUsageType usageType, Type type, const Element& texelFormat, uint16 width, uint16 height, uint16 depth, uint16 numSamples, uint16 numSlices, uint16 numMips, const Sampler& sampler)
{
    TexturePointer tex = std::make_shared<Texture>(usageType);
    tex->_storage.reset(new MemoryStorage());
    tex->_type = type;
    tex->_storage->assignTexture(tex.get());
    tex->resize(type, texelFormat, width, height, depth, numSamples, numSlices, numMips);

    tex->_sampler = sampler;

    return tex;
}

Texture::Texture(TextureUsageType usageType) :
    Resource(), _usageType(usageType) {
    _textureCPUCount.increment();
}

Texture::~Texture() {
    _textureCPUCount.decrement();
    if (_usageType == TextureUsageType::EXTERNAL) {
        Texture::ExternalUpdates externalUpdates;
        {
            Lock lock(_externalMutex);
            _externalUpdates.swap(externalUpdates);
        }
        for (const auto& update : externalUpdates) {
            assert(_externalRecycler);
            _externalRecycler(update.first, update.second);
        }
        // Force the GL object to be destroyed here
        // If we let the normal destructor do it, then it will be 
        // cleared after the _externalRecycler has been destroyed, 
        // resulting in leaked texture memory
        gpuObject.setGPUObject(nullptr);
    }
}

Texture::Size Texture::resize(Type type, const Element& texelFormat, uint16 width, uint16 height, uint16 depth, uint16 numSamples, uint16 numSlices, uint16 numMips) {
    if (width && height && depth && numSamples) {
        bool changed = false;

        if ( _type != type) {
            _type = type;
            changed = true;
        }

        if (_numSlices != numSlices) {
            _numSlices = numSlices;
            changed = true;
        }

        numSamples = evalNumSamplesUsed(numSamples);
        if ((_type >= TEX_2D) && (_numSamples != numSamples)) {
            _numSamples = numSamples;
            changed = true;
        }

        if (_width != width) {
            _width = width;
            changed = true;
        }

        if ((_type >= TEX_2D) && (_height != height)) {
            _height = height;
            changed = true;
        }


        if ((_type >= TEX_3D) && (_depth != depth)) {
            _depth = depth;
            changed = true;
        }

        if ((_maxMipLevel + 1) != numMips) {
            _maxMipLevel = safeNumMips(numMips) - 1;
            changed = true;
        }

        if (texelFormat != _texelFormat) {
            _texelFormat = texelFormat;
            changed = true;
        }

        // Evaluate the new size with the new format
        Size size = NUM_FACES_PER_TYPE[_type] * _height * _depth * evalPaddedSize(_numSamples * _width * _texelFormat.getSize());

        // If size change then we need to reset 
        if (changed || (size != getSize())) {
            _size = size;
            _storage->reset();
            _stamp++;
        }

        // Here the Texture has been fully defined from the gpu point of view (size and format)
        _defined = true;
    } else {
         _stamp++;
    }

    return _size;
}

bool Texture::isColorRenderTarget() const {
    return (_texelFormat.getSemantic() == gpu::RGBA);
}

bool Texture::isDepthStencilRenderTarget() const {
    return (_texelFormat.getSemantic() == gpu::DEPTH) || (_texelFormat.getSemantic() == gpu::DEPTH_STENCIL);
}

uint16 Texture::evalDimMaxNumMips(uint16 size) {
    double largerDim = size;
    double val = log(largerDim)/log(2.0);
    return 1 + (uint16) val;
}

static const double LOG_2 = log(2.0);

uint16 Texture::evalMaxNumMips(const Vec3u& dimensions) {
    double largerDim = glm::compMax(dimensions);
    double val = log(largerDim) / LOG_2;
    return 1 + (uint16)val;
}

// The number mips that the texture could have if all existed
// = log2(max(width, height, depth))
uint16 Texture::evalMaxNumMips() const {
    return evalMaxNumMips({ _width, _height, _depth });
}

// Check a num of mips requested against the maximum possible specified
// if passing -1 then answer the max
// simply does (askedNumMips == 0 ? maxNumMips : (numstd::min(askedNumMips, maxNumMips))
uint16 Texture::safeNumMips(uint16 askedNumMips, uint16 maxNumMips) {
    if (askedNumMips > 0) {
        return std::min(askedNumMips, maxNumMips);
    } else {
        return maxNumMips;
    }
}

// Same but applied to this texture's num max mips from evalNumMips()
uint16 Texture::safeNumMips(uint16 askedNumMips) const {
    return safeNumMips(askedNumMips, evalMaxNumMips());
}

Size Texture::evalTotalSize(uint16 startingMip) const {
    Size size = 0;
    uint16 minMipLevel = std::max(getMinMip(), startingMip);
    uint16 maxMipLevel = getMaxMip();
    for (uint16 level = minMipLevel; level <= maxMipLevel; level++) {
        size += evalMipSize(level);
    }
    return size * getNumSlices();
}

void Texture::setStoredMipFormat(const Element& format) {
    _storage->setFormat(format);
}

Element Texture::getStoredMipFormat() const {
    if (_storage) {
        return _storage->getFormat();
    } else {
        return Element();
    }
}

void Texture::assignStoredMip(uint16 level, Size size, const Byte* bytes) {
    // TODO Skip the extra allocation here
    storage::StoragePointer storage = std::make_shared<storage::MemoryStorage>(size, bytes);
    assignStoredMip(level, storage);
}

void Texture::assignStoredMipFace(uint16 level, uint8 face, Size size, const Byte* bytes) {
    storage::StoragePointer storage = std::make_shared<storage::MemoryStorage>(size, bytes);
    assignStoredMipFace(level, face, storage);
}

void Texture::assignStoredMip(uint16 level, storage::StoragePointer& storage) {
    // Check that level accessed make sense
    if (level != 0) {
        if (_autoGenerateMips) {
            return;
        }
        if (level >= getNumMips()) {
            return;
        }
    }

    // THen check that the mem texture passed make sense with its format
    Size expectedSize = evalStoredMipSize(level, getStoredMipFormat());
    auto size = storage->size();
    // NOTE: doing the same thing in all the next block but beeing able to breakpoint with more accuracy
    if (storage->size() < expectedSize) {
        _storage->assignMipData(level, storage);
        _stamp++;
    } else if (size == expectedSize) {
        _storage->assignMipData(level, storage);
        _stamp++;
    } else if (size > expectedSize) {
        // NOTE: We are facing this case sometime because apparently QImage (from where we get the bits) is generating images
        // and alligning the line of pixels to 32 bits.
        // We should probably consider something a bit more smart to get the correct result but for now (UI elements)
        // it seems to work...
        _storage->assignMipData(level, storage);
        _stamp++;
    }
}

void Texture::assignStoredMipFace(uint16 level, uint8 face, storage::StoragePointer& storage) {
    // Check that level accessed make sense
    if (level != 0) {
        if (_autoGenerateMips) {
            return;
        }
        if (level >= getNumMips()) {
            return;
        }
    }

    // THen check that the mem texture passed make sense with its format
    Size expectedSize = evalStoredMipFaceSize(level, getStoredMipFormat());
    auto size = storage->size();
    // NOTE: doing the same thing in all the next block but beeing able to breakpoint with more accuracy
    if (size < expectedSize) {
        _storage->assignMipFaceData(level, face, storage);
        _stamp++;
    } else if (size == expectedSize) {
        _storage->assignMipFaceData(level, face, storage);
        _stamp++;
    } else if (size > expectedSize) {
        // NOTE: We are facing this case sometime because apparently QImage (from where we get the bits) is generating images
        // and alligning the line of pixels to 32 bits.
        // We should probably consider something a bit more smart to get the correct result but for now (UI elements)
        // it seems to work...
        _storage->assignMipFaceData(level, face, storage);
        _stamp++;
    }
}

bool Texture::isStoredMipFaceAvailable(uint16 level, uint8 face) const {
    return _storage->isMipAvailable(level, face);
}

void Texture::setAutoGenerateMips(bool enable) {
    bool changed = false;
    if (!_autoGenerateMips) {
        changed = true;
        _autoGenerateMips = true;
    }

    if (changed) {
        _stamp++;
    }
}

Size Texture::getStoredMipSize(uint16 level) const {
    Size size = 0;
    for (int face = 0; face < getNumFaces(); face++) {
        if (isStoredMipFaceAvailable(level, face)) {
            size += getStoredMipFaceSize(level, face);
        }
    }
    return size;
}

Size Texture::getStoredSize() const {
    Size size = 0;
    for (int level = 0; level < getNumMips(); level++) {
        size += getStoredMipSize(level);
    }
    return size;
}

uint16 Texture::evalNumSamplesUsed(uint16 numSamplesTried) {
    uint16 sample = numSamplesTried;
    if (numSamplesTried <= 1)
        sample = 1;
    else if (numSamplesTried < 4)
        sample = 2;
    else if (numSamplesTried < 8)
        sample = 4;
    else if (numSamplesTried < 16)
        sample = 8;
    else
        sample = 8;

    return sample;
}

void Texture::setSampler(const Sampler& sampler) {
    _sampler = sampler;
    _samplerStamp++;
}


bool Texture::generateIrradiance() {
    if (getType() != TEX_CUBE) {
        return false;
    }
    if (!isDefined()) {
        return false;
    }
    if (!_irradiance) {
        _irradiance = std::make_shared<SphericalHarmonics>();
    }

    _irradiance->evalFromTexture(*this);
    return true;
}

void SphericalHarmonics::assignPreset(int p) {
    switch (p) {
    case OLD_TOWN_SQUARE: {
            L00    = glm::vec3( 0.871297f, 0.875222f, 0.864470f);
            L1m1   = glm::vec3( 0.175058f, 0.245335f, 0.312891f);
            L10    = glm::vec3( 0.034675f, 0.036107f, 0.037362f);
            L11    = glm::vec3(-0.004629f,-0.029448f,-0.048028f);
            L2m2   = glm::vec3(-0.120535f,-0.121160f,-0.117507f);
            L2m1   = glm::vec3( 0.003242f, 0.003624f, 0.007511f);
            L20    = glm::vec3(-0.028667f,-0.024926f,-0.020998f);
            L21    = glm::vec3(-0.077539f,-0.086325f,-0.091591f);
            L22    = glm::vec3(-0.161784f,-0.191783f,-0.219152f);
        }
        break;
    case GRACE_CATHEDRAL: {
            L00    = glm::vec3( 0.79f,  0.44f,  0.54f);
            L1m1   = glm::vec3( 0.39f,  0.35f,  0.60f);
            L10    = glm::vec3(-0.34f, -0.18f, -0.27f);
            L11    = glm::vec3(-0.29f, -0.06f,  0.01f);
            L2m2   = glm::vec3(-0.11f, -0.05f, -0.12f);
            L2m1   = glm::vec3(-0.26f, -0.22f, -0.47f);
            L20    = glm::vec3(-0.16f, -0.09f, -0.15f);
            L21    = glm::vec3( 0.56f,  0.21f,  0.14f);
            L22    = glm::vec3( 0.21f, -0.05f, -0.30f);
        }
        break;
    case EUCALYPTUS_GROVE: {
            L00    = glm::vec3( 0.38f,  0.43f,  0.45f);
            L1m1   = glm::vec3( 0.29f,  0.36f,  0.41f);
            L10    = glm::vec3( 0.04f,  0.03f,  0.01f);
            L11    = glm::vec3(-0.10f, -0.10f, -0.09f);
            L2m2   = glm::vec3(-0.06f, -0.06f, -0.04f);
            L2m1   = glm::vec3( 0.01f, -0.01f, -0.05f);
            L20    = glm::vec3(-0.09f, -0.13f, -0.15f);
            L21    = glm::vec3(-0.06f, -0.05f, -0.04f);
            L22    = glm::vec3( 0.02f,  0.00f, -0.05f);
        }
        break;
    case ST_PETERS_BASILICA: {
            L00    = glm::vec3( 0.36f,  0.26f,  0.23f);
            L1m1   = glm::vec3( 0.18f,  0.14f,  0.13f);
            L10    = glm::vec3(-0.02f, -0.01f,  0.00f);
            L11    = glm::vec3( 0.03f,  0.02f, -0.00f);
            L2m2   = glm::vec3( 0.02f,  0.01f, -0.00f);
            L2m1   = glm::vec3(-0.05f, -0.03f, -0.01f);
            L20    = glm::vec3(-0.09f, -0.08f, -0.07f);
            L21    = glm::vec3( 0.01f,  0.00f,  0.00f);
            L22    = glm::vec3(-0.08f, -0.03f, -0.00f);
        }
        break;
    case UFFIZI_GALLERY: {
            L00    = glm::vec3( 0.32f,  0.31f,  0.35f);
            L1m1   = glm::vec3( 0.37f,  0.37f,  0.43f);
            L10    = glm::vec3( 0.00f,  0.00f,  0.00f);
            L11    = glm::vec3(-0.01f, -0.01f, -0.01f);
            L2m2   = glm::vec3(-0.02f, -0.02f, -0.03f);
            L2m1   = glm::vec3(-0.01f, -0.01f, -0.01f);
            L20    = glm::vec3(-0.28f, -0.28f, -0.32f);
            L21    = glm::vec3( 0.00f,  0.00f,  0.00f);
            L22    = glm::vec3(-0.24f, -0.24f, -0.28f);
        }
        break;
    case GALILEOS_TOMB: {
            L00    = glm::vec3( 1.04f,  0.76f,  0.71f);
            L1m1   = glm::vec3( 0.44f,  0.34f,  0.34f);
            L10    = glm::vec3(-0.22f, -0.18f, -0.17f);
            L11    = glm::vec3( 0.71f,  0.54f,  0.56f);
            L2m2   = glm::vec3( 0.64f,  0.50f,  0.52f);
            L2m1   = glm::vec3(-0.12f, -0.09f, -0.08f);
            L20    = glm::vec3(-0.37f, -0.28f, -0.32f);
            L21    = glm::vec3(-0.17f, -0.13f, -0.13f);
            L22    = glm::vec3( 0.55f,  0.42f,  0.42f);
        }
        break;
    case VINE_STREET_KITCHEN: {
            L00    = glm::vec3( 0.64f,  0.67f,  0.73f);
            L1m1   = glm::vec3( 0.28f,  0.32f,  0.33f);
            L10    = glm::vec3( 0.42f,  0.60f,  0.77f);
            L11    = glm::vec3(-0.05f, -0.04f, -0.02f);
            L2m2   = glm::vec3(-0.10f, -0.08f, -0.05f);
            L2m1   = glm::vec3( 0.25f,  0.39f,  0.53f);
            L20    = glm::vec3( 0.38f,  0.54f,  0.71f);
            L21    = glm::vec3( 0.06f,  0.01f, -0.02f);
            L22    = glm::vec3(-0.03f, -0.02f, -0.03f);
        }
        break;
    case BREEZEWAY: {
            L00    = glm::vec3( 0.32f,  0.36f,  0.38f);
            L1m1   = glm::vec3( 0.37f,  0.41f,  0.45f);
            L10    = glm::vec3(-0.01f, -0.01f, -0.01f);
            L11    = glm::vec3(-0.10f, -0.12f, -0.12f);
            L2m2   = glm::vec3(-0.13f, -0.15f, -0.17f);
            L2m1   = glm::vec3(-0.01f, -0.02f,  0.02f);
            L20    = glm::vec3(-0.07f, -0.08f, -0.09f);
            L21    = glm::vec3( 0.02f,  0.03f,  0.03f);
            L22    = glm::vec3(-0.29f, -0.32f, -0.36f);
        }
        break;
    case CAMPUS_SUNSET: {
            L00    = glm::vec3( 0.79f,  0.94f,  0.98f);
            L1m1   = glm::vec3( 0.44f,  0.56f,  0.70f);
            L10    = glm::vec3(-0.10f, -0.18f, -0.27f);
            L11    = glm::vec3( 0.45f,  0.38f,  0.20f);
            L2m2   = glm::vec3( 0.18f,  0.14f,  0.05f);
            L2m1   = glm::vec3(-0.14f, -0.22f, -0.31f);
            L20    = glm::vec3(-0.39f, -0.40f, -0.36f);
            L21    = glm::vec3( 0.09f,  0.07f,  0.04f);
            L22    = glm::vec3( 0.67f,  0.67f,  0.52f);
        }
        break;
    case FUNSTON_BEACH_SUNSET: {
            L00    = glm::vec3( 0.68f,  0.69f,  0.70f);
            L1m1   = glm::vec3( 0.32f,  0.37f,  0.44f);
            L10    = glm::vec3(-0.17f, -0.17f, -0.17f);
            L11    = glm::vec3(-0.45f, -0.42f, -0.34f);
            L2m2   = glm::vec3(-0.17f, -0.17f, -0.15f);
            L2m1   = glm::vec3(-0.08f, -0.09f, -0.10f);
            L20    = glm::vec3(-0.03f, -0.02f, -0.01f);
            L21    = glm::vec3( 0.16f,  0.14f,  0.10f);
            L22    = glm::vec3( 0.37f,  0.31f,  0.20f);
        }
        break;
    }
}

// Originial code for the Spherical Harmonics taken from "Sun and Black Cat- Igor Dykhta (igor dykhta email) � 2007-2014 "
void sphericalHarmonicsAdd(float * result, int order, const float * inputA, const float * inputB) {
   const int numCoeff = order * order;
   for(int i=0; i < numCoeff; i++) {
      result[i] = inputA[i] + inputB[i];
   }
}

void sphericalHarmonicsScale(float * result, int order, const float * input, float scale) {
   const int numCoeff = order * order;
   for(int i=0; i < numCoeff; i++) {
      result[i] = input[i] * scale;
   }
}

void sphericalHarmonicsEvaluateDirection(float * result, int order,  const glm::vec3 & dir) {
   // calculate coefficients for first 3 bands of spherical harmonics
   double P_0_0 = 0.282094791773878140;
   double P_1_0 = 0.488602511902919920 * (double)dir.z;
   double P_1_1 = -0.488602511902919920;
   double P_2_0 = 0.946174695757560080 * (double)dir.z * (double)dir.z - 0.315391565252520050;
   double P_2_1 = -1.092548430592079200 * (double)dir.z;
   double P_2_2 = 0.546274215296039590;
   result[0] = P_0_0;
   result[1] = P_1_1 * (double)dir.y;
   result[2] = P_1_0;
   result[3] = P_1_1 * (double)dir.x;
   result[4] = P_2_2 * ((double)dir.x * (double)dir.y + (double)dir.y * (double)dir.x);
   result[5] = P_2_1 * (double)dir.y;
   result[6] = P_2_0;
   result[7] = P_2_1 * (double)dir.x;
   result[8] = P_2_2 * ((double)dir.x * (double)dir.x - (double)dir.y * (double)dir.y);
}

bool sphericalHarmonicsFromTexture(const gpu::Texture& cubeTexture, std::vector<glm::vec3> & output, const uint order) {
    int width = cubeTexture.getWidth();
    if(width != cubeTexture.getHeight()) {
        return false;
    }

    PROFILE_RANGE(render_gpu, "sphericalHarmonicsFromTexture");

    const uint sqOrder = order*order;

    // allocate memory for calculations
    output.resize(sqOrder);
    std::vector<float> resultR(sqOrder);
    std::vector<float> resultG(sqOrder);
    std::vector<float> resultB(sqOrder);

    // initialize values
    float fWt = 0.0f;
    for(uint i=0; i < sqOrder; i++) {
        output[i] = glm::vec3(0.0f);
        resultR[i] = 0.0f;
        resultG[i] = 0;
        resultB[i] = 0;
    }
    std::vector<float> shBuff(sqOrder);
    std::vector<float> shBuffB(sqOrder);

    // We trade accuracy for speed by breaking the image into 32x32 parts
    // and approximating the distance for all the pixels in each part to be
    // the distance to the part's center.
    int numDivisionsPerSide = 32;
    if (width < numDivisionsPerSide) {
        numDivisionsPerSide = width;
    }
    int stride = width / numDivisionsPerSide;
    int halfStride = stride / 2;

    // for each face of cube texture
    for(int face=0; face < gpu::Texture::NUM_CUBE_FACES; face++) {
        PROFILE_RANGE(render_gpu, "ProcessFace");

        auto mipFormat = cubeTexture.getStoredMipFormat();
        auto numComponents = mipFormat.getScalarCount();
        int roffset { 0 };
        int goffset { 1 };
        int boffset { 2 };
        if ((mipFormat.getSemantic() == gpu::BGRA) || (mipFormat.getSemantic() == gpu::SBGRA)) {
            roffset = 2;
            boffset = 0;
        }

        auto data = cubeTexture.accessStoredMipFace(0, face)->readData();
        if (data == nullptr) {
            continue;
        }

        // step between two texels for range [0, 1]
        float invWidth = 1.0f / float(width);
        // initial negative bound for range [-1, 1]
        float negativeBound = -1.0f + invWidth;
        // step between two texels for range [-1, 1]
        float invWidthBy2 = 2.0f / float(width);

        for(int y=halfStride; y < width-halfStride; y += stride) {
            // texture coordinate V in range [-1 to 1]
            const float fV = negativeBound + float(y) * invWidthBy2;

            for(int x=halfStride; x < width - halfStride; x += stride) {
                // texture coordinate U in range [-1 to 1]
                const float fU = negativeBound + float(x) * invWidthBy2;

                // determine direction from center of cube texture to current texel
                glm::vec3 dir;
                switch(face) {
                case gpu::Texture::CUBE_FACE_RIGHT_POS_X: {
                    dir.x = 1.0f;
                    dir.y = 1.0f - (invWidthBy2 * float(y) + invWidth);
                    dir.z = 1.0f - (invWidthBy2 * float(x) + invWidth);
                    dir = -dir;
                    break;
                }
                case gpu::Texture::CUBE_FACE_LEFT_NEG_X: {
                    dir.x = -1.0f;
                    dir.y = 1.0f - (invWidthBy2 * float(y) + invWidth);
                    dir.z = -1.0f + (invWidthBy2 * float(x) + invWidth);
                    dir = -dir;
                    break;
                }
                case gpu::Texture::CUBE_FACE_TOP_POS_Y: {
                    dir.x = - 1.0f + (invWidthBy2 * float(x) + invWidth);
                    dir.y = 1.0f;
                    dir.z = - 1.0f + (invWidthBy2 * float(y) + invWidth);
                    dir = -dir;
                    break;
                }
                case gpu::Texture::CUBE_FACE_BOTTOM_NEG_Y: {
                    dir.x = - 1.0f + (invWidthBy2 * float(x) + invWidth);
                    dir.y = - 1.0f;
                    dir.z = 1.0f - (invWidthBy2 * float(y) + invWidth);
                    dir = -dir;
                    break;
                }
                case gpu::Texture::CUBE_FACE_BACK_POS_Z: {
                    dir.x = - 1.0f + (invWidthBy2 * float(x) + invWidth);
                    dir.y = 1.0f - (invWidthBy2 * float(y) + invWidth);
                    dir.z = 1.0f;
                    break;
                }
                case gpu::Texture::CUBE_FACE_FRONT_NEG_Z: {
                    dir.x = 1.0f - (invWidthBy2 * float(x) + invWidth);
                    dir.y = 1.0f - (invWidthBy2 * float(y) + invWidth);
                    dir.z = - 1.0f;
                    break;
                }
                default:
                    return false;
                }

                // normalize direction
                dir = glm::normalize(dir);

                // scale factor depending on distance from center of the face
                const float fDiffSolid = 4.0f / ((1.0f + fU*fU + fV*fV) *
                                            sqrtf(1.0f + fU*fU + fV*fV));
                fWt += fDiffSolid;

                // calculate coefficients of spherical harmonics for current direction
                sphericalHarmonicsEvaluateDirection(shBuff.data(), order, dir);

                // index of texel in texture

                // get color from texture and map to range [0, 1]
                float red { 0.0f };
                float green { 0.0f };
                float blue { 0.0f };
                for (int i = 0; i < stride; ++i) {
                    for (int j = 0; j < stride; ++j) {
                        int k = (int)(x + i - halfStride + (y + j - halfStride) * width) * numComponents;
                        red += ColorUtils::sRGB8ToLinearFloat(data[k + roffset]);
                        green += ColorUtils::sRGB8ToLinearFloat(data[k + goffset]);
                        blue += ColorUtils::sRGB8ToLinearFloat(data[k + boffset]);
                    }
                }
                glm::vec3 clr(red, green, blue);

                // scale color and add to previously accumulated coefficients
                // red
                sphericalHarmonicsScale(shBuffB.data(), order, shBuff.data(), clr.r * fDiffSolid);
                sphericalHarmonicsAdd(resultR.data(), order, resultR.data(), shBuffB.data());
                // green
                sphericalHarmonicsScale(shBuffB.data(), order, shBuff.data(), clr.g * fDiffSolid);
                sphericalHarmonicsAdd(resultG.data(), order, resultG.data(), shBuffB.data());
                // blue
                sphericalHarmonicsScale(shBuffB.data(), order, shBuff.data(), clr.b * fDiffSolid);
                sphericalHarmonicsAdd(resultB.data(), order, resultB.data(), shBuffB.data());
            }
        }
    }

    // final scale for coefficients
    const float fNormProj = (4.0f * glm::pi<float>()) / (fWt * (float)(stride * stride));
    sphericalHarmonicsScale(resultR.data(), order, resultR.data(), fNormProj);
    sphericalHarmonicsScale(resultG.data(), order, resultG.data(), fNormProj);
    sphericalHarmonicsScale(resultB.data(), order, resultB.data(), fNormProj);

    // save result
    for(uint i=0; i < sqOrder; i++) {
        output[i] = glm::vec3(resultR[i], resultG[i], resultB[i]);
    }

    return true;
}

void SphericalHarmonics::evalFromTexture(const Texture& texture) {
    if (texture.isDefined()) {
        std::vector< glm::vec3 > coefs;
        sphericalHarmonicsFromTexture(texture, coefs, 3);

        L00 = coefs[0];
        L1m1 = coefs[1];
        L10  = coefs[2];
        L11  = coefs[3];
        L2m2 = coefs[4];
        L2m1 = coefs[5];
        L20 = coefs[6];
        L21 = coefs[7];
        L22 = coefs[8];
    }
}


// TextureSource
TextureSource::TextureSource() {
}

TextureSource::~TextureSource() {
}

void TextureSource::reset(const QUrl& url) {
    _imageUrl = url;
}

void TextureSource::resetTexture(gpu::TexturePointer texture) {
    _gpuTexture = texture;
}

bool TextureSource::isDefined() const {
    if (_gpuTexture) {
        return _gpuTexture->isDefined();
    } else {
        return false;
    }
}

bool Texture::setMinMip(uint16 newMinMip) {
    uint16 oldMinMip = _minMip;
    _minMip = std::min(std::max(_minMip, newMinMip), getMaxMip());
    return oldMinMip != _minMip;
}

bool Texture::incremementMinMip(uint16 count) {
    return setMinMip(_minMip + count);
}

Vec3u Texture::evalMipDimensions(uint16 level) const { 
    auto dimensions = getDimensions();
    dimensions >>= level; 
    return glm::max(dimensions, Vec3u(1));
}

void Texture::setExternalRecycler(const ExternalRecycler& recycler) { 
    Lock lock(_externalMutex);
    _externalRecycler = recycler;
}

Texture::ExternalRecycler Texture::getExternalRecycler() const {
    Lock lock(_externalMutex);
    Texture::ExternalRecycler result = _externalRecycler;
    return result;
}

void Texture::setExternalTexture(uint32 externalId, void* externalFence) {
    Lock lock(_externalMutex);
    assert(_externalRecycler);
    _externalUpdates.push_back({ externalId, externalFence });
}

Texture::ExternalUpdates Texture::getUpdates() const {
    Texture::ExternalUpdates result;
    {
        Lock lock(_externalMutex);
        _externalUpdates.swap(result);
    }
    return result;
}

void Texture::setStorage(std::unique_ptr<Storage>& newStorage) {
    _storage.swap(newStorage);
}
