//
//  TestGeometry.cpp
//  interface
//
//  Created by Brad Hefta-Gaub on 9/22/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.

// include this before QOpenGLFramebufferObject, which includes an earlier version of OpenGL
#include "InterfaceConfig.h"

#include <QOpenGLFramebufferObject>

#include "Application.h"
#include "TestGeometry.h"
#include "ProgramObject.h"
#include "RenderUtil.h"

TestGeometry::TestGeometry()
    : _initialized(false)
{
}

TestGeometry::~TestGeometry() {
    if (_initialized) {
        delete _testProgram;
    }
}

static ProgramObject* createGeometryShaderProgram(const QString& name) {
    ProgramObject* program = new ProgramObject();
    program->addShaderFromSourceFile(QGLShader::Vertex, "resources/shaders/passthrough.vert" );


    program->addShaderFromSourceFile(QGLShader::Geometry, "resources/shaders/" + name + ".geom" );

    program->setGeometryInputType(GL_LINES);
    program->setGeometryOutputType(GL_LINE_STRIP);
    program->setGeometryOutputVertexCount(100); // hack?

    program->link();
    //program->log();
    
    return program;
}

void TestGeometry::init() {
    if (_initialized) {
        qDebug("[ERROR] TestProgram is already initialized.\n");
        return;
    }

    switchToResourcesParentIfRequired();
    
    _testProgram = createGeometryShaderProgram("passthrough");
    _initialized = true;
}

void TestGeometry::begin() {
    _testProgram->bind();
}

void TestGeometry::end() {
    _testProgram->release();
}

