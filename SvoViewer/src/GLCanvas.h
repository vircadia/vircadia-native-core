//
//  GLCanvas.h
//
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//

#ifndef __hifi__GLCanvas__
#define __hifi__GLCanvas__

#include <QGLWidget>

/// customized canvas that simply forwards requests/events to the singleton application
class GLCanvas : public QGLWidget {
public:
    GLCanvas();
protected:
    
    virtual void initializeGL();
    virtual void paintGL();
    virtual void resizeGL(int width, int height);
    
    virtual void keyPressEvent(QKeyEvent* event);
    virtual void keyReleaseEvent(QKeyEvent* event);
    
    virtual void mouseMoveEvent(QMouseEvent* event);
    virtual void mousePressEvent(QMouseEvent* event);
    virtual void mouseReleaseEvent(QMouseEvent* event);
    
    virtual bool event(QEvent* event);
    
    virtual void wheelEvent(QWheelEvent* event);

    virtual void dragEnterEvent(QDragEnterEvent *event);
    virtual void dropEvent(QDropEvent* event);
};

#endif /* defined(__hifi__GLCanvas__) */
