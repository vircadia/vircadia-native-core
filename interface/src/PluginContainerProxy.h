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
    virtual void addMenu(const QString& menuName) override;
    virtual void removeMenu(const QString& menuName) override;
    virtual QAction* addMenuItem(PluginType type, const QString& path, const QString& name, std::function<void(bool)> onClicked, bool checkable = false, bool checked = false, const QString& groupName = "") override;
    virtual void removeMenuItem(const QString& menuName, const QString& menuItem) override;
    virtual bool isOptionChecked(const QString& name) override;
    virtual void setIsOptionChecked(const QString& path, bool checked) override;
    virtual void setFullscreen(const QScreen* targetScreen, bool hideMenu = true) override;
    virtual void unsetFullscreen(const QScreen* avoidScreen = nullptr) override;
    virtual void showDisplayPluginsTools() override;
    virtual void requestReset() override;
    virtual bool makeRenderingContextCurrent() override;
    virtual void releaseSceneTexture(uint32_t texture) override;
    virtual void releaseOverlayTexture(uint32_t texture) override;
    virtual GLWidget* getPrimaryWidget() override;
    virtual QWindow* getPrimaryWindow() override;
    virtual QOpenGLContext* getPrimaryContext() override;
    virtual bool isForeground() override;
    virtual const DisplayPlugin* getActiveDisplayPlugin() const override;

    /// settings interface
    virtual bool getBoolSetting(const QString& settingName, bool defaultValue) override;
    virtual void setBoolSetting(const QString& settingName, bool value) override;

    QRect _savedGeometry{ 10, 120, 800, 600 };
    std::map<QString, QActionGroup*> _exclusiveGroups;

    friend class Application;

};

#endif