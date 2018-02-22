//
//  Created by Bradley Austin Davis on 2016/05/15
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_gpu_gl_GLTexelFormat_h
#define hifi_gpu_gl_GLTexelFormat_h

#include "GLShared.h"

namespace gpu { namespace gl {

class GLTexelFormat {
public:
    GLenum internalFormat;
    GLenum format;
    GLenum type;

    GLTexelFormat(GLenum glinternalFormat, GLenum glformat, GLenum gltype) : internalFormat(glinternalFormat), format(glformat), type(gltype) {}
    GLTexelFormat(GLenum glinternalFormat) : internalFormat(glinternalFormat) {}

    bool isCompressed() const;

    static GLTexelFormat evalGLTexelFormat(const Element& dstFormat) {
        return evalGLTexelFormat(dstFormat, dstFormat);
    }
    static GLenum evalGLTexelFormatInternal(const Element& dstFormat);

    static GLTexelFormat evalGLTexelFormat(const Element& dstFormat, const Element& srcFormat);
};

} }


#endif
