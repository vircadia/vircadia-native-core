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
#include <QFutureWatcher>
#include <QObject>
#include <QWidget>
#include <QSharedPointer>

#include "ui_console.h"
#include "ScriptEngine.h"

const QString CONSOLE_TITLE = "Scripting Console";
const float CONSOLE_WINDOW_OPACITY = 0.95f;
const int CONSOLE_WIDTH = 800;
const int CONSOLE_HEIGHT = 200;

class JSConsole : public QWidget {
    Q_OBJECT
public:
    JSConsole(QWidget* parent, const ScriptEnginePointer& scriptEngine = ScriptEnginePointer());
    ~JSConsole();

    void setScriptEngine(const ScriptEnginePointer& scriptEngine = ScriptEnginePointer());
    void clear();

public slots:
    void executeCommand(const QString& command);

protected:
    void setAndSelectCommand(const QString& command);
    virtual bool eventFilter(QObject* sender, QEvent* event) override;
    virtual void mouseReleaseEvent(QMouseEvent* event) override;
    virtual void showEvent(QShowEvent* event) override;

protected slots:
    void scrollToBottom();
    void resizeTextInput();
    void handlePrint(const QString& message, const QString& scriptName);
    void handleInfo(const QString& message, const QString& scriptName);
    void handleWarning(const QString& message, const QString& scriptName);
    void handleError(const QString& message, const QString& scriptName);
    void commandFinished();

private:
    void appendMessage(const QString& gutter, const QString& message);
    void setToNextCommandInHistory();
    void setToPreviousCommandInHistory();
    void resetCurrentCommandHistory();

    QFutureWatcher<QScriptValue> _executeWatcher;
    Ui::Console* _ui;
    int _currentCommandInHistory;
    QString _savedHistoryFilename;
    QList<QString> _commandHistory;
    QString _rootCommand;
    ScriptEnginePointer _scriptEngine;
    static const QString _consoleFileName;
};


#endif // hifi_JSConsole_h
