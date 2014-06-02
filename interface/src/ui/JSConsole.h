//
//  JSConsole.h
//  interface/src/ui
//
//  Created by Ryan Huffman on 05/12/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_JSConsole_h
#define hifi_JSConsole_h

#include <QDialog>
#include <QEvent>
#include <QObject>
#include <QWidget>

#include "ui_console.h"
#include "ScriptEngine.h"

class JSConsole : public QWidget {
    Q_OBJECT
public:
    JSConsole(QWidget* parent, ScriptEngine* scriptEngine = NULL);
    ~JSConsole();

public slots:
    void executeCommand(const QString& command);

signals:
    void commandExecuting(const QString& command);
    void commandFinished(const QString& result);

protected:
    void setAndSelectCommand(const QString& command);
    virtual bool eventFilter(QObject* sender, QEvent* event);
    virtual void mouseReleaseEvent(QMouseEvent* event);
    virtual void showEvent(QShowEvent* event);

protected slots:
    void scrollToBottom();
    void resizeTextInput();
    void handleEvalutationFinished(QScriptValue result, bool isException);
    void handlePrint(const QString& message);

private:
    void appendMessage(const QString& gutter, const QString& message);
    void setToNextCommandInHistory();
    void setToPreviousCommandInHistory();
    void resetCurrentCommandHistory();

    Ui::Console* _ui;
    int _currentCommandInHistory;
    QList<QString> _commandHistory;
    QString _rootCommand;
    ScriptEngine* _scriptEngine;
};


#endif // hifi_JSConsole_h
