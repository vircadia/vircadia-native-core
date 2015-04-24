//
//  MenuConstants.h
//
//  Created by Bradley Austin Davis on 2015/04/21
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_MenuContants_h
#define hifi_MenuConstants_h

#include <QQuickItem>
#include <QHash>
#include <QList>
#include <QSignalMapper>
#include "OffscreenUi.h"

class HifiMenu : public QQuickItem {
    Q_OBJECT
    HIFI_QML_DECL_LAMBDA

public:
    HifiMenu(QQuickItem* parent = nullptr);

    void setToggleAction(const QString& name, std::function<void(bool)> f);
    void setTriggerAction(const QString& name, std::function<void()> f);

    void addMenu(const QString& parentMenu, const QString& menuOption);
    void removeMenu(const QString& menuName);
    bool menuExists(const QString& menuName) const;

    void addSeparator(const QString& menuName, const QString& separatorName);
    void removeSeparator(const QString& menuName, const QString& separatorName);

    void addMenuItem(const QString& parentMenu, const QString& menuOption);
    void addCheckableMenuItem(const QString& parentMenu, const QString& menuOption, bool checked = false);
    void addCheckableMenuItem(const QString& parentMenu, const QString& menuOption, bool checked, std::function<void(bool)> f);
    void addMenuItem(const QString& parentMenu, const QString& menuOption, std::function<void()> f);
    void removeMenuItem(const QString& menuitem);
    bool menuItemExists(const QString& menuName, const QString& menuitem) const;
    void triggerMenuItem(const QString& menuOption);
    void enableMenuItem(const QString& menuOption, bool enabled = true);
    bool isChecked(const QString& menuOption) const;
    void setChecked(const QString& menuOption, bool isChecked = true);
    void setCheckable(const QString& menuOption, bool checkable = true);
    void setExclusiveGroup(const QString& menuOption, const QString & groupName);
    void setText(const QString& menuOption, const QString& text);

    void setRootMenu(QObject * rootMenu);

private slots:
    void onTriggeredByName(const QString& name);
    void onToggledByName(const QString& name);

protected:
    QHash<QString, std::function<void()>> _triggerActions;
    QHash<QString, std::function<void(bool)>> _toggleActions;
    QObject* findMenuObject(const QString& name);
    const QObject* findMenuObject(const QString& name) const;
    QObject* _rootMenu{ nullptr };
    QSignalMapper _triggerMapper;
    QSignalMapper _toggleMapper;
};

#endif // hifi_MenuConstants_h




