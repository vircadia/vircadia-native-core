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

#include <NetworkAccessManager.h>

#include "Application.h"
#include "ScriptHighlighting.h"

ScriptEditorWidget::ScriptEditorWidget() :
    _scriptEditorWidgetUI(new Ui::ScriptEditorWidget),
    _scriptEngine(NULL),
    _isRestarting(false),
    _isReloading(false)
{
    setAttribute(Qt::WA_DeleteOnClose);

    _scriptEditorWidgetUI->setupUi(this);

    connect(_scriptEditorWidgetUI->scriptEdit->document(), &QTextDocument::modificationChanged, this,
            &ScriptEditorWidget::scriptModified);
    connect(_scriptEditorWidgetUI->scriptEdit->document(), &QTextDocument::contentsChanged, this,
            &ScriptEditorWidget::onScriptModified);

    // remove the title bar (see the Qt docs on setTitleBarWidget)
    setTitleBarWidget(new QWidget());
    QFontMetrics fm(_scriptEditorWidgetUI->scriptEdit->font());
    _scriptEditorWidgetUI->scriptEdit->setTabStopWidth(fm.width('0') * 4);
    // We create a new ScriptHighligting QObject and provide it with a parent so this is NOT a memory leak.
    new ScriptHighlighting(_scriptEditorWidgetUI->scriptEdit->document());
    QTimer::singleShot(0, _scriptEditorWidgetUI->scriptEdit, SLOT(setFocus()));
}

ScriptEditorWidget::~ScriptEditorWidget() {
    delete _scriptEditorWidgetUI;
}

void ScriptEditorWidget::onScriptModified() {
    if(_scriptEditorWidgetUI->onTheFlyCheckBox->isChecked() && isModified() && isRunning() && !_isReloading) {
        _isRestarting = true;
        setRunning(false);
        // Script is restarted once current script instance finishes.
    }
}

void ScriptEditorWidget::onScriptFinished(const QString& scriptPath) {
    _scriptEngine = NULL;
    if (_isRestarting) {
        _isRestarting = false;
        setRunning(true);
    }
}

bool ScriptEditorWidget::isModified() {
    return _scriptEditorWidgetUI->scriptEdit->document()->isModified();
}

bool ScriptEditorWidget::isRunning() {
    return (_scriptEngine != NULL) ? _scriptEngine->isRunning() : false;
}

bool ScriptEditorWidget::setRunning(bool run) {
    if (run && isModified() && !save()) {
        return false;
    }

    if (_scriptEngine != NULL) {
        disconnect(_scriptEngine, &ScriptEngine::runningStateChanged, this, &ScriptEditorWidget::runningStateChanged);
        disconnect(_scriptEngine, &ScriptEngine::errorMessage, this, &ScriptEditorWidget::onScriptError);
        disconnect(_scriptEngine, &ScriptEngine::printedMessage, this, &ScriptEditorWidget::onScriptPrint);
        disconnect(_scriptEngine, &ScriptEngine::update, this, &ScriptEditorWidget::onScriptModified);
        disconnect(_scriptEngine, &ScriptEngine::finished, this, &ScriptEditorWidget::onScriptFinished);
    }

    if (run) {
        const QString& scriptURLString = QUrl(_currentScript).toString();
        _scriptEngine = Application::getInstance()->loadScript(scriptURLString, true, true);
        connect(_scriptEngine, &ScriptEngine::runningStateChanged, this, &ScriptEditorWidget::runningStateChanged);
        connect(_scriptEngine, &ScriptEngine::errorMessage, this, &ScriptEditorWidget::onScriptError);
        connect(_scriptEngine, &ScriptEngine::printedMessage, this, &ScriptEditorWidget::onScriptPrint);
        connect(_scriptEngine, &ScriptEngine::update, this, &ScriptEditorWidget::onScriptModified);
        connect(_scriptEngine, &ScriptEngine::finished, this, &ScriptEditorWidget::onScriptFinished);
    } else {
        connect(_scriptEngine, &ScriptEngine::finished, this, &ScriptEditorWidget::onScriptFinished);
        Application::getInstance()->stopScript(_currentScript);
        _scriptEngine = NULL;
    }
    return true;
}

bool ScriptEditorWidget::saveFile(const QString &scriptPath) {
     QFile file(scriptPath);
     if (!file.open(QFile::WriteOnly | QFile::Text)) {
         QMessageBox::warning(this, tr("Interface"), tr("Cannot write script %1:\n%2.").arg(scriptPath)
             .arg(file.errorString()));
         return false;
     }

     QTextStream out(&file);
     out << _scriptEditorWidgetUI->scriptEdit->toPlainText();
     file.close();

     setScriptFile(scriptPath);
     return true;
}

void ScriptEditorWidget::loadFile(const QString& scriptPath) {
    QUrl url(scriptPath);

    // if the scheme length is one or lower, maybe they typed in a file, let's try
    const int WINDOWS_DRIVE_LETTER_SIZE = 1;
    if (url.scheme().size() <= WINDOWS_DRIVE_LETTER_SIZE) {
        QFile file(scriptPath);
        if (!file.open(QFile::ReadOnly | QFile::Text)) {
            QMessageBox::warning(this, tr("Interface"), tr("Cannot read script %1:\n%2.").arg(scriptPath)
                                                                                         .arg(file.errorString()));
            return;
        }
        QTextStream in(&file);
        _scriptEditorWidgetUI->scriptEdit->setPlainText(in.readAll());
        file.close();
        setScriptFile(scriptPath);

        if (_scriptEngine != NULL) {
            disconnect(_scriptEngine, &ScriptEngine::runningStateChanged, this, &ScriptEditorWidget::runningStateChanged);
            disconnect(_scriptEngine, &ScriptEngine::errorMessage, this, &ScriptEditorWidget::onScriptError);
            disconnect(_scriptEngine, &ScriptEngine::printedMessage, this, &ScriptEditorWidget::onScriptPrint);
            disconnect(_scriptEngine, &ScriptEngine::update, this, &ScriptEditorWidget::onScriptModified);
            disconnect(_scriptEngine, &ScriptEngine::finished, this, &ScriptEditorWidget::onScriptFinished);
        }
    } else {
        QNetworkAccessManager& networkAccessManager = NetworkAccessManager::getInstance();
        QNetworkReply* reply = networkAccessManager.get(QNetworkRequest(url));
        qDebug() << "Downloading included script at" << scriptPath;
        QEventLoop loop;
        QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec();
        _scriptEditorWidgetUI->scriptEdit->setPlainText(reply->readAll());
        if (!saveAs()) {
            static_cast<ScriptEditorWindow*>(this->parent()->parent()->parent())->terminateCurrentTab();
        }
    }

    const QString& scriptURLString = QUrl(_currentScript).toString();
    _scriptEngine = Application::getInstance()->getScriptEngine(scriptURLString);
    if (_scriptEngine != NULL) {
        connect(_scriptEngine, &ScriptEngine::runningStateChanged, this, &ScriptEditorWidget::runningStateChanged);
        connect(_scriptEngine, &ScriptEngine::errorMessage, this, &ScriptEditorWidget::onScriptError);
        connect(_scriptEngine, &ScriptEngine::printedMessage, this, &ScriptEditorWidget::onScriptPrint);
        connect(_scriptEngine, &ScriptEngine::update, this, &ScriptEditorWidget::onScriptModified);
        connect(_scriptEngine, &ScriptEngine::finished, this, &ScriptEditorWidget::onScriptFinished);
    }
}

bool ScriptEditorWidget::save() {
    return _currentScript.isEmpty() ? saveAs() : saveFile(_currentScript);
}

bool ScriptEditorWidget::saveAs() {
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save script"),
                                                    Application::getInstance()->getPreviousScriptLocation(),
                                                    tr("JavaScript Files (*.js)"));
    if (!fileName.isEmpty()) {
        Application::getInstance()->setPreviousScriptLocation(fileName);
        return saveFile(fileName);
    } else {
        return false;
    }
}

void ScriptEditorWidget::setScriptFile(const QString& scriptPath) {
    _currentScript = scriptPath;
    _currentScriptModified = QFileInfo(_currentScript).lastModified();
    _scriptEditorWidgetUI->scriptEdit->document()->setModified(false);
    setWindowModified(false);

    emit scriptnameChanged();
}

bool ScriptEditorWidget::questionSave() {
    if (_scriptEditorWidgetUI->scriptEdit->document()->isModified()) {
        QMessageBox::StandardButton button = QMessageBox::warning(this, tr("Interface"),
            tr("The script has been modified.\nDo you want to save your changes?"), QMessageBox::Save | QMessageBox::Discard |
            QMessageBox::Cancel, QMessageBox::Save);
        return button == QMessageBox::Save ? save() : (button == QMessageBox::Cancel ? false : true);
    }
    return true;
}

void ScriptEditorWidget::onScriptError(const QString& message) {
    _scriptEditorWidgetUI->debugText->appendPlainText("ERROR: " + message);
}

void ScriptEditorWidget::onScriptPrint(const QString& message) {
    _scriptEditorWidgetUI->debugText->appendPlainText("> " + message);
}

void ScriptEditorWidget::onWindowActivated() {
    if (!_isReloading) {
        _isReloading = true;
        
        if (QFileInfo(_currentScript).lastModified() > _currentScriptModified) {
            if (static_cast<ScriptEditorWindow*>(this->parent()->parent()->parent())->autoReloadScripts()
                || QMessageBox::warning(this, _currentScript,
                    tr("This file has been modified outside of the Interface editor.") + "\n\n"
                        + (isModified()
                        ? tr("Do you want to reload it and lose the changes you've made in the Interface editor?")
                        : tr("Do you want to reload it?")),
                    QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
                loadFile(_currentScript);
                if (_scriptEditorWidgetUI->onTheFlyCheckBox->isChecked() && isRunning()) {
                    _isRestarting = true;
                    setRunning(false);
                    // Script is restarted once current script instance finishes.
                }

            }
        }

        _isReloading = false;
    }
}
