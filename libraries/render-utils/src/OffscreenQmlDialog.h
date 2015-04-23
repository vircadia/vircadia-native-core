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
#include "OffscreenUi.h"

#define QML_DIALOG_DECL \
private: \
    static const QString NAME; \
    static const QUrl QML; \
public: \
    static void registerType(); \
    static void show(); \
    static void toggle(); \
private:

#define QML_DIALOG_DEF(x) \
    const QUrl x::QML = QUrl(#x ".qml"); \
    const QString x::NAME = #x; \
    \
    void x::registerType() { \
        qmlRegisterType<x>("Hifi", 1, 0, NAME.toLocal8Bit().constData()); \
    } \
    \
    void x::show() { \
        auto offscreenUi = DependencyManager::get<OffscreenUi>(); \
        offscreenUi->show(QML, NAME); \
    } \
    \
    void x::toggle() { \
        auto offscreenUi = DependencyManager::get<OffscreenUi>(); \
        offscreenUi->toggle(QML, NAME); \
    }

class OffscreenQmlDialog : public QQuickItem
{
    Q_OBJECT
public:
    OffscreenQmlDialog(QQuickItem* parent = 0);

protected:
    void hide();
};

#endif
