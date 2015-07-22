#pragma once

#include <QString>
#include <functional>

class GlWindow;

class PluginContainer {
public:
    virtual void addMenu(const QString& menuName) = 0;
    virtual void removeMenu(const QString& menuName) = 0;
    virtual void addMenuItem(const QString& path, const QString& name, std::function<void(bool)> onClicked, bool checkable = false, bool checked = false, const QString& groupName = "") = 0;
    virtual void removeMenuItem(const QString& menuName, const QString& menuItem) = 0;
    virtual bool isOptionChecked(const QString& name) = 0;
    virtual void setIsOptionChecked(const QString& path, bool checked) = 0;
    virtual GlWindow* getVisibleWindow() = 0;
};
