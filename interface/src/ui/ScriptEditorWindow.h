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

#include <Application.h>

namespace Ui {
class ScriptEditorWindow;
}

class ScriptEditorWindow : public QWidget {
    Q_OBJECT

public:
    ScriptEditorWindow();
    ~ScriptEditorWindow();

private:
    Ui::ScriptEditorWindow* ui;
    void addScriptEditorWidget(QString title);

private slots:
    void loadScriptClicked();
    void newScriptClicked();
    void toggleRunScriptClicked();
    void saveScriptClicked();
};

#endif // hifi_ScriptEditorWindow_h
