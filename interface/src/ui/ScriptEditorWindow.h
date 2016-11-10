//
//  ScriptEditorWindow.h
//  interface/src/ui
//
//  Created by Thijs Wenker on 4/14/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ScriptEditorWindow_h
#define hifi_ScriptEditorWindow_h

#include "ScriptEditorWidget.h"

namespace Ui {
    class ScriptEditorWindow;
}

class ScriptEditorWindow : public QWidget {
    Q_OBJECT

public:
    ScriptEditorWindow(QWidget* parent = nullptr);
    ~ScriptEditorWindow();

    void terminateCurrentTab();
    bool autoReloadScripts();

    bool inModalDialog { false };
    bool ignoreCloseForModal(QCloseEvent* event);

signals:
    void windowActivated();

protected:
    void closeEvent(QCloseEvent* event) override;
    virtual bool event(QEvent* event) override;

private:
    Ui::ScriptEditorWindow* _ScriptEditorWindowUI;
    QMenu* _loadMenu;
    QMenu* _saveMenu;

    ScriptEditorWidget* addScriptEditorWidget(QString title);
    void setRunningState(bool run);
    void setScriptName(const QString& scriptName);

private slots:
    void loadScriptMenu(const QString& scriptName);
    void loadScriptClicked();
    void newScriptClicked();
    void toggleRunScriptClicked();
    void saveScriptClicked();
    void saveScriptAsClicked();
    void loadMenuAboutToShow();
    void tabSwitched(int tabIndex);
    void tabCloseRequested(int tabIndex);
    void updateScriptNameOrStatus();
    void updateButtons();
};

#endif // hifi_ScriptEditorWindow_h
