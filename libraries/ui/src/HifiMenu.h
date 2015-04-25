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
#include <QAction>
#include <QMenu>
#include "OffscreenUi.h"

class VrMenu : public QQuickItem {
    Q_OBJECT
    HIFI_QML_DECL_LAMBDA

public:
    static VrMenu* instance();
    VrMenu(QQuickItem* parent = nullptr);
    void addMenu(QMenu* menu);
    void addAction(QMenu* parent, QAction* action);
    void insertAction(QAction* before, QAction* action);
    void removeAction(QAction* action);

    void setRootMenu(QObject* rootMenu);

protected:
    QObject* _rootMenu{ nullptr };

    QObject* findMenuObject(const QString& name);
    const QObject* findMenuObject(const QString& name) const;

    static VrMenu* _instance;
    friend class MenuUserData;
};

#endif // hifi_MenuConstants_h




