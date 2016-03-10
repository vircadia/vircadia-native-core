//
//  VrMenu.cpp
//
//  Created by Bradley Austin Davis on 2015/04/21
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "VrMenu.h"
#include <QtQml>
#include <QMenuBar>

// Binds together a Qt Action or Menu with the QML Menu or MenuItem
//
// TODO On reflection, it may be pointless to use the UUID.  Perhaps
// simply creating the bidirectional link pointing to both the widget
// and qml object and inject the pointer into both objects
class MenuUserData : public QObjectUserData {
    static const int USER_DATA_ID;

public:
    MenuUserData(QAction* action, QObject* qmlObject) {
        init(action, qmlObject);
    }
    MenuUserData(QMenu* menu, QObject* qmlObject) {
        init(menu, qmlObject);
    }

    ~MenuUserData() {
        _widget->setUserData(USER_DATA_ID, nullptr);
        _qml->setUserData(USER_DATA_ID, nullptr);
    }

    const QUuid uuid{ QUuid::createUuid() };

    static MenuUserData* forObject(QObject* object) {
        if (!object) {
            qWarning() << "Attempted to fetch MenuUserData for null object";
            return nullptr;
        }
        auto result = static_cast<MenuUserData*>(object->userData(USER_DATA_ID));
        if (!result) {
            qWarning() << "Unable to find MenuUserData for object " << object;
            return nullptr;
        }
        return result;
    }

private:
    MenuUserData(const MenuUserData&);
    void init(QObject* widgetObject, QObject* qmlObject) {
        _widget = widgetObject;
        _qml = qmlObject;
        widgetObject->setUserData(USER_DATA_ID, this);
        qmlObject->setUserData(USER_DATA_ID, this);
        qmlObject->setObjectName(uuid.toString());
        // Make sure we can find it again in the future
        Q_ASSERT(VrMenu::_instance->findMenuObject(uuid.toString()));
    }

    QObject* _widget { nullptr };
    QObject* _qml { nullptr };
};

const int MenuUserData::USER_DATA_ID = QObject::registerUserData();

VrMenu* VrMenu::_instance{ nullptr };
static QQueue<std::function<void(VrMenu*)>> queuedLambdas;

void VrMenu::executeOrQueue(std::function<void(VrMenu*)> f) {
    if (_instance) {
        foreach(std::function<void(VrMenu*)> priorLambda, queuedLambdas) {
            priorLambda(_instance);
        }
        f(_instance);
    } else {
        queuedLambdas.push_back(f);
    }
}


VrMenu::VrMenu(QObject* parent) : QObject(parent) {
    _instance = this;
    auto offscreenUi = DependencyManager::get<OffscreenUi>();
    _rootMenu = offscreenUi->getRootItem()->findChild<QObject*>("rootMenu");
    offscreenUi->getRootContext()->setContextProperty("rootMenu", _rootMenu);
    foreach(std::function<void(VrMenu*)> f, queuedLambdas) {
        f(this);
    }
    queuedLambdas.clear();
}

QObject* VrMenu::findMenuObject(const QString& menuOption) {
    if (menuOption.isEmpty()) {
        return _rootMenu;
    }
    QObject* result = _rootMenu->findChild<QObject*>(menuOption);
    return result;
}

void updateQmlItemFromAction(QObject* target, QAction* source) {
    target->setProperty("checkable", source->isCheckable());
    target->setProperty("enabled", source->isEnabled());
    target->setProperty("text", source->text());
    target->setProperty("checked", source->isChecked());
    target->setProperty("visible", source->isVisible());
}

void VrMenu::addMenu(QMenu* menu) {
    Q_ASSERT(!MenuUserData::forObject(menu));
    QObject* parent = menu->parent();
    QObject* qmlParent = nullptr;
    if (dynamic_cast<QMenu*>(parent)) {
        MenuUserData* userData = MenuUserData::forObject(parent);
        if (!userData) {
            return;
        }
        qmlParent = findMenuObject(userData->uuid.toString());
    } else if (dynamic_cast<QMenuBar*>(parent)) {
        qmlParent = _rootMenu;
    } else {
        Q_ASSERT(false);
    }
    QVariant returnedValue;
    bool invokeResult = QMetaObject::invokeMethod(qmlParent, "addMenu", Qt::DirectConnection,
                                                  Q_RETURN_ARG(QVariant, returnedValue),
                                                  Q_ARG(QVariant, QVariant::fromValue(menu->title())));
    Q_ASSERT(invokeResult);
    Q_UNUSED(invokeResult); // FIXME - apparently we haven't upgraded the Qt on our unix Jenkins environments to 5.5.x
    QObject* result = returnedValue.value<QObject*>();
    Q_ASSERT(result);
    if (!result) {
        qWarning() << "Unable to create QML menu for widget menu: " << menu->title();
        return;
    }

    // Bind the QML and Widget together
    new MenuUserData(menu, result);
    auto menuAction = menu->menuAction();
    updateQmlItemFromAction(result, menuAction);
    auto connection = QObject::connect(menuAction, &QAction::changed, [=] {
        updateQmlItemFromAction(result, menuAction);
    });
    QObject::connect(qApp, &QCoreApplication::aboutToQuit, [=] {
        QObject::disconnect(connection);
    });

}

void bindActionToQmlAction(QObject* qmlAction, QAction* action) {
    new MenuUserData(action, qmlAction);
    updateQmlItemFromAction(qmlAction, action);
    auto connection = QObject::connect(action, &QAction::changed, [=] {
        updateQmlItemFromAction(qmlAction, action);
    });
    QObject::connect(qApp, &QCoreApplication::aboutToQuit, [=] {
        QObject::disconnect(connection);
    });

    QObject::connect(action, &QAction::toggled, [=](bool checked) {
        qmlAction->setProperty("checked", checked);
    });
    QObject::connect(qmlAction, SIGNAL(triggered()), action, SLOT(trigger()));
}

class QQuickMenuItem;

void VrMenu::addAction(QMenu* menu, QAction* action) {
    Q_ASSERT(!MenuUserData::forObject(action));
    Q_ASSERT(MenuUserData::forObject(menu));
    MenuUserData* userData = MenuUserData::forObject(menu);
    if (!userData) {
        return;
    }
    QObject* menuQml = findMenuObject(userData->uuid.toString());
    Q_ASSERT(menuQml);
    QQuickMenuItem* returnedValue { nullptr };
    
    bool invokeResult = QMetaObject::invokeMethod(menuQml, "addItem", Qt::DirectConnection,
        Q_RETURN_ARG(QQuickMenuItem*, returnedValue),
        Q_ARG(QString, action->text()));

    Q_ASSERT(invokeResult);
    Q_UNUSED(invokeResult); // FIXME - apparently we haven't upgraded the Qt on our unix Jenkins environments to 5.5.x
    QObject* result = reinterpret_cast<QObject*>(returnedValue); // returnedValue.value<QObject*>();
    Q_ASSERT(result);
    // Bind the QML and Widget together
    bindActionToQmlAction(result, action);
}

void VrMenu::insertAction(QAction* before, QAction* action) {
    QObject* beforeQml{ nullptr };
    {
        MenuUserData* beforeUserData = MenuUserData::forObject(before);
        Q_ASSERT(beforeUserData);
        if (!beforeUserData) {
            return;
        }
        beforeQml = findMenuObject(beforeUserData->uuid.toString());
    }
    QObject* menu = beforeQml->parent();
    QQuickMenuItem* returnedValue { nullptr };
    // FIXME this needs to find the index of the beforeQml item and call insertItem(int, object)
    bool invokeResult = QMetaObject::invokeMethod(menu, "addItem", Qt::DirectConnection,
        Q_RETURN_ARG(QQuickMenuItem*, returnedValue),
        Q_ARG(QString, action->text()));
    Q_ASSERT(invokeResult);
    QObject* result = reinterpret_cast<QObject*>(returnedValue); // returnedValue.value<QObject*>();
    Q_ASSERT(result);
    bindActionToQmlAction(result, action);
}

class QQuickMenuBase;

void VrMenu::removeAction(QAction* action) {
    if (!action) {
        qWarning("Attempted to remove invalid menu action");
        return;
    }
    MenuUserData* userData = MenuUserData::forObject(action);
    if (!userData) {
        qWarning("Attempted to remove menu action with no found QML object");
        return;
    }
    
    QObject* item = findMenuObject(userData->uuid.toString());
    QObject* menu = item->parent();
    // Proxy QuickItem requests through the QML layer
    QQuickMenuBase* qmlItem = reinterpret_cast<QQuickMenuBase*>(item);
    bool invokeResult = QMetaObject::invokeMethod(menu, "removeItem", Qt::DirectConnection,
        Q_ARG(QQuickMenuBase*, qmlItem));
    Q_ASSERT(invokeResult);
}
