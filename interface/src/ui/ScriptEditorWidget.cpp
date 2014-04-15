//
//  ScriptEditorWidget.cpp
//  interface/src/ui
//
//  Created by Thijs Wenker on 4/14/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QGridLayout>
#include <QFrame>
#include <QLayoutItem>
#include <QMainWindow>
#include <QPalette>
#include <QScrollBar>
#include <QSizePolicy>
#include <QTimer>
#include <QWidget>

#include "Application.h"
#include "ScriptHighlighting.h"
#include "ui_scriptEditorWidget.h"

#include "ScriptEditorWidget.h"

ScriptEditorWidget::ScriptEditorWidget() :
    ui(new Ui::ScriptEditorWidget)
{
    ui->setupUi(this);

    // remove the title bar (see the Qt docs on setTitleBarWidget)
    setTitleBarWidget(new QWidget());
    QFontMetrics fm(this->ui->scriptEdit->font());
    this->ui->scriptEdit->setTabStopWidth(fm.width('0') * 4);
    ScriptHighlighting* highlighting = new ScriptHighlighting(this->ui->scriptEdit->document());
}

ScriptEditorWidget::~ScriptEditorWidget() {
    delete ui;
}