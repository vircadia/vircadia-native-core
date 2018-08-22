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

    // Needs to match values in shaders/Shaders.h
    enum class BindingType
    {
        INVALID = -1,
        INPUT = 0,
        OUTPUT,
        TEXTURE,
        SAMPLER,
        UNIFORM_BUFFER,
        RESOURCE_BUFFER,
        UNIFORM,
    };

    using LocationMap = std::unordered_map<std::string, int32_t>;
    using ReflectionMap = std::map<BindingType, LocationMap>;

    class Source {
    public:
        enum Language
        {
            INVALID = -1,
            GLSL = 0,
            SPIRV = 1,
            MSL = 2,
            HLSL = 3,
        };

        Source() {}
        Source(const std::string& code, const ReflectionMap& reflection, Language lang = GLSL) :
            _code(code), _reflection(reflection), _lang(lang) {}
        Source(const Source& source) : _code(source._code), _reflection(source._reflection), _lang(source._lang) {}
        virtual ~Source() {}

        virtual const std::string& getCode() const { return _code; }
        virtual const ReflectionMap& getReflection() const { return _reflection; }

        class Less {
        public:
            bool operator()(const Source& x, const Source& y) const {
                if (x._lang == y._lang) {
                    return x._code < y._code;
                } else {
                    return (x._lang < y._lang);
                }
            }
        };

    protected:
        std::string _code;
        ReflectionMap _reflection;
        Language _lang;
    };

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

    class Slot {
    public:
        std::string _name;
        int32 _location{ INVALID_LOCATION };
        Element _element;
        uint16 _resourceType{ Resource::BUFFER };
        uint32 _size{ 0 };

        Slot(const Slot& s) :
            _name(s._name), _location(s._location), _element(s._element), _resourceType(s._resourceType), _size(s._size) {}
        Slot(Slot&& s) :
            _name(s._name), _location(s._location), _element(s._element), _resourceType(s._resourceType), _size(s._size) {}
        Slot(const std::string& name,
             int32 location,
             const Element& element,
             uint16 resourceType = Resource::BUFFER,
             uint32 size = 0) :
            _name(name),
            _location(location), _element(element), _resourceType(resourceType), _size(size) {}
        Slot(const std::string& name) : _name(name) {}

        Slot& operator=(const Slot& s) {
            _name = s._name;
            _location = s._location;
            _element = s._element;
            _resourceType = s._resourceType;
            _size = s._size;
            return (*this);
        }
    };

    class SlotSet : protected std::set<Slot, Less<Slot>> {
        using Parent = std::set<Slot, Less<Slot>>;

    public:
        void insert(const Parent::value_type& value) {
            Parent::insert(value);
            if (value._location != INVALID_LOCATION) {
                _validSlots.insert(value._location);
            }
        }

        using Parent::begin;
        using Parent::empty;
        using Parent::end;
        using Parent::size;

        using LocationMap = std::unordered_map<std::string, int32>;
        using NameVector = std::vector<std::string>;

        NameVector getNames() const { 
            NameVector result; 
            for (const auto& entry : *this) {
                result.push_back(entry._name);
            }
            return result;
        }

        LocationMap getLocationsByName() const {
            LocationMap result;
            for (const auto& entry : *this) {
                result.insert({ entry._name, entry._location });
            }
            return result;
        }

        bool isValid(int32 slot) const { return 0 != _validSlots.count(slot); }

    protected:
        std::unordered_set<int> _validSlots;
    };

    static Source getShaderSource(uint32_t id);
    static Source getVertexShaderSource(uint32_t id) { return getShaderSource(id); }
    static Source getFragmentShaderSource(uint32_t id) { return getShaderSource(id); }

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

    ID getID() const { return _ID; }

    Type getType() const { return _type; }
    bool isProgram() const { return getType() > NUM_DOMAINS; }
    bool isDomain() const { return getType() < NUM_DOMAINS; }

    const Source& getSource() const { return _source; }

    const Shaders& getShaders() const { return _shaders; }

    // Access the exposed uniform, input and output slot
    const SlotSet& getUniforms() const { return _uniforms; }
    const SlotSet& getUniformBuffers() const { return _uniformBuffers; }
    const SlotSet& getResourceBuffers() const { return _resourceBuffers; }
    const SlotSet& getTextures() const { return _textures; }
    const SlotSet& getSamplers() const { return _samplers; }

    // Compilation Handler can be passed while compiling a shader (in the makeProgram call) to be able to give the hand to
    // the caller thread if the comilation fails and to prvide a different version of the source for it
    // @param0 the Shader object that just failed to compile
    // @param1 the original source code as submited to the compiler
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
    Shader(Type type, const Source& source);
    Shader(Type type, const Pointer& vertex, const Pointer& geometry, const Pointer& pixel);

    Shader(const Shader& shader);             // deep copy of the sysmem shader
    Shader& operator=(const Shader& shader);  // deep copy of the sysmem texture

    // Source contains the actual source code or nothing if the shader is a program
    Source _source;

    // if shader is composed of sub shaders, here they are
    Shaders _shaders;

    // List of exposed uniform, input and output slots
    SlotSet _uniforms;
    SlotSet _uniformBuffers;
    SlotSet _resourceBuffers;
    SlotSet _textures;
    SlotSet _samplers;
    SlotSet _inputs;
    SlotSet _outputs;

    // The type of the shader, the master key
    Type _type;

    // The unique identifier of a shader in the GPU lib
    uint32_t _ID{ 0 };

    // Number of attempts to compile the shader
    mutable uint32_t _numCompilationAttempts{ 0 };
    // Compilation logs (one for each versions generated)
    mutable CompilationLogs _compilationLogs;

    // Whether or not the shader compilation failed
    bool _compilationHasFailed{ false };

    // Global maps of the shaders
    // Unique shader ID
    static std::atomic<ID> _nextShaderID;

    using ShaderMap = std::map<Source, std::weak_ptr<Shader>, Source::Less>;
    using DomainShaderMaps = std::array<ShaderMap, NUM_DOMAINS>;
    static DomainShaderMaps _domainShaderMaps;

    static ShaderPointer createOrReuseDomainShader(Type type, const Source& source);

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
};

typedef Shader::Pointer ShaderPointer;
typedef std::vector<ShaderPointer> Shaders;

};  // namespace gpu

#endif
