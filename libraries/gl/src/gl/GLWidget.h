//
//  Created by Bradley Austin Davis on 2015/12/03
//  Derived from interface/src/GLCanvas.h created by Stephen Birarda on 8/14/13.
//  Copyright 2013-2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_GLWidget_h
#define hifi_GLWidget_h

#include <QGLWidget>

/// customized canvas that simply forwards requests/events to the singleton application
class GLWidget : public QGLWidget {
    Q_OBJECT
    
public:
    GLWidget();
    int getDeviceWidth() const;
    int getDeviceHeight() const;
    QSize getDeviceSize() const { return QSize(getDeviceWidth(), getDeviceHeight()); }
    bool isVsyncSupported() const;
    virtual void initializeGL() override;

protected:
    virtual bool event(QEvent* event) override;
    virtual void paintEvent(QPaintEvent* event) override;
    virtual void resizeEvent(QResizeEvent* event) override;

private slots:
    virtual bool eventFilter(QObject*, QEvent* event) override;

private:
    bool _vsyncSupported { false };
};


#endif // hifi_GLCanvas_h
