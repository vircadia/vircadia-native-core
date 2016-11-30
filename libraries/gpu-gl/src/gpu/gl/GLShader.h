//
//  Created by Bradley Austin Davis on 2016/05/15
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_gpu_gl_GLShader_h
#define hifi_gpu_gl_GLShader_h

#include "GLShared.h"

namespace gpu { namespace gl {

class GLShader : public GPUObject {
public:
    static GLShader* sync(GLBackend& backend, const Shader& shader);
    static bool makeProgram(GLBackend& backend, Shader& shader, const Shader::BindingSet& slotBindings);

    enum Version {
        Mono = 0,
        Stereo,

        NumVersions
    };

    using ShaderObject = gpu::gl::ShaderObject;
    using ShaderObjects = std::array< ShaderObject, NumVersions >;

    using UniformMapping = std::map<GLint, GLint>;
    using UniformMappingVersions = std::vector<UniformMapping>;

    GLShader(const std::weak_ptr<GLBackend>& backend);
    ~GLShader();

    ShaderObjects _shaderObjects;
    UniformMappingVersions _uniformMappings;

    GLuint getProgram(Version version = Mono) const {
        return _shaderObjects[version].glprogram;
    }

    GLint getUniformLocation(GLint srcLoc, Version version = Mono) {
        // THIS will be used in the future PR as we grow the number of versions
        return _uniformMappings[version][srcLoc];
       // return srcLoc;
    }

    const std::weak_ptr<GLBackend> _backend;
};

} }


#endif
