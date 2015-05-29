#pragma once

#include <QString>

class PluginContainer {
    virtual void addMenuItem(const QString& path, std::function<void()> onClicked, bool checkable = false, bool checked = false, const QString& groupName = "") = 0;
};
