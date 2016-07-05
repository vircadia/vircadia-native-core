//
//  WebWindowClass.h
//  interface/src/scripting
//
//  Created by Ryan Huffman on 11/06/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_WebWindowClass_h
#define hifi_WebWindowClass_h

#include <QScriptContext>
#include <QScriptEngine>
#include <QWebEngineView>

class ScriptEventBridge : public QObject {
    Q_OBJECT
public:
    ScriptEventBridge(QObject* parent = NULL);

public slots:
    void emitWebEvent(const QString& data);
    void emitScriptEvent(const QString& data);

signals:
    void webEventReceived(const QString& data);
    void scriptEventReceived(const QString& data);

};

class WebWindowClass : public QObject {
    Q_OBJECT
    Q_PROPERTY(QObject* eventBridge READ getEventBridge)
    Q_PROPERTY(QString url READ getURL)
    Q_PROPERTY(glm::vec2 position READ getPosition WRITE setPosition);
    Q_PROPERTY(QSizeF size READ getSize WRITE setSize);

public:
    WebWindowClass(const QString& title, const QString& url, int width, int height);
    ~WebWindowClass();

    static QScriptValue constructor(QScriptContext* context, QScriptEngine* engine);

public slots:
    void setVisible(bool visible);
    glm::vec2 getPosition() const;
    void setPosition(int x, int y);
    void setPosition(glm::vec2 position);
    QSizeF getSize() const;
    void setSize(QSizeF size);
    void setSize(int width, int height);
    QString getURL() const { return _webView->url().url(); }
    void setURL(const QString& url);
    void raise();
    ScriptEventBridge* getEventBridge() const { return _eventBridge; }
    void setTitle(const QString& title);

signals:
    void visibilityChanged(bool visible);  // Tool window
    void moved(glm::vec2 position);
    void resized(QSizeF size);
    void closed();

protected:
    virtual bool eventFilter(QObject* sender, QEvent* event) override;

private slots:
    void hasClosed();

private:
    QWidget* _windowWidget;
    QWebEngineView* _webView;
    ScriptEventBridge* _eventBridge;
};

#endif
