//
//  Created by Bradley Austin Davis on 2016/05/15
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "GLTexelFormat.h"

using namespace gpu;
using namespace gpu::gl;

bool GLTexelFormat::isCompressed() const {
    switch (internalFormat) {
        case GL_COMPRESSED_SRGB_S3TC_DXT1_EXT:
        case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT:
        case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT:
        case GL_COMPRESSED_RED_RGTC1:
        case GL_COMPRESSED_RG_RGTC2:
        case GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM:
            return true;
        default:
            return false;
    }
}

GLenum GLTexelFormat::evalGLTexelFormatInternal(const gpu::Element& dstFormat) {
    GLenum result = GL_RGBA8;
    switch (dstFormat.getDimension()) {
        case gpu::SCALAR: {
            switch (dstFormat.getSemantic()) {
                case gpu::RED:
                case gpu::RGB:
                case gpu::RGBA:
                case gpu::SRGB:
                case gpu::SRGBA:
                    switch (dstFormat.getType()) {
                        case gpu::UINT32:
                            result = GL_R32UI;
                            break;
                        case gpu::INT32:
                            result = GL_R32I;
                            break;
                        case gpu::NUINT32:
                            result = GL_R8;
                            break;
                        case gpu::NINT32:
                            result = GL_R8_SNORM;
                            break;
                        case gpu::FLOAT:
                            result = GL_R32F;
                            break;
                        case gpu::UINT16:
                            result = GL_R16UI;
                            break;
                        case gpu::INT16:
                            result = GL_R16I;
                            break;
                        case gpu::NUINT16:
                            result = GL_R16;
                            break;
                        case gpu::NINT16:
                            result = GL_R16_SNORM;
                            break;
                        case gpu::HALF:
                            result = GL_R16F;
                            break;
                        case gpu::UINT8:
                            result = GL_R8UI;
                            break;
                        case gpu::INT8:
                            result = GL_R8I;
                            break;
                        case gpu::NUINT8:
                            if ((dstFormat.getSemantic() == gpu::SRGB || dstFormat.getSemantic() == gpu::SRGBA)) {
                                result = GL_SLUMINANCE8;
                            } else {
                                result = GL_R8;
                            }
                            break;
                        case gpu::NINT8:
                            result = GL_R8_SNORM;
                            break;

                        default:
                            Q_UNREACHABLE();
                            break;
                    }
                    break;

                case gpu::R11G11B10:
                    // the type should be float
                    result = GL_R11F_G11F_B10F;
                    break;

                case gpu::DEPTH:
                    result = GL_DEPTH_COMPONENT32;
                    switch (dstFormat.getType()) {
                        case gpu::UINT32:
                        case gpu::INT32:
                        case gpu::NUINT32:
                        case gpu::NINT32:
                            result = GL_DEPTH_COMPONENT32;
                            break;
                        case gpu::FLOAT:
                            result = GL_DEPTH_COMPONENT32F;
                            break;
                        case gpu::UINT16:
                        case gpu::INT16:
                        case gpu::NUINT16:
                        case gpu::NINT16:
                        case gpu::HALF:
                            result = GL_DEPTH_COMPONENT16;
                            break;
                        case gpu::UINT8:
                        case gpu::INT8:
                        case gpu::NUINT8:
                        case gpu::NINT8:
                            result = GL_DEPTH_COMPONENT24;
                            break;
                        default:
                            Q_UNREACHABLE();
                            break;
                    }
                    break;

                 case gpu::DEPTH_STENCIL:
                    result = GL_DEPTH24_STENCIL8;
                    break;

                default:
                    qCWarning(gpugllogging) << "Unknown combination of texel format";
            }
            break;
        }

        case gpu::VEC2: {
            switch (dstFormat.getSemantic()) {
                case gpu::RGB:
                case gpu::RGBA:
                case gpu::XY:
                    result = GL_RG8;
                    break;
                default:
                    qCWarning(gpugllogging) << "Unknown combination of texel format";
            }

            break;
        }

        case gpu::VEC3: {
            switch (dstFormat.getSemantic()) {
                case gpu::RGB:
                case gpu::RGBA:
                    result = GL_RGB8;
                    break;
                case gpu::SRGB:
                case gpu::SRGBA:
                    result = GL_SRGB8; // standard 2.2 gamma correction color
                    break;
                default:
                    qCWarning(gpugllogging) << "Unknown combination of texel format";
            }

            break;
        }

        case gpu::VEC4: {
            switch (dstFormat.getSemantic()) {
                case gpu::RGB:
                    result = GL_RGB8;
                    break;
                case gpu::RGBA:
                    switch (dstFormat.getType()) {
                        case gpu::UINT32:
                            result = GL_RGBA32UI;
                            break;
                        case gpu::INT32:
                            result = GL_RGBA32I;
                            break;
                        case gpu::FLOAT:
                            result = GL_RGBA32F;
                            break;
                        case gpu::UINT16:
                            result = GL_RGBA16UI;
                            break;
                        case gpu::INT16:
                            result = GL_RGBA16I;
                            break;
                        case gpu::NUINT16:
                            result = GL_RGBA16;
                            break;
                        case gpu::NINT16:
                            result = GL_RGBA16_SNORM;
                            break;
                        case gpu::HALF:
                            result = GL_RGBA16F;
                            break;
                        case gpu::UINT8:
                            result = GL_RGBA8UI;
                            break;
                        case gpu::INT8:
                            result = GL_RGBA8I;
                            break;
                        case gpu::NUINT8:
                            result = GL_RGBA8;
                            break;
                        case gpu::NINT8:
                            result = GL_RGBA8_SNORM;
                            break;
                        case gpu::NUINT32:
                        case gpu::NINT32:
                        case gpu::COMPRESSED:
                        case gpu::NUM_TYPES: // quiet compiler
                            Q_UNREACHABLE();
                    }
                    break;
                case gpu::SRGB:
                    result = GL_SRGB8;
                    break;
                case gpu::SRGBA:
                    result = GL_SRGB8_ALPHA8; // standard 2.2 gamma correction color
                    break;
                default:
                    qCWarning(gpugllogging) << "Unknown combination of texel format";
            }
            break;
        }
        case gpu::TILE4x4: {
            switch (dstFormat.getSemantic()) {
                case gpu::COMPRESSED_BC4_RED:
                    result = GL_COMPRESSED_RED_RGTC1;
                    break;
                case gpu::COMPRESSED_BC1_SRGB:
                    result = GL_COMPRESSED_SRGB_S3TC_DXT1_EXT;
                    break;
                case gpu::COMPRESSED_BC1_SRGBA:
                    result = GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT;
                    break;
                case gpu::COMPRESSED_BC3_SRGBA:
                    result = GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT;
                    break;
                case gpu::COMPRESSED_BC5_XY:
                    result = GL_COMPRESSED_RG_RGTC2;
                    break;
                case gpu::COMPRESSED_BC7_SRGBA:
                    result = GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM;
                    break;

                default:
                    qCWarning(gpugllogging) << "Unknown combination of texel format";
            }
            break;
        }

        default:
            qCWarning(gpugllogging) << "Unknown combination of texel format";
    }
    return result;
}

GLTexelFormat GLTexelFormat::evalGLTexelFormat(const Element& dstFormat, const Element& srcFormat) {
    if (dstFormat != srcFormat) {
        GLTexelFormat texel = { GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE };

        switch (dstFormat.getDimension()) {
        case gpu::SCALAR: {
            texel.format = GL_RED;
            texel.type = ELEMENT_TYPE_TO_GL[dstFormat.getType()];

            switch (dstFormat.getSemantic()) {
            case gpu::RED:
            case gpu::RGB:
            case gpu::RGBA:
                texel.internalFormat = GL_R8;
                break;

            case gpu::DEPTH:
                texel.format = GL_DEPTH_COMPONENT;
                texel.internalFormat = GL_DEPTH_COMPONENT32;
                break;

            case gpu::DEPTH_STENCIL:
                texel.type = GL_UNSIGNED_INT_24_8;
                texel.format = GL_DEPTH_STENCIL;
                texel.internalFormat = GL_DEPTH24_STENCIL8;
                break;
            default:
                qCWarning(gpugllogging) << "Unknown combination of texel format";
            }
            break;
        }

        case gpu::VEC2: {
            texel.format = GL_RG;
            texel.type = ELEMENT_TYPE_TO_GL[dstFormat.getType()];

            switch (dstFormat.getSemantic()) {
            case gpu::RGB:
            case gpu::RGBA:
            case gpu::XY:
                texel.internalFormat = GL_RG8;
                break;
            default:
                qCWarning(gpugllogging) << "Unknown combination of texel format";
            }

            break;
        }

        case gpu::VEC3: {
            texel.format = GL_RGB;

            texel.type = ELEMENT_TYPE_TO_GL[dstFormat.getType()];

            switch (dstFormat.getSemantic()) {
            case gpu::RGB:
            case gpu::RGBA:
                texel.internalFormat = GL_RGB8;
                break;
            default:
                qCWarning(gpugllogging) << "Unknown combination of texel format";
            }

            break;
        }

        case gpu::VEC4: {
            texel.format = GL_RGBA;
            texel.type = ELEMENT_TYPE_TO_GL[dstFormat.getType()];

            switch (srcFormat.getSemantic()) {
            case gpu::BGRA:
            case gpu::SBGRA:
                texel.format = GL_BGRA;
                break;
            case gpu::RGB:
            case gpu::RGBA:
            case gpu::SRGB:
            case gpu::SRGBA:
            default:
                break;
            };

            switch (dstFormat.getSemantic()) {
            case gpu::RGB:
                texel.internalFormat = GL_RGB8;
                break;
            case gpu::RGBA:
                texel.internalFormat = GL_RGBA8;
                break;
            case gpu::SRGB:
                texel.internalFormat = GL_SRGB8;
                break;
            case gpu::SRGBA:
                texel.internalFormat = GL_SRGB8_ALPHA8;
                break;
            default:
                qCWarning(gpugllogging) << "Unknown combination of texel format";
            }

            break;
        }

        case gpu::TILE4x4: {
            texel.format = GL_RGBA;
            texel.type = ELEMENT_TYPE_TO_GL[srcFormat.getType()];

            switch (srcFormat.getSemantic()) {
            case gpu::BGRA:
            case gpu::SBGRA:
                texel.format = GL_BGRA;
                break;
            case gpu::RGB:
            case gpu::RGBA:
            case gpu::SRGB:
            case gpu::SRGBA:
            default:
                break;
            };

            switch (dstFormat.getSemantic()) {
            case gpu::COMPRESSED_BC4_RED:
                texel.internalFormat = GL_COMPRESSED_RED_RGTC1;
                break;
            case gpu::COMPRESSED_BC1_SRGB:
                texel.internalFormat = GL_COMPRESSED_SRGB_S3TC_DXT1_EXT;
                break;
            case gpu::COMPRESSED_BC1_SRGBA:
                texel.internalFormat = GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT;
                break;
            case gpu::COMPRESSED_BC3_SRGBA:
                texel.internalFormat = GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT;
                break;
            case gpu::COMPRESSED_BC5_XY:
                texel.internalFormat = GL_COMPRESSED_RG_RGTC2;
                break;
            case gpu::COMPRESSED_BC7_SRGBA:
                texel.internalFormat = GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM;
                break;
            default:
                qCWarning(gpugllogging) << "Unknown combination of texel format";
            }
            break;
        }

        default:
            qCWarning(gpugllogging) << "Unknown combination of texel format";
        }
        return texel;
    } else {
        GLTexelFormat texel = { GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE };

        switch (dstFormat.getDimension()) {
        case gpu::SCALAR: {
            texel.format = GL_RED;
            texel.type = ELEMENT_TYPE_TO_GL[dstFormat.getType()];

            switch (dstFormat.getSemantic()) {
            case gpu::RED:
            case gpu::RGB:
            case gpu::RGBA:
            case gpu::SRGB:
            case gpu::SRGBA:
                texel.internalFormat = GL_RED;
                switch (dstFormat.getType()) {
                case gpu::UINT32: {
                    texel.internalFormat = GL_R32UI;
                    break;
                }
                case gpu::INT32: {
                    texel.internalFormat = GL_R32I;
                    break;
                }
                case gpu::NUINT32: {
                    texel.internalFormat = GL_R8;
                    break;
                }
                case gpu::NINT32: {
                    texel.internalFormat = GL_R8_SNORM;
                    break;
                }
                case gpu::FLOAT: {
                    texel.internalFormat = GL_R32F;
                    break;
                }
                case gpu::UINT16: {
                    texel.internalFormat = GL_R16UI;
                    break;
                }
                case gpu::INT16: {
                    texel.internalFormat = GL_R16I;
                    break;
                }
                case gpu::NUINT16: {
                    texel.internalFormat = GL_R16;
                    break;
                }
                case gpu::NINT16: {
                    texel.internalFormat = GL_R16_SNORM;
                    break;
                }
                case gpu::HALF: {
                    texel.internalFormat = GL_R16F;
                    break;
                }
                case gpu::UINT8: {
                    texel.internalFormat = GL_R8UI;
                    break;
                }
                case gpu::INT8: {
                    texel.internalFormat = GL_R8I;
                    break;
                }
                case gpu::NUINT8: {
                    if ((dstFormat.getSemantic() == gpu::SRGB || dstFormat.getSemantic() == gpu::SRGBA)) {
                        texel.internalFormat = GL_SLUMINANCE8;
                    } else {
                        texel.internalFormat = GL_R8;
                    }
                    break;
                }
                case gpu::NINT8: {
                    texel.internalFormat = GL_R8_SNORM;
                    break;
                }
                case gpu::COMPRESSED:
                case gpu::NUM_TYPES: { // quiet compiler
                    Q_UNREACHABLE();
                }

                }
                break;

            case gpu::R11G11B10:
                texel.format = GL_RGB;
                // the type should be float
                texel.internalFormat = GL_R11F_G11F_B10F;
                break;

            case gpu::DEPTH:
                texel.format = GL_DEPTH_COMPONENT; // It's depth component to load it
                texel.internalFormat = GL_DEPTH_COMPONENT32;
                switch (dstFormat.getType()) {
                case gpu::UINT32:
                case gpu::INT32:
                case gpu::NUINT32:
                case gpu::NINT32: {
                    texel.internalFormat = GL_DEPTH_COMPONENT32;
                    break;
                }
                case gpu::FLOAT: {
                    texel.internalFormat = GL_DEPTH_COMPONENT32F;
                    break;
                }
                case gpu::UINT16:
                case gpu::INT16:
                case gpu::NUINT16:
                case gpu::NINT16:
                case gpu::HALF: {
                    texel.internalFormat = GL_DEPTH_COMPONENT16;
                    break;
                }
                case gpu::UINT8:
                case gpu::INT8:
                case gpu::NUINT8:
                case gpu::NINT8: {
                    texel.internalFormat = GL_DEPTH_COMPONENT24;
                    break;
                }
                case gpu::COMPRESSED:
                case gpu::NUM_TYPES: { // quiet compiler
                    Q_UNREACHABLE();
                }
                }
                break;
            case gpu::DEPTH_STENCIL:
                texel.type = GL_UNSIGNED_INT_24_8;
                texel.format = GL_DEPTH_STENCIL;
                texel.internalFormat = GL_DEPTH24_STENCIL8;
                break;
            default:
                qCWarning(gpugllogging) << "Unknown combination of texel format";
            }

            break;
        }

        case gpu::VEC2: {
            texel.format = GL_RG;
            texel.type = ELEMENT_TYPE_TO_GL[dstFormat.getType()];

            switch (dstFormat.getSemantic()) {
            case gpu::RGB:
            case gpu::RGBA:
            case gpu::XY:
                texel.internalFormat = GL_RG8;
                break;
            default:
                qCWarning(gpugllogging) << "Unknown combination of texel format";
            }

            break;
        }

        case gpu::VEC3: {
            texel.format = GL_RGB;

            texel.type = ELEMENT_TYPE_TO_GL[dstFormat.getType()];

            switch (dstFormat.getSemantic()) {
            case gpu::RGB:
            case gpu::RGBA:
                texel.internalFormat = GL_RGB8;
                break;
            case gpu::SRGB:
            case gpu::SRGBA:
                texel.internalFormat = GL_SRGB8; // standard 2.2 gamma correction color
                break;
            default:
                qCWarning(gpugllogging) << "Unknown combination of texel format";
            }
            break;
        }

        case gpu::VEC4: {
            texel.format = GL_RGBA;
            texel.type = ELEMENT_TYPE_TO_GL[dstFormat.getType()];

            switch (dstFormat.getSemantic()) {
            case gpu::RGB:
                texel.internalFormat = GL_RGB8;
                break;
            case gpu::RGBA:
                texel.internalFormat = GL_RGBA8;
                switch (dstFormat.getType()) {
                case gpu::UINT32:
                    texel.format = GL_RGBA_INTEGER;
                    texel.internalFormat = GL_RGBA32UI;
                    break;
                case gpu::INT32:
                    texel.format = GL_RGBA_INTEGER;
                    texel.internalFormat = GL_RGBA32I;
                    break;
                case gpu::FLOAT:
                    texel.internalFormat = GL_RGBA32F;
                    break;
                case gpu::UINT16:
                    texel.format = GL_RGBA_INTEGER;
                    texel.internalFormat = GL_RGBA16UI;
                    break;
                case gpu::INT16:
                    texel.format = GL_RGBA_INTEGER;
                    texel.internalFormat = GL_RGBA16I;
                    break;
                case gpu::NUINT16:
                    texel.format = GL_RGBA;
                    texel.internalFormat = GL_RGBA16;
                    break;
                case gpu::NINT16:
                    texel.format = GL_RGBA;
                    texel.internalFormat = GL_RGBA16_SNORM;
                    break;
                case gpu::HALF:
                    texel.format = GL_RGBA;
                    texel.internalFormat = GL_RGBA16F;
                    break;
                case gpu::UINT8:
                    texel.format = GL_RGBA_INTEGER;
                    texel.internalFormat = GL_RGBA8UI;
                    break;
                case gpu::INT8:
                    texel.format = GL_RGBA_INTEGER;
                    texel.internalFormat = GL_RGBA8I;
                    break;
                case gpu::NUINT8:
                    texel.format = GL_RGBA;
                    texel.internalFormat = GL_RGBA8;
                    break;
                case gpu::NINT8:
                    texel.format = GL_RGBA;
                    texel.internalFormat = GL_RGBA8_SNORM;
                    break;
                case gpu::NUINT32:
                case gpu::NINT32:
                case gpu::COMPRESSED:
                case gpu::NUM_TYPES: // quiet compiler
                    Q_UNREACHABLE();
                }
                break;
            case gpu::SRGB:
                texel.internalFormat = GL_SRGB8;
                break;
            case gpu::SRGBA:
                texel.internalFormat = GL_SRGB8_ALPHA8; // standard 2.2 gamma correction color
                break;
            default:
                qCWarning(gpugllogging) << "Unknown combination of texel format";
            }
            break;
        }

        case gpu::TILE4x4: {
            texel.format = GL_RGBA;
            texel.type = ELEMENT_TYPE_TO_GL[srcFormat.getType()];

            switch (srcFormat.getSemantic()) {
            case gpu::BGRA:
            case gpu::SBGRA:
                texel.format = GL_BGRA;
                break;
            case gpu::RGB:
            case gpu::RGBA:
            case gpu::SRGB:
            case gpu::SRGBA:
            default:
                break;
            };

            switch (dstFormat.getSemantic()) {
            case gpu::COMPRESSED_BC4_RED:
                texel.internalFormat = GL_COMPRESSED_RED_RGTC1;
                break;
            case gpu::COMPRESSED_BC1_SRGB:
                texel.internalFormat = GL_COMPRESSED_SRGB_S3TC_DXT1_EXT;
                break;
            case gpu::COMPRESSED_BC1_SRGBA:
                texel.internalFormat = GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT;
                break;
            case gpu::COMPRESSED_BC3_SRGBA:
                texel.internalFormat = GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT;
                break;
            case gpu::COMPRESSED_BC5_XY:
                texel.internalFormat = GL_COMPRESSED_RG_RGTC2;
                break;
            case gpu::COMPRESSED_BC7_SRGBA:
                texel.internalFormat = GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM;
                break;

            default:
                qCWarning(gpugllogging) << "Unknown combination of texel format";
            }
            break;
        }

        default:
            qCWarning(gpugllogging) << "Unknown combination of texel format";
        }
        return texel;
    }
}
