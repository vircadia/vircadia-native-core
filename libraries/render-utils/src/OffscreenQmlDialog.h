//
//  OffscreenQmlDialog.h
//
//  Created by Bradley Austin Davis on 2015/04/14
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_OffscreenQmlDialog_h
#define hifi_OffscreenQmlDialog_h

#include <QQuickItem>
#include <5.4.1/QtGui/qpa/qplatformdialoghelper.h>

#include "OffscreenUi.h"

#define QML_DIALOG_DECL \
private: \
    static const QString NAME; \
    static const QUrl QML; \
public: \
    static void registerType(); \
    static void show(std::function<void(QQmlContext*, QQuickItem *)> f = [](QQmlContext*, QQuickItem*) {}); \
    static void toggle(std::function<void(QQmlContext*, QQuickItem *)> f = [](QQmlContext*, QQuickItem*) {}); \
private:

#define QML_DIALOG_DEF(x) \
    const QUrl x::QML = QUrl(#x ".qml"); \
    const QString x::NAME = #x; \
    \
    void x::registerType() { \
        qmlRegisterType<x>("Hifi", 1, 0, NAME.toLocal8Bit().constData()); \
    } \
    \
    void x::show(std::function<void(QQmlContext*, QQuickItem *)> f) { \
        auto offscreenUi = DependencyManager::get<OffscreenUi>(); \
        offscreenUi->show(QML, NAME, f); \
    } \
    \
    void x::toggle(std::function<void(QQmlContext*, QQuickItem *)> f) { \
        auto offscreenUi = DependencyManager::get<OffscreenUi>(); \
        offscreenUi->toggle(QML, NAME, f); \
    }

class OffscreenQmlDialog : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)
    Q_ENUMS(StandardButton)
    Q_FLAGS(StandardButtons)

public:
    OffscreenQmlDialog(QQuickItem* parent = nullptr);
    virtual ~OffscreenQmlDialog();

    enum StandardButton {
        NoButton = QPlatformDialogHelper::NoButton,
        Ok = QPlatformDialogHelper::Ok,
        Save = QPlatformDialogHelper::Save,
        SaveAll = QPlatformDialogHelper::SaveAll,
        Open = QPlatformDialogHelper::Open,
        Yes = QPlatformDialogHelper::Yes,
        YesToAll = QPlatformDialogHelper::YesToAll,
        No = QPlatformDialogHelper::No,
        NoToAll = QPlatformDialogHelper::NoToAll,
        Abort = QPlatformDialogHelper::Abort,
        Retry = QPlatformDialogHelper::Retry,
        Ignore = QPlatformDialogHelper::Ignore,
        Close = QPlatformDialogHelper::Close,
        Cancel = QPlatformDialogHelper::Cancel,
        Discard = QPlatformDialogHelper::Discard,
        Help = QPlatformDialogHelper::Help,
        Apply = QPlatformDialogHelper::Apply,
        Reset = QPlatformDialogHelper::Reset,
        RestoreDefaults = QPlatformDialogHelper::RestoreDefaults,
        NButtons
    };
    Q_DECLARE_FLAGS(StandardButtons, StandardButton)

protected:
    void hide();
    virtual void accept();
    virtual void reject();

public:    
    QString title() const;
    void setTitle(const QString &arg);

signals:
    void accepted();
    void rejected();
    void titleChanged();

private:
    QString _title;

};

Q_DECLARE_OPERATORS_FOR_FLAGS(OffscreenQmlDialog::StandardButtons)

#endif
