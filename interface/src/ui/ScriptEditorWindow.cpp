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

#include <QFileDialog>

#include "ui_scriptEditorWindow.h"
#include "ScriptEditorWindow.h"
#include "ScriptEditorWidget.h"

#include <QGridLayout>
#include <QtCore/QSignalMapper>
#include <QFrame>
#include <QLayoutItem>
#include <QMainWindow>
#include <QMessageBox>
#include <QPalette>
#include <QScrollBar>
#include <QShortcut>
#include <QSizePolicy>
#include <QTimer>
#include <QWidget>

#include <ScriptEngines.h>
#include "Application.h"
#include "PathUtils.h"

ScriptEditorWindow::ScriptEditorWindow(QWidget* parent) :
    QWidget(parent),
    _ScriptEditorWindowUI(new Ui::ScriptEditorWindow),
    _loadMenu(new QMenu),
    _saveMenu(new QMenu)
{
    setAttribute(Qt::WA_DeleteOnClose);

    _ScriptEditorWindowUI->setupUi(this);

    this->setWindowFlags(Qt::Tool);
    addScriptEditorWidget("New script");
    connect(_loadMenu, &QMenu::aboutToShow, this, &ScriptEditorWindow::loadMenuAboutToShow);
    _ScriptEditorWindowUI->loadButton->setMenu(_loadMenu);

    _saveMenu->addAction("Save as..", this, SLOT(saveScriptAsClicked()), Qt::CTRL | Qt::SHIFT | Qt::Key_S);

    _ScriptEditorWindowUI->saveButton->setMenu(_saveMenu);

    connect(new QShortcut(QKeySequence("Ctrl+N"), this), &QShortcut::activated, this, &ScriptEditorWindow::newScriptClicked);
    connect(new QShortcut(QKeySequence("Ctrl+S"), this), &QShortcut::activated, this,&ScriptEditorWindow::saveScriptClicked);
    connect(new QShortcut(QKeySequence("Ctrl+O"), this), &QShortcut::activated, this, &ScriptEditorWindow::loadScriptClicked);
    connect(new QShortcut(QKeySequence("F5"), this), &QShortcut::activated, this, &ScriptEditorWindow::toggleRunScriptClicked);

    _ScriptEditorWindowUI->loadButton->setIcon(QIcon(QPixmap(PathUtils::resourcesPath() + "icons/load-script.svg")));
    _ScriptEditorWindowUI->newButton->setIcon(QIcon(QPixmap(PathUtils::resourcesPath() + "icons/new-script.svg")));
    _ScriptEditorWindowUI->saveButton->setIcon(QIcon(QPixmap(PathUtils::resourcesPath() + "icons/save-script.svg")));
    _ScriptEditorWindowUI->toggleRunButton->setIcon(QIcon(QPixmap(PathUtils::resourcesPath() + "icons/start-script.svg")));
}

ScriptEditorWindow::~ScriptEditorWindow() {
    delete _ScriptEditorWindowUI;
}

void ScriptEditorWindow::setRunningState(bool run) {
    if (_ScriptEditorWindowUI->tabWidget->currentIndex() != -1) {
        static_cast<ScriptEditorWidget*>(_ScriptEditorWindowUI->tabWidget->currentWidget())->setRunning(run);
    }
    this->updateButtons();
}

void ScriptEditorWindow::updateButtons() {
    bool isRunning = _ScriptEditorWindowUI->tabWidget->currentIndex() != -1 &&
        static_cast<ScriptEditorWidget*>(_ScriptEditorWindowUI->tabWidget->currentWidget())->isRunning();
    _ScriptEditorWindowUI->toggleRunButton->setEnabled(_ScriptEditorWindowUI->tabWidget->currentIndex() != -1);
    _ScriptEditorWindowUI->toggleRunButton->setIcon(QIcon(QPixmap(PathUtils::resourcesPath() + ((isRunning ?
        "icons/stop-script.svg" : "icons/start-script.svg")))));
}

void ScriptEditorWindow::loadScriptMenu(const QString& scriptName) {
    addScriptEditorWidget("loading...")->loadFile(scriptName);
    updateButtons();
}

void ScriptEditorWindow::loadScriptClicked() {
    QString scriptName = QFileDialog::getOpenFileName(this, tr("Interface"),
                                                      qApp->getPreviousScriptLocation(),
                                                      tr("JavaScript Files (*.js)"));
    if (!scriptName.isEmpty()) {
        qApp->setPreviousScriptLocation(scriptName);
        addScriptEditorWidget("loading...")->loadFile(scriptName);
        updateButtons();
    }
}

void ScriptEditorWindow::loadMenuAboutToShow() {
    _loadMenu->clear();
    QStringList runningScripts = DependencyManager::get<ScriptEngines>()->getRunningScripts();
    if (runningScripts.count() > 0) {
        QSignalMapper* signalMapper = new QSignalMapper(this);
        foreach (const QString& runningScript, runningScripts) {
            QAction* runningScriptAction = new QAction(runningScript, _loadMenu);
            connect(runningScriptAction, SIGNAL(triggered()), signalMapper, SLOT(map()));
            signalMapper->setMapping(runningScriptAction, runningScript);
            _loadMenu->addAction(runningScriptAction);
        }
        connect(signalMapper, SIGNAL(mapped(const QString &)), this, SLOT(loadScriptMenu(const QString&)));
    } else {
        QAction* naAction = new QAction("(no running scripts)", _loadMenu);
        naAction->setDisabled(true);
        _loadMenu->addAction(naAction);
    }
}

void ScriptEditorWindow::newScriptClicked() {
    addScriptEditorWidget(QString("New script"));
}

void ScriptEditorWindow::toggleRunScriptClicked() {
    this->setRunningState(!(_ScriptEditorWindowUI->tabWidget->currentIndex() !=-1
        && static_cast<ScriptEditorWidget*>(_ScriptEditorWindowUI->tabWidget->currentWidget())->isRunning()));
}

void ScriptEditorWindow::saveScriptClicked() {
    if (_ScriptEditorWindowUI->tabWidget->currentIndex() != -1) {
        ScriptEditorWidget* currentScriptWidget = static_cast<ScriptEditorWidget*>(_ScriptEditorWindowUI->tabWidget
                                                                                   ->currentWidget());
        currentScriptWidget->save();
    }
}

void ScriptEditorWindow::saveScriptAsClicked() {
    if (_ScriptEditorWindowUI->tabWidget->currentIndex() != -1) {
        ScriptEditorWidget* currentScriptWidget = static_cast<ScriptEditorWidget*>(_ScriptEditorWindowUI->tabWidget
                                                                                   ->currentWidget());
        currentScriptWidget->saveAs();
    }
}

ScriptEditorWidget* ScriptEditorWindow::addScriptEditorWidget(QString title) {
    ScriptEditorWidget* newScriptEditorWidget = new ScriptEditorWidget();
    connect(newScriptEditorWidget, &ScriptEditorWidget::scriptnameChanged, this, &ScriptEditorWindow::updateScriptNameOrStatus);
    connect(newScriptEditorWidget, &ScriptEditorWidget::scriptModified, this, &ScriptEditorWindow::updateScriptNameOrStatus);
    connect(newScriptEditorWidget, &ScriptEditorWidget::runningStateChanged, this, &ScriptEditorWindow::updateButtons);
    connect(this, &ScriptEditorWindow::windowActivated, newScriptEditorWidget, &ScriptEditorWidget::onWindowActivated);
    _ScriptEditorWindowUI->tabWidget->addTab(newScriptEditorWidget, title);
    _ScriptEditorWindowUI->tabWidget->setCurrentWidget(newScriptEditorWidget);
    newScriptEditorWidget->setUpdatesEnabled(true);
    newScriptEditorWidget->adjustSize();
    return newScriptEditorWidget;
}

void ScriptEditorWindow::tabSwitched(int tabIndex) {
    this->updateButtons();
    if (_ScriptEditorWindowUI->tabWidget->currentIndex() != -1) {
        ScriptEditorWidget* currentScriptWidget = static_cast<ScriptEditorWidget*>(_ScriptEditorWindowUI->tabWidget
                                                                                   ->currentWidget());
        QString modifiedStar = (currentScriptWidget->isModified() ? "*" : "");
        if (currentScriptWidget->getScriptName().length() > 0) {
            this->setWindowTitle("Script Editor [" + currentScriptWidget->getScriptName() + modifiedStar + "]");
        } else {
            this->setWindowTitle("Script Editor [New script" + modifiedStar + "]");
        }
    } else {
        this->setWindowTitle("Script Editor");
    }
}

void ScriptEditorWindow::tabCloseRequested(int tabIndex) {
    if (ignoreCloseForModal(nullptr)) {
        return;
    }
    ScriptEditorWidget* closingScriptWidget = static_cast<ScriptEditorWidget*>(_ScriptEditorWindowUI->tabWidget
                                                                               ->widget(tabIndex));
    if(closingScriptWidget->questionSave()) {
        _ScriptEditorWindowUI->tabWidget->removeTab(tabIndex);
    }
}

// If this operating system window causes a qml overlay modal dialog (which might not even be seen by the user), closing this window
// will crash the code that was waiting on the dialog result. So that code whousl set inModalDialog to true while the question is up.
// This code will not be necessary when switch out all operating system windows for qml overlays.
bool ScriptEditorWindow::ignoreCloseForModal(QCloseEvent* event) {
    if (!inModalDialog) {
        return false;
    }
    // Deliberately not using OffscreenUi, so that the dialog is seen.
    QMessageBox::information(this, tr("Interface"), tr("There is a modal dialog that must be answered before closing."),
                             QMessageBox::Discard, QMessageBox::Discard);
    if (event) {
        event->ignore(); // don't close
    }
    return true;
}

void ScriptEditorWindow::closeEvent(QCloseEvent *event) {
    if (ignoreCloseForModal(event)) {
        return;
    }
    bool unsaved_docs_warning = false;
    for (int i = 0; i < _ScriptEditorWindowUI->tabWidget->count(); i++){
        if(static_cast<ScriptEditorWidget*>(_ScriptEditorWindowUI->tabWidget->widget(i))->isModified()){
            unsaved_docs_warning = true;
            break;
        }
    }

    if (!unsaved_docs_warning || QMessageBox::warning(this, tr("Interface"),
        tr("There are some unsaved scripts, are you sure you want to close the editor? Changes will be lost!"),
        QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Cancel) == QMessageBox::Discard) {
        event->accept();
    } else {
        event->ignore();
    }
}

void ScriptEditorWindow::updateScriptNameOrStatus() {
    ScriptEditorWidget* source = static_cast<ScriptEditorWidget*>(QObject::sender());
    QString modifiedStar = (source->isModified()? "*" : "");
    if (source->getScriptName().length() > 0) {
        for (int i = 0; i < _ScriptEditorWindowUI->tabWidget->count(); i++){
            if (_ScriptEditorWindowUI->tabWidget->widget(i) == source) {
                _ScriptEditorWindowUI->tabWidget->setTabText(i, modifiedStar + QFileInfo(source->getScriptName()).fileName());
                _ScriptEditorWindowUI->tabWidget->setTabToolTip(i, source->getScriptName());
            }
        }
    }

    if (_ScriptEditorWindowUI->tabWidget->currentWidget() == source) {
        if (source->getScriptName().length() > 0) {
            this->setWindowTitle("Script Editor [" + source->getScriptName() + modifiedStar + "]");
        } else {
            this->setWindowTitle("Script Editor [New script" + modifiedStar + "]");
        }
    }
}

void ScriptEditorWindow::terminateCurrentTab() {
    if (_ScriptEditorWindowUI->tabWidget->currentIndex() != -1) {
        _ScriptEditorWindowUI->tabWidget->removeTab(_ScriptEditorWindowUI->tabWidget->currentIndex());
        this->raise();
    }
}

bool ScriptEditorWindow::autoReloadScripts() {
    return _ScriptEditorWindowUI->autoReloadCheckBox->isChecked();
}

bool ScriptEditorWindow::event(QEvent* event) {
    if (event->type() == QEvent::WindowActivate) {
        emit windowActivated();
    }
    return QWidget::event(event);
}

