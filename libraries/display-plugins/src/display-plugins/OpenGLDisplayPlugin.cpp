//
//  Created by Bradley Austin Davis on 2015/05/29
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "OpenGLDisplayPlugin.h"

#include <QOpenGLContext>
#include <QCoreApplication>

#include <GLWindow.h>
#include <GLMHelpers.h>


OpenGLDisplayPlugin::OpenGLDisplayPlugin() {
    connect(&_timer, &QTimer::timeout, this, [&] {
        emit requestRender();
    });
}

OpenGLDisplayPlugin::~OpenGLDisplayPlugin() {
}

void OpenGLDisplayPlugin::preDisplay() {
    makeCurrent();
};

void OpenGLDisplayPlugin::preRender() {
    // NOOP
}

void OpenGLDisplayPlugin::finishFrame() {
    swapBuffers();
    doneCurrent();
};

static float PLANE_VERTICES[] = {
    -1, -1, 0, 0,
    -1, +1, 0, 1,
    +1, -1, 1, 0,
    +1, +1, 1, 1,
};

void OpenGLDisplayPlugin::customizeContext(PluginContainer * container) {
    using namespace oglplus;
    Context::BlendFunc(BlendFunction::SrcAlpha, BlendFunction::OneMinusSrcAlpha);
    Context::Disable(Capability::Blend);
    Context::Disable(Capability::DepthTest);
    Context::Disable(Capability::CullFace);
    glEnable(GL_TEXTURE_2D);
    
    _program = loadDefaultShader();
    auto attribs = _program->ActiveAttribs();
    for(size_t i = 0; i < attribs.Size(); ++i) {
        auto attrib = attribs.At(i);
        if (String("Position") == attrib.Name()) {
            _positionAttribute = attrib.Index();
        } else if (String("TexCoord") == attrib.Name()) {
            _texCoordAttribute = attrib.Index();
        }
        qDebug() << attrib.Name().c_str();
    }
    _vertexBuffer.reset(new oglplus::Buffer());
    _vertexBuffer->Bind(Buffer::Target::Array);
    _vertexBuffer->Data(Buffer::Target::Array, BufferData(PLANE_VERTICES));
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void OpenGLDisplayPlugin::activate(PluginContainer * container) {
    _timer.start(1);
}

void OpenGLDisplayPlugin::deactivate() {
    _timer.stop();

    makeCurrent();
    Q_ASSERT(0 == glGetError());
    _vertexBuffer.reset();
 //   glDeleteBuffers(1, &_vertexBuffer);
//    _vertexBuffer = 0;
    doneCurrent();
}

// Pressing Alt (and Meta) key alone activates the menubar because its style inherits the
// SHMenuBarAltKeyNavigation from QWindowsStyle. This makes it impossible for a scripts to
// receive keyPress events for the Alt (and Meta) key in a reliable manner.
//
// This filter catches events before QMenuBar can steal the keyboard focus.
// The idea was borrowed from
// http://www.archivum.info/qt-interest@trolltech.com/2006-09/00053/Re-(Qt4)-Alt-key-focus-QMenuBar-(solved).html

// Pass input events on to the application
bool OpenGLDisplayPlugin::eventFilter(QObject* receiver, QEvent* event) {
    switch (event->type()) {
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseButtonDblClick:
    case QEvent::MouseMove:
    case QEvent::Wheel:

    case QEvent::TouchBegin:
    case QEvent::TouchEnd:
    case QEvent::TouchUpdate:

    case QEvent::FocusIn:
    case QEvent::FocusOut:

    case QEvent::KeyPress:
    case QEvent::KeyRelease:
    case QEvent::ShortcutOverride:

    case QEvent::DragEnter:
    case QEvent::Drop:

    case QEvent::Resize:
        if (QCoreApplication::sendEvent(QCoreApplication::instance(), event)) {
            return true;
        }
        break;
    }
    return false;
}

void OpenGLDisplayPlugin::display(
    GLuint finalTexture, const glm::uvec2& sceneSize) {
    using namespace oglplus;
    uvec2 size = getRecommendedRenderSize();
    Context::Viewport(size.x, size.y);
    glBindTexture(GL_TEXTURE_2D, finalTexture);
    drawUnitQuad();
}

void OpenGLDisplayPlugin::drawUnitQuad() {
    using namespace oglplus;
    _program->Bind();
    _vertexBuffer->Bind(Buffer::Target::Array);
    glEnableVertexAttribArray(_positionAttribute);
    glVertexAttribPointer(_positionAttribute, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, 0);
    glEnableVertexAttribArray(_texCoordAttribute);
    glVertexAttribPointer(_texCoordAttribute, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void*)(sizeof(float) * 2));
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glDisableVertexAttribArray(_positionAttribute);
    glDisableVertexAttribArray(_texCoordAttribute);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glUseProgram(0);
}