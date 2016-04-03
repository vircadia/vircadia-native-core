#include "PluginContainerProxy.h"

#include <QtGui/QScreen>
#include <QtGui/QWindow>

#include <plugins/Plugin.h>
#include <plugins/PluginManager.h>
#include <display-plugins/DisplayPlugin.h>
#include <DependencyManager.h>
#include <FramebufferCache.h>

#include "Application.h"
#include "MainWindow.h"
#include "GLCanvas.h"
#include "ui/DialogsManager.h"

#include <gl/OffscreenGLCanvas.h>
#include <QtGui/QOpenGLContext>

PluginContainerProxy::PluginContainerProxy() {
}

PluginContainerProxy::~PluginContainerProxy() {
}

ui::Menu* PluginContainerProxy::getPrimaryMenu() {
    auto appMenu = qApp->_window->menuBar();
    auto uiMenu = dynamic_cast<ui::Menu*>(appMenu);
    return uiMenu;
}

bool PluginContainerProxy::isForeground() {
    return qApp->isForeground() && !qApp->getWindow()->isMinimized();
}

void PluginContainerProxy::requestReset() {
    // We could signal qApp to sequence this, but it turns out that requestReset is only used from within the main thread anyway.
    qApp->resetSensors(true);
}

void PluginContainerProxy::showDisplayPluginsTools(bool show) {
    DependencyManager::get<DialogsManager>()->hmdTools(show);
}

GLWidget* PluginContainerProxy::getPrimaryWidget() {
    return qApp->_glWidget;
}

MainWindow* PluginContainerProxy::getPrimaryWindow() {
    return qApp->getWindow();
}

QOpenGLContext* PluginContainerProxy::getPrimaryContext() {
    return qApp->_glWidget->context()->contextHandle();
}

const DisplayPlugin* PluginContainerProxy::getActiveDisplayPlugin() const {
    return qApp->getActiveDisplayPlugin();
}

bool PluginContainerProxy::makeRenderingContextCurrent() {
    return qApp->_offscreenContext->makeCurrent();
}

void PluginContainerProxy::releaseSceneTexture(const gpu::TexturePointer& texture) {
    Q_ASSERT(QThread::currentThread() == qApp->thread());
    auto& framebufferMap = qApp->_lockedFramebufferMap;
    Q_ASSERT(framebufferMap.contains(texture));
    auto framebufferPointer = framebufferMap[texture];
    framebufferMap.remove(texture);
    auto framebufferCache = DependencyManager::get<FramebufferCache>();
    framebufferCache->releaseFramebuffer(framebufferPointer);
}

void PluginContainerProxy::releaseOverlayTexture(const gpu::TexturePointer& texture) {
    qApp->_applicationOverlay.releaseOverlay(texture);
}

