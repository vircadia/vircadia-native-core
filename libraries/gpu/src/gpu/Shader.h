//
//  Shader.h
//  libraries/gpu/src/gpu
//
//  Created by Sam Gateau on 2/27/2015.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_gpu_Shader_h
#define hifi_gpu_Shader_h

#include "Resource.h"
#include <memory>
 
namespace gpu {

class Shader {
public:

    typedef QSharedPointer< Shader > Pointer;
    typedef std::vector< Pointer > Shaders;

    class Source {
    public:
        enum Language {
            GLSL = 0,
        };

        Source() {}
        Source(const std::string& code, Language lang = GLSL) : _code(code), _lang(lang) {}
        Source(const Source& source) : _code(source._code), _lang(source._lang) {}
        virtual ~Source() {}
 
        virtual const std::string& getCode() const { return _code; }

    protected:
        std::string _code;
        Language _lang = GLSL;
    };

    enum Type {
        VERTEX = 0,
        PIXEL,
        GEOMETRY,
        NUM_DOMAINS,

        PROGRAM,
    };

    static Shader* createVertex(const Source& source);
    static Shader* createPixel(const Source& source);

    static Shader* createProgram(Pointer& vertexShader, Pointer& pixelShader);


    ~Shader();

    Type getType() const { return _type; }
    bool isProgram() const { return getType() > NUM_DOMAINS; }
    bool isDomain() const { return getType() < NUM_DOMAINS; }

    const Source& getSource() const { return _source; }

    const Shaders& getShaders() const { return _shaders; }

protected:
    Shader(Type type, const Source& source);
    Shader(Type type, Pointer& vertex, Pointer& pixel);

    Shader(const Shader& shader); // deep copy of the sysmem shader
    Shader& operator=(const Shader& shader); // deep copy of the sysmem texture

    // Source contains the actual source code or nothing if the shader is a program
    Source _source;

    // if shader is composed of sub shaders, here they are
    Shaders _shaders;

    // The type of the shader, the master key
    Type _type;

    // This shouldn't be used by anything else than the Backend class with the proper casting.
    mutable GPUObject* _gpuObject = NULL;
    void setGPUObject(GPUObject* gpuObject) const { _gpuObject = gpuObject; }
    GPUObject* getGPUObject() const { return _gpuObject; }
    friend class Backend;
};

typedef Shader::Pointer ShaderPointer;
typedef std::vector< ShaderPointer > Shaders;

/*
class Program {
public:

    enum Type {
        GRAPHICS = 0,

        NUM_TYPES,
    };


    Program();
    Program(const Program& program); // deep copy of the sysmem shader
    Program& operator=(const Program& program); // deep copy of the sysmem texture
    ~Program();

protected:
    Shaders _shaders;
    Type _type;

    // This shouldn't be used by anything else than the Backend class with the proper casting.
    mutable GPUObject* _gpuObject = NULL;
    void setGPUObject(GPUObject* gpuObject) const { _gpuObject = gpuObject; }
    GPUObject* getGPUObject() const { return _gpuObject; }
    friend class Backend;
};

typedef QSharedPointer<Shader> ShaderPointer;
*/
};


#endif
