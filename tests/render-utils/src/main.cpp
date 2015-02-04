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

//#define USE_OGLPLUS
#ifdef USE_OGLPLUS
#include <oglplus/all.hpp>
#include <oglplus/shapes/plane.hpp>
#include <oglplus/shapes/wrapper.hpp>
using ProgramPtr = std::shared_ptr < oglplus::Program >;
using ShapeWrapperPtr = std::shared_ptr < oglplus::shapes::ShapeWrapper >;
#endif



const char simple_vs[] = R"XXXX(#version 330

uniform mat4 Projection = mat4(1);
uniform mat4 ModelView = mat4(1);
layout(location = 0) in vec3 Position;

void main() {
  gl_Position = Projection * ModelView * vec4(Position, 1);
}
)XXXX";

const char example_gs[] = R"XXXX(#version 330
    layout(triangles) in;
    layout(triangle_strip, max_vertices = 3) out;

    void main() {
        for (int i = 0; i < 3; ++i) {
          gl_Position = vec4(gl_in[i].gl_Position.xyz / 2.0f, 1.0);
          EmitVertex();
        }
        EndPrimitive();
    }
)XXXX";

const char colored_fs[] = R"XXXX(#version 330

uniform vec4 Color = vec4(1, 0, 1, 1);
out vec4 FragColor;

void main() {
  FragColor = Color;
}
)XXXX";



class QTestWindow : public QWindow {
  Q_OBJECT
  QOpenGLContext * m_context;
#ifdef USE_OGLPLUS
  ProgramPtr shader;
  ShapeWrapperPtr plane;
#endif
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
  _textRenderer[1] = TextRenderer::getInstance(SERIF_FONT_FAMILY, 12, false, TextRenderer::SHADOW_EFFECT);
  _textRenderer[2] = TextRenderer::getInstance(MONO_FONT_FAMILY, 48, -1, false, TextRenderer::OUTLINE_EFFECT);
  _textRenderer[3] = TextRenderer::getInstance(INCONSOLATA_FONT_FAMILY, 24);


#ifdef USE_OGLPLUS
  using namespace oglplus;
  shader = ProgramPtr(new Program());
  // attach the shaders to the program
  shader->AttachShader(
    VertexShader()
    .Source(GLSLSource((const char*)simple_vs))
    .Compile()
  );
  shader->AttachShader(
    GeometryShader()
    .Source(GLSLSource((const char*)example_gs))
    .Compile()
  );
  shader->AttachShader(
    FragmentShader()
    .Source(GLSLSource((const char*)colored_fs))
    .Compile()
  );
  shader->Link();
  
  size_t uniformCount = shader->ActiveUniforms().Size();
  for (size_t i = 0; i < uniformCount; ++i) {
    std::string name = shader->ActiveUniforms().At(i).Name();
    int location = shader->ActiveUniforms().At(i).Index();
    qDebug() << name.c_str() << " " << location;
  }
  plane = ShapeWrapperPtr(
    new shapes::ShapeWrapper(
      { "Position", "TexCoord" },
      shapes::Plane(Vec3f(1, 0, 0), Vec3f(0, 1, 0)),
      *shader
    )
  );
  plane->Use();
#endif
  glDisable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

QTestWindow::~QTestWindow() {
}

void QTestWindow::makeCurrent() {
  m_context->makeCurrent(this);
}

static const wchar_t * EXAMPLE_TEXT = L"Áy Hello 1.0\nyÁ line 2\nÁy";
static const glm::uvec2 QUAD_OFFSET(10, 10);
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
  //{
  //  using namespace oglplus;
  //  shader->Use();
  //  Uniform<Vec4f>(*shader, "Color").Set(Vec4f(1.0f, 0, 0, 1.0f));
  //  plane->Use();
  //  plane->Draw();
  //  NoProgram().Bind();
  //}
   
  // m_context->swapBuffers(this); return;

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

  QString str = QString::fromWCharArray(EXAMPLE_TEXT);
  for (int i = 0; i < 4; ++i) {
    glm::vec2 bounds = _textRenderer[i]->computeExtent(str);
    glPushMatrix(); {
      glTranslatef(offsets[i].x, offsets[i].y, 0);
      glColor3f(0, 0, 0);
      glBegin(GL_QUADS); {
        glVertex2f(0, 0);
        glVertex2f(0, bounds.y);
        glVertex2f(bounds.x, bounds.y);
        glVertex2f(bounds.x, 0);
      } glEnd();
    } glPopMatrix();
    _textRenderer[i]->draw(offsets[i].x, offsets[i].y, str, glm::vec4(COLORS[i], 1.0f));
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