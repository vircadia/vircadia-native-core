//
//  Texture.h
//  libraries/gpu/src/gpu
//
//  Created by Sam Gateau on 1/16/2015.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_gpu_Texture_h
#define hifi_gpu_Texture_h

#include <algorithm> //min max and more
#include <bitset>

#include <QMetaType>
#include <QUrl>

#include "Forward.h"
#include "Resource.h"

namespace gpu {

// THe spherical harmonics is a nice tool for cubemap, so if required, the irradiance SH can be automatically generated
// with the cube texture
class Texture;
class SphericalHarmonics {
public:
    glm::vec3 L00    ; float spare0;
    glm::vec3 L1m1   ; float spare1;
    glm::vec3 L10    ; float spare2;
    glm::vec3 L11    ; float spare3;
    glm::vec3 L2m2   ; float spare4;
    glm::vec3 L2m1   ; float spare5;
    glm::vec3 L20    ; float spare6;
    glm::vec3 L21    ; float spare7;
    glm::vec3 L22    ; float spare8;

    static const int NUM_COEFFICIENTS = 9;

    enum Preset {
        OLD_TOWN_SQUARE = 0,
        GRACE_CATHEDRAL,
        EUCALYPTUS_GROVE,
        ST_PETERS_BASILICA,
        UFFIZI_GALLERY,
        GALILEOS_TOMB,
        VINE_STREET_KITCHEN,
        BREEZEWAY,
        CAMPUS_SUNSET,
        FUNSTON_BEACH_SUNSET,

        NUM_PRESET,
    };

    void assignPreset(int p);

    void evalFromTexture(const Texture& texture);
};
typedef std::shared_ptr< SphericalHarmonics > SHPointer;

class Sampler {
public:

    enum Filter {
        FILTER_MIN_MAG_POINT, // top mip only
        FILTER_MIN_POINT_MAG_LINEAR, // top mip only
        FILTER_MIN_LINEAR_MAG_POINT, // top mip only
        FILTER_MIN_MAG_LINEAR, // top mip only

        FILTER_MIN_MAG_MIP_POINT,
        FILTER_MIN_MAG_POINT_MIP_LINEAR,
        FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT,
        FILTER_MIN_POINT_MAG_MIP_LINEAR,
        FILTER_MIN_LINEAR_MAG_MIP_POINT,
        FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR,
        FILTER_MIN_MAG_LINEAR_MIP_POINT,
        FILTER_MIN_MAG_MIP_LINEAR,
        FILTER_ANISOTROPIC,

        NUM_FILTERS,
    };

    enum WrapMode {
        WRAP_REPEAT = 0,
        WRAP_MIRROR,
        WRAP_CLAMP,
        WRAP_BORDER,
        WRAP_MIRROR_ONCE,

        NUM_WRAP_MODES
    };

    static const uint8 MAX_MIP_LEVEL = 0xFF;

    class Desc {
    public:
        glm::vec4 _borderColor{ 1.0f };
        uint32 _maxAnisotropy = 16;

        uint8 _filter = FILTER_MIN_MAG_POINT;
        uint8 _comparisonFunc = ALWAYS;

        uint8 _wrapModeU = WRAP_REPEAT;
        uint8 _wrapModeV = WRAP_REPEAT;
        uint8 _wrapModeW = WRAP_REPEAT;
            
        uint8 _mipOffset = 0;
        uint8 _minMip = 0;
        uint8 _maxMip = MAX_MIP_LEVEL;

        Desc() {}
        Desc(const Filter filter, const WrapMode wrap = WRAP_REPEAT) : _filter(filter), _wrapModeU(wrap), _wrapModeV(wrap), _wrapModeW(wrap) {}
    };

    Sampler() {}
    Sampler(const Filter filter, const WrapMode wrap = WRAP_REPEAT) : _desc(filter, wrap) {}
    Sampler(const Desc& desc) : _desc(desc) {}
    ~Sampler() {}

    const glm::vec4& getBorderColor() const { return _desc._borderColor; }

    uint32 getMaxAnisotropy() const { return _desc._maxAnisotropy; }

    WrapMode getWrapModeU() const { return WrapMode(_desc._wrapModeU); }
    WrapMode getWrapModeV() const { return WrapMode(_desc._wrapModeV); }
    WrapMode getWrapModeW() const { return WrapMode(_desc._wrapModeW); }

    Filter getFilter() const { return Filter(_desc._filter); }
    ComparisonFunction getComparisonFunction() const { return ComparisonFunction(_desc._comparisonFunc); }
    bool doComparison() const { return getComparisonFunction() != ALWAYS; }

    uint8 getMipOffset() const { return _desc._mipOffset; }
    uint8 getMinMip() const { return _desc._minMip; }
    uint8 getMaxMip() const { return _desc._maxMip; }

protected:
    Desc _desc;
};

class Texture : public Resource {
    static std::atomic<uint32_t> _textureCPUCount;
    static std::atomic<Size> _textureCPUMemoryUsage;
    static std::atomic<Size> _allowedCPUMemoryUsage;
    static void updateTextureCPUMemoryUsage(Size prevObjectSize, Size newObjectSize);
public:
    static uint32_t getTextureCPUCount();
    static Size getTextureCPUMemoryUsage();
    static uint32_t getTextureGPUCount();
    static Size getTextureGPUMemoryUsage();
    static Size getTextureGPUVirtualMemoryUsage();
    static uint32_t getTextureGPUTransferCount();
    static Size getAllowedGPUMemoryUsage();
    static void setAllowedGPUMemoryUsage(Size size);

    class Usage {
    public:
        enum FlagBit {
            COLOR = 0,   // Texture is a color map
            NORMAL,      // Texture is a normal map
            ALPHA,      // Texture has an alpha channel
            ALPHA_MASK,       // Texture alpha channel is a Mask 0/1

            NUM_FLAGS,  
        };
        typedef std::bitset<NUM_FLAGS> Flags;

        // The key is the Flags
        Flags _flags;

        Usage() : _flags(0) {}
        Usage(const Flags& flags) : _flags(flags) {}

        bool operator== (const Usage& rhs) const { return _flags == rhs._flags; }
        bool operator!= (const Usage& rhs) const { return _flags != rhs._flags; }

        class Builder {
            friend class Usage;
            Flags _flags{ 0 };
        public:
            Builder() {}

            Usage build() const { return Usage(_flags); }

            Builder& withColor() { _flags.set(COLOR); return (*this); }
            Builder& withNormal() { _flags.set(NORMAL); return (*this); }
            Builder& withAlpha() { _flags.set(ALPHA); return (*this); }
            Builder& withAlphaMask() { _flags.set(ALPHA_MASK); return (*this); }
        };
        Usage(const Builder& builder) : Usage(builder._flags) {}

        bool isColor() const { return _flags[COLOR]; }
        bool isNormal() const { return _flags[NORMAL]; }

        bool isAlpha() const { return _flags[ALPHA]; }
        bool isAlphaMask() const { return _flags[ALPHA_MASK]; }


        bool operator==(const Usage& usage) { return (_flags == usage._flags); }
        bool operator!=(const Usage& usage) { return (_flags != usage._flags); }
    };

    class Pixels {
    public:
        Pixels() {}
        Pixels(const Pixels& pixels) = default;
        Pixels(const Element& format, Size size, const Byte* bytes);
        ~Pixels();

        const Byte* readData() const { return _sysmem.readData(); }
        Size getSize() const { return _sysmem.getSize(); }
        Size resize(Size pSize);
        Size setData(const Element& format, Size size, const Byte* bytes );
        
        const Element& getFormat() const { return _format; }
        
        void notifyGPULoaded();
        
    protected:
        Element _format;
        Sysmem _sysmem;
        bool _isGPULoaded;
        
        friend class Texture;
    };
    typedef std::shared_ptr< Pixels > PixelsPointer;

    enum Type {
        TEX_1D = 0,
        TEX_2D,
        TEX_3D,
        TEX_CUBE,

        NUM_TYPES,
    };

    // Definition of the cube face name and layout
    enum CubeFace {
        CUBE_FACE_RIGHT_POS_X = 0,
        CUBE_FACE_LEFT_NEG_X,
        CUBE_FACE_TOP_POS_Y,
        CUBE_FACE_BOTTOM_NEG_Y,
        CUBE_FACE_BACK_POS_Z,
        CUBE_FACE_FRONT_NEG_Z,

        NUM_CUBE_FACES, // Not a valid vace index
    };

    class Storage {
    public:
        Storage() {}
        virtual ~Storage() {}
        virtual void reset();
        virtual PixelsPointer editMipFace(uint16 level, uint8 face = 0);
        virtual const PixelsPointer getMipFace(uint16 level, uint8 face = 0) const;
        virtual bool allocateMip(uint16 level);
        virtual bool assignMipData(uint16 level, const Element& format, Size size, const Byte* bytes);
        virtual bool assignMipFaceData(uint16 level, const Element& format, Size size, const Byte* bytes, uint8 face);
        virtual bool isMipAvailable(uint16 level, uint8 face = 0) const;

        Texture::Type getType() const { return _type; }
        
        Stamp getStamp() const { return _stamp; }
        Stamp bumpStamp() { return ++_stamp; }
    protected:
        Stamp _stamp = 0;
        Texture* _texture = nullptr; // Points to the parent texture (not owned)
        Texture::Type _type = Texture::TEX_2D; // The type of texture is needed to know the number of faces to expect
        std::vector<std::vector<PixelsPointer>> _mips; // an array of mips, each mip is an array of faces

        virtual void assignTexture(Texture* tex); // Texture storage is pointing to ONE corrresponding Texture.
        const Texture* getTexture() const { return _texture; }
 
        friend class Texture;
        
        // THis should be only called by the Texture from the Backend to notify the storage that the specified mip face pixels
        //  have been uploaded to the GPU memory. IT is possible for the storage to free the system memory then
        virtual void notifyMipFaceGPULoaded(uint16 level, uint8 face) const;
    };

 
    static Texture* create1D(const Element& texelFormat, uint16 width, const Sampler& sampler = Sampler());
    static Texture* create2D(const Element& texelFormat, uint16 width, uint16 height, const Sampler& sampler = Sampler());
    static Texture* create3D(const Element& texelFormat, uint16 width, uint16 height, uint16 depth, const Sampler& sampler = Sampler());
    static Texture* createCube(const Element& texelFormat, uint16 width, const Sampler& sampler = Sampler());

    Texture();
    Texture(const Texture& buf); // deep copy of the sysmem texture
    Texture& operator=(const Texture& buf); // deep copy of the sysmem texture
    ~Texture();

    Stamp getStamp() const { return _stamp; }
    Stamp getDataStamp() const { return _storage->getStamp(); }

    // The theoretical size in bytes of data stored in the texture
    Size getSize() const { return _size; }

    // The actual size in bytes of data stored in the texture
    Size getStoredSize() const;

    // Resize, unless auto mips mode would destroy all the sub mips
    Size resize1D(uint16 width, uint16 numSamples);
    Size resize2D(uint16 width, uint16 height, uint16 numSamples);
    Size resize3D(uint16 width, uint16 height, uint16 depth, uint16 numSamples);
    Size resizeCube(uint16 width, uint16 numSamples);

    // Reformat, unless auto mips mode would destroy all the sub mips
    Size reformat(const Element& texelFormat);

    // Size and format
    Type getType() const { return _type; }

    bool isColorRenderTarget() const;
    bool isDepthStencilRenderTarget() const;

    const Element& getTexelFormat() const { return _texelFormat; }
    bool  hasBorder() const { return false; }

    Vec3u getDimensions() const { return Vec3u(_width, _height, _depth); }
    uint16 getWidth() const { return _width; }
    uint16 getHeight() const { return _height; }
    uint16 getDepth() const { return _depth; }

    uint32 getRowPitch() const { return getWidth() * getTexelFormat().getSize(); }
 
    // The number of faces is mostly used for cube map, and maybe for stereo ? otherwise it's 1
    // For cube maps, this means the pixels of the different faces are supposed to be packed back to back in a mip
    // as if the height was NUM_FACES time bigger.
    static uint8 NUM_FACES_PER_TYPE[NUM_TYPES];
    uint8 getNumFaces() const { return NUM_FACES_PER_TYPE[getType()]; }

    uint32 getNumTexels() const { return _width * _height * _depth * getNumFaces(); }

    uint16 getNumSlices() const { return _numSlices; }
    uint16 getNumSamples() const { return _numSamples; }


    // NumSamples can only have certain values based on the hw
    static uint16 evalNumSamplesUsed(uint16 numSamplesTried);

    // Mips size evaluation

    // The number mips that a dimension could haves
    // = 1 + log2(size)
    static uint16 evalDimNumMips(uint16 size);

    // The number mips that the texture could have if all existed
    // = 1 + log2(max(width, height, depth))
    uint16 evalNumMips() const;

    // Eval the size that the mips level SHOULD have
    // not the one stored in the Texture
    static const uint MIN_DIMENSION = 1;
    Vec3u evalMipDimensions(uint16 level) const;
    uint16 evalMipWidth(uint16 level) const { return std::max(_width >> level, 1); }
    uint16 evalMipHeight(uint16 level) const { return std::max(_height >> level, 1); }
    uint16 evalMipDepth(uint16 level) const { return std::max(_depth >> level, 1); }

    // Size for each face of a mip at a particular level
    uint32 evalMipFaceNumTexels(uint16 level) const { return evalMipWidth(level) * evalMipHeight(level) * evalMipDepth(level); }
    uint32 evalMipFaceSize(uint16 level) const { return evalMipFaceNumTexels(level) * getTexelFormat().getSize(); }
    
    // Total size for the mip
    uint32 evalMipNumTexels(uint16 level) const { return evalMipFaceNumTexels(level) * getNumFaces(); }
    uint32 evalMipSize(uint16 level) const { return evalMipNumTexels(level) * getTexelFormat().getSize(); }

    uint32 evalStoredMipFaceSize(uint16 level, const Element& format) const { return evalMipFaceNumTexels(level) * format.getSize(); }
    uint32 evalStoredMipSize(uint16 level, const Element& format) const { return evalMipNumTexels(level) * format.getSize(); }

    uint32 evalTotalSize() const {
        uint32 size = 0;
        uint16 minMipLevel = minMip();
        uint16 maxMipLevel = maxMip();
        for (uint16 l = minMipLevel; l <= maxMipLevel; l++) {
            size += evalMipSize(l);
        }
        return size * getNumSlices();
    }

    // max mip is in the range [ 0 if no sub mips, log2(max(width, height, depth))]
    // if autoGenerateMip is on => will provide the maxMIp level specified
    // else provide the deepest mip level provided through assignMip
    uint16 maxMip() const { return _maxMip; }

    uint16 minMip() const { return _minMip; }
    
    uint16 mipLevels() const { return _maxMip + 1; }
    
    uint16 usedMipLevels() const { return (_maxMip - _minMip) + 1; }

    bool setMinMip(uint16 newMinMip);
    bool incremementMinMip(uint16 count = 1);

    // Generate the mips automatically
    // But the sysmem version is not available
    // Only works for the standard formats
    // Specify the maximum Mip level available
    // 0 is the default one
    // 1 is the first level
    // ...
    // nbMips - 1 is the last mip level
    //
    // If -1 then all the mips are generated
    //
    // Return the totalnumber of mips that will be available
    uint16 autoGenerateMips(uint16 maxMip);
    bool isAutogenerateMips() const { return _autoGenerateMips; }

    // Managing Storage and mips

    // Manually allocate the mips down until the specified maxMip
    // this is just allocating the sysmem version of it
    // in case autoGen is on, this doesn't allocate
    // Explicitely assign mip data for a certain level
    // If Bytes is NULL then simply allocate the space so mip sysmem can be accessed
    bool assignStoredMip(uint16 level, const Element& format, Size size, const Byte* bytes);
    bool assignStoredMipFace(uint16 level, const Element& format, Size size, const Byte* bytes, uint8 face);

    // Access the the sub mips
    bool isStoredMipFaceAvailable(uint16 level, uint8 face = 0) const { return _storage->isMipAvailable(level, face); }
    const PixelsPointer accessStoredMipFace(uint16 level, uint8 face = 0) const { return _storage->getMipFace(level, face); }

    // access sizes for the stored mips
    uint16 getStoredMipWidth(uint16 level) const;
    uint16 getStoredMipHeight(uint16 level) const;
    uint16 getStoredMipDepth(uint16 level) const;
    uint32 getStoredMipNumTexels(uint16 level) const;
    uint32 getStoredMipSize(uint16 level) const;
 
    bool isDefined() const { return _defined; }

    // Usage is a a set of flags providing Semantic about the usage of the Texture.
    void setUsage(const Usage& usage) { _usage = usage; }
    Usage getUsage() const { return _usage; }

    // For Cube Texture, it's possible to generate the irradiance spherical harmonics and make them availalbe with the texture
    bool generateIrradiance();
    const SHPointer& getIrradiance(uint16 slice = 0) const { return _irradiance; }
    bool isIrradianceValid() const { return _isIrradianceValid; }

    // Own sampler
    void setSampler(const Sampler& sampler);
    const Sampler& getSampler() const { return _sampler; }
    Stamp getSamplerStamp() const { return _samplerStamp; }

    // Only callable by the Backend
    void notifyMipFaceGPULoaded(uint16 level, uint8 face = 0) const { return _storage->notifyMipFaceGPULoaded(level, face); }

    const GPUObjectPointer gpuObject {};

protected:
    std::unique_ptr< Storage > _storage;

    Stamp _stamp = 0;

    Sampler _sampler;
    Stamp _samplerStamp;

    uint32 _size = 0;
    Element _texelFormat;

    uint16 _width = 1;
    uint16 _height = 1;
    uint16 _depth = 1;

    uint16 _numSamples = 1;
    uint16 _numSlices = 1;

    uint16 _maxMip { 0 };
    uint16 _minMip { 0 };
 
    Type _type = TEX_1D;

    Usage _usage;

    SHPointer _irradiance;
    bool _autoGenerateMips = false;
    bool _isIrradianceValid = false;
    bool _defined = false;
   
    static Texture* create(Type type, const Element& texelFormat, uint16 width, uint16 height, uint16 depth, uint16 numSamples, uint16 numSlices, const Sampler& sampler);

    Size resize(Type type, const Element& texelFormat, uint16 width, uint16 height, uint16 depth, uint16 numSamples, uint16 numSlices);
};

typedef std::shared_ptr<Texture> TexturePointer;
typedef std::vector< TexturePointer > Textures;

 // TODO: For now TextureView works with Texture as a place holder for the Texture.
 // The overall logic should be about the same except that the Texture will be a real GL Texture under the hood
class TextureView {
public:
    typedef Resource::Size Size;

    TexturePointer _texture = TexturePointer(NULL);
    uint16 _subresource = 0;
    Element _element = Element(gpu::VEC4, gpu::NUINT8, gpu::RGBA);

    TextureView() {};

    TextureView(const Element& element) :
         _element(element)
    {};

    // create the TextureView and own the Texture
    TextureView(Texture* newTexture, const Element& element) :
        _texture(newTexture),
        _subresource(0),
        _element(element)
    {};
    TextureView(const TexturePointer& texture, uint16 subresource, const Element& element) :
        _texture(texture),
        _subresource(subresource),
        _element(element)
    {};

    TextureView(const TexturePointer& texture, uint16 subresource) :
        _texture(texture),
        _subresource(subresource)
    {};

    ~TextureView() {}
    TextureView(const TextureView& view) = default;
    TextureView& operator=(const TextureView& view) = default;

    explicit operator bool() const { return bool(_texture); }
    bool operator !() const { return (!_texture); }

    bool isValid() const { return bool(_texture); }
};
typedef std::vector<TextureView> TextureViews;

// TextureSource is the bridge between a URL or a a way to produce an image and the final gpu::Texture that will be used to render it.
// It provides the mechanism to create a texture using a customizable TextureLoader
class TextureSource {
public:
    TextureSource();
    ~TextureSource();

    const QUrl& getUrl() const { return _imageUrl; }
    const gpu::TexturePointer getGPUTexture() const { return _gpuTexture; }

    void reset(const QUrl& url);

    void resetTexture(gpu::TexturePointer texture);

    bool isDefined() const;

protected:
    gpu::TexturePointer _gpuTexture;
    QUrl _imageUrl;
};
typedef std::shared_ptr< TextureSource > TextureSourcePointer;

};

Q_DECLARE_METATYPE(gpu::TexturePointer)

#endif
