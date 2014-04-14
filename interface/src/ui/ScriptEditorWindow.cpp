//
//  ScriptEditorWindow.cpp
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
#include "FlowLayout.h"
#include "ui_scriptEditorWindow.h"
#include "ScriptEditorWidget.h"

#include "ScriptEditorWindow.h"

ScriptEditorWindow::ScriptEditorWindow() :
    ui(new Ui::ScriptEditorWindow)
{
    ui->setupUi(this);
    show();
}

ScriptEditorWindow::~ScriptEditorWindow() {
    delete ui;
}

void ScriptEditorWindow::loadScriptClicked(){

}

void ScriptEditorWindow::newScriptClicked(){
    addScriptEditorWidget(QString("new Script"));
}

void ScriptEditorWindow::toggleRunScriptClicked(){

}

void ScriptEditorWindow::saveScriptClicked(){

}

void ScriptEditorWindow::addScriptEditorWidget(QString title){
    ScriptEditorWidget* newScriptEditorWidget = new ScriptEditorWidget();//ScriptEditorWidget();
    ui->tabWidget->addTab(newScriptEditorWidget, title);
    ui->tabWidget->setCurrentWidget(newScriptEditorWidget);
    newScriptEditorWidget->setUpdatesEnabled(true);
    newScriptEditorWidget->adjustSize();
}