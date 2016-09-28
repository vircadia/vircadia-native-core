//
//  Created by Bradley Austin Davis on 2015-12-15
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ui_QmlWebWindowClass_h
#define hifi_ui_QmlWebWindowClass_h

#include "QmlWindowClass.h"

// FIXME refactor this class to be a QQuickItem derived type and eliminate the needless wrapping 
class QmlWebWindowClass : public QmlWindowClass {
    Q_OBJECT
    Q_PROPERTY(QString url READ getURL CONSTANT)

public:
    static QScriptValue constructor(QScriptContext* context, QScriptEngine* engine);

    public slots:
    QString getURL() const;
    void setURL(const QString& url);

    void emitScriptEvent(const QVariant& scriptMessage);
    void emitWebEvent(const QVariant& webMessage);

signals:
    void urlChanged();
    void scriptEventReceived(const QVariant& message);
    void webEventReceived(const QVariant& message);

protected:
    QString qmlSource() const override { return "QmlWebWindow.qml"; }

private:
    void setKeyboardRaised(QObject* object, bool raised);
};

#endif
