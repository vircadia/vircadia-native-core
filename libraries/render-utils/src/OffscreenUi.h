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


#include <QOpenGLContext>
#include <QOffscreenSurface>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QQuickItem>
#include <QQuickWindow>
#include <QQuickRenderControl>
#include <QQuickImageProvider>
#include <GLMHelpers.h>
#include <ThreadHelpers.h>
#include <QQueue>
#include <QTimer>
#include <QApplication>
#include <atomic>
#include <functional>
#include <DependencyManager.h>

#include "FboCache.h"
#include "OffscreenGlCanvas.h"

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
    const QUrl x::QML = #x ".qml"; \
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


class OffscreenUi : public OffscreenGlCanvas, public Dependency {
    Q_OBJECT

    class QMyQuickRenderControl : public QQuickRenderControl {
    protected:
        QWindow * renderWindow(QPoint * offset) Q_DECL_OVERRIDE{
            if (nullptr == _renderWindow) {
                return QQuickRenderControl::renderWindow(offset);
            }
            if (nullptr != offset) {
                offset->rx() = offset->ry() = 0;
            }
            return _renderWindow;
        }

    private:
        QWindow * _renderWindow{ nullptr };
        friend class OffscreenUi;
    };

public:
    using MouseTranslator = std::function < QPointF(const QPointF &) > ;
    OffscreenUi();
    virtual ~OffscreenUi();
    void create(QOpenGLContext * context);
    void resize(const QSize & size);
    void loadQml(const QUrl & qmlSource, std::function<void(QQmlContext*)> f = [](QQmlContext*) {});
    void load(const QUrl & url);
    void show(const QUrl & url, const QString & name);
    void toggle(const QUrl & url, const QString & name);

    QQmlContext * qmlContext();

    void pause();
    void resume();
    bool isPaused() const;
    void setProxyWindow(QWindow * window);
    QPointF mapWindowToUi(const QPointF & p, QObject * dest);
    virtual bool eventFilter(QObject * dest, QEvent * e);
    void setMouseTranslator(MouseTranslator mt) {
        _mouseTranslator = mt;
    }

protected:

private slots:
    void updateQuick();
    void finishQmlLoad();

public slots:
    void requestUpdate();
    void requestRender();
    void lockTexture(int texture);
    void releaseTexture(int texture);

signals:
    void textureUpdated(GLuint texture);

private:
    QMyQuickRenderControl  *_renderControl{ new QMyQuickRenderControl };
    QQuickWindow *_quickWindow{ nullptr };
    QQmlEngine *_qmlEngine{ nullptr };
    QQmlComponent *_qmlComponent{ nullptr };
    QQuickItem * _rootItem{ nullptr };
    QTimer _updateTimer;
    FboCache _fboCache;
    bool _polish{ true };
    bool _paused{ true };
    MouseTranslator _mouseTranslator{ [](const QPointF & p) { return p;  } };
};

#endif