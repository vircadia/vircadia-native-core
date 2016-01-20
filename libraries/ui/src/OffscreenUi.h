//
//  OffscreenUi.h
//  interface/src/entities
//
//  Created by Bradley Austin Davis on 2015-04-04
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once
#ifndef hifi_OffscreenUi_h
#define hifi_OffscreenUi_h

#include <QtCore/QVariant>
#include <gl/OffscreenQmlSurface.h>

#include <QMessageBox>
#include <DependencyManager.h>

#include "OffscreenQmlElement.h"


class OffscreenUi : public OffscreenQmlSurface, public Dependency {
    Q_OBJECT

public:
    OffscreenUi();
    virtual void create(QOpenGLContext* context) override;
    void createDesktop();
    void show(const QUrl& url, const QString& name, std::function<void(QQmlContext*, QObject*)> f = [](QQmlContext*, QObject*) {});
    void toggle(const QUrl& url, const QString& name, std::function<void(QQmlContext*, QObject*)> f = [](QQmlContext*, QObject*) {});
    bool shouldSwallowShortcut(QEvent* event);
    bool navigationFocused();
    void setNavigationFocused(bool focused);
    void unfocusWindows();
    QQuickItem* getDesktop();
    QQuickItem* getToolWindow();

    Q_INVOKABLE void executeOnUiThread(std::function<void()> function);
    Q_INVOKABLE QVariant returnFromUiThread(std::function<QVariant()> function);

    /// Same design as QMessageBox::critical(), will block, returns result
    static QMessageBox::StandardButton critical(void* ignored, const QString& title, const QString& text,
        QMessageBox::StandardButtons buttons = QMessageBox::Ok,
        QMessageBox::StandardButton defaultButton = QMessageBox::NoButton) {
        return critical(title, text, buttons, defaultButton);
    }
    /// Same design as QMessageBox::information(), will block, returns result
    static QMessageBox::StandardButton information(void* ignored, const QString& title, const QString& text,
        QMessageBox::StandardButtons buttons = QMessageBox::Ok,
        QMessageBox::StandardButton defaultButton = QMessageBox::NoButton) {
        return information(title, text, buttons, defaultButton);
    }
    /// Same design as QMessageBox::question(), will block, returns result
    static QMessageBox::StandardButton question(void* ignored, const QString& title, const QString& text,
        QMessageBox::StandardButtons buttons = QMessageBox::Ok,
        QMessageBox::StandardButton defaultButton = QMessageBox::NoButton) {
        return question(title, text, buttons, defaultButton);
    }
    /// Same design as QMessageBox::warning(), will block, returns result
    static QMessageBox::StandardButton warning(void* ignored, const QString& title, const QString& text,
        QMessageBox::StandardButtons buttons = QMessageBox::Ok,
        QMessageBox::StandardButton defaultButton = QMessageBox::NoButton) {
        return warning(title, text, buttons, defaultButton);
    }

    static QMessageBox::StandardButton critical(const QString& title, const QString& text,
        QMessageBox::StandardButtons buttons = QMessageBox::Ok,
        QMessageBox::StandardButton defaultButton = QMessageBox::NoButton);
    static QMessageBox::StandardButton information(const QString& title, const QString& text,
        QMessageBox::StandardButtons buttons = QMessageBox::Ok,
        QMessageBox::StandardButton defaultButton = QMessageBox::NoButton);
    static QMessageBox::StandardButton question(const QString& title, const QString& text,
        QMessageBox::StandardButtons buttons = QMessageBox::Ok,
        QMessageBox::StandardButton defaultButton = QMessageBox::NoButton);
    static QMessageBox::StandardButton warning(const QString& title, const QString& text,
        QMessageBox::StandardButtons buttons = QMessageBox::Ok,
        QMessageBox::StandardButton defaultButton = QMessageBox::NoButton);

    QMessageBox::StandardButton messageBox(QMessageBox::Icon icon, const QString& title, const QString& text, QMessageBox::StandardButtons buttons, QMessageBox::StandardButton defaultButton);

private:
    QQuickItem* _desktop { nullptr };
    QQuickItem* _toolWindow { nullptr };
};

#endif
