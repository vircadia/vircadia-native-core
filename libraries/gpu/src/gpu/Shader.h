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
#include <string>
#include <memory>
#include <set>
#include <unordered_set>
#include <unordered_map>
#include <map>
#include <functional>
#include <shaders/Shaders.h>
#include <QUrl>

namespace gpu {

class Shader {
public:
    // unique identifier of a shader
    using ID = uint32_t;

    enum Type
    {
        VERTEX = 0,
        PIXEL,
        FRAGMENT = PIXEL,
        GEOMETRY,
        NUM_DOMAINS,

        PROGRAM,
    };

    typedef std::shared_ptr<Shader> Pointer;
    typedef std::vector<Pointer> Shaders;

    using Source = shader::Source;
    using Reflection = shader::Reflection;
    using Dialect = shader::Dialect;
    using Variant = shader::Variant;

    struct CompilationLog {
        std::string message;
        bool compiled{ false };

        CompilationLog() {}
        CompilationLog(const CompilationLog& src) : message(src.message), compiled(src.compiled) {}
    };
    using CompilationLogs = std::vector<CompilationLog>;

    static const int32 INVALID_LOCATION = -1;

    template <typename T>
    class Less {
    public:
        bool operator()(const T& x, const T& y) const { return x._name < y._name; }
    };

    static const Source& getShaderSource(uint32_t id);
    static const Source& getVertexShaderSource(uint32_t id) { return getShaderSource(id); }
    static const Source& getFragmentShaderSource(uint32_t id) { return getShaderSource(id); }
    static Pointer createVertex(const Source& source);
    static Pointer createPixel(const Source& source);
    static Pointer createGeometry(const Source& source);

    static Pointer createVertex(uint32_t shaderId);
    static Pointer createPixel(uint32_t shaderId);
    static Pointer createGeometry(uint32_t shaderId);

    static Pointer createProgram(uint32_t programId);
    static Pointer createProgram(const Pointer& vertexShader, const Pointer& pixelShader);
    static Pointer createProgram(const Pointer& vertexShader, const Pointer& geometryShader, const Pointer& pixelShader);

    ~Shader();

    ID getID() const;
    Type getType() const { return _type; }
    bool isProgram() const { return getType() > NUM_DOMAINS; }
    bool isDomain() const { return getType() < NUM_DOMAINS; }

    const Source& getSource() const { return _source; }

    const Shaders& getShaders() const { return _shaders; }

    Reflection getReflection(shader::Dialect dialect, shader::Variant variant) const;
    Reflection getReflection() const; // get the default version of the reflection

    // Compilation Handler can be passed while compiling a shader (in the makeProgram call) to be able to give the hand to
    // the caller thread if the compilation fails and to provide a different version of the source for it
    // @param0 the Shader object that just failed to compile
    // @param1 the original source code as submitted to the compiler
    // @param2 the compilation log containing the error message
    // @param3 a new string ready to be filled with the new version of the source that could be proposed from the handler functor
    // @return boolean true if the backend should keep trying to compile the shader with the new source returned or false to stop and fail that shader compilation
    using CompilationHandler = std::function<bool(const Shader&, const std::string&, CompilationLog&, std::string&)>;

    // Check the compilation state
    bool compilationHasFailed() const { return _compilationHasFailed; }
    const CompilationLogs& getCompilationLogs() const { return _compilationLogs; }
    uint32_t getNumCompilationAttempts() const { return _numCompilationAttempts; }

    // Set COmpilation logs can only be called by the Backend layers
    void setCompilationHasFailed(bool compilationHasFailed) { _compilationHasFailed = compilationHasFailed; }
    void setCompilationLogs(const CompilationLogs& logs) const;
    void incrementCompilationAttempt() const;

    const GPUObjectPointer gpuObject{};

protected:
    Shader(Type type, const Source& source, bool dynamic);
    Shader(Type type, const Pointer& vertex, const Pointer& geometry, const Pointer& pixel);

    // Source contains the actual source code or nothing if the shader is a program
    const Source _source;

    // if shader is composed of sub shaders, here they are
    const Shaders _shaders;

     
    // The type of the shader, the master key
    const Type _type;

    // Number of attempts to compile the shader
    mutable uint32_t _numCompilationAttempts{ 0 };
    // Compilation logs (one for each versions generated)
    mutable CompilationLogs _compilationLogs;

    // Whether or not the shader compilation failed
    bool _compilationHasFailed{ false };

    // Global maps of the shaders
    // Unique shader ID
    //static std::atomic<ID> _nextShaderID;

    static ShaderPointer createOrReuseDomainShader(Type type, uint32_t sourceId);

    using ProgramMapKey = glm::uvec3;  // The IDs of the shaders in a program make its key
    class ProgramKeyLess {
    public:
        bool operator()(const ProgramMapKey& l, const ProgramMapKey& r) const {
            if (l.x == r.x) {
                if (l.y == r.y) {
                    return (l.z < r.z);
                } else {
                    return (l.y < r.y);
                }
            } else {
                return (l.x < r.x);
            }
        }
    };
    using ProgramMap = std::map<ProgramMapKey, std::weak_ptr<Shader>, ProgramKeyLess>;
    static ProgramMap _programMap;

    static ShaderPointer createOrReuseProgramShader(Type type,
                                                    const Pointer& vertexShader,
                                                    const Pointer& geometryShader,
                                                    const Pointer& pixelShader);
    friend class Serializer;
    friend class Deserializer;
};

typedef Shader::Pointer ShaderPointer;
typedef std::vector<ShaderPointer> Shaders;

};  // namespace gpu

#endif
