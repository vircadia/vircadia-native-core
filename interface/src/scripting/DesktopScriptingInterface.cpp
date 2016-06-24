//
//  DesktopScriptingInterface.h
//  interface/src/scripting
//
//  Created by David Rowe on 25 Aug 2015.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "DesktopScriptingInterface.h"

#include <QWindow>
#include <QScreen>

#include "Application.h"
#include "MainWindow.h"
#include <display-plugins/CompositorHelper.h>

int DesktopScriptingInterface::getWidth() {
    QSize size = qApp->getWindow()->windowHandle()->screen()->virtualSize();
    return size.width();
}
int DesktopScriptingInterface::getHeight() {
    QSize size = qApp->getWindow()->windowHandle()->screen()->virtualSize();
    return size.height();
}

void DesktopScriptingInterface::setOverlayAlpha(float alpha) {
    qApp->getApplicationCompositor().setAlpha(alpha);
}

