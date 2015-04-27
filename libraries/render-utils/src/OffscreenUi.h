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


class OffscreenUi : public OffscreenGlCanvas, public Dependency {
    Q_OBJECT

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
        friend class OffscreenUi;
    };

public:
    using MouseTranslator = std::function<QPointF(const QPointF&)>;
    OffscreenUi();
    virtual ~OffscreenUi();
    void create(QOpenGLContext* context);
    void resize(const QSize& size);
    void load(const QUrl& qmlSource, std::function<void(QQmlContext*)> f = [](QQmlContext*) {});
    void load(const QString& qmlSourceFile, std::function<void(QQmlContext*)> f = [](QQmlContext*) {}) {
        load(QUrl(qmlSourceFile), f);
    }
    void show(const QUrl& url, const QString& name);
    void toggle(const QUrl& url, const QString& name);
    void setBaseUrl(const QUrl& baseUrl);
    void addImportPath(const QString& path);
    QQmlContext* qmlContext();

    void pause();
    void resume();
    bool isPaused() const;
    void setProxyWindow(QWindow* window);
    bool shouldSwallowShortcut(QEvent* event);
    QPointF mapWindowToUi(const QPointF& sourcePosition, QObject* sourceObject);
    virtual bool eventFilter(QObject* originalDestination, QEvent* event);
    void setMouseTranslator(MouseTranslator mouseTranslator) {
        _mouseTranslator = mouseTranslator;
    }


    // Messagebox replacement functions
    using ButtonCallback = std::function<void(QMessageBox::StandardButton)>;
    static ButtonCallback NO_OP_CALLBACK;

    static void messageBox(const QString& title, const QString& text,
        QMessageBox::Icon icon, 
        QMessageBox::StandardButtons buttons, 
        ButtonCallback f);

    static void information(const QString& title, const QString& text,
        QMessageBox::StandardButtons buttons = QMessageBox::Ok,
        ButtonCallback callback = NO_OP_CALLBACK);

    static void question(const QString& title, const QString& text,
        QMessageBox::StandardButtons buttons = QMessageBox::StandardButtons(QMessageBox::Yes | QMessageBox::No),
        ButtonCallback callback = [](QMessageBox::StandardButton) {});

    static void warning(const QString& title, const QString& text,
        QMessageBox::StandardButtons buttons = QMessageBox::Ok,
        ButtonCallback callback = [](QMessageBox::StandardButton) {});

    static void critical(const QString& title, const QString& text,
        QMessageBox::StandardButtons buttons = QMessageBox::Ok,
        ButtonCallback callback = [](QMessageBox::StandardButton) {});

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
    QMyQuickRenderControl* _renderControl{ new QMyQuickRenderControl };
    QQuickWindow* _quickWindow{ nullptr };
    QQmlEngine* _qmlEngine{ nullptr };
    QQmlComponent* _qmlComponent{ nullptr };
    QQuickItem* _rootItem{ nullptr };
    QTimer _updateTimer;
    FboCache _fboCache;
    bool _polish{ true };
    bool _paused{ true };
    MouseTranslator _mouseTranslator{ [](const QPointF& p) { return p;  } };
};

#endif
