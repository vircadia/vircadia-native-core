//
//  Created by Bradley Austin Davis on 2016/05/15
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_gpu_gl_GLState_h
#define hifi_gpu_gl_GLState_h

#include "GLShared.h"

#include <gpu/State.h>

namespace gpu { namespace gl {
    
class GLBackend;
class GLState : public GPUObject {
public:
    static GLState* sync(const State& state);

    class Command {
    public:
        virtual void run(GLBackend* backend) = 0;
        Command() {}
        virtual ~Command() {};
    };

    template <class T> class Command1 : public Command {
    public:
        typedef void (GLBackend::*GLFunction)(T);
        void run(GLBackend* backend) { (backend->*(_func))(_param); }
        Command1(GLFunction func, T param) : _func(func), _param(param) {};
        GLFunction _func;
        T _param;
    };
    template <class T, class U> class Command2 : public Command {
    public:
        typedef void (GLBackend::*GLFunction)(T, U);
        void run(GLBackend* backend) { (backend->*(_func))(_param0, _param1); }
        Command2(GLFunction func, T param0, U param1) : _func(func), _param0(param0), _param1(param1) {};
        GLFunction _func;
        T _param0;
        U _param1;
    };

    template <class T, class U, class V> class Command3 : public Command {
    public:
        typedef void (GLBackend::*GLFunction)(T, U, V);
        void run(GLBackend* backend) { (backend->*(_func))(_param0, _param1, _param2); }
        Command3(GLFunction func, T param0, U param1, V param2) : _func(func), _param0(param0), _param1(param1), _param2(param2) {};
        GLFunction _func;
        T _param0;
        U _param1;
        V _param2;
    };

    typedef std::shared_ptr< Command > CommandPointer;
    typedef std::vector< CommandPointer > Commands;

    Commands _commands;
    Stamp _stamp;
    State::Signature _signature;

    // The state commands to reset to default,
    static const Commands _resetStateCommands;

    friend class GLBackend;
};

} }

#endif
