#pragma once

#include <QString>
#include <functional>

class QGLWidget;
class QScreen;

class PluginContainer {
public:
    //class Menu {
    //    virtual void addMenu(const QString& menuName) = 0;
    //    virtual void removeMenu(const QString& menuName) = 0;
    //    virtual void addMenuItem(const QString& path, const QString& name, std::function<void(bool)> onClicked, bool checkable = false, bool checked = false, const QString& groupName = "") = 0;
    //    virtual void removeMenuItem(const QString& menuName, const QString& menuItem) = 0;
    //    virtual bool isOptionChecked(const QString& name) = 0;
    //    virtual void setIsOptionChecked(const QString& path, bool checked) = 0;
    //};
    //virtual Menu* getMenu();
    //class Surface {
    //    virtual bool makeCurrent() = 0;
    //    virtual void doneCurrent() = 0;
    //    virtual void swapBuffers() = 0;
    //    virtual void setFullscreen(const QScreen* targetScreen) = 0;
    //    virtual void unsetFullscreen() = 0;
    //};
    //virtual Surface* getSurface();
    virtual void addMenu(const QString& menuName) = 0;
    virtual void removeMenu(const QString& menuName) = 0;
    virtual void addMenuItem(const QString& path, const QString& name, std::function<void(bool)> onClicked, bool checkable = false, bool checked = false, const QString& groupName = "") = 0;
    virtual void removeMenuItem(const QString& menuName, const QString& menuItem) = 0;
    virtual bool isOptionChecked(const QString& name) = 0;
    virtual void setIsOptionChecked(const QString& path, bool checked) = 0;
    virtual void setFullscreen(const QScreen* targetScreen) = 0;
    virtual void unsetFullscreen(const QScreen* avoidScreen = nullptr) = 0;
    virtual void showDisplayPluginsTools() = 0;
    virtual QGLWidget* getPrimarySurface() = 0;
};
