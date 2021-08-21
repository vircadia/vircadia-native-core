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

/*@jsdoc
 * The possible docking locations of an <code>InteractiveWindow</code>.
 * @typedef {object} InteractiveWindow.DockAreas
 * @property {InteractiveWindow.DockArea} TOP - Dock to the top edge of the Interface window.
 * @property {InteractiveWindow.DockArea} BOTTOM - Dock to the bottom edge of the Interface window.
 * @property {InteractiveWindow.DockArea} LEFT - Dock to the left edge of the Interface window.
 * @property {InteractiveWindow.DockArea} RIGHT - Dock to the right edge of the Interface window.
 */
static const QVariantMap DOCK_AREA {
    { "TOP", DockArea::TOP },
    { "BOTTOM", DockArea::BOTTOM },
    { "LEFT", DockArea::LEFT },
    { "RIGHT", DockArea::RIGHT }
};

/*@jsdoc
 * The possible relative position anchors of an <code>InteractiveWindow</code> relative to the Interface window.
 * @typedef {object} InteractiveWindow.RelativePositionAnchors
 * @property {InteractiveWindow.RelativePositionAnchor} NO_ANCHOR - Position is not relative to any part of the Interface 
 *     window.
 * @property {InteractiveWindow.RelativePositionAnchor} TOP_LEFT - Position is offset from the top left of the Interface window.
 * @property {InteractiveWindow.RelativePositionAnchor} TOP_RIGHT - Position is offset from the top right of the Interface 
 *     window.
 * @property {InteractiveWindow.RelativePositionAnchor} BOTTOM_RIGHT - Position offset from the bottom right of the Interface 
 *     window.
 * @property {InteractiveWindow.RelativePositionAnchor} BOTTOM_LEFT - Position is offset from the bottom left of the Interface 
 *     window.
 */
static const QVariantMap RELATIVE_POSITION_ANCHOR {
    { "NO_ANCHOR", RelativePositionAnchor::NO_ANCHOR },
    { "TOP_LEFT", RelativePositionAnchor::TOP_LEFT },
    { "TOP_RIGHT", RelativePositionAnchor::TOP_RIGHT },
    { "BOTTOM_RIGHT", RelativePositionAnchor::BOTTOM_RIGHT },
    { "BOTTOM_LEFT", RelativePositionAnchor::BOTTOM_LEFT }
};

DesktopScriptingInterface::DesktopScriptingInterface(QObject* parent, bool restricted) 
    : QObject(parent), _restricted(restricted) { }

int DesktopScriptingInterface::getWidth() {
    QSize size = qApp->getWindow()->windowHandle()->screen()->virtualSize();
    return size.width();
}
int DesktopScriptingInterface::getHeight() {
    QSize size = qApp->getWindow()->windowHandle()->screen()->virtualSize();
    return size.height();
}

/*@jsdoc
 * The possible display modes for an <code>InteractiveWindow</code>.
 * @typedef {object} InteractiveWindow.PresentationModes
 * @property {InteractiveWindow.PresentationMode} VIRTUAL - The window is displayed inside Interface: in the desktop window in 
 *     desktop mode or on the HUD surface in HMD mode.
 * @property {InteractiveWindow.PresentationMode} NATIVE - The window is displayed separately from the Interface window, as its 
 *     own separate window.
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

QVariantMap DesktopScriptingInterface::getRelativePositionAnchor() {
    return RELATIVE_POSITION_ANCHOR;
}

void DesktopScriptingInterface::setHUDAlpha(float alpha) {
    qApp->getApplicationCompositor().setAlpha(alpha);
}

void DesktopScriptingInterface::show(const QString& path, const QString&  title) {
    auto offscreenUI = DependencyManager::get<OffscreenUi>();
    if (!offscreenUI) {
        return;
    }

    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "show", Qt::QueuedConnection, Q_ARG(QString, path), Q_ARG(QString, title));
        return;
    }
    offscreenUI->show(path, title);
}

InteractiveWindowPointer DesktopScriptingInterface::createWindow(const QString& sourceUrl, const QVariantMap& properties) {
    if (QThread::currentThread() != thread()) {
        InteractiveWindowPointer interactiveWindow = nullptr;
        BLOCKING_INVOKE_METHOD(this, "createWindowOnThread",
            Q_RETURN_ARG(InteractiveWindowPointer, interactiveWindow),
            Q_ARG(QString, sourceUrl),
            Q_ARG(QVariantMap, properties),
            Q_ARG(QThread*, QThread::currentThread()));
        return interactiveWindow;
    }


    // The offscreen surface already validates against non-local QML sources, but we also need to ensure that 
    // if we create top level QML, like dock widgets or other types of QQuickView containing desktop windows 
    // that the source URL is permitted
    const auto& urlValidator = OffscreenQmlSurface::getUrlValidator();
    if (!urlValidator(sourceUrl)) {
        return nullptr;
    }

    return new InteractiveWindow(sourceUrl, properties, _restricted);
}

InteractiveWindowPointer DesktopScriptingInterface::createWindowOnThread(const QString& sourceUrl, const QVariantMap& properties, QThread* targetThread) {
    // The offscreen surface already validates against non-local QML sources, but we also need to ensure that 
    // if we create top level QML, like dock widgets or other types of QQuickView containing desktop windows 
    // that the source URL is permitted
    const auto& urlValidator = OffscreenQmlSurface::getUrlValidator();
    if (!urlValidator(sourceUrl)) {
        return nullptr;
    }
    InteractiveWindowPointer window = new InteractiveWindow(sourceUrl, properties, _restricted);
    window->moveToThread(targetThread);
    return window;
}
