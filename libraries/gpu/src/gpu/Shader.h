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

#include <QUrl>
 
namespace gpu {

class Shader {
public:

    typedef std::shared_ptr< Shader > Pointer;
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

    static const int32 INVALID_LOCATION = -1;

    class Slot {
    public:

        std::string _name;
        int32 _location{INVALID_LOCATION};
        Element _element;
        uint16 _resourceType{Resource::BUFFER};
        uint32 _size { 0 };
 
        Slot(const Slot& s) : _name(s._name), _location(s._location), _element(s._element), _resourceType(s._resourceType), _size(s._size) {}
        Slot(Slot&& s) : _name(s._name), _location(s._location), _element(s._element), _resourceType(s._resourceType), _size(s._size) {}
        Slot(const std::string& name, int32 location, const Element& element, uint16 resourceType = Resource::BUFFER, uint32 size = 0) :
        _name(name), _location(location), _element(element), _resourceType(resourceType), _size(size) {}
        Slot(const std::string& name) : _name(name) {}

        Slot& operator= (const Slot& s) {
            _name = s._name;
            _location = s._location;
            _element = s._element;
            _resourceType = s._resourceType;
            _size = s._size;
            return (*this);
        }
    };

    class Binding {
    public:
        std::string _name;
        int32 _location;
        Binding(const std::string& name, int32 loc = INVALID_LOCATION) : _name(name), _location(loc) {}
    };

    template <typename T> class Less {
    public:
        bool operator() (const T& x, const T& y) const { return x._name < y._name; }
    };

    class SlotSet : public std::set<Slot, Less<Slot>> {
    public:
        Slot findSlot(const std::string& name) const {
            auto key = Slot(name);
            auto found = static_cast<const std::set<Slot, Less<Slot>>*>(this)->find(key);
            if (found != end()) {
                return (*found);
            }
            return key;
        }
        int32 findLocation(const std::string& name) const {
            return findSlot(name)._location;
        }
    protected:
    };
    
    typedef std::set<Binding, Less<Binding>> BindingSet;


    enum Type {
        VERTEX = 0,
        PIXEL,
        GEOMETRY,
        NUM_DOMAINS,

        PROGRAM,
    };

    static Pointer createVertex(const Source& source);
    static Pointer createPixel(const Source& source);
    static Pointer createGeometry(const Source& source);

    static Pointer createProgram(const Pointer& vertexShader, const Pointer& pixelShader);
    static Pointer createProgram(const Pointer& vertexShader, const Pointer& geometryShader, const Pointer& pixelShader);


    ~Shader();

    Type getType() const { return _type; }
    bool isProgram() const { return getType() > NUM_DOMAINS; }
    bool isDomain() const { return getType() < NUM_DOMAINS; }

    void setCompilationHasFailed(bool compilationHasFailed) { _compilationHasFailed = compilationHasFailed; }
    bool compilationHasFailed() const { return _compilationHasFailed; }

    const Source& getSource() const { return _source; }

    const Shaders& getShaders() const { return _shaders; }

    // Access the exposed uniform, input and output slot
    const SlotSet& getUniforms() const { return _uniforms; }
    const SlotSet& getBuffers() const { return _buffers; }
    const SlotSet& getTextures() const { return _textures; }
    const SlotSet& getSamplers() const { return _samplers; }

    const SlotSet& getInputs() const { return _inputs; }
    const SlotSet& getOutputs() const { return _outputs; }

    // Define the list of uniforms, inputs and outputs for the shader
    // This call is intendend to build the list of exposed slots in order
    // to correctly bind resource to the shader.
    // These can be build "manually" from knowledge of the atual shader code
    // or automatically by calling "makeShader()", this is the preferred way
    void defineSlots(const SlotSet& uniforms, const SlotSet& buffers, const SlotSet& textures, const SlotSet& samplers, const SlotSet& inputs, const SlotSet& outputs);

    // makeProgram(...) make a program shader ready to be used in a Batch.
    // It compiles the sub shaders, link them and defines the Slots and their bindings.
    // If the shader passed is not a program, nothing happens. 
    //
    // It is possible to provide a set of slot bindings (from the name of the slot to a unit number) allowing
    // to make sure slots with the same semantics can be always bound on the same location from shader to shader.
    // For example, the "diffuseMap" can always be bound to texture unit #1 for different shaders by specifying a Binding("diffuseMap", 1)
    //
    // As of now (03/2015), the call to makeProgram is in fact calling gpu::Context::makeProgram and does rely
    // on the underneath gpu::Context::Backend available. Since we only support glsl, this means that it relies
    // on a gl Context and the driver to compile the glsl shader. 
    // Hoppefully in a few years the shader compilation will be completely abstracted in a separate shader compiler library
    // independant of the graphics api in use underneath (looking at you opengl & vulkan).
    static bool makeProgram(Shader& shader, const Shader::BindingSet& bindings = Shader::BindingSet());

    const GPUObjectPointer gpuObject {};
    
protected:
    Shader(Type type, const Source& source);
    Shader(Type type, const Pointer& vertex, const Pointer& pixel);
    Shader(Type type, const Pointer& vertex, const Pointer& geometry, const Pointer& pixel);

    Shader(const Shader& shader); // deep copy of the sysmem shader
    Shader& operator=(const Shader& shader); // deep copy of the sysmem texture

    // Source contains the actual source code or nothing if the shader is a program
    Source _source;

    // if shader is composed of sub shaders, here they are
    Shaders _shaders;

    // List of exposed uniform, input and output slots
    SlotSet _uniforms;
    SlotSet _buffers;
    SlotSet _textures;
    SlotSet _samplers;
    SlotSet _inputs;
    SlotSet _outputs;

    // The type of the shader, the master key
    Type _type;

    // Whether or not the shader compilation failed
    bool _compilationHasFailed { false };
};

typedef Shader::Pointer ShaderPointer;
typedef std::vector< ShaderPointer > Shaders;

};


#endif
