//
//  MenuScriptingInterface.h
//  hifi
//
//  Created by Brad Hefta-Gaub on 2/25/14
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//

#ifndef __hifi__MenuScriptingInterface__
#define __hifi__MenuScriptingInterface__

#include <QMutex>
#include <QObject>
#include <QString>

#include "Menu.h"
#include <MenuTypes.h>

class MenuScriptingInterface : public QObject {
    Q_OBJECT
    MenuScriptingInterface() { };
public:
    static MenuScriptingInterface* getInstance();
    static void deleteLaterIfExists();

private slots:
    friend class Menu;
    void menuItemTriggered();

public slots:
    void addMenu(const QString& menuName);
    void removeMenu(const QString& menuName);

    void addSeparator(const QString& menuName, const QString& separatorName);
    
    void addMenuItem(const MenuItemProperties& properties);
    void addMenuItem(const QString& menuName, const QString& menuitem, const QString& shortcutKey);
    void addMenuItem(const QString& menuName, const QString& menuitem);

    void removeMenuItem(const QString& menuName, const QString& menuitem);

    bool isOptionChecked(const QString& menuOption);
    void setIsOptionChecked(const QString& menuOption, bool isChecked);
    
signals:
    void menuItemEvent(const QString& menuItem);
    
private:
    static QMutex _instanceMutex;
    static MenuScriptingInterface* _instance;
};

#endif /* defined(__hifi__MenuScriptingInterface__) */
