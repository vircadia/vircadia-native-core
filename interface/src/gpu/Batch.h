//
//  Batch.h
//  interface/src/gpu
//
//  Created by Sam Gateau on 10/14/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_gpu_Batch_h
#define hifi_gpu_Batch_h

#include <assert.h>
#include "InterfaceConfig.h"

#include <vector>

namespace gpu {

class Buffer;
class Resource;
typedef int  Stamp;
typedef unsigned int uint32;
typedef int int32;

// TODO: move the backend namespace into dedicated files, for now we keep it close to the gpu objects definition for convenience
namespace backend {

};

enum Primitive {
    PRIMITIVE_POINTS = 0,
    PRIMITIVE_LINES,
    PRIMITIVE_LINE_STRIP,
    PRIMITIVE_TRIANGLES,
    PRIMITIVE_TRIANGLE_STRIP,
    PRIMITIVE_QUADS,
};

class Batch {
public:

    Batch();
    Batch(const Batch& batch);
    ~Batch();

    void clear();

    void draw( Primitive primitiveType, int nbVertices, int startVertex = 0);
    void drawIndexed( Primitive primitiveType, int nbIndices, int startIndex = 0 );
    void drawInstanced( uint32 nbInstances, Primitive primitiveType, int nbVertices, int startVertex = 0, int startInstance = 0);
    void drawIndexedInstanced( uint32 nbInstances, Primitive primitiveType, int nbIndices, int startIndex = 0, int startInstance = 0);

protected:

    enum Command {
        COMMAND_DRAW = 0,
        COMMAND_DRAW_INDEXED,
        COMMAND_DRAW_INSTANCED,
        COMMAND_DRAW_INDEXED_INSTANCED,
        
        COMMAND_SET_PIPE_STATE,
        COMMAND_SET_VIEWPORT,
        COMMAND_SET_FRAMEBUFFER,
        COMMAND_SET_RESOURCE,
        COMMAND_SET_VERTEX_STREAM,
        COMMAND_SET_INDEX_STREAM,

        COMMAND_GL_SET_UNIFORM,

    };
    typedef std::vector<Command> Commands;

    class Param {
    public:
        union {
            uint32 _uint;
            char _chars[4];
        };
        Param( uint32 val ): _uint(val) {}
    };
    typedef std::vector<Param> Params;

    class ResourceCache {
    public:
        Resource* _resource;
    };
    typedef std::vector<ResourceCache> Resources;

    Commands _commands;
    Params _params;
    Resources _resources;
};

};

#endif
