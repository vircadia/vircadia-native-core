//
//  MenuScriptingInterface.h
//  interface/src/scripting
//
//  Created by Brad Hefta-Gaub on 2/25/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_MenuScriptingInterface_h
#define hifi_MenuScriptingInterface_h

#include <QObject>
#include <QString>

class MenuItemProperties;

class MenuScriptingInterface : public QObject {
    Q_OBJECT
    MenuScriptingInterface() { };
public:
    static MenuScriptingInterface* getInstance();

private slots:
    friend class Menu;
    void menuItemTriggered();

public slots:
    void addMenu(const QString& menuName, const QString& grouping = QString());
    void removeMenu(const QString& menuName);
    bool menuExists(const QString& menuName);

    void addSeparator(const QString& menuName, const QString& separatorName);
    void removeSeparator(const QString& menuName, const QString& separatorName);
    
    void addMenuItem(const MenuItemProperties& properties);
    void addMenuItem(const QString& menuName, const QString& menuitem, const QString& shortcutKey);
    void addMenuItem(const QString& menuName, const QString& menuitem);

    void removeMenuItem(const QString& menuName, const QString& menuitem);
    bool menuItemExists(const QString& menuName, const QString& menuitem);

    void addActionGroup(const QString& groupName, const QStringList& actionList,
                        const QString& selected = QString());
    void removeActionGroup(const QString& groupName);
    
    bool isOptionChecked(const QString& menuOption);
    void setIsOptionChecked(const QString& menuOption, bool isChecked);

    void triggerOption(const QString& menuOption);

    bool isMenuEnabled(const QString& menuName);
    void setMenuEnabled(const QString& menuName, bool isEnabled);
    
signals:
    void menuItemEvent(const QString& menuItem);
};

#endif // hifi_MenuScriptingInterface_h
