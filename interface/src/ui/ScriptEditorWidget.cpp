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

#include "ui_scriptEditorWidget.h"
#include "ScriptEditorWidget.h"

#include <QGridLayout>
#include <QFrame>
#include <QLayoutItem>
#include <QMainWindow>
#include <QMessageBox>
#include <QPalette>
#include <QScrollBar>
#include <QSizePolicy>
#include <QTimer>
#include <QWidget>

#include "Application.h"
#include "ScriptHighlighting.h"

ScriptEditorWidget::ScriptEditorWidget() :
    ui(new Ui::ScriptEditorWidget)
{
    ui->setupUi(this);

    scriptEngine = NULL;

    connect(ui->scriptEdit->document(), SIGNAL(modificationChanged(bool)), this, SIGNAL(scriptModified()));
    connect(ui->scriptEdit->document(), SIGNAL(contentsChanged()), this, SLOT(onScriptModified()));

    // remove the title bar (see the Qt docs on setTitleBarWidget)
    setTitleBarWidget(new QWidget());
    QFontMetrics fm(this->ui->scriptEdit->font());
    this->ui->scriptEdit->setTabStopWidth(fm.width('0') * 4);
    ScriptHighlighting* highlighting = new ScriptHighlighting(this->ui->scriptEdit->document());
    QTimer::singleShot(0, this->ui->scriptEdit, SLOT(setFocus()));
}

ScriptEditorWidget::~ScriptEditorWidget() {
    delete ui;
}

void ScriptEditorWidget::onScriptModified() {
    if(ui->onTheFlyCheckBox->isChecked() && isRunning()) {
        setRunning(false);
        setRunning(true);
    }
}

bool ScriptEditorWidget::isModified() {
    return ui->scriptEdit->document()->isModified();
}

bool ScriptEditorWidget::isRunning() {
    return (scriptEngine != NULL) ? scriptEngine->isRunning() : false;
}

bool ScriptEditorWidget::setRunning(bool run) {
    if (run && !save()) {
        return false;
    }
    // Clean-up old connections.
    disconnect(this, SLOT(onScriptError(const QString&)));
    disconnect(this, SLOT(onScriptPrint(const QString&)));

    if (run) {
        scriptEngine = Application::getInstance()->loadScript(this->currentScript, false);
        connect(scriptEngine, SIGNAL(runningStateChanged()), this, SIGNAL(runningStateChanged()));

        // Make new connections.
        connect(scriptEngine, SIGNAL(errorMessage(const QString&)), this, SLOT(onScriptError(const QString&)));
        connect(scriptEngine, SIGNAL(printedMessage(const QString&)), this, SLOT(onScriptPrint(const QString&)));
    } else {
        Application::getInstance()->stopScript(this->currentScript);
        scriptEngine = NULL;
    }
    return true;
}

bool ScriptEditorWidget::saveFile(const QString &scriptPath) {
     QFile file(scriptPath);
     if (!file.open(QFile::WriteOnly | QFile::Text)) {
         QMessageBox::warning(this, tr("Interface"), tr("Cannot write script %1:\n%2.").arg(scriptPath).arg(file.errorString()));
         return false;
     }

     QTextStream out(&file);
     out << ui->scriptEdit->toPlainText();

     setScriptFile(scriptPath);
     return true;
}

void ScriptEditorWidget::loadFile(const QString &scriptPath) {
    QFile file(scriptPath);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        QMessageBox::warning(this, tr("Interface"), tr("Cannot read script %1:\n%2.").arg(scriptPath).arg(file.errorString()));
        return;
    }

    QTextStream in(&file);
    ui->scriptEdit->setPlainText(in.readAll());

    setScriptFile(scriptPath);

    disconnect(this, SLOT(onScriptError(const QString&)));
    disconnect(this, SLOT(onScriptPrint(const QString&)));

    scriptEngine = Application::getInstance()->getScriptEngine(scriptPath);
    if (scriptEngine != NULL) {
        connect(scriptEngine, SIGNAL(runningStateChanged()), this, SIGNAL(runningStateChanged()));
        connect(scriptEngine, SIGNAL(errorMessage(const QString&)), this, SLOT(onScriptError(const QString&)));
        connect(scriptEngine, SIGNAL(printedMessage(const QString&)), this, SLOT(onScriptPrint(const QString&)));
    }
}

bool ScriptEditorWidget::save() {
    return currentScript.isEmpty() ? saveAs() : saveFile(currentScript);
}

bool ScriptEditorWidget::saveAs() {
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save script"), QString(), tr("Javascript (*.js)"));
    return !fileName.isEmpty() ? saveFile(fileName) : false;
}

void ScriptEditorWidget::setScriptFile(const QString& scriptPath) {
    currentScript = scriptPath;
    ui->scriptEdit->document()->setModified(false);
    setWindowModified(false);

    emit scriptnameChanged();
}

bool ScriptEditorWidget::questionSave() {
    if (ui->scriptEdit->document()->isModified()) {
        QMessageBox::StandardButton button = QMessageBox::warning(this, tr("Interface"), tr("The script has been modified.\nDo you want to save your changes?"),  QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Save);
        return button == QMessageBox::Save ? save() : (button == QMessageBox::Cancel ? false : true);
    }
    return true;
}

void ScriptEditorWidget::onScriptError(const QString& message) {
    ui->debugText->appendPlainText("ERROR: "+ message);
}

void ScriptEditorWidget::onScriptPrint(const QString& message) {
    ui->debugText->appendPlainText("> "+message);
}