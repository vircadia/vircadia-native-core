//
//  KTX.h
//  ktx/src/ktx
//
//  Created by Zach Pomerantz on 2/08/2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once
#ifndef hifi_ktx_KTX_h
#define hifi_ktx_KTX_h

#include <array>
#include <list>
#include <vector>
#include <cstdint>
#include <cstring>
#include <string>
#include <memory>

#include <shared/Storage.h>

/* KTX Spec:

Byte[12] identifier
UInt32 endianness
UInt32 glType
UInt32 glTypeSize
UInt32 glFormat
Uint32 glInternalFormat
Uint32 glBaseInternalFormat
UInt32 pixelWidth
UInt32 pixelHeight
UInt32 pixelDepth
UInt32 numberOfArrayElements
UInt32 numberOfFaces
UInt32 numberOfMipmapLevels
UInt32 bytesOfKeyValueData
  
for each keyValuePair that fits in bytesOfKeyValueData
    UInt32   keyAndValueByteSize
    Byte     keyAndValue[keyAndValueByteSize]
    Byte     valuePadding[3 - ((keyAndValueByteSize + 3) % 4)]
end
  
for each mipmap_level in numberOfMipmapLevels*
    UInt32 imageSize; 
    for each array_element in numberOfArrayElements*
       for each face in numberOfFaces
           for each z_slice in pixelDepth*
               for each row or row_of_blocks in pixelHeight*
                   for each pixel or block_of_pixels in pixelWidth
                       Byte data[format-specific-number-of-bytes]**
                   end
               end
           end
           Byte cubePadding[0-3]
       end
    end
    Byte mipPadding[3 - ((imageSize + 3) % 4)]
end

* Replace with 1 if this field is 0.

** Uncompressed texture data matches a GL_UNPACK_ALIGNMENT of 4.
*/



namespace ktx {
    const uint32_t PACKING_SIZE { sizeof(uint32_t) };
    const std::string HIFI_MIN_POPULATED_MIP_KEY{ "hifi.minMip" };

    using Byte = uint8_t;

    enum class GLType : uint32_t {
        COMPRESSED_TYPE                 = 0,

        // GL 4.4 Table 8.2
        UNSIGNED_BYTE                   = 0x1401,
        BYTE                            = 0x1400,
        UNSIGNED_SHORT                  = 0x1403,
        SHORT                           = 0x1402,
        UNSIGNED_INT                    = 0x1405,
        INT                             = 0x1404,
        HALF_FLOAT                      = 0x140B,
        FLOAT                           = 0x1406,
        UNSIGNED_BYTE_3_3_2             = 0x8032,
        UNSIGNED_BYTE_2_3_3_REV         = 0x8362,
        UNSIGNED_SHORT_5_6_5            = 0x8363,
        UNSIGNED_SHORT_5_6_5_REV        = 0x8364,
        UNSIGNED_SHORT_4_4_4_4          = 0x8033,
        UNSIGNED_SHORT_4_4_4_4_REV      = 0x8365,
        UNSIGNED_SHORT_5_5_5_1          = 0x8034,
        UNSIGNED_SHORT_1_5_5_5_REV      = 0x8366,
        UNSIGNED_INT_8_8_8_8            = 0x8035,
        UNSIGNED_INT_8_8_8_8_REV        = 0x8367,
        UNSIGNED_INT_10_10_10_2         = 0x8036,
        UNSIGNED_INT_2_10_10_10_REV     = 0x8368,
        UNSIGNED_INT_24_8               = 0x84FA,
        UNSIGNED_INT_10F_11F_11F_REV    = 0x8C3B,
        UNSIGNED_INT_5_9_9_9_REV        = 0x8C3E,
        FLOAT_32_UNSIGNED_INT_24_8_REV  = 0x8DAD,
    };

    enum class GLFormat : uint32_t {
        COMPRESSED_FORMAT               = 0,

        // GL 4.4 Table 8.3
        STENCIL_INDEX                   = 0x1901,
        DEPTH_COMPONENT                 = 0x1902,
        DEPTH_STENCIL                   = 0x84F9,

        RED                             = 0x1903,
        GREEN                           = 0x1904,
        BLUE                            = 0x1905,
        RG                              = 0x8227,
        RGB                             = 0x1907,
        RGBA                            = 0x1908,
        BGR                             = 0x80E0,
        BGRA                            = 0x80E1,

        RG_INTEGER                      = 0x8228,
        RED_INTEGER                     = 0x8D94,
        GREEN_INTEGER                   = 0x8D95,
        BLUE_INTEGER                    = 0x8D96,
        RGB_INTEGER                     = 0x8D98,
        RGBA_INTEGER                    = 0x8D99,
        BGR_INTEGER                     = 0x8D9A,
        BGRA_INTEGER                    = 0x8D9B,
    };

    enum class GLInternalFormat_Uncompressed : uint32_t {
        // GL 4.4 Table 8.12
        R8                              = 0x8229,
        R8_SNORM                        = 0x8F94,

        R16                             = 0x822A,
        R16_SNORM                       = 0x8F98,

        RG8                             = 0x822B,
        RG8_SNORM                       = 0x8F95,

        RG16                            = 0x822C,
        RG16_SNORM                      = 0x8F99,

        R3_G3_B2                        = 0x2A10,
        RGB4                            = 0x804F,
        RGB5                            = 0x8050,
        RGB565                          = 0x8D62,

        RGB8                            = 0x8051,
        RGB8_SNORM                      = 0x8F96,
        RGB10                           = 0x8052,
        RGB12                           = 0x8053,

        RGB16                           = 0x8054,
        RGB16_SNORM                     = 0x8F9A,

        RGBA2                           = 0x8055,
        RGBA4                           = 0x8056,
        RGB5_A1                         = 0x8057,
        RGBA8                           = 0x8058,
        RGBA8_SNORM                     = 0x8F97,

        RGB10_A2                        = 0x8059,
        RGB10_A2UI                      = 0x906F,

        RGBA12                          = 0x805A,
        RGBA16                          = 0x805B,
        RGBA16_SNORM                    = 0x8F9B,

        SRGB8                           = 0x8C41,
        SRGB8_ALPHA8                    = 0x8C43,

        R16F                            = 0x822D,
        RG16F                           = 0x822F,
        RGB16F                          = 0x881B,
        RGBA16F                         = 0x881A,

        R32F      = 0x822E,
        RG32F      = 0x8230,
        RGB32F      = 0x8815,
        RGBA32F      = 0x8814,

        R11F_G11F_B10F      = 0x8C3A,
        RGB9_E5      = 0x8C3D,


        R8I      = 0x8231,
        R8UI      = 0x8232,
        R16I      = 0x8233,
        R16UI      = 0x8234,
        R32I      = 0x8235,
        R32UI      = 0x8236,
        RG8I      = 0x8237,
        RG8UI      = 0x8238,
        RG16I      = 0x8239,
        RG16UI      = 0x823A,
        RG32I      = 0x823B,
        RG32UI      = 0x823C,

        RGB8I      = 0x8D8F,
        RGB8UI      = 0x8D7D,
        RGB16I      = 0x8D89,
        RGB16UI      = 0x8D77,

        RGB32I      = 0x8D83,
        RGB32UI      = 0x8D71,
        RGBA8I      = 0x8D8E,
        RGBA8UI      = 0x8D7C,
        RGBA16I      = 0x8D88,
        RGBA16UI      = 0x8D76,
        RGBA32I      = 0x8D82,

        RGBA32UI      = 0x8D70,

        // GL 4.4 Table 8.13
        DEPTH_COMPONENT16 = 0x81A5,
        DEPTH_COMPONENT24 = 0x81A6,
        DEPTH_COMPONENT32 = 0x81A7,

        DEPTH_COMPONENT32F = 0x8CAC,
        DEPTH24_STENCIL8 = 0x88F0,
        DEPTH32F_STENCIL8 = 0x8CAD,

        STENCIL_INDEX1 = 0x8D46,
        STENCIL_INDEX4 = 0x8D47,
        STENCIL_INDEX8 = 0x8D48,
        STENCIL_INDEX16 = 0x8D49,
    };

    enum class GLInternalFormat_Compressed : uint32_t {
        // GL 4.4 Table 8.14
        COMPRESSED_RED = 0x8225,
        COMPRESSED_RG = 0x8226,
        COMPRESSED_RGB = 0x84ED,
        COMPRESSED_RGBA = 0x84EE,

        COMPRESSED_SRGB = 0x8C48,
        COMPRESSED_SRGB_ALPHA = 0x8C49,

        COMPRESSED_SRGB_S3TC_DXT1_EXT = 0x8C4C,
        COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT = 0x8C4D,
        COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT = 0x8C4E,
        COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT = 0x8C4F,

        COMPRESSED_RED_RGTC1 = 0x8DBB,
        COMPRESSED_SIGNED_RED_RGTC1 = 0x8DBC,
        COMPRESSED_RG_RGTC2 = 0x8DBD,
        COMPRESSED_SIGNED_RG_RGTC2 = 0x8DBE,

        COMPRESSED_RGBA_BPTC_UNORM = 0x8E8C,
        COMPRESSED_SRGB_ALPHA_BPTC_UNORM = 0x8E8D,
        COMPRESSED_RGB_BPTC_SIGNED_FLOAT = 0x8E8E,
        COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT = 0x8E8F,

        COMPRESSED_RGB8_ETC2 = 0x9274,
        COMPRESSED_SRGB8_ETC2 = 0x9275,
        COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2 = 0x9276,
        COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2 = 0x9277,
        COMPRESSED_RGBA8_ETC2_EAC = 0x9278,
        COMPRESSED_SRGB8_ALPHA8_ETC2_EAC = 0x9279,

        COMPRESSED_R11_EAC = 0x9270,
        COMPRESSED_SIGNED_R11_EAC = 0x9271,
        COMPRESSED_RG11_EAC = 0x9272,
        COMPRESSED_SIGNED_RG11_EAC = 0x9273,
    };
 
    enum class GLBaseInternalFormat : uint32_t {
        // GL 4.4 Table 8.11
        DEPTH_COMPONENT = 0x1902,
        DEPTH_STENCIL = 0x84F9,
        RED = 0x1903,
        RG = 0x8227,
        RGB = 0x1907,
        RGBA = 0x1908,
        STENCIL_INDEX = 0x1901,
    };

    enum CubeMapFace {
        POS_X = 0,
        NEG_X = 1,
        POS_Y = 2,
        NEG_Y = 3,
        POS_Z = 4,
        NEG_Z = 5,
        NUM_CUBEMAPFACES = 6,
    };

    using Storage = storage::Storage;
    using StoragePointer = std::shared_ptr<Storage>;

    struct ImageDescriptor;
    using ImageDescriptors = std::vector<ImageDescriptor>;

    bool checkIdentifier(const Byte* identifier);

    // Header
    struct Header {
        static const size_t IDENTIFIER_LENGTH = 12;
        using Identifier = std::array<uint8_t, IDENTIFIER_LENGTH>;
        static const Identifier IDENTIFIER;

        static const uint32_t ENDIAN_TEST = 0x04030201;
        static const uint32_t REVERSE_ENDIAN_TEST = 0x01020304;

        static uint32_t evalPadding(size_t byteSize);

        Header();

        Byte identifier[IDENTIFIER_LENGTH];
        uint32_t endianness { ENDIAN_TEST };

        uint32_t glType;
        uint32_t glTypeSize { 0 };
        uint32_t glFormat;
        uint32_t glInternalFormat;
        uint32_t glBaseInternalFormat;

        uint32_t pixelWidth { 1 };
        uint32_t pixelHeight { 0 };
        uint32_t pixelDepth { 0 };
        uint32_t numberOfArrayElements { 0 };
        uint32_t numberOfFaces { 1 };
        uint32_t numberOfMipmapLevels { 1 };

        uint32_t bytesOfKeyValueData { 0 };

        uint32_t getPixelWidth() const { return (pixelWidth ? pixelWidth : 1); }
        uint32_t getPixelHeight() const { return (pixelHeight ? pixelHeight : 1); }
        uint32_t getPixelDepth() const { return (pixelDepth ? pixelDepth : 1); }
        uint32_t getNumberOfSlices() const { return (numberOfArrayElements ? numberOfArrayElements : 1); }
        uint32_t getNumberOfLevels() const { return (numberOfMipmapLevels ? numberOfMipmapLevels : 1); }

        uint32_t evalMaxDimension() const;
        uint32_t evalPixelOrBlockWidth(uint32_t level) const;
        uint32_t evalPixelOrBlockHeight(uint32_t level) const;
        uint32_t evalPixelOrBlockDepth(uint32_t level) const;

        size_t evalPixelOrBlockSize() const;
        size_t evalRowSize(uint32_t level) const;
        size_t evalFaceSize(uint32_t level) const;
        size_t evalImageSize(uint32_t level) const;

        void setUncompressed(GLType type, uint32_t typeSize, GLFormat format, GLInternalFormat_Uncompressed internalFormat, GLBaseInternalFormat baseInternalFormat) {
            glType = (uint32_t) type;
            glTypeSize = typeSize;
            glFormat = (uint32_t) format;
            glInternalFormat = (uint32_t) internalFormat;
            glBaseInternalFormat = (uint32_t) baseInternalFormat;
        }
        void setCompressed(GLInternalFormat_Compressed internalFormat, GLBaseInternalFormat baseInternalFormat) {
            glType = (uint32_t) GLType::COMPRESSED_TYPE;
            glTypeSize = 1;
            glFormat = (uint32_t) GLFormat::COMPRESSED_FORMAT;
            glInternalFormat = (uint32_t) internalFormat;
            glBaseInternalFormat = (uint32_t) baseInternalFormat;
        }

        GLType getGLType() const { return (GLType)glType; }
        uint32_t getTypeSize() const { return glTypeSize; }
        GLFormat getGLFormat() const { return (GLFormat)glFormat; }
        GLInternalFormat_Uncompressed getGLInternaFormat_Uncompressed() const { return (GLInternalFormat_Uncompressed)glInternalFormat; }
        GLInternalFormat_Compressed getGLInternaFormat_Compressed() const { return (GLInternalFormat_Compressed)glInternalFormat; }
        GLBaseInternalFormat getGLBaseInternalFormat() const { return (GLBaseInternalFormat)glBaseInternalFormat; }


        void setDimensions(uint32_t width, uint32_t height = 0, uint32_t depth = 0, uint32_t numSlices = 0, uint32_t numFaces = 1) {
            pixelWidth = (width > 0 ? width : 1);
            pixelHeight = height;
            pixelDepth = depth;
            numberOfArrayElements = numSlices;
            numberOfFaces = ((numFaces == 1) || (numFaces == NUM_CUBEMAPFACES) ? numFaces : 1);
        }
        void set1D(uint32_t width) { setDimensions(width); }
        void set1DArray(uint32_t width, uint32_t numSlices) { setDimensions(width, 0, 0, (numSlices > 0 ? numSlices : 1)); }
        void set2D(uint32_t width, uint32_t height) { setDimensions(width, height); }
        void set2DArray(uint32_t width, uint32_t height, uint32_t numSlices) { setDimensions(width, height, 0, (numSlices > 0 ? numSlices : 1)); }
        void set3D(uint32_t width, uint32_t height, uint32_t depth) { setDimensions(width, height, depth); }
        void set3DArray(uint32_t width, uint32_t height, uint32_t depth, uint32_t numSlices) { setDimensions(width, height, depth, (numSlices > 0 ? numSlices : 1)); }
        void setCube(uint32_t width, uint32_t height) { setDimensions(width, height, 0, 0, NUM_CUBEMAPFACES); }
        void setCubeArray(uint32_t width, uint32_t height, uint32_t numSlices) { setDimensions(width, height, 0, (numSlices > 0 ? numSlices : 1), NUM_CUBEMAPFACES); }

        ImageDescriptors generateImageDescriptors() const;
    };
    static const size_t KTX_HEADER_SIZE = 64;
    static_assert(sizeof(Header) == KTX_HEADER_SIZE, "KTX Header size is static and should not change from the spec");
    static const size_t KV_SIZE_WIDTH = 4; // Number of bytes for keyAndValueByteSize
    static const size_t IMAGE_SIZE_WIDTH = 4; // Number of bytes for imageSize

    // Key Values
    struct KeyValue {
        uint32_t _byteSize { 0 };
        std::string _key;
        std::vector<Byte> _value;


        KeyValue(const std::string& key, uint32_t valueByteSize, const Byte* value);

        KeyValue(const std::string& key, const std::string& value);

        uint32_t serializedByteSize() const;

        static KeyValue parseSerializedKeyAndValue(uint32_t srcSize, const Byte* srcBytes);
        static uint32_t writeSerializedKeyAndValue(Byte* destBytes, uint32_t destByteSize, const KeyValue& keyval);

        using KeyValues = std::list<KeyValue>;
        static uint32_t serializedKeyValuesByteSize(const KeyValues& keyValues);

    };
    using KeyValues = KeyValue::KeyValues;

    struct ImageHeader {
        using FaceOffsets = std::vector<size_t>;
        using FaceBytes = std::vector<const Byte*>;

        // This is the byte offset from the _start_ of the image region. For example, level 0
        // will have a byte offset of 0.
        const uint32_t _numFaces;
        const size_t _imageOffset;
        const uint32_t _imageSize;
        const uint32_t _faceSize;
        const uint32_t _padding;
        ImageHeader(bool cube, size_t imageOffset, uint32_t imageSize, uint32_t padding) :
            _numFaces(cube ? NUM_CUBEMAPFACES : 1),
            _imageOffset(imageOffset),
            _imageSize(imageSize * _numFaces),
            _faceSize(imageSize),
            _padding(padding) {
        }
    };

    struct Image;

    // Image without the image data itself
    struct ImageDescriptor : public ImageHeader {
        const FaceOffsets _faceOffsets;
        ImageDescriptor(const ImageHeader& header, const FaceOffsets& offsets) : ImageHeader(header), _faceOffsets(offsets) {}
        Image toImage(const ktx::StoragePointer& storage) const;
    };

    // Image with the image data itself
    struct Image : public ImageHeader {
        FaceBytes _faceBytes;
        Image(const ImageHeader& header, const FaceBytes& faces) : ImageHeader(header), _faceBytes(faces) {}
        Image(size_t imageOffset, uint32_t imageSize, uint32_t padding, const Byte* bytes) :
            ImageHeader(false, imageOffset, imageSize, padding),
            _faceBytes(1, bytes) {}
        Image(size_t imageOffset, uint32_t pageSize, uint32_t padding, const FaceBytes& cubeFaceBytes) :
            ImageHeader(true, imageOffset, pageSize, padding)
            {
                if (cubeFaceBytes.size() == NUM_CUBEMAPFACES) {
                    _faceBytes = cubeFaceBytes;
                }
            }

        ImageDescriptor toImageDescriptor(const Byte* baseAddress) const;
    };

    using Images = std::vector<Image>;

    class KTX;

    // A KTX descriptor is a lightweight container for all the information about a serialized KTX file, but without the 
    // actual image / face data available.
    struct KTXDescriptor {
        KTXDescriptor(const Header& header, const KeyValues& keyValues, const ImageDescriptors& imageDescriptors) : header(header), keyValues(keyValues), images(imageDescriptors) {}
        const Header header;
        const KeyValues keyValues;
        const ImageDescriptors images;
        size_t getMipFaceTexelsSize(uint16_t mip = 0, uint8_t face = 0) const;
        size_t getMipFaceTexelsOffset(uint16_t mip = 0, uint8_t face = 0) const;
        size_t getValueOffsetForKey(const std::string& key) const;
    };

    class KTX {
        void resetStorage(const StoragePointer& src);

        KTX();
        KTX(const StoragePointer& storage, const Header& header, const KeyValues& keyValues, const Images& images);
    public:
        ~KTX();

        // Define a KTX object manually to write it somewhere (in a file on disk?)
        // This path allocate the Storage where to store header, keyvalues and copy mips
        // Then COPY all the data
        static std::unique_ptr<KTX> create(const Header& header, const Images& images, const KeyValues& keyValues = KeyValues());
        static std::unique_ptr<KTX> createBare(const Header& header, const KeyValues& keyValues = KeyValues());

        // Instead of creating a full Copy of the src data in a KTX object, the write serialization can be performed with the
        // following two functions
        //   size_t sizeNeeded = KTX::evalStorageSize(header, images);
        //   
        //   //allocate a buffer of size "sizeNeeded" or map a file with enough capacity
        //   Byte* destBytes = new Byte[sizeNeeded];
        //
        //   // THen perform the writing of the src data to the destinnation buffer
        //   write(destBytes, sizeNeeded, header, images);
        //
        // This is exactly what is done in the create function
        static size_t evalStorageSize(const Header& header, const Images& images, const KeyValues& keyValues = KeyValues());
        static size_t evalStorageSize(const Header& header, const ImageDescriptors& images, const KeyValues& keyValues = KeyValues());
        static size_t write(Byte* destBytes, size_t destByteSize, const Header& header, const Images& images, const KeyValues& keyValues = KeyValues());
        static size_t writeWithoutImages(Byte* destBytes, size_t destByteSize, const Header& header, const ImageDescriptors& descriptors, const KeyValues& keyValues = KeyValues());
        static size_t writeKeyValues(Byte* destBytes, size_t destByteSize, const KeyValues& keyValues);
        static Images writeImages(Byte* destBytes, size_t destByteSize, const Images& images);

        void writeMipData(uint16_t level, const Byte* sourceBytes, size_t source_size);

        // Parse a block of memory and create a KTX object from it
        static std::unique_ptr<KTX> create(const StoragePointer& src);

        static bool checkHeaderFromStorage(size_t srcSize, const Byte* srcBytes);
        static KeyValues parseKeyValues(size_t srcSize, const Byte* srcBytes);
        static Images parseImages(const Header& header, size_t srcSize, const Byte* srcBytes);

        // Access raw pointers to the main sections of the KTX
        const Header& getHeader() const;

        const Byte* getKeyValueData() const;
        const Byte* getTexelsData() const;
        storage::StoragePointer getMipFaceTexelsData(uint16_t mip = 0, uint8_t face = 0) const;
        const StoragePointer& getStorage() const { return _storage; }

        KTXDescriptor toDescriptor() const;
        size_t getKeyValueDataSize() const;
        size_t getTexelsDataSize() const;

        Header _header;
        StoragePointer _storage;
        KeyValues _keyValues;
        Images _images;

        friend struct KTXDescriptor;
    };

}

#endif // hifi_ktx_KTX_h
