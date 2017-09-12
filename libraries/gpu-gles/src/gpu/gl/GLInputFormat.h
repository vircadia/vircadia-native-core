//
//  Created by Sam Gateau on 2016/07/21
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_gpu_gl_GLInputFormat_h
#define hifi_gpu_gl_GLInputFormat_h

#include "GLShared.h"

namespace gpu {
namespace gl {

class GLInputFormat : public GPUObject {
    public:
        static GLInputFormat* sync(const Stream::Format& inputFormat);

    GLInputFormat();
    ~GLInputFormat();

    std::string key;
};

}
}

#endif
