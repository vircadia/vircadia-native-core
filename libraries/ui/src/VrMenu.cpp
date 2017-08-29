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
#include <QDebug>

#include "OffscreenUi.h"
#include "ui/Logging.h"

static unsigned int USER_DATA_ID = 0;

// Binds together a Qt Action or Menu with the QML Menu or MenuItem
//
// TODO On reflection, it may be pointless to use the UUID.  Perhaps
// simply creating the bidirectional link pointing to both the widget
// and qml object and inject the pointer into both objects
class MenuUserData : public QObjectUserData {
public:
    MenuUserData(QAction* action, QObject* qmlObject, QObject* qmlParent) {
        if (!USER_DATA_ID) {
            USER_DATA_ID = DependencyManager::get<OffscreenUi>()->getMenuUserDataId();
        }
        _action = action;
        _qml = qmlObject;
        _qmlParent = qmlParent;

        action->setUserData(USER_DATA_ID, this);
        qmlObject->setUserData(USER_DATA_ID, this);
        qmlObject->setObjectName(uuid.toString());
        // Make sure we can find it again in the future
        updateQmlItemFromAction();
        _changedConnection = QObject::connect(action, &QAction::changed, [=] {
            updateQmlItemFromAction();
        });
        _shutdownConnection = QObject::connect(qApp, &QCoreApplication::aboutToQuit, [=] {
            QObject::disconnect(_changedConnection);
        });

        class ExclusionGroupSetter : public QObject {
        public:
            ExclusionGroupSetter(QObject* from, QObject* to, QObject* qmlParent) : QObject(from), _from(from), _to(to), _qmlParent(qmlParent) {
                _from->installEventFilter(this);
            }

            ~ExclusionGroupSetter() {
                _from->removeEventFilter(this);
            }
        protected:
            virtual bool eventFilter(QObject* o, QEvent* e) override {
                if (e->type() == QEvent::DynamicPropertyChange) {
                    QDynamicPropertyChangeEvent* dpc = static_cast<QDynamicPropertyChangeEvent*>(e);
                    if (dpc->propertyName() == "exclusionGroup")
                    {
                        // unfortunately Qt doesn't support passing dynamic properties between C++ / QML, so we have to use this ugly helper function
                        QMetaObject::invokeMethod(_qmlParent,
                            "addExclusionGroup",
                            Qt::DirectConnection,
                            Q_ARG(QVariant, QVariant::fromValue(_to)),
                            Q_ARG(QVariant, _from->property(dpc->propertyName())));
                    }
                }

                return QObject::eventFilter(o, e);
            }

        private:
            QObject* _from;
            QObject* _to;
            QObject* _qmlParent;
        };

        new ExclusionGroupSetter(action, qmlObject, qmlParent);
    }

    ~MenuUserData() {
        QObject::disconnect(_changedConnection);
        QObject::disconnect(_shutdownConnection);
        _action->setUserData(USER_DATA_ID, nullptr);
        _qml->setUserData(USER_DATA_ID, nullptr);
    }

    void updateQmlItemFromAction() {
        _qml->setProperty("checkable", _action->isCheckable());
        _qml->setProperty("enabled", _action->isEnabled());
        QString text = _action->text();
        _qml->setProperty("text", text);
        _qml->setProperty("shortcut", _action->shortcut().toString());
        _qml->setProperty("checked", _action->isChecked());
        _qml->setProperty("visible", _action->isVisible());
    }

    void clear() {
        _qml->setProperty("checkable", 0);
        _qml->setProperty("enabled", 0);
        _qml->setProperty("text", 0);
        _qml->setProperty("shortcut", 0);
        _qml->setProperty("checked", 0);
        _qml->setProperty("visible", 0);

        _action->setUserData(USER_DATA_ID, nullptr);
        _qml->setUserData(USER_DATA_ID, nullptr);
    }


    const QUuid uuid{ QUuid::createUuid() };

    static bool hasData(QAction* object) {
        if (!object) {
            qWarning() << "Attempted to fetch MenuUserData for null object";
            return false;
        }
        return (nullptr != static_cast<MenuUserData*>(object->userData(USER_DATA_ID)));
    }

    static MenuUserData* forObject(QAction* object) {
        if (!object) {
            qWarning() << "Attempted to fetch MenuUserData for null object";
            return nullptr;
        }
        auto result = static_cast<MenuUserData*>(object->userData(USER_DATA_ID));
        if (!result) {
            qWarning() << "Unable to find MenuUserData for object " << object;
            if (auto action = dynamic_cast<QAction*>(object)) {
                qWarning() << action->text();
            } else if (auto menu = dynamic_cast<QMenu*>(object)) {
                qWarning() << menu->title();
            }
            return nullptr;
        }
        return result;
    }

private:
    MenuUserData(const MenuUserData&);

    QMetaObject::Connection _shutdownConnection;
    QMetaObject::Connection _changedConnection;
    QAction* _action { nullptr };
    QObject* _qml { nullptr };
    QObject* _qmlParent{ nullptr };
};


VrMenu::VrMenu(OffscreenUi* parent) : QObject(parent) {
    _rootMenu = parent->getRootItem()->findChild<QObject*>("rootMenu");
    parent->getSurfaceContext()->setContextProperty("rootMenu", _rootMenu);
}

QObject* VrMenu::findMenuObject(const QString& menuOption) {
    if (menuOption.isEmpty()) {
        return _rootMenu;
    }
    QObject* result = _rootMenu->findChild<QObject*>(menuOption);
    return result;
}


void VrMenu::addMenu(QMenu* menu) {
    Q_ASSERT(!MenuUserData::hasData(menu->menuAction()));
    QObject* parent = menu->parent();
    QObject* qmlParent = nullptr;
    QMenu* parentMenu = dynamic_cast<QMenu*>(parent);
    if (parentMenu) {
        MenuUserData* userData = MenuUserData::forObject(parentMenu->menuAction());
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
    new MenuUserData(menu->menuAction(), result, qmlParent);
}

void bindActionToQmlAction(QObject* qmlAction, QAction* action, QObject* qmlParent) {
    auto text = action->text();
    if (text == "Login") {
        qDebug(uiLogging) << "Login action " << action;
    }

    new MenuUserData(action, qmlAction, qmlParent);
    QObject::connect(action, &QAction::toggled, [=](bool checked) {
        qmlAction->setProperty("checked", checked);
    });
    QObject::connect(qmlAction, SIGNAL(triggered()), action, SLOT(trigger()));
}

class QQuickMenuItem1;

void VrMenu::addAction(QMenu* menu, QAction* action) {
    Q_ASSERT(!MenuUserData::hasData(action));

    Q_ASSERT(MenuUserData::hasData(menu->menuAction()));
    MenuUserData* userData = MenuUserData::forObject(menu->menuAction());
    if (!userData) {
        return;
    }
    QObject* menuQml = findMenuObject(userData->uuid.toString());
    Q_ASSERT(menuQml);
    QQuickMenuItem1* returnedValue { nullptr };
    bool invokeResult = QMetaObject::invokeMethod(menuQml, "addItem", Qt::DirectConnection,
        Q_RETURN_ARG(QQuickMenuItem1*, returnedValue),
        Q_ARG(QString, action->text()));

    Q_ASSERT(invokeResult);
    Q_UNUSED(invokeResult); // FIXME - apparently we haven't upgraded the Qt on our unix Jenkins environments to 5.5.x
    QObject* result = reinterpret_cast<QObject*>(returnedValue); // returnedValue.value<QObject*>();
    Q_ASSERT(result);
    // Bind the QML and Widget together
    bindActionToQmlAction(result, action, _rootMenu);
}

void VrMenu::addSeparator(QMenu* menu) {
    Q_ASSERT(MenuUserData::hasData(menu->menuAction()));
    MenuUserData* userData = MenuUserData::forObject(menu->menuAction());
    if (!userData) {
        return;
    }
    QObject* menuQml = findMenuObject(userData->uuid.toString());
    Q_ASSERT(menuQml);

    bool invokeResult = QMetaObject::invokeMethod(menuQml, "addSeparator", Qt::DirectConnection);
    Q_ASSERT(invokeResult);
    Q_UNUSED(invokeResult); // FIXME - apparently we haven't upgraded the Qt on our unix Jenkins environments to 5.5.x
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
    QQuickMenuItem1* returnedValue { nullptr };
    // FIXME this needs to find the index of the beforeQml item and call insertItem(int, object)
    bool invokeResult = QMetaObject::invokeMethod(menu, "addItem", Qt::DirectConnection,
        Q_RETURN_ARG(QQuickMenuItem1*, returnedValue),
        Q_ARG(QString, action->text()));
    Q_ASSERT(invokeResult);
    QObject* result = reinterpret_cast<QObject*>(returnedValue); // returnedValue.value<QObject*>();
    Q_ASSERT(result);
    bindActionToQmlAction(result, action, _rootMenu);
}

class QQuickMenuBase;
class QQuickMenu1;

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

    userData->clear();
    delete userData;
}
