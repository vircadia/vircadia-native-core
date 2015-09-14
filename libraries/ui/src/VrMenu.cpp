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

    const QUuid uuid{ QUuid::createUuid() };

    static MenuUserData* forObject(QObject* object) {
        return static_cast<MenuUserData*>(object->userData(USER_DATA_ID));
    }

private:
    MenuUserData(const MenuUserData&);

    void init(QObject* widgetObject, QObject* qmlObject) {
        widgetObject->setUserData(USER_DATA_ID, this);
        qmlObject->setUserData(USER_DATA_ID, this);
        qmlObject->setObjectName(uuid.toString());
        // Make sure we can find it again in the future
        Q_ASSERT(VrMenu::_instance->findMenuObject(uuid.toString()));
    }
};

const int MenuUserData::USER_DATA_ID = QObject::registerUserData();

HIFI_QML_DEF_LAMBDA(VrMenu, [&](QQmlContext* context, QObject* newItem) {
    auto offscreenUi = DependencyManager::get<OffscreenUi>();
    QObject* rootMenu = offscreenUi->getRootItem()->findChild<QObject*>("rootMenu");
    Q_ASSERT(rootMenu);
    static_cast<VrMenu*>(newItem)->setRootMenu(rootMenu);
    context->setContextProperty("rootMenu", rootMenu);
});

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

void VrMenu::executeQueuedLambdas() {
    Q_ASSERT(_instance);
    foreach(std::function<void(VrMenu*)> f, queuedLambdas) {
        f(_instance);
    }
    queuedLambdas.clear();
}

VrMenu::VrMenu(QQuickItem* parent) : QQuickItem(parent) {
    _instance = this;
    this->setEnabled(false);
}

QObject* VrMenu::findMenuObject(const QString& menuOption) {
    if (menuOption.isEmpty()) {
        return _rootMenu;
    }
    QObject* result = _rootMenu->findChild<QObject*>(menuOption);
    return result;
}

void VrMenu::setRootMenu(QObject* rootMenu) {
    _rootMenu = rootMenu;
}

void VrMenu::addMenu(QMenu* menu) {
    Q_ASSERT(!MenuUserData::forObject(menu));
    QObject* parent = menu->parent();
    QObject* qmlParent = nullptr;
    if (dynamic_cast<QMenu*>(parent)) {
        MenuUserData* userData = MenuUserData::forObject(parent);
        qmlParent = findMenuObject(userData->uuid.toString());
    } else if (dynamic_cast<QMenuBar*>(parent)) {
        qmlParent = _rootMenu;
    } else {
        Q_ASSERT(false);
    }
    QVariant returnedValue;
    #ifndef QT_NO_DEBUG
    bool invokeResult =
    #endif
    QMetaObject::invokeMethod(this, "addMenu", Qt::DirectConnection,
        Q_RETURN_ARG(QVariant, returnedValue),
        Q_ARG(QVariant, QVariant::fromValue(qmlParent)),
        Q_ARG(QVariant, QVariant::fromValue(menu->title())));
    Q_ASSERT(invokeResult);
    QObject* result = returnedValue.value<QObject*>();
    Q_ASSERT(result);

    // Bind the QML and Widget together
    new MenuUserData(menu, result);
}

void updateQmlItemFromAction(QObject* target, QAction* source) {
    target->setProperty("checkable", source->isCheckable());
    target->setProperty("enabled", source->isEnabled());
    target->setProperty("visible", source->isVisible());
    target->setProperty("text", source->text());
    target->setProperty("checked", source->isChecked());
}

void bindActionToQmlAction(QObject* qmlAction, QAction* action) {
    new MenuUserData(action, qmlAction);
    updateQmlItemFromAction(qmlAction, action);
    QObject::connect(action, &QAction::changed, [=] {
        updateQmlItemFromAction(qmlAction, action);
    });
    QObject::connect(action, &QAction::toggled, [=](bool checked) {
        qmlAction->setProperty("checked", checked);
    });
    QObject::connect(qmlAction, SIGNAL(triggered()), action, SLOT(trigger()));
}

void VrMenu::addAction(QMenu* menu, QAction* action) {
    Q_ASSERT(!MenuUserData::forObject(action));
    Q_ASSERT(MenuUserData::forObject(menu));
    MenuUserData* userData = MenuUserData::forObject(menu);
    QObject* menuQml = findMenuObject(userData->uuid.toString());
    Q_ASSERT(menuQml);
    QVariant returnedValue;
    #ifndef QT_NO_DEBUG
    bool invokeResult =
    #endif
    QMetaObject::invokeMethod(this, "addItem", Qt::DirectConnection,
        Q_RETURN_ARG(QVariant, returnedValue),
        Q_ARG(QVariant, QVariant::fromValue(menuQml)),
        Q_ARG(QVariant, QVariant::fromValue(action->text())));
    Q_ASSERT(invokeResult);
    QObject* result = returnedValue.value<QObject*>();
    Q_ASSERT(result);
    // Bind the QML and Widget together
    bindActionToQmlAction(result, action);
}

void VrMenu::insertAction(QAction* before, QAction* action) {
    QObject* beforeQml{ nullptr };
    {
        MenuUserData* beforeUserData = MenuUserData::forObject(before);
        Q_ASSERT(beforeUserData);
        beforeQml = findMenuObject(beforeUserData->uuid.toString());
    }
    QObject* menu = beforeQml->parent();
    QVariant returnedValue;
    #ifndef QT_NO_DEBUG
    bool invokeResult =
    #endif
    QMetaObject::invokeMethod(this, "insertItem", Qt::DirectConnection,
        Q_RETURN_ARG(QVariant, returnedValue),
        Q_ARG(QVariant, QVariant::fromValue(menu)),
        Q_ARG(QVariant, QVariant::fromValue(beforeQml)),
        Q_ARG(QVariant, QVariant::fromValue(action->text())));
    Q_ASSERT(invokeResult);
    QObject* result = returnedValue.value<QObject*>();
    Q_ASSERT(result);
    bindActionToQmlAction(result, action);
}

void VrMenu::removeAction(QAction* action) {
    MenuUserData* userData = MenuUserData::forObject(action);
    if (!userData) {
        qWarning("Attempted to remove menu action with no found QML object");
        return;
    }
    QObject* item = findMenuObject(userData->uuid.toString());
    QObject* menu = item->parent();
    // Proxy QuickItem requests through the QML layer
    bool invokeResult = QMetaObject::invokeMethod(this, "removeItem", Qt::DirectConnection,
        Q_ARG(QVariant, QVariant::fromValue(menu)),
        Q_ARG(QVariant, QVariant::fromValue(item)));
#ifndef QT_NO_DEBUG
    Q_ASSERT(invokeResult);
#else
    Q_UNUSED(invokeResult);
#endif
}
