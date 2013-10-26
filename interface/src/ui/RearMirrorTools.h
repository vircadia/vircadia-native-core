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

class RearMirrorTools : public QObject {
    Q_OBJECT
public:
    RearMirrorTools(QGLWidget* parent, QRect& bounds);
    void render();
    bool mousePressEvent(int x, int y);

signals:
    void closeView();
    void restoreView();
    
private:
    QGLWidget* _parent;
    QRect _bounds;
    GLuint _closeTextureId;
    GLuint _restoreTextureId;
    bool _visible;
    
    void displayTools();
};

#endif /* defined(__hifi__RearMirrorTools__) */
