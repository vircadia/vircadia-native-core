//
//  VoxelShader.cpp
//  interface
//
//  Created by Brad Hefta-Gaub on 9/22/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.

// include this before QOpenGLFramebufferObject, which includes an earlier version of OpenGL
#include "InterfaceConfig.h"

#include <QOpenGLFramebufferObject>

#include "Application.h"
#include "VoxelShader.h"
#include "ProgramObject.h"
#include "RenderUtil.h"

VoxelShader::VoxelShader()
    : _initialized(false)
{
    _program = NULL;
}

VoxelShader::~VoxelShader() {
    if (_initialized) {
        delete _program;
    }
}

ProgramObject* VoxelShader::createGeometryShaderProgram(const QString& name) {
    ProgramObject* program = new ProgramObject();
    program->addShaderFromSourceFile(QGLShader::Vertex, Application::resourcesPath() + "shaders/passthrough.vert" );
    program->addShaderFromSourceFile(QGLShader::Geometry, Application::resourcesPath() + "shaders/" + name + ".geom" );
    program->setGeometryInputType(GL_POINTS);
    program->setGeometryOutputType(GL_TRIANGLE_STRIP);
    const int VERTICES_PER_FACE = 4;
    const int FACES_PER_VOXEL = 6;
    const int VERTICES_PER_VOXEL = VERTICES_PER_FACE * FACES_PER_VOXEL;
    program->setGeometryOutputVertexCount(VERTICES_PER_VOXEL);
    program->link();
    return program;
}

void VoxelShader::init() {
    if (_initialized) {
        qDebug("[ERROR] TestProgram is already initialized.");
        return;
    }
    
    _program = createGeometryShaderProgram("voxel");
    _initialized = true;
}

void VoxelShader::begin() {
    _program->bind();
}

void VoxelShader::end() {
    _program->release();
}

int VoxelShader::attributeLocation(const char * name) const {
    if (_program) {
        return _program->attributeLocation(name);
    } else {
        return -1;
    }
}

