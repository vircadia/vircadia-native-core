//
//  Created by Bradley Austin Davis on 2015/08/08
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include <functional>
#include <map>
#include <stdint.h>

#include <QtCore/QString>
#include <QtCore/QVector>
#include <QtCore/QPair>
#include <QtCore/QRect>

#include <plugins/Forward.h>

class QAction;
class GLWidget;
class QScreen;
class QOpenGLContext;
class QWindow;

class DisplayPlugin;

namespace gpu {
    class Texture;
    using TexturePointer = std::shared_ptr<Texture>;
}

namespace ui {
    class Menu;
}

class QActionGroup;
class MainWindow;

class PluginContainer {
public:
    static PluginContainer& getInstance();
    PluginContainer();
    virtual ~PluginContainer();

    void addMenu(const QString& menuName);
    void removeMenu(const QString& menuName);
    QAction* addMenuItem(PluginType pluginType, const QString& path, const QString& name, std::function<void(bool)> onClicked, bool checkable = false, bool checked = false, const QString& groupName = "");
    void removeMenuItem(const QString& menuName, const QString& menuItem);
    bool isOptionChecked(const QString& name);
    void setIsOptionChecked(const QString& path, bool checked);

    void setFullscreen(const QScreen* targetScreen, bool hideMenu = false);
    void unsetFullscreen(const QScreen* avoidScreen = nullptr);

    virtual ui::Menu* getPrimaryMenu() = 0;
    virtual void showDisplayPluginsTools(bool show = true) = 0;
    virtual void requestReset() = 0;
    virtual bool makeRenderingContextCurrent() = 0;
    virtual void releaseSceneTexture(const gpu::TexturePointer& texture) = 0;
    virtual void releaseOverlayTexture(const gpu::TexturePointer& texture) = 0;
    virtual GLWidget* getPrimaryWidget() = 0;
    virtual MainWindow* getPrimaryWindow() = 0;
    virtual QOpenGLContext* getPrimaryContext() = 0;
    virtual bool isForeground() const = 0;
    virtual DisplayPluginPointer getActiveDisplayPlugin() const = 0;

    /// settings interface
    bool getBoolSetting(const QString& settingName, bool defaultValue);
    void setBoolSetting(const QString& settingName, bool value);

    QVector<QPair<QString, QString>>& currentDisplayActions() {
        return _currentDisplayPluginActions;
    }

    QVector<QPair<QString, QString>>& currentInputActions() {
        return _currentInputPluginActions;
    }

protected:
    QVector<QPair<QString, QString>> _currentDisplayPluginActions;
    QVector<QPair<QString, QString>> _currentInputPluginActions;
    std::map<QString, QActionGroup*> _exclusiveGroups;
    QRect _savedGeometry { 10, 120, 800, 600 };
};

