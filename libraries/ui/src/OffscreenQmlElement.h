//
//  OffscreenUi.h
//  interface/src/entities
//
//  Created by Bradley Austin Davis on 2015-07-17
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once
#ifndef hifi_OffscreenQmlElement_h
#define hifi_OffscreenQmlElement_h

#include <QQuickItem>

#include <PathUtils.h>

class QQmlContext;

#define HIFI_QML_DECL \
private: \
    static const QString NAME; \
    static const QUrl QML; \
    static bool registered; \
public: \
    static void registerType(); \
    static void show(std::function<void(QQmlContext*, QObject*)> f = [](QQmlContext*, QObject*) {}); \
    static void hide(); \
    static void toggle(std::function<void(QQmlContext*, QObject*)> f = [](QQmlContext*, QObject*) {}); \
    static void load(std::function<void(QQmlContext*, QObject*)> f = [](QQmlContext*, QObject*) {}); \
private:

#define HIFI_QML_DECL_LAMBDA \
protected: \
    static const QString NAME; \
    static const QUrl QML; \
public: \
    static void registerType(); \
    static void show(); \
    static void hide(); \
    static void toggle(); \
    static void load(); \
private:

#define HIFI_QML_DEF(x) \
    const QUrl x::QML = PathUtils::qmlUrl(#x ".qml"); \
    const QString x::NAME = #x; \
    bool x::registered = false; \
    \
    void x::registerType() { \
        qmlRegisterType<x>("Hifi", 1, 0, NAME.toLocal8Bit().constData()); \
    } \
    \
    void x::show(std::function<void(QQmlContext*, QObject*)> f) { \
        auto offscreenUI = DependencyManager::get<OffscreenUi>(); \
        if (!registered) { \
            x::registerType(); \
        } \
        if (offscreenUI) { \
            offscreenUI->show(QML, NAME, f); \
        } \
    } \
    \
    void x::hide() { \
        if (auto offscreenUI = DependencyManager::get<OffscreenUi>()) { \
            offscreenUI->hide(NAME); \
        } \
    } \
    \
    void x::toggle(std::function<void(QQmlContext*, QObject*)> f) { \
        if (auto offscreenUI = DependencyManager::get<OffscreenUi>()) { \
            offscreenUI->toggle(QML, NAME, f); \
        } \
    } \
    void x::load(std::function<void(QQmlContext*, QObject*)> f) { \
        if (auto offscreenUI = DependencyManager::get<OffscreenUi>()) { \
            offscreenUI->load(QML, f); \
        } \
    }

#define HIFI_QML_DEF_LAMBDA(x, f) \
    const QUrl x::QML = QUrl(#x ".qml"); \
    const QString x::NAME = #x; \
    \
    void x::registerType() { \
        qmlRegisterType<x>("Hifi", 1, 0, NAME.toLocal8Bit().constData()); \
    } \
    void x::show() { \
        if (auto offscreenUI = DependencyManager::get<OffscreenUi>()) { \
            offscreenUI->show(QML, NAME, f); \
        } \
    } \
    void x::hide() { \
        if (auto offscreenUI = DependencyManager::get<OffscreenUi>()) { \
            offscreenUI->hide(NAME); \
        } \
    } \
    \
    void x::toggle() { \
        if (auto offscreenUI = DependencyManager::get<OffscreenUi>()) { \
            offscreenUI->toggle(QML, NAME, f); \
        } \
    } \
    void x::load() { \
        if (auto offscreenUI = DependencyManager::get<OffscreenUi>()) { \
            offscreenUI->load(QML, f); \
        } \
    }

#endif
