//
//  Created by Bradley Austin Davis on 2018/10/21
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#include <QtGui/QWindow>
#include <QtCore/QSettings>

#include <qml/OffscreenSurface.h>
#include <gpu/Forward.h>
#include "RenderThread.h"

// Create a simple OpenGL window that renders text in various ways
class PlayerWindow : public QWindow {
    Q_OBJECT

public:
    PlayerWindow();
    virtual ~PlayerWindow();

protected:
    void touchEvent(QTouchEvent *ev) override;

public slots:
    void loadFile(QString filename);

private:
    hifi::qml::OffscreenSurface _surface;
    QSettings _settings;
    RenderThread _renderThread;
};
