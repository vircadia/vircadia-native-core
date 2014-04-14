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

#include <Application.h>

namespace Ui {
class ScriptEditorWidget;
}

class ScriptEditorWidget : public QDockWidget {
    Q_OBJECT

public:
    ScriptEditorWidget();
    ~ScriptEditorWidget();

private:
    Ui::ScriptEditorWidget* ui;
};

#endif // hifi_ScriptEditorWidget_h
