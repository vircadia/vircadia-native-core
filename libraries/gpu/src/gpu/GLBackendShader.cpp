//
//  GLBackendShader.cpp
//  libraries/gpu/src/gpu
//
//  Created by Sam Gateau on 2/28/2015.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "GLBackendShared.h"


GLBackend::GLShader::GLShader() :
    _shader(0),
    _program(0)
{}

GLBackend::GLShader::~GLShader() {
    if (_shader != 0) {
        glDeleteShader(_shader);
    }
    if (_program != 0) {
        glDeleteProgram(_program);
    }
}

bool compileShader(const Shader& shader, GLBackend::GLShader& object) {
    // Any GLSLprogram ? normally yes...
    const std::string& shaderSource = shader.getSource().getCode();
    if (shaderSource.empty()) {
        qDebug() << "GLShader::compileShader - no GLSL shader source code ? so failed to create";
        return false;
    }

    // Shader domain
    const GLenum SHADER_DOMAINS[2] = { GL_VERTEX_SHADER, GL_FRAGMENT_SHADER };
    GLenum shaderDomain = SHADER_DOMAINS[shader.getType()];

    // Create the shader object
    GLuint glshader = glCreateShader(shaderDomain);
    if (!glshader) {
        qDebug() << "GLShader::compileShader - failed to create the gl shader & gl program object";
        return false;
    }

    // Assign the source
    const GLchar* srcstr = shaderSource.c_str();
    glShaderSource(glshader, 1, &srcstr, NULL);

    // Compile !
    glCompileShader(glshader);

    // check if shader compiled
    GLint compiled = 0;
    glGetShaderiv(glshader, GL_COMPILE_STATUS, &compiled);

    // if compilation fails
    if (!compiled)
    {
        // save the source code to a temp file so we can debug easily
       /* std::ofstream filestream;
        filestream.open( "debugshader.glsl" );
        if ( filestream.is_open() )
        {
            filestream << shaderSource->source;
            filestream.close();
        }
        */

        GLint infoLength = 0;
        glGetShaderiv(glshader, GL_INFO_LOG_LENGTH, &infoLength);

        char* temp = new char[infoLength] ;
        glGetShaderInfoLog( glshader, infoLength, NULL, temp);

        qDebug() << "GLShader::compileShader - failed to compile the gl shader object:";
        qDebug() << temp;

        /*
        filestream.open( "debugshader.glsl.info.txt" );
        if ( filestream.is_open() )
        {
            filestream << String( temp );
            filestream.close();
        }
        */
        delete[] temp;

        glDeleteShader( glshader);
        return false;
    }

    // so far so good, program is almost done, need to link:
    GLuint glprogram = glCreateProgram();
    if (!glprogram) {
        qDebug() << "GLShader::compileShader - failed to create the gl shader & gl program object";
        return false;
    }

    glProgramParameteri(glprogram, GL_PROGRAM_SEPARABLE, GL_TRUE);
    glAttachShader(glprogram, glshader);
    glLinkProgram(glprogram);

    GLint linked = 0;
    glGetProgramiv(glprogram, GL_LINK_STATUS, &linked);

    if (!linked)
    {
        /*
        // save the source code to a temp file so we can debug easily
        std::ofstream filestream;
        filestream.open( "debugshader.glsl" );
        if ( filestream.is_open() )
        {
            filestream << shaderSource->source;
            filestream.close();
        }
        */

        GLint infoLength = 0;
        glGetProgramiv( glprogram, GL_INFO_LOG_LENGTH, &infoLength );

        char* temp = new char[infoLength] ;
        glGetProgramInfoLog( glprogram, infoLength, NULL, temp);

        qDebug() << "GLShader::compileShader -  failed to LINK the gl program object :";
        qDebug() << temp;

        /*
        filestream.open( "debugshader.glsl.info.txt" );
        if ( filestream.is_open() )
        {
            filestream << String( temp );
            filestream.close();
        }
        */
        delete[] temp;

        glDeleteShader( glshader);
        glDeleteProgram( glprogram);
        return false;
    }

    // So far so good, the shader is created successfully
    object._shader = glshader;
    object._program = glprogram;

    return true;
}

bool compileProgram(const Shader& program, GLBackend::GLShader& object) {
    if(!program.isProgram()) {
        return false;
    }

    // Let's go through every shaders and make sure they are ready to go
    std::vector< GLuint > shaderObjects;
    for (auto subShader : program.getShaders()) {
        GLuint so = GLBackend::getShaderID(subShader);
        if (!so) {
            qDebug() << "GLShader::compileProgram - One of the shaders of the program is not compiled?";
            return false;
        }
        shaderObjects.push_back(so);
    }

    // so far so good, program is almost done, need to link:
    GLuint glprogram = glCreateProgram();
    if (!glprogram) {
        qDebug() << "GLShader::compileProgram - failed to create the gl program object";
        return false;
    }

    // glProgramParameteri(glprogram, GL_PROGRAM_, GL_TRUE);
    // Create the program from the sub shaders
    for (auto so : shaderObjects) {
        glAttachShader(glprogram, so);
    }

    // Link!
    glLinkProgram(glprogram);

    GLint linked = 0;
    glGetProgramiv(glprogram, GL_LINK_STATUS, &linked);

    if (!linked)
    {
        /*
        // save the source code to a temp file so we can debug easily
        std::ofstream filestream;
        filestream.open( "debugshader.glsl" );
        if ( filestream.is_open() )
        {
            filestream << shaderSource->source;
            filestream.close();
        }
        */

        GLint infoLength = 0;
        glGetProgramiv( glprogram, GL_INFO_LOG_LENGTH, &infoLength );

        char* temp = new char[infoLength] ;
        glGetProgramInfoLog( glprogram, infoLength, NULL, temp);

        qDebug() << "GLShader::compileProgram -  failed to LINK the gl program object :";
        qDebug() << temp;

        /*
        filestream.open( "debugshader.glsl.info.txt" );
        if ( filestream.is_open() )
        {
            filestream << String( temp );
            filestream.close();
        }
        */
        delete[] temp;

        glDeleteProgram( glprogram);
        return false;
    }

    // So far so good, the program is created successfully
    object._shader = 0;
    object._program = glprogram;

    return true;
}

GLBackend::GLShader* GLBackend::syncGPUObject(const Shader& shader) {
    GLShader* object = Backend::getGPUObject<GLBackend::GLShader>(shader);

    // If GPU object already created then good
    if (object) {
        return object;
    }

    // need to have a gpu object?

    // GO through the process of allocating the correct storage and/or update the content
    if (shader.isProgram()) {
         GLShader tempObject;
        if (compileProgram(shader, tempObject)) {
            object = new GLShader(tempObject);
            Backend::setGPUObject(shader, object);
        }
    } else if (shader.isDomain()) {
        GLShader tempObject;
        if (compileShader(shader, tempObject)) {
            object = new GLShader(tempObject);
            Backend::setGPUObject(shader, object);
        }
    }

    return object;
}



GLuint GLBackend::getShaderID(const ShaderPointer& shader) {
    if (!shader) {
        return 0;
    }
    GLShader* object = GLBackend::syncGPUObject(*shader);
    if (object) {
        if (shader->isProgram()) {
            return object->_program;
        } else {
            return object->_shader;
        }
    } else {
        return 0;
    }
}

