//
//  main.cpp
//  tests/physics/src
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "TextRenderer.h"
#include "MatrixStack.h"

#include <QWindow>
#include <QFile>
#include <QImage>
#include <QTimer>
#include <QOpenGLContext>
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>
#include <QOpenGLWindow>
#include <QResizeEvent>
#include <QOpenGLTexture>
#include <QOpenGLVertexArrayObject>
#include <QApplication>
#include <unordered_map>
#include <memory>
#include <glm/glm.hpp>
#include <PathUtils.h>

using TexturePtr = std::shared_ptr < QOpenGLTexture > ;
using VertexArrayPtr = std::shared_ptr < QOpenGLVertexArrayObject >;
using ProgramPtr = std::shared_ptr < QOpenGLShaderProgram >;
using BufferPtr = std::shared_ptr < QOpenGLBuffer >;


const char textured_vs[] = R"XXXX(#version 330

uniform mat4 Projection = mat4(1);
uniform mat4 ModelView = mat4(1);
uniform vec2 UvMultiplier = vec2(1);

layout(location = 0) in vec3 Position;
layout(location = 1) in vec2 TexCoord;

out vec2 vTexCoord;

void main() {
  gl_Position = Projection * ModelView * vec4(Position, 1);
  vTexCoord = TexCoord * UvMultiplier;
}
)XXXX";

const char textured_fs[] = R"XXXX(#version 330

uniform sampler2D sampler;
uniform float Alpha = 1.0;

in vec2 vTexCoord;
out vec4 vFragColor;

void main() {
  vec4 c = texture(sampler, vTexCoord);
  c.a = min(Alpha, c.a);
  vFragColor = c; 
  //vFragColor = vec4(fract(vTexCoord), log(vTexCoord.x), 1.0);
})XXXX";

class QTestWindow : public QWindow {
  Q_OBJECT
  QOpenGLContext * m_context;
  ProgramPtr shader;
  VertexArrayPtr vao;
  BufferPtr geometry;
  QSize _size;
  TextRenderer* _textRenderer[4];

public:
  QTestWindow();
  virtual ~QTestWindow();
  void makeCurrent();
  void draw();

protected:
  void resizeEvent(QResizeEvent * ev) override;
};


QTestWindow::QTestWindow() {
  setSurfaceType(QSurface::OpenGLSurface);

  QSurfaceFormat format;
  // Qt Quick may need a depth and stencil buffer. Always make sure these are available.
  format.setDepthBufferSize(16);
  format.setStencilBufferSize(8);
  format.setVersion(4, 3);
  format.setProfile(QSurfaceFormat::OpenGLContextProfile::CompatibilityProfile);
  setFormat(format);

  m_context = new QOpenGLContext;
  m_context->setFormat(format);
  m_context->create();

  show();
  makeCurrent();

#ifdef WIN32
  glewExperimental = true;
  GLenum err = glewInit();
  if (GLEW_OK != err) {
    /* Problem: glewInit failed, something is seriously wrong. */
    const GLubyte * errStr = glewGetErrorString(err);
    qDebug("Error: %s\n", errStr);
  }
  qDebug("Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));

  if (wglewGetExtension("WGL_EXT_swap_control")) {
    int swapInterval = wglGetSwapIntervalEXT();
    qDebug("V-Sync is %s\n", (swapInterval > 0 ? "ON" : "OFF"));
  }
  glGetError();
#endif

  setFramePosition(QPoint(100, -900));
  resize(QSize(800, 600));
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  _textRenderer[0] = TextRenderer::getInstance(SANS_FONT_FAMILY, 12, false);
  _textRenderer[1] = TextRenderer::getInstance("Times New Roman", 12, TextRenderer::SHADOW_EFFECT);
  _textRenderer[2] = TextRenderer::getInstance(MONO_FONT_FAMILY, 48, -1, false, TextRenderer::OUTLINE_EFFECT);
  _textRenderer[3] = TextRenderer::getInstance(INCONSOLATA_FONT_FAMILY, 24);
}

QTestWindow::~QTestWindow() {
}

void QTestWindow::makeCurrent() {
  m_context->makeCurrent(this);
}

static const wchar_t * EXAMPLE_TEXT = L"Hello 1.0\nline 2\ndescent ggg\nascent  ¡¡¡";
static const glm::uvec2 QUAD_OFFSET(10, 20);
static const glm::vec3 COLORS[4] = {
  { 1, 1, 1 },
  { 0.5, 1.0, 0.5 },
  { 1.0, 0.5, 0.5 },
  { 0.5, 0.5, 1.0 },
};

void QTestWindow::draw() {
  makeCurrent();

  glViewport(0, 0, _size.width(), _size.height());
  glClearColor(0.2f, 0.2f, 0.2f, 1);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glDisable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0, _size.width(), _size.height(), 0, 1, -1);
  glMatrixMode(GL_MODELVIEW);


  const glm::uvec2 size = glm::uvec2(_size.width() / 2, _size.height() / 2);
  const glm::uvec2 offsets[4] = {
    { QUAD_OFFSET.x, QUAD_OFFSET.y },
    { size.x + QUAD_OFFSET.x, QUAD_OFFSET.y },
    { size.x + QUAD_OFFSET.x, size.y + QUAD_OFFSET.y },
    { QUAD_OFFSET.x, size.y + QUAD_OFFSET.y },
  };

  {
    glColor3f(0.9, 0.9, 0.9);
    glBegin(GL_LINES);
    glVertex2f(0, size.y);
    glVertex2f(_size.width(), size.y);
    glVertex2f(size.x, 0);
    glVertex2f(size.x, _size.height());
    glEnd();
  }
  QString str = QString::fromWCharArray(EXAMPLE_TEXT);
  for (int i = 0; i < 4; ++i) {
    _textRenderer[i]->draw(offsets[i].x, offsets[i].y, str.toLocal8Bit().constData(), glm::vec4(COLORS[i], 1.0f));
  }
  m_context->swapBuffers(this);
}

void QTestWindow::resizeEvent(QResizeEvent * ev) {
  QWindow::resizeEvent(ev);
  _size = ev->size();
}

int main(int argc, char** argv) {
  QApplication app(argc, argv);
  QTestWindow window;
  QTimer timer;
  timer.setInterval(10);
  app.connect(&timer, &QTimer::timeout, &app, [&] {
    window.draw();
  });
  timer.start();
  app.exec();
  return 0;
}

#include "main.moc"