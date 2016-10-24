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
class QQmlContext;

#define HIFI_QML_DECL \
private: \
    static const QString NAME; \
    static const QUrl QML; \
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
    const QUrl x::QML = QUrl(#x ".qml"); \
    const QString x::NAME = #x; \
    \
    void x::registerType() { \
        qmlRegisterType<x>("Hifi", 1, 0, NAME.toLocal8Bit().constData()); \
    } \
    \
    void x::show(std::function<void(QQmlContext*, QObject*)> f) { \
        auto offscreenUi = DependencyManager::get<OffscreenUi>(); \
        offscreenUi->show(QML, NAME, f); \
    } \
    \
    void x::hide() { \
        auto offscreenUi = DependencyManager::get<OffscreenUi>(); \
        offscreenUi->hide(NAME); \
    } \
    \
    void x::toggle(std::function<void(QQmlContext*, QObject*)> f) { \
        auto offscreenUi = DependencyManager::get<OffscreenUi>(); \
        offscreenUi->toggle(QML, NAME, f); \
    } \
    void x::load(std::function<void(QQmlContext*, QObject*)> f) { \
        auto offscreenUi = DependencyManager::get<OffscreenUi>(); \
        offscreenUi->load(QML, f); \
    }

#define HIFI_QML_DEF_LAMBDA(x, f) \
    const QUrl x::QML = QUrl(#x ".qml"); \
    const QString x::NAME = #x; \
    \
    void x::registerType() { \
        qmlRegisterType<x>("Hifi", 1, 0, NAME.toLocal8Bit().constData()); \
    } \
    void x::show() { \
        auto offscreenUi = DependencyManager::get<OffscreenUi>(); \
        offscreenUi->show(QML, NAME, f); \
    } \
    void x::hide() { \
        auto offscreenUi = DependencyManager::get<OffscreenUi>(); \
        offscreenUi->hide(NAME); \
    } \
    \
    void x::toggle() { \
        auto offscreenUi = DependencyManager::get<OffscreenUi>(); \
        offscreenUi->toggle(QML, NAME, f); \
    } \
    void x::load() { \
        auto offscreenUi = DependencyManager::get<OffscreenUi>(); \
        offscreenUi->load(QML, f); \
    }

#endif
