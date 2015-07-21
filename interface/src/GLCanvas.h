//
//  GLCanvas.h
//  interface/src
//
//  Created by Stephen Birarda on 8/14/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_GLCanvas_h
#define hifi_GLCanvas_h

#include <QDebug>
#include <gpu/GPUConfig.h>
#include <QGLWidget>
#include <QTimer>

/// customized canvas that simply forwards requests/events to the singleton application
class GLCanvas : public QGLWidget {
    Q_OBJECT
    
public:
    GLCanvas();
    
    void stopFrameTimer();

    bool isThrottleRendering() const;
    
    int getDeviceWidth() const;
    int getDeviceHeight() const;
    QSize getDeviceSize() const { return QSize(getDeviceWidth(), getDeviceHeight()); }
    
protected:

    QTimer _frameTimer;
    bool _throttleRendering;
    int _idleRenderInterval;

    virtual void initializeGL();
    virtual void paintGL();
    virtual void resizeGL(int width, int height);
    virtual bool event(QEvent* event);

private slots:
    void activeChanged(Qt::ApplicationState state);
    void throttleRender();
    bool eventFilter(QObject*, QEvent* event);
   
};


#endif // hifi_GLCanvas_h
