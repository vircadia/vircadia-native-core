//
//  RearMirrorTools.h
//  interface
//
//  Created by stojce on 23.10.2013.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.


#ifndef __hifi__RearMirrorTools__
#define __hifi__RearMirrorTools__

#include "InterfaceConfig.h"

#include <QGLWidget>
#include <QSettings>

enum ZoomLevel {
    HEAD,
    BODY
};

class RearMirrorTools : public QObject {
    Q_OBJECT
public:
    RearMirrorTools(QGLWidget* parent, QRect& bounds, QSettings* settings);
    void render(bool fullScreen);
    bool mousePressEvent(int x, int y);
    ZoomLevel getZoomLevel() { return _zoomLevel; }
    void saveSettings(QSettings* settings);

signals:
    void closeView();
    void shrinkView();
    void resetView();
    void restoreView();
    
private:
    QGLWidget* _parent;
    QRect _bounds;
    GLuint _closeTextureId;
    GLuint _resetTextureId;
    GLuint _zoomBodyTextureId;
    GLuint _zoomHeadTextureId;
    ZoomLevel _zoomLevel;
    
    QRect _closeIconRect;
    QRect _resetIconRect;
    QRect _shrinkIconRect;
    QRect _headZoomIconRect;
    QRect _bodyZoomIconRect;
    
    bool _windowed;
    bool _fullScreen;
    
    void displayIcon(QRect bounds, QRect iconBounds, GLuint textureId, bool selected = false);
};

#endif /* defined(__hifi__RearMirrorTools__) */
