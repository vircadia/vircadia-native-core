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

#include <QQmlEngine>
#include <QQmlComponent>
#include <QQuickItem>
#include <QQuickWindow>
#include <QQuickRenderControl>
#include <QQuickImageProvider>
#include <QTimer>
#include <QMessageBox>

#include <atomic>
#include <functional>

#include <GLMHelpers.h>
#include <ThreadHelpers.h>
#include <DependencyManager.h>

#include "OffscreenGlCanvas.h"
#include "FboCache.h"
#include <QQuickItem>

#define HIFI_QML_DECL \
private: \
    static const QString NAME; \
    static const QUrl QML; \
public: \
    static void registerType(); \
    static void show(std::function<void(QQmlContext*, QObject*)> f = [](QQmlContext*, QObject*) {}); \
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
    void x::toggle() { \
        auto offscreenUi = DependencyManager::get<OffscreenUi>(); \
        offscreenUi->toggle(QML, NAME, f); \
    } \
    void x::load() { \
        auto offscreenUi = DependencyManager::get<OffscreenUi>(); \
        offscreenUi->load(QML, f); \
    } 

class OffscreenQmlSurface : public OffscreenGlCanvas {
    Q_OBJECT
protected:
    class QMyQuickRenderControl : public QQuickRenderControl {
    protected:
        QWindow* renderWindow(QPoint* offset) Q_DECL_OVERRIDE{
            if (nullptr == _renderWindow) {
                return QQuickRenderControl::renderWindow(offset);
            }
            if (nullptr != offset) {
                offset->rx() = offset->ry() = 0;
            }
            return _renderWindow;
        }

    private:
        QWindow* _renderWindow{ nullptr };
        friend class OffscreenQmlSurface;
    };
public:
    OffscreenQmlSurface();
    virtual ~OffscreenQmlSurface();

    using MouseTranslator = std::function<QPointF(const QPointF&)>;

    void create(QOpenGLContext* context);
    void resize(const QSize& size);
    QObject* load(const QUrl& qmlSource, std::function<void(QQmlContext*, QObject*)> f = [](QQmlContext*, QObject*) {});
    QObject* load(const QString& qmlSourceFile, std::function<void(QQmlContext*, QObject*)> f = [](QQmlContext*, QObject*) {}) {
        return load(QUrl(qmlSourceFile), f);
    }

    // Optional values for event handling
    void setProxyWindow(QWindow* window);
    void setMouseTranslator(MouseTranslator mouseTranslator) {
        _mouseTranslator = mouseTranslator;
    }

    void pause();
    void resume();
    bool isPaused() const;

    void setBaseUrl(const QUrl& baseUrl);
    QQuickItem* getRootItem();

    virtual bool eventFilter(QObject* originalDestination, QEvent* event);

signals:
    void textureUpdated(GLuint texture);

public slots:
    void requestUpdate();
    void requestRender();
    void lockTexture(int texture);
    void releaseTexture(int texture);

private:
    QObject* finishQmlLoad(std::function<void(QQmlContext*, QObject*)> f);
    QPointF mapWindowToUi(const QPointF& sourcePosition, QObject* sourceObject);

private slots:
    void updateQuick();

protected:
    QQuickWindow* _quickWindow{ nullptr };

private:
    QMyQuickRenderControl* _renderControl{ new QMyQuickRenderControl };
    QQmlEngine* _qmlEngine{ nullptr };
    QQmlComponent* _qmlComponent{ nullptr };
    QQuickItem* _rootItem{ nullptr };
    QTimer _updateTimer;
    FboCache _fboCache;
    bool _polish{ true };
    bool _paused{ true };
    MouseTranslator _mouseTranslator{ [](const QPointF& p) { return p;  } };
};

class OffscreenUi : public OffscreenQmlSurface, public Dependency {
    Q_OBJECT

public:
    OffscreenUi();
    virtual ~OffscreenUi();
    void show(const QUrl& url, const QString& name, std::function<void(QQmlContext*, QObject*)> f = [](QQmlContext*, QObject*) {});
    void toggle(const QUrl& url, const QString& name, std::function<void(QQmlContext*, QObject*)> f = [](QQmlContext*, QObject*) {});
    bool shouldSwallowShortcut(QEvent* event);

    // Messagebox replacement functions
    using ButtonCallback = std::function<void(QMessageBox::StandardButton)>;
    static ButtonCallback NO_OP_CALLBACK;

    static void messageBox(const QString& title, const QString& text,
        ButtonCallback f,
        QMessageBox::Icon icon, 
        QMessageBox::StandardButtons buttons);

    static void information(const QString& title, const QString& text,
        ButtonCallback callback = NO_OP_CALLBACK,
        QMessageBox::StandardButtons buttons = QMessageBox::Ok);

    static void question(const QString& title, const QString& text,
        ButtonCallback callback = NO_OP_CALLBACK,
        QMessageBox::StandardButtons buttons = QMessageBox::StandardButtons(QMessageBox::Yes | QMessageBox::No));

    static void warning(const QString& title, const QString& text,
        ButtonCallback callback = NO_OP_CALLBACK,
        QMessageBox::StandardButtons buttons = QMessageBox::Ok);

    static void critical(const QString& title, const QString& text,
        ButtonCallback callback = NO_OP_CALLBACK,
        QMessageBox::StandardButtons buttons = QMessageBox::Ok);
};

#endif
