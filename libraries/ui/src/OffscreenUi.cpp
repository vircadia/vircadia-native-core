//
//  OffscreenUi.cpp
//  interface/src/render-utils
//
//  Created by Bradley Austin Davis on 2015-04-04
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "OffscreenUi.h"
#include <QOpenGLFramebufferObject>
#include <QOpenGLDebugLogger>
#include <QQuickWindow>
#include <QGLWidget>
#include <QtQml>
#include "ErrorDialog.h"
#include "MessageDialog.h"

class OffscreenUiRoot : public QQuickItem {
    Q_OBJECT
public:

    OffscreenUiRoot(QQuickItem* parent = 0);
    Q_INVOKABLE void information(const QString& title, const QString& text);
    Q_INVOKABLE void loadChild(const QUrl& url) {
        DependencyManager::get<OffscreenUi>()->load(url);
    }
};



// This hack allows the QML UI to work with keys that are also bound as 
// shortcuts at the application level.  However, it seems as though the 
// bound actions are still getting triggered.  At least for backspace.  
// Not sure why.
//
// However, the problem may go away once we switch to the new menu system,
// so I think it's OK for the time being.
bool OffscreenUi::shouldSwallowShortcut(QEvent* event) {
    Q_ASSERT(event->type() == QEvent::ShortcutOverride);
    QObject* focusObject = _quickWindow->focusObject();
    if (focusObject != _quickWindow && focusObject != getRootItem()) {
        //qDebug() << "Swallowed shortcut " << static_cast<QKeyEvent*>(event)->key();
        event->accept();
        return true;
    }
    return false;
}

OffscreenUiRoot::OffscreenUiRoot(QQuickItem* parent) : QQuickItem(parent) {
}

void OffscreenUiRoot::information(const QString& title, const QString& text) {
    OffscreenUi::information(title, text);
}

OffscreenUi::OffscreenUi() {
    ::qmlRegisterType<OffscreenUiRoot>("Hifi", 1, 0, "Root");
}

OffscreenUi::~OffscreenUi() {
}


void OffscreenUi::show(const QUrl& url, const QString& name, std::function<void(QQmlContext*, QObject*)> f) {
    QQuickItem* item = getRootItem()->findChild<QQuickItem*>(name);
    // First load?
    if (!item) {
        load(url, f);
        item = getRootItem()->findChild<QQuickItem*>(name);
    }
    if (item) {
        item->setEnabled(true);
    }
}

void OffscreenUi::toggle(const QUrl& url, const QString& name, std::function<void(QQmlContext*, QObject*)> f) {
    QQuickItem* item = getRootItem()->findChild<QQuickItem*>(name);
    // First load?
    if (!item) {
        load(url, f);
        item = getRootItem()->findChild<QQuickItem*>(name);
    }
    if (item) {
        item->setEnabled(!item->isEnabled());
    }
}

void OffscreenUi::messageBox(const QString& title, const QString& text,
    ButtonCallback callback,
    QMessageBox::Icon icon,
    QMessageBox::StandardButtons buttons) {
    MessageDialog* pDialog{ nullptr };
    MessageDialog::show([&](QQmlContext* ctx, QObject* item) {
        pDialog = item->findChild<MessageDialog*>();
        pDialog->setIcon((MessageDialog::Icon)icon);
        pDialog->setTitle(title);
        pDialog->setText(text);
        pDialog->setStandardButtons(MessageDialog::StandardButtons(static_cast<int>(buttons)));
        pDialog->setResultCallback(callback);
    });
    pDialog->setEnabled(true);
}

void OffscreenUi::information(const QString& title, const QString& text,
    ButtonCallback callback,
    QMessageBox::StandardButtons buttons) {
    messageBox(title, text, callback,
            static_cast<QMessageBox::Icon>(MessageDialog::Information), buttons);
}

void OffscreenUi::question(const QString& title, const QString& text,
    ButtonCallback callback,
    QMessageBox::StandardButtons buttons) {
    messageBox(title, text, callback,
            static_cast<QMessageBox::Icon>(MessageDialog::Question), buttons);
}

void OffscreenUi::warning(const QString& title, const QString& text,
    ButtonCallback callback,
    QMessageBox::StandardButtons buttons) {
    messageBox(title, text, callback,
            static_cast<QMessageBox::Icon>(MessageDialog::Warning), buttons);
}

void OffscreenUi::critical(const QString& title, const QString& text,
    ButtonCallback callback,
    QMessageBox::StandardButtons buttons) {
    messageBox(title, text, callback,
            static_cast<QMessageBox::Icon>(MessageDialog::Critical), buttons);
}

void OffscreenUi::error(const QString& text) {
    ErrorDialog* pDialog{ nullptr };
    ErrorDialog::show([&](QQmlContext* ctx, QObject* item) {
        pDialog = item->findChild<ErrorDialog*>();
        pDialog->setText(text);
    });
    pDialog->setEnabled(true);
}


OffscreenUi::ButtonCallback OffscreenUi::NO_OP_CALLBACK = [](QMessageBox::StandardButton) {};

#include "OffscreenUi.moc"
