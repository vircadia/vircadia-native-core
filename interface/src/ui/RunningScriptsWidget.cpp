//
//  RunningScriptsWidget.cpp
//  interface/src/ui
//
//  Created by Mohammed Nafees on 03/28/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ui_runningScriptsWidget.h"
#include "RunningScriptsWidget.h"

#include <QKeyEvent>
#include <QTableWidgetItem>

#include "Application.h"

RunningScriptsWidget::RunningScriptsWidget(QDockWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::RunningScriptsWidget)
{
    ui->setupUi(this);

    // remove the title bar (see the Qt docs on setTitleBarWidget)
    setTitleBarWidget(new QWidget());

    ui->runningScriptsTableWidget->setColumnCount(2);
    ui->runningScriptsTableWidget->verticalHeader()->setVisible(false);
    ui->runningScriptsTableWidget->horizontalHeader()->setVisible(false);
    ui->runningScriptsTableWidget->setSelectionMode(QAbstractItemView::NoSelection);
    ui->runningScriptsTableWidget->setShowGrid(false);
    ui->runningScriptsTableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->runningScriptsTableWidget->setColumnWidth(0, 235);
    ui->runningScriptsTableWidget->setColumnWidth(1, 25);
    connect(ui->runningScriptsTableWidget, &QTableWidget::cellClicked, this, &RunningScriptsWidget::stopScript);

    ui->recentlyLoadedScriptsTableWidget->setColumnCount(2);
    ui->recentlyLoadedScriptsTableWidget->verticalHeader()->setVisible(false);
    ui->recentlyLoadedScriptsTableWidget->horizontalHeader()->setVisible(false);
    ui->recentlyLoadedScriptsTableWidget->setSelectionMode(QAbstractItemView::NoSelection);
    ui->recentlyLoadedScriptsTableWidget->setShowGrid(false);
    ui->recentlyLoadedScriptsTableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->recentlyLoadedScriptsTableWidget->setColumnWidth(0, 25);
    ui->recentlyLoadedScriptsTableWidget->setColumnWidth(1, 235);
    connect(ui->recentlyLoadedScriptsTableWidget, &QTableWidget::cellClicked,
            this, &RunningScriptsWidget::loadScript);

    connect(ui->hideWidgetButton, &QPushButton::clicked,
            Application::getInstance(), &Application::toggleRunningScriptsWidget);
    connect(ui->reloadAllButton, &QPushButton::clicked,
            Application::getInstance(), &Application::reloadAllScripts);
    connect(ui->stopAllButton, &QPushButton::clicked,
            this, &RunningScriptsWidget::allScriptsStopped);
}

RunningScriptsWidget::~RunningScriptsWidget()
{
    delete ui;
}

void RunningScriptsWidget::setRunningScripts(const QStringList& list)
{
    ui->runningScriptsTableWidget->setRowCount(list.size());

    ui->noRunningScriptsLabel->setVisible(list.isEmpty());
    ui->currentlyRunningLabel->setVisible(!list.isEmpty());
    ui->line1->setVisible(!list.isEmpty());
    ui->runningScriptsTableWidget->setVisible(!list.isEmpty());
    ui->reloadAllButton->setVisible(!list.isEmpty());
    ui->stopAllButton->setVisible(!list.isEmpty());

    for (int i = 0; i < list.size(); ++i) {
        QTableWidgetItem *scriptName = new QTableWidgetItem;
        scriptName->setText(list.at(i));
        scriptName->setToolTip(list.at(i));
        scriptName->setTextAlignment(Qt::AlignCenter);
        QTableWidgetItem *closeIcon = new QTableWidgetItem;
        closeIcon->setIcon(QIcon(Application::resourcesPath() + "/images/kill-script.svg"));

        ui->runningScriptsTableWidget->setItem(i, 0, scriptName);
        ui->runningScriptsTableWidget->setItem(i, 1, closeIcon);
    }

    createRecentlyLoadedScriptsTable();
}

void RunningScriptsWidget::keyPressEvent(QKeyEvent *e)
{
    switch(e->key()) {
    case Qt::Key_Escape:
        Application::getInstance()->toggleRunningScriptsWidget();
        break;

    case Qt::Key_1:
        if (_recentlyLoadedScripts.size() > 0) {
            Application::getInstance()->loadScript(_recentlyLoadedScripts.at(0));
        }
        break;

    case Qt::Key_2:
        if (_recentlyLoadedScripts.size() > 0 && _recentlyLoadedScripts.size() >= 2) {
            Application::getInstance()->loadScript(_recentlyLoadedScripts.at(1));
        }
        break;

    case Qt::Key_3:
        if (_recentlyLoadedScripts.size() > 0 && _recentlyLoadedScripts.size() >= 3) {
            Application::getInstance()->loadScript(_recentlyLoadedScripts.at(2));
        }
        break;

    case Qt::Key_4:
        if (_recentlyLoadedScripts.size() > 0 && _recentlyLoadedScripts.size() >= 4) {
            Application::getInstance()->loadScript(_recentlyLoadedScripts.at(3));
        }
        break;
    case Qt::Key_5:
        if (_recentlyLoadedScripts.size() > 0 && _recentlyLoadedScripts.size() >= 5) {
            Application::getInstance()->loadScript(_recentlyLoadedScripts.at(4));
        }
        break;

    case Qt::Key_6:
        if (_recentlyLoadedScripts.size() > 0 && _recentlyLoadedScripts.size() >= 6) {
            Application::getInstance()->loadScript(_recentlyLoadedScripts.at(5));
        }
        break;

    case Qt::Key_7:
        if (_recentlyLoadedScripts.size() > 0 && _recentlyLoadedScripts.size() >= 7) {
            Application::getInstance()->loadScript(_recentlyLoadedScripts.at(6));
        }
        break;
    case Qt::Key_8:
        if (_recentlyLoadedScripts.size() > 0 && _recentlyLoadedScripts.size() >= 8) {
            Application::getInstance()->loadScript(_recentlyLoadedScripts.at(7));
        }
        break;

    case Qt::Key_9:
        if (_recentlyLoadedScripts.size() > 0 && _recentlyLoadedScripts.size() >= 9) {
            Application::getInstance()->loadScript(_recentlyLoadedScripts.at(8));
        }
        break;

    default:
        break;
    }
}

void RunningScriptsWidget::stopScript(int row, int column)
{
    if (column == 1) { // make sure the user has clicked on the close icon
        _lastStoppedScript = ui->runningScriptsTableWidget->item(row, 0)->text();
        emit stopScriptName(ui->runningScriptsTableWidget->item(row, 0)->text());
    }
}

void RunningScriptsWidget::loadScript(int row, int column)
{
    Application::getInstance()->loadScript(ui->recentlyLoadedScriptsTableWidget->item(row, column)->text());
}

void RunningScriptsWidget::allScriptsStopped()
{
    QStringList list = Application::getInstance()->getRunningScripts();
    for (int i = 0; i < list.size(); ++i) {
        _recentlyLoadedScripts.prepend(list.at(i));
    }

    Application::getInstance()->stopAllScripts();
}

void RunningScriptsWidget::createRecentlyLoadedScriptsTable()
{
    if (!_recentlyLoadedScripts.contains(_lastStoppedScript) && !_lastStoppedScript.isEmpty()) {
        _recentlyLoadedScripts.prepend(_lastStoppedScript);
        _lastStoppedScript = "";
    }

    for (int i = 0; i < _recentlyLoadedScripts.size(); ++i) {
        if (Application::getInstance()->getRunningScripts().contains(_recentlyLoadedScripts.at(i))) {
            _recentlyLoadedScripts.removeOne(_recentlyLoadedScripts.at(i));
        }
    }

    ui->recentlyLoadedLabel->setVisible(!_recentlyLoadedScripts.isEmpty());
    ui->line2->setVisible(!_recentlyLoadedScripts.isEmpty());
    ui->recentlyLoadedScriptsTableWidget->setVisible(!_recentlyLoadedScripts.isEmpty());
    ui->recentlyLoadedInstruction->setVisible(!_recentlyLoadedScripts.isEmpty());

    int limit = _recentlyLoadedScripts.size() > 9 ? 9 : _recentlyLoadedScripts.size();
    ui->recentlyLoadedScriptsTableWidget->setRowCount(limit);
    for (int i = 0; i < limit; ++i) {
        QTableWidgetItem *scriptName = new QTableWidgetItem;
        scriptName->setText(_recentlyLoadedScripts.at(i));
        scriptName->setToolTip(_recentlyLoadedScripts.at(i));
        scriptName->setTextAlignment(Qt::AlignCenter);
        QTableWidgetItem *number = new QTableWidgetItem;
        number->setText(QString::number(i+1) + ".");

        ui->recentlyLoadedScriptsTableWidget->setItem(i, 0, number);
        ui->recentlyLoadedScriptsTableWidget->setItem(i, 1, scriptName);
    }
}
