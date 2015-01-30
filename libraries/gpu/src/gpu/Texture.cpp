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
#include <math.h>
#include <QDebug>

using namespace gpu;

Texture::Pixels::Pixels(const Element& format, Size size, const Byte* bytes) :
    _sysmem(size, bytes),
    _format(format) {
}

Texture::Pixels::~Pixels() {
}

void Texture::Storage::assignTexture(Texture* texture) {
    _texture = texture;
}

Stamp Texture::Storage::getStamp(uint16 level) const {
    PixelsPointer mip = getMip(level);
    if (mip) {
        return mip->_sysmem.getStamp();
    }
    return 0;
}

void Texture::Storage::reset() {
    _mips.clear();
}

Texture::PixelsPointer Texture::Storage::editMip(uint16 level) {
    if (level < _mips.size()) {
        return _mips[level];
    }
    return PixelsPointer();
}

const Texture::PixelsPointer Texture::Storage::getMip(uint16 level) const {
    if (level < _mips.size()) {
        return _mips[level];
    }
    return PixelsPointer();
}

bool Texture::Storage::isMipAvailable(uint16 level) const {
    PixelsPointer mip = getMip(level);
    return (mip && mip->_sysmem.getSize());
}

bool Texture::Storage::allocateMip(uint16 level) {
    bool changed = false;
    if (level >= _mips.size()) {
        _mips.resize(level+1, PixelsPointer());
        changed = true;
    }

    if (!_mips[level]) {
        _mips[level] = PixelsPointer(new Pixels());
        changed = true;
    }

    return changed;
}

bool Texture::Storage::assignMipData(uint16 level, const Element& format, Size size, const Byte* bytes) {
        // Ok we should be able to do that...
    allocateMip(level);
    _mips[level]->_format = format;
    Size allocated = _mips[level]->_sysmem.setData(size, bytes);
    return allocated == size;
}

Texture* Texture::create1D(const Element& texelFormat, uint16 width) {
    return create(TEX_1D, texelFormat, width, 1, 1, 1, 1);
}

Texture* Texture::create2D(const Element& texelFormat, uint16 width, uint16 height) {
    return create(TEX_2D, texelFormat, width, height, 1, 1, 1);
}

Texture* Texture::create3D(const Element& texelFormat, uint16 width, uint16 height, uint16 depth) {
    return create(TEX_3D, texelFormat, width, height, depth, 1, 1);
}

Texture* Texture::createCube(const Element& texelFormat, uint16 width) {
    return create(TEX_CUBE, texelFormat, width, width, 1, 1, 1);
}

Texture* Texture::create(Type type, const Element& texelFormat, uint16 width, uint16 height, uint16 depth, uint16 numSamples, uint16 numSlices)
{
    Texture* tex = new Texture();
    tex->_storage.reset(new Storage());
    tex->_storage->_texture = tex;
    tex->_type = type;
    tex->_maxMip = 0;
    tex->resize(type, texelFormat, width, height, depth, numSamples, numSlices);

    return tex;
}

Texture* Texture::createFromStorage(Storage* storage) {
   Texture* tex = new Texture();
   tex->_storage.reset(storage);
   storage->assignTexture(tex);
   return tex;
}

Texture::Texture():
    Resource(),
    _storage(),
    _stamp(0),
    _size(0),
    _width(1),
    _height(1),
    _depth(1),
    _numSamples(1),
    _numSlices(1),
    _maxMip(0),
    _type(TEX_1D),
    _autoGenerateMips(false),
    _defined(false)
{
}

Texture::~Texture()
{
}

Texture::Size Texture::resize(Type type, const Element& texelFormat, uint16 width, uint16 height, uint16 depth, uint16 numSamples, uint16 numSlices) {
    if (width && height && depth && numSamples && numSlices) {
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

        // Evaluate the new size with the new format
        const int DIM_SIZE[] = {1, 1, 1, 6};
        int size = DIM_SIZE[_type] *_width * _height * _depth * _numSamples * texelFormat.getSize();

        // If size change then we need to reset 
        if (changed || (size != getSize())) {
            _size = size;
            _storage->reset();
            _stamp++;
        }

        // TexelFormat might have change, but it's mostly interpretation
        if (texelFormat != _texelFormat) {
            _texelFormat = texelFormat;
            _stamp++;
        }

        // Here the Texture has been fully defined from the gpu point of view (size and format)
         _defined = true;
    } else {
         _stamp++;
    }

    return _size;
}

Texture::Size Texture::resize1D(uint16 width, uint16 numSamples) {
    return resize(TEX_1D, getTexelFormat(), width, 1, 1, numSamples, 1);
}
Texture::Size Texture::resize2D(uint16 width, uint16 height, uint16 numSamples) {
    return resize(TEX_2D, getTexelFormat(), width, height, 1, numSamples, 1);
}
Texture::Size Texture::resize3D(uint16 width, uint16 height, uint16 depth, uint16 numSamples) {
    return resize(TEX_3D, getTexelFormat(), width, height, depth, numSamples, 1);
}
Texture::Size Texture::resizeCube(uint16 width, uint16 numSamples) {
    return resize(TEX_CUBE, getTexelFormat(), width, 1, 1, numSamples, 1);
}

Texture::Size Texture::reformat(const Element& texelFormat) {
    return resize(_type, texelFormat, getWidth(), getHeight(), getDepth(), getNumSamples(), getNumSlices());
}

bool Texture::isColorRenderTarget() const {
    return (_texelFormat.getSemantic() == gpu::RGBA);
}

bool Texture::isDepthStencilRenderTarget() const {
    return (_texelFormat.getSemantic() == gpu::DEPTH) || (_texelFormat.getSemantic() == gpu::DEPTH_STENCIL);
}

uint16 Texture::evalDimNumMips(uint16 size) {
    double largerDim = size;
    double val = log(largerDim)/log(2.0);
    return 1 + (uint16) val;
}

// The number mips that the texture could have if all existed
// = log2(max(width, height, depth))
uint16 Texture::evalNumMips() const {
    double largerDim = std::max(std::max(_width, _height), _depth);
    double val = log(largerDim)/log(2.0);
    return 1 + (uint16) val;
}

uint16 Texture::maxMip() const {
    return _maxMip;
}

bool Texture::assignStoredMip(uint16 level, const Element& format, Size size, const Byte* bytes) {
    // Check that level accessed make sense
    if (level != 0) {
        if (_autoGenerateMips) {
            return false;
        }
        if (level >= evalNumMips()) {
            return false;
        }
    }

    // THen check that the mem buffer passed make sense with its format
    Size expectedSize = evalStoredMipSize(level, format);
    if (size == expectedSize) {
        _storage->assignMipData(level, format, size, bytes);
        _stamp++;
        return true;
    } else if (size > expectedSize) {
        // NOTE: We are facing this case sometime because apparently QImage (from where we get the bits) is generating images
        // and alligning the line of pixels to 32 bits.
        // We should probably consider something a bit more smart to get the correct result but for now (UI elements)
        // it seems to work...
        _storage->assignMipData(level, format, size, bytes);
        _stamp++;
        return true;
    }

    return false;
}

uint16 Texture::autoGenerateMips(uint16 maxMip) {
    _autoGenerateMips = true;
    _maxMip = std::min((uint16) (evalNumMips() - 1), maxMip);
    _stamp++;
    return _maxMip;
}

uint16 Texture::getStoredMipWidth(uint16 level) const {
    PixelsPointer mip = accessStoredMip(level);
    if (mip && mip->_sysmem.getSize()) {
        return evalMipWidth(level);
    }
    return 0;
}

uint16 Texture::getStoredMipHeight(uint16 level) const {
    PixelsPointer mip = accessStoredMip(level);
    if (mip && mip->_sysmem.getSize()) {
        return evalMipHeight(level);
    }
        return 0;
}

uint16 Texture::getStoredMipDepth(uint16 level) const {
    PixelsPointer mip = accessStoredMip(level);
    if (mip && mip->_sysmem.getSize()) {
        return evalMipDepth(level);
    }
    return 0;
}

uint32 Texture::getStoredMipNumTexels(uint16 level) const {
    PixelsPointer mip = accessStoredMip(level);
    if (mip && mip->_sysmem.getSize()) {
        return evalMipWidth(level) * evalMipHeight(level) * evalMipDepth(level);
    }
    return 0;
}

uint32 Texture::getStoredMipSize(uint16 level) const {
    PixelsPointer mip = accessStoredMip(level);
    if (mip && mip->_sysmem.getSize()) {
        return evalMipWidth(level) * evalMipHeight(level) * evalMipDepth(level) * getTexelFormat().getSize();
    }
    return 0;
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
