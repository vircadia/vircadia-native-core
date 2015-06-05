#pragma once

#include <QString>

class GlWindow;

class PluginContainer {
public:
    virtual void addMenuItem(const QString& path, std::function<void()> onClicked, bool checkable = false, bool checked = false, const QString& groupName = "") = 0;
    virtual GlWindow* getVisibleWindow() = 0;
};
