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
#include <QUuid>
#include "OffscreenUi.h"



// Binds together a Qt Action or Menu with the QML Menu or MenuItem
//
// TODO On reflection, it may be pointless to use the UUID.  Perhaps
// simply creating the bidirectional link pointing to both the widget
// and qml object and inject the pointer into both objects
class MenuUserData : public QObject {
    Q_OBJECT
public:
    MenuUserData(QAction* action, QObject* qmlObject, QObject* qmlParent);
    ~MenuUserData();
    void updateQmlItemFromAction();
    void clear();

    const QUuid uuid{ QUuid::createUuid() };

    static bool hasData(QAction* object);

    static MenuUserData* forObject(QAction* object);

private:
    Q_DISABLE_COPY(MenuUserData);

    static constexpr const char *USER_DATA{"user_data"};

    QMetaObject::Connection _shutdownConnection;
    QMetaObject::Connection _changedConnection;
    QAction* _action { nullptr };
    QObject* _qml { nullptr };
    QObject* _qmlParent{ nullptr };
};



// FIXME break up the rendering code (VrMenu) and the code for mirroring a Widget based menu in QML
class VrMenu : public QObject {
    Q_OBJECT
public:
    VrMenu(OffscreenUi* parent = nullptr);
    void addMenu(QMenu* menu);
    void addAction(QMenu* parent, QAction* action);
    void addSeparator(QMenu* parent);
    void insertAction(QAction* before, QAction* action);
    void removeAction(QAction* action);

protected:
    QObject* _rootMenu{ nullptr };
    QObject* findMenuObject(const QString& name);

    friend class MenuUserData;
    friend class OffscreenUi;
};

#endif // hifi_VrMenu_h
