#pragma once
#ifndef hifi_PluginContainerProxy_h
#define hifi_PluginContainerProxy_h

#include <QtCore/QObject>
#include <QtCore/QRect>

#include <plugins/Forward.h>
#include <plugins/PluginContainer.h>

class QActionGroup;

class PluginContainerProxy : public QObject, PluginContainer {
    Q_OBJECT
    PluginContainerProxy();
    virtual ~PluginContainerProxy();
    virtual void showDisplayPluginsTools() override;
    virtual void requestReset() override;
    virtual bool makeRenderingContextCurrent() override;
    virtual void releaseSceneTexture(const gpu::TexturePointer& texture) override;
    virtual void releaseOverlayTexture(const gpu::TexturePointer& texture) override;
    virtual GLWidget* getPrimaryWidget() override;
    virtual MainWindow* getPrimaryWindow() override;
    virtual ui::Menu* getPrimaryMenu() override;
    virtual QOpenGLContext* getPrimaryContext() override;
    virtual bool isForeground() override;
    virtual const DisplayPlugin* getActiveDisplayPlugin() const override;

    friend class Application;

};

#endif
