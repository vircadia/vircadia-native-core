//
//  InterfaceView.h
//  gvr-interface/src
//
//  Created by Stephen Birarda on 1/28/14.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_InterfaceView_h
#define hifi_InterfaceView_h

#include <QtWidgets/QOpenGLWidget>

class InterfaceView : public QOpenGLWidget {
    Q_OBJECT
public:
    InterfaceView(QWidget* parent = 0, Qt::WindowFlags flags = 0);
};

#endif // hifi_InterfaceView_h
