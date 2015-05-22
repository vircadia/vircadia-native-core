//
//  WindowDisplayPlugin.h
//
//  Created by Bradley Austin Davis on 2014/04/13.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include "SimpleDisplayPlugin.h"

#include <GlWindow.h>
#include <QTimer>

class WindowDisplayPlugin : public GlWindowDisplayPlugin {
    Q_OBJECT
public:
    static const QString NAME;

    WindowDisplayPlugin();

    virtual const QString & getName();

    virtual void activate();
    virtual void deactivate();
    virtual bool eventFilter(QObject* object, QEvent* event);
private:
    QTimer _timer;
};
