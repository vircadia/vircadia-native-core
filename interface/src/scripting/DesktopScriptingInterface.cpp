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

#include <shared/QtHelpers.h>

#include "Application.h"
#include "MainWindow.h"
#include <display-plugins/CompositorHelper.h>
#include <DependencyManager.h>
#include <OffscreenUi.h>

static const QVariantMap DOCK_AREA {
    { "TOP", DockArea::TOP },
    { "BOTTOM", DockArea::BOTTOM },
    { "LEFT", DockArea::LEFT },
    { "RIGHT", DockArea::RIGHT }
};

int DesktopScriptingInterface::getWidth() {
    QSize size = qApp->getWindow()->windowHandle()->screen()->virtualSize();
    return size.width();
}
int DesktopScriptingInterface::getHeight() {
    QSize size = qApp->getWindow()->windowHandle()->screen()->virtualSize();
    return size.height();
}

/**jsdoc
 * The presentation mode specifies how an {@link InteractiveWindow} is displayed.
 * @typedef {object} InteractiveWindow.PresentationMode
 * @property {number} VIRTUAL - The window is displayed inside Interface: in the desktop window in desktop mode or on the HUD 
 *     surface in HMD mode.
 * @property {number} NATIVE - The window is displayed separately from the Interface window, as its own separate window.
 */
QVariantMap DesktopScriptingInterface::getPresentationMode() {
    static QVariantMap presentationModes {
        { "VIRTUAL", Virtual },
        { "NATIVE", Native }
    };
    return presentationModes;
}

QVariantMap DesktopScriptingInterface::getDockArea() {
    return DOCK_AREA;
}

void DesktopScriptingInterface::setHUDAlpha(float alpha) {
    qApp->getApplicationCompositor().setAlpha(alpha);
}

void DesktopScriptingInterface::show(const QString& path, const QString&  title) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "show", Qt::QueuedConnection, Q_ARG(QString, path), Q_ARG(QString, title));
        return;
    }
    DependencyManager::get<OffscreenUi>()->show(path, title);
}

InteractiveWindowPointer DesktopScriptingInterface::createWindow(const QString& sourceUrl, const QVariantMap& properties) {
    if (QThread::currentThread() != thread()) {
        InteractiveWindowPointer interactiveWindow = nullptr;
        BLOCKING_INVOKE_METHOD(this, "createWindow",
            Q_RETURN_ARG(InteractiveWindowPointer, interactiveWindow),
            Q_ARG(QString, sourceUrl),
            Q_ARG(QVariantMap, properties));
        return interactiveWindow;
    }
    return new InteractiveWindow(sourceUrl, properties);;
}
