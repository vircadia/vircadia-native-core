#pragma once

#include <QString>

class QMainWindow;

class PluginContainer {
public:
    virtual void addMenuItem(const QString& path, std::function<void()> onClicked, bool checkable = false, bool checked = false, const QString& groupName = "") = 0;
    virtual QMainWindow* getAppMainWindow() = 0;
};
