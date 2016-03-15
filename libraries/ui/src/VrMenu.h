//
//  VrMenu.h
//
//  Created by Bradley Austin Davis on 2015/04/21
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_VrMenu_h
#define hifi_VrMenu_h

#include <QQuickItem>
#include <QHash>
#include <QList>
#include <QSignalMapper>
#include <QAction>
#include <QMenu>
#include "OffscreenUi.h"

// FIXME break up the rendering code (VrMenu) and the code for mirroring a Widget based menu in QML
class VrMenu : public QObject {
    Q_OBJECT
public:
    static void executeOrQueue(std::function<void(VrMenu*)> f);
    VrMenu(QObject* parent = nullptr);
    void addMenu(QMenu* menu);
    void addAction(QMenu* parent, QAction* action);
    void addSeparator(QMenu* parent);
    void insertAction(QAction* before, QAction* action);
    void removeAction(QAction* action);

protected:
    QObject* _rootMenu{ nullptr };
    QObject* findMenuObject(const QString& name);

    static VrMenu* _instance;
    friend class MenuUserData;
};

#endif // hifi_VrMenu_h
