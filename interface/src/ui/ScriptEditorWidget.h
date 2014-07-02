//
//  ScriptEditorWidget.h
//  interface/src/ui
//
//  Created by Thijs Wenker on 4/14/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ScriptEditorWidget_h
#define hifi_ScriptEditorWidget_h

#include <QDockWidget>

#include "ScriptEngine.h"

namespace Ui {
    class ScriptEditorWidget;
}

class ScriptEditorWidget : public QDockWidget {
    Q_OBJECT

public:
    ScriptEditorWidget();
    ~ScriptEditorWidget();

    bool isModified();
    bool isRunning();
    bool setRunning(bool run);
    bool saveFile(const QString& scriptPath);
    void loadFile(const QString& scriptPath);
    void setScriptFile(const QString& scriptPath);
    bool save();
    bool saveAs();
    bool questionSave();
    const QString getScriptName() const { return _currentScript; };

signals:
    void runningStateChanged();
    void scriptnameChanged();
    void scriptModified();

public slots:
    void onWindowActivated();

private slots:
    void onScriptError(const QString& message);
    void onScriptPrint(const QString& message);
    void onScriptModified();
    void onScriptFinished(const QString& scriptName);

private:
    Ui::ScriptEditorWidget* _scriptEditorWidgetUI;
    ScriptEngine* _scriptEngine;
    QString _currentScript;
    QDateTime _currentScriptModified;
    bool _isRestarting;
    bool _isReloading;
};

#endif // hifi_ScriptEditorWidget_h
