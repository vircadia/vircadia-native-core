//
//  HifiMenu.h
//
//  Created by Bradley Austin Davis on 2015/04/21
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_HifiMenu_h
#define hifi_HifiMenu_h

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

    void addItem(const QString& parentMenu, const QString& menuOption);
    void addItem(const QString& parentMenu, const QString& menuOption, std::function<void()> f);
    void addItem(const QString& parentMenu, const QString& menuOption, QObject* receiver, const char* slot);

    void addCheckableItem(const QString& parentMenu, const QString& menuOption, bool checked = false);
    void addCheckableItem(const QString& parentMenu, const QString& menuOption, bool checked, std::function<void(bool)> f);
    void addCheckableItem(const QString& parentMenu, const QString& menuOption, bool checked, QObject* receiver, const char* slot);
    void connectCheckable(const QString& menuOption, QObject* receiver, const char* slot);
    void connectItem(const QString& menuOption, QObject* receiver, const char* slot);

    void removeItem(const QString& menuitem);
    bool itemExists(const QString& menuName, const QString& menuitem) const;
    void triggerItem(const QString& menuOption);
    void enableItem(const QString& menuOption, bool enabled = true);
    bool isChecked(const QString& menuOption) const;
    void setChecked(const QString& menuOption, bool checked = true);
    void setCheckable(const QString& menuOption, bool checkable = true);
    void setExclusiveGroup(const QString& menuOption, const QString& groupName);
    void setItemText(const QString& menuOption, const QString& text);
    void setItemVisible(const QString& menuOption, bool visible = true);
    bool isItemVisible(const QString& menuOption);

    void setItemShortcut(const QString& menuOption, const QString& shortcut);
    QString getItemShortcut(const QString& menuOption);

    void setRootMenu(QObject* rootMenu);

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

#endif // hifi_HifiMenu_h
