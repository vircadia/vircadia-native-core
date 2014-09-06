//
//  RunningScriptsWidget.cpp
//  interface/src/ui
//
//  Created by Mohammed Nafees on 03/28/2014.
//  Updated by Ryan Huffman on 05/13/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ui_runningScriptsWidget.h"
#include "RunningScriptsWidget.h"

#include <QAbstractProxyModel>
#include <QFileInfo>
#include <QKeyEvent>
#include <QPainter>
#include <QTableWidgetItem>

#include "Application.h"
#include "Menu.h"
#include "ScriptsModel.h"
#include "UIUtil.h"

RunningScriptsWidget::RunningScriptsWidget(QWidget* parent) :
    QWidget(parent, Qt::Window | Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowMinMaxButtonsHint |
            Qt::WindowCloseButtonHint),
    ui(new Ui::RunningScriptsWidget),
    _signalMapper(this),
    _proxyModel(this),
    _scriptsModel(this) {
    ui->setupUi(this);

    setAttribute(Qt::WA_DeleteOnClose, false);

    ui->filterLineEdit->installEventFilter(this);

    connect(&_proxyModel, &QSortFilterProxyModel::modelReset,
            this, &RunningScriptsWidget::selectFirstInList);

    _proxyModel.setSourceModel(&_scriptsModel);
    _proxyModel.sort(0, Qt::AscendingOrder);
    _proxyModel.setDynamicSortFilter(true);
    ui->scriptListView->setModel(&_proxyModel);

    connect(ui->filterLineEdit, &QLineEdit::textChanged, this, &RunningScriptsWidget::updateFileFilter);
    connect(ui->scriptListView, &QListView::doubleClicked, this, &RunningScriptsWidget::loadScriptFromList);

    connect(ui->reloadAllButton, &QPushButton::clicked,
            Application::getInstance(), &Application::reloadAllScripts);
    connect(ui->stopAllButton, &QPushButton::clicked,
            this, &RunningScriptsWidget::allScriptsStopped);
    connect(ui->loadScriptButton, &QPushButton::clicked,
            Application::getInstance(), &Application::loadDialog);
    connect(&_signalMapper, SIGNAL(mapped(QString)), Application::getInstance(), SLOT(stopScript(const QString&)));
}

RunningScriptsWidget::~RunningScriptsWidget() {
    delete ui;
}

void RunningScriptsWidget::updateFileFilter(const QString& filter) {
    QRegExp regex("^.*" + QRegExp::escape(filter) + ".*$", Qt::CaseInsensitive);
    _proxyModel.setFilterRegExp(regex);
    selectFirstInList();
}

void RunningScriptsWidget::loadScriptFromList(const QModelIndex& index) {
    QVariant scriptFile = _proxyModel.data(index, ScriptsModel::ScriptPath);
    Application::getInstance()->loadScript(scriptFile.toString());
}

void RunningScriptsWidget::loadSelectedScript() {
    QModelIndex selectedIndex = ui->scriptListView->currentIndex();
    if (selectedIndex.isValid()) {
        loadScriptFromList(selectedIndex);
    }
}

void RunningScriptsWidget::setBoundary(const QRect& rect) {
    _boundary = rect;
}

void RunningScriptsWidget::setRunningScripts(const QStringList& list) {
    setUpdatesEnabled(false);
    QLayoutItem* widget;
    while ((widget = ui->scrollAreaWidgetContents->layout()->takeAt(0)) != NULL) {
        delete widget->widget();
        delete widget;
    }
    QHash<QString, int> hash;
    const int CLOSE_ICON_HEIGHT = 12;
    for (int i = 0; i < list.size(); i++) {
        if (!hash.contains(list.at(i))) {
            hash.insert(list.at(i), 1);
        }
        QWidget* row = new QWidget(ui->scrollAreaWidgetContents);
        row->setLayout(new QHBoxLayout(row));

        QUrl url = QUrl(list.at(i));
        QLabel* name = new QLabel(url.fileName(), row);
        if (hash.find(list.at(i)).value() != 1) {
            name->setText(name->text() + "(" + QString::number(hash.find(list.at(i)).value()) + ")");
        }
        ++hash[list.at(i)];
        QPushButton* closeButton = new QPushButton(row);
        closeButton->setFlat(true);
        closeButton->setIcon(
            QIcon(QPixmap(Application::resourcesPath() + "images/kill-script.svg").scaledToHeight(CLOSE_ICON_HEIGHT)));
        closeButton->setSizePolicy(QSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred));
        closeButton->setStyleSheet("border: 0;");
        closeButton->setCursor(Qt::PointingHandCursor);

        connect(closeButton, SIGNAL(clicked()), &_signalMapper, SLOT(map()));
        _signalMapper.setMapping(closeButton, url.toString());

        row->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));

        row->layout()->setContentsMargins(4, 4, 4, 4);
        row->layout()->setSpacing(0);

        row->layout()->addWidget(name);
        row->layout()->addWidget(closeButton);

        row->setToolTip(url.toString());

        QFrame* line = new QFrame(row);
        line->setFrameShape(QFrame::HLine);
        line->setStyleSheet("color: #E1E1E1; margin-left: 6px; margin-right: 6px;");

        ui->scrollAreaWidgetContents->layout()->addWidget(row);
        ui->scrollAreaWidgetContents->layout()->addWidget(line);
    }


    ui->noRunningScriptsLabel->setVisible(list.isEmpty());
    ui->reloadAllButton->setVisible(!list.isEmpty());
    ui->stopAllButton->setVisible(!list.isEmpty());

    ui->scrollAreaWidgetContents->updateGeometry();
    setUpdatesEnabled(true);
    Application::processEvents();
    repaint();
}

void RunningScriptsWidget::showEvent(QShowEvent* event) {
    if (!event->spontaneous()) {
        ui->filterLineEdit->setFocus();
    }

    const QRect parentGeometry = parentWidget()->geometry();
    int titleBarHeight = UIUtil::getWindowTitleBarHeight(this);
    int menuBarHeight = Menu::getInstance()->geometry().height();
    int topMargin = titleBarHeight + menuBarHeight;

    setGeometry(parentGeometry.topLeft().x(), parentGeometry.topLeft().y() + topMargin,
                size().width(), parentWidget()->height() - topMargin);

    QWidget::showEvent(event);
}

void RunningScriptsWidget::selectFirstInList() {
    if (_proxyModel.rowCount() > 0) {
        ui->scriptListView->setCurrentIndex(_proxyModel.index(0, 0));
    }
}

bool RunningScriptsWidget::eventFilter(QObject* sender, QEvent* event) {
    if (sender == ui->filterLineEdit) {
        if (event->type() != QEvent::KeyPress) {
            return false;
        }
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
            QModelIndex selectedIndex = ui->scriptListView->currentIndex();
            if (selectedIndex.isValid()) {
                loadScriptFromList(selectedIndex);
            }
            event->accept();
            return true;
        }
        return false;
    }

    return QWidget::eventFilter(sender, event);
}

void RunningScriptsWidget::keyPressEvent(QKeyEvent *keyEvent) {
    if (keyEvent->key() == Qt::Key_Escape) {
        return;
    } else {
        QWidget::keyPressEvent(keyEvent);
    }
}

void RunningScriptsWidget::scriptStopped(const QString& scriptName) {
}

void RunningScriptsWidget::allScriptsStopped() {
    Application::getInstance()->stopAllScripts();
}
