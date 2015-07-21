#pragma once

#include <QString>
#include <functional>

class GlWindow;

class PluginContainer {
public:
    virtual void addMenuItem(const QString& path, const QString& name, std::function<void()> onClicked, bool checkable = false, bool checked = false, const QString& groupName = "") = 0;
    virtual GlWindow* getVisibleWindow() = 0;
};
