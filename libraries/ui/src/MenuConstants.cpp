#include "MenuConstants.h"
#include <QtQml>

QML_DIALOG_DEF(HifiMenu)

static bool init = false;

HifiMenu::HifiMenu(QQuickItem * parent) : QQuickItem(parent) {
    this->setEnabled(false);
    qWarning() << "Setting up connection";
    connect(this, &HifiMenu::enabledChanged, this, [=]() {
        if (init) {
            return;
        }
        init = true;
        foreach(QObject * action, findChildren<QObject*>(QRegularExpression(".*HifiAction"))) {
            connect(action, SIGNAL(triggeredByName(QString)), this, SLOT(onTriggeredByName(QString)));
            connect(action, SIGNAL(toggledByName(QString)), this, SLOT(onToggledByName(QString)));
        }
    });
}

void HifiMenu::onTriggeredByName(const QString & name) {
    qDebug() << name << " triggered";
    if (triggerActions.count(name)) {
        triggerActions[name]();
    }
}

void HifiMenu::onToggledByName(const QString & name) {
    qDebug() << name << " toggled";
    if (triggerActions.count(name)) {
        if (toggleActions.count(name)) {
            QObject * action = findChild<QObject*>(name + "HifiAction");
            bool checked = action->property("checked").toBool();
            toggleActions[name](checked);
        }
    }
}

QHash<QString, std::function<void()>> HifiMenu::triggerActions;
QHash<QString, std::function<void(bool)>> HifiMenu::toggleActions;

void HifiMenu::setToggleAction(const QString & name, std::function<void(bool)> f) {
    toggleActions[name] = f;
}

void HifiMenu::setTriggerAction(const QString & name, std::function<void()> f) {
    triggerActions[name] = f;
}
