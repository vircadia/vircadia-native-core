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

#include <QFileInfo>
#include <QKeyEvent>
#include <QPainter>
#include <QTableWidgetItem>

#include "Application.h"

RunningScriptsWidget::RunningScriptsWidget(QWidget* parent) :
    FramelessDialog(parent, 0, POSITION_LEFT),
    ui(new Ui::RunningScriptsWidget) {
    ui->setupUi(this);

    setAllowResize(false);

    ui->hideWidgetButton->setIcon(QIcon(Application::resourcesPath() + "images/close.svg"));
    ui->reloadAllButton->setIcon(QIcon(Application::resourcesPath() + "images/reload.svg"));
    ui->stopAllButton->setIcon(QIcon(Application::resourcesPath() + "images/stop.svg"));
    ui->loadScriptButton->setIcon(QIcon(Application::resourcesPath() + "images/plus-white.svg"));

    _runningScriptsTable = new ScriptsTableWidget(ui->runningScriptsTableWidget);
    _runningScriptsTable->setColumnCount(2);
    _runningScriptsTable->setColumnWidth(0, 245);
    _runningScriptsTable->setColumnWidth(1, 22);
    connect(_runningScriptsTable, &QTableWidget::cellClicked, this, &RunningScriptsWidget::stopScript);

    _recentlyLoadedScriptsTable = new ScriptsTableWidget(ui->recentlyLoadedScriptsTableWidget);
    _recentlyLoadedScriptsTable->setColumnCount(1);
    _recentlyLoadedScriptsTable->setColumnWidth(0, 265);
    connect(_recentlyLoadedScriptsTable, &QTableWidget::cellClicked,
            this, &RunningScriptsWidget::loadScript);

    connect(ui->hideWidgetButton, &QPushButton::clicked,
            Application::getInstance(), &Application::toggleRunningScriptsWidget);
    connect(ui->reloadAllButton, &QPushButton::clicked,
            Application::getInstance(), &Application::reloadAllScripts);
    connect(ui->stopAllButton, &QPushButton::clicked,
            this, &RunningScriptsWidget::allScriptsStopped);
    connect(ui->loadScriptButton, &QPushButton::clicked,
            Application::getInstance(), &Application::loadDialog);
}

RunningScriptsWidget::~RunningScriptsWidget() {
    delete ui;
}

void RunningScriptsWidget::setBoundary(const QRect& rect) {
    _boundary = rect;
}

void RunningScriptsWidget::setRunningScripts(const QStringList& list) {
    _runningScriptsTable->setRowCount(list.size());

    ui->noRunningScriptsLabel->setVisible(list.isEmpty());
    ui->currentlyRunningLabel->setVisible(!list.isEmpty());
    ui->runningScriptsTableWidget->setVisible(!list.isEmpty());
    ui->reloadAllButton->setVisible(!list.isEmpty());
    ui->stopAllButton->setVisible(!list.isEmpty());

    const int CLOSE_ICON_HEIGHT = 12;

    for (int i = 0; i < list.size(); ++i) {
        QTableWidgetItem *scriptName = new QTableWidgetItem;
        scriptName->setText(QFileInfo(list.at(i)).fileName());
        scriptName->setToolTip(list.at(i));
        scriptName->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        QTableWidgetItem *closeIcon = new QTableWidgetItem;
        closeIcon->setIcon(QIcon(QPixmap(Application::resourcesPath() + "images/kill-script.svg").scaledToHeight(CLOSE_ICON_HEIGHT)));

        _runningScriptsTable->setItem(i, 0, scriptName);
        _runningScriptsTable->setItem(i, 1, closeIcon);
    }

    const int RUNNING_SCRIPTS_TABLE_LEFT_MARGIN = 12;
    const int RECENTLY_LOADED_TOP_MARGIN = 61;
    const int RECENTLY_LOADED_LABEL_TOP_MARGIN = 19;

    int y = ui->runningScriptsTableWidget->y() + RUNNING_SCRIPTS_TABLE_LEFT_MARGIN;
    for (int i = 0; i < _runningScriptsTable->rowCount(); ++i) {
        y += _runningScriptsTable->rowHeight(i);
    }

    ui->runningScriptsTableWidget->resize(ui->runningScriptsTableWidget->width(), y - RUNNING_SCRIPTS_TABLE_LEFT_MARGIN);
    _runningScriptsTable->resize(_runningScriptsTable->width(), y - RUNNING_SCRIPTS_TABLE_LEFT_MARGIN);
    ui->reloadAllButton->move(ui->reloadAllButton->x(), y);
    ui->stopAllButton->move(ui->stopAllButton->x(), y);
    ui->recentlyLoadedLabel->move(ui->recentlyLoadedLabel->x(),
                                  ui->stopAllButton->y() + ui->stopAllButton->height() + RECENTLY_LOADED_TOP_MARGIN);
    ui->recentlyLoadedScriptsTableWidget->move(ui->recentlyLoadedScriptsTableWidget->x(),
                                               ui->recentlyLoadedLabel->y() + RECENTLY_LOADED_LABEL_TOP_MARGIN);


    createRecentlyLoadedScriptsTable();
}

void RunningScriptsWidget::keyPressEvent(QKeyEvent *keyEvent) {
    if (keyEvent->key() == Qt::Key_Escape) {
        return;
    } else {
        FramelessDialog::keyPressEvent(keyEvent);
    }
}

void RunningScriptsWidget::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    painter.setPen(QColor::fromRgb(225, 225, 225)); // #e1e1e1

    if (ui->currentlyRunningLabel->isVisible()) {
        // line below the 'Currently Running' label
        painter.drawLine(36, ui->currentlyRunningLabel->y() + ui->currentlyRunningLabel->height(),
                         300, ui->currentlyRunningLabel->y() + ui->currentlyRunningLabel->height());
    }

    if (ui->recentlyLoadedLabel->isVisible()) {
        // line below the 'Recently loaded' label
        painter.drawLine(36, ui->recentlyLoadedLabel->y() + ui->recentlyLoadedLabel->height(),
                         300, ui->recentlyLoadedLabel->y() + ui->recentlyLoadedLabel->height());
    }

    painter.end();
}

void RunningScriptsWidget::scriptStopped(const QString& scriptName) {
    _recentlyLoadedScripts.prepend(scriptName);
}

void RunningScriptsWidget::stopScript(int row, int column) {
    if (column == 1) { // make sure the user has clicked on the close icon
        _lastStoppedScript = _runningScriptsTable->item(row, 0)->toolTip();
        emit stopScriptName(_runningScriptsTable->item(row, 0)->toolTip());
    }
}

void RunningScriptsWidget::loadScript(int row, int column) {
    Application::getInstance()->loadScript(_recentlyLoadedScriptsTable->item(row, column)->toolTip());
}

void RunningScriptsWidget::allScriptsStopped() {
    Application::getInstance()->stopAllScripts();
}

void RunningScriptsWidget::createRecentlyLoadedScriptsTable() {
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
    ui->recentlyLoadedScriptsTableWidget->setVisible(!_recentlyLoadedScripts.isEmpty());
    ui->recentlyLoadedInstruction->setVisible(!_recentlyLoadedScripts.isEmpty());

    int limit = _recentlyLoadedScripts.size() > 9 ? 9 : _recentlyLoadedScripts.size();
    _recentlyLoadedScriptsTable->setRowCount(limit);
    for (int i = 0; i < limit; i++) {
        QTableWidgetItem *scriptName = new QTableWidgetItem;
        scriptName->setText(QString::number(i + 1) + ". " + QFileInfo(_recentlyLoadedScripts.at(i)).fileName());
        scriptName->setToolTip(_recentlyLoadedScripts.at(i));
        scriptName->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);

        _recentlyLoadedScriptsTable->setItem(i, 0, scriptName);
    }

    int y = ui->recentlyLoadedScriptsTableWidget->y() + 15;
    for (int i = 0; i < _recentlyLoadedScriptsTable->rowCount(); ++i) {
        y += _recentlyLoadedScriptsTable->rowHeight(i);
    }

    ui->recentlyLoadedInstruction->setGeometry(36, y,
                                               ui->recentlyLoadedInstruction->width(),
                                               ui->recentlyLoadedInstruction->height());

    repaint();
}
