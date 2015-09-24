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
#include <QScreen>
#include <QTableWidgetItem>
#include <QWindow>

#include <PathUtils.h>

#include "Application.h"
#include "Menu.h"
#include "ScriptsModel.h"
#include "UIUtil.h"

RunningScriptsWidget::RunningScriptsWidget(QWidget* parent) :
    QWidget(parent, Qt::Window | Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowMinMaxButtonsHint |
            Qt::WindowCloseButtonHint),
    ui(new Ui::RunningScriptsWidget),
    _reloadSignalMapper(this),
    _stopSignalMapper(this),
    _scriptsModelFilter(this),
    _scriptsModel(this) {
    ui->setupUi(this);

    setAttribute(Qt::WA_DeleteOnClose, false);

    ui->filterLineEdit->installEventFilter(this);

    connect(&_scriptsModelFilter, &QSortFilterProxyModel::modelReset,
            this, &RunningScriptsWidget::selectFirstInList);

    // FIXME: menu isn't prepared at this point.
    //QString shortcutText = Menu::getInstance()->getActionForOption(MenuOption::ReloadAllScripts)->shortcut().toString(QKeySequence::NativeText);
    //ui->tipLabel->setText("Tip: Use " + shortcutText + " to reload all scripts.");

    _scriptsModelFilter.setSourceModel(&_scriptsModel);
    _scriptsModelFilter.sort(0, Qt::AscendingOrder);
    _scriptsModelFilter.setDynamicSortFilter(true);
    ui->scriptTreeView->setModel(&_scriptsModelFilter);

    connect(ui->filterLineEdit, &QLineEdit::textChanged, this, &RunningScriptsWidget::updateFileFilter);
    connect(ui->scriptTreeView, &QTreeView::doubleClicked, this, &RunningScriptsWidget::loadScriptFromList);

    connect(ui->reloadAllButton, &QPushButton::clicked, Application::getInstance(), &Application::reloadAllScripts);
    connect(ui->stopAllButton, &QPushButton::clicked, this, &RunningScriptsWidget::allScriptsStopped);
    connect(ui->loadScriptFromDiskButton, &QPushButton::clicked, Application::getInstance(), &Application::loadDialog);
    connect(ui->loadScriptFromURLButton, &QPushButton::clicked, Application::getInstance(), &Application::loadScriptURLDialog);
    connect(&_reloadSignalMapper, static_cast<void(QSignalMapper::*)(const QString&)>(&QSignalMapper::mapped),
            Application::getInstance(), &Application::reloadOneScript);

    connect(&_stopSignalMapper, static_cast<void(QSignalMapper::*)(const QString&)>(&QSignalMapper::mapped),
            [](const QString& script) { Application::getInstance()->stopScript(script); });

    UIUtil::scaleWidgetFontSizes(this);
}

RunningScriptsWidget::~RunningScriptsWidget() {
    delete ui;
    _scriptsModel.deleteLater();
}

void RunningScriptsWidget::updateFileFilter(const QString& filter) {
    QRegExp regex("^.*" + QRegExp::escape(filter) + ".*$", Qt::CaseInsensitive);
    _scriptsModelFilter.setFilterRegExp(regex);
    selectFirstInList();
}

void RunningScriptsWidget::loadScriptFromList(const QModelIndex& index) {
    QVariant scriptFile = _scriptsModelFilter.data(index, ScriptsModel::ScriptPath);
    Application::getInstance()->loadScript(scriptFile.toString());
}

void RunningScriptsWidget::loadSelectedScript() {
    QModelIndex selectedIndex = ui->scriptTreeView->currentIndex();
    if (selectedIndex.isValid()) {
        loadScriptFromList(selectedIndex);
    }
}

void RunningScriptsWidget::setRunningScripts(const QStringList& list) {
    setUpdatesEnabled(false);
    QLayoutItem* widget;
    while ((widget = ui->scriptListWidget->layout()->takeAt(0)) != NULL) {
        delete widget->widget();
        delete widget;
    }
    QHash<QString, int> hash;
    const int CLOSE_ICON_HEIGHT = 12;
    for (int i = 0; i < list.size(); i++) {
        if (!hash.contains(list.at(i))) {
            hash.insert(list.at(i), 1);
        }
        QWidget* row = new QWidget(ui->scriptListWidget);
        row->setFont(ui->scriptListWidget->font());
        row->setLayout(new QHBoxLayout(row));

        QUrl url = QUrl(list.at(i));
        QLabel* name = new QLabel(url.fileName(), row);
        if (hash.find(list.at(i)).value() != 1) {
            name->setText(name->text() + "(" + QString::number(hash.find(list.at(i)).value()) + ")");
        }
        ++hash[list.at(i)];

        QPushButton* reloadButton = new QPushButton(row);
        reloadButton->setFlat(true);
        reloadButton->setIcon(
            QIcon(QPixmap(PathUtils::resourcesPath() + "images/reload-script.svg").scaledToHeight(CLOSE_ICON_HEIGHT)));
        reloadButton->setSizePolicy(QSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred));
        reloadButton->setStyleSheet("border: 0;");
        reloadButton->setCursor(Qt::PointingHandCursor);
        connect(reloadButton, SIGNAL(clicked()), &_reloadSignalMapper, SLOT(map()));
        _reloadSignalMapper.setMapping(reloadButton, url.toString());

        QPushButton* closeButton = new QPushButton(row);
        closeButton->setFlat(true);
        closeButton->setIcon(
            QIcon(QPixmap(PathUtils::resourcesPath() + "images/kill-script.svg").scaledToHeight(CLOSE_ICON_HEIGHT)));
        closeButton->setSizePolicy(QSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred));
        closeButton->setStyleSheet("border: 0;");
        closeButton->setCursor(Qt::PointingHandCursor);
        connect(closeButton, SIGNAL(clicked()), &_stopSignalMapper, SLOT(map()));
        _stopSignalMapper.setMapping(closeButton, url.toString());

        row->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));

        row->layout()->setContentsMargins(4, 4, 4, 4);
        row->layout()->setSpacing(0);

        row->layout()->addWidget(name);
        row->layout()->addWidget(reloadButton);
        row->layout()->addWidget(closeButton);

        row->setToolTip(url.toString());

        QFrame* line = new QFrame(row);
        line->setFrameShape(QFrame::HLine);
        line->setStyleSheet("color: #E1E1E1; margin-left: 6px; margin-right: 6px;");

        ui->scriptListWidget->layout()->addWidget(row);
        ui->scriptListWidget->layout()->addWidget(line);
    }


    ui->noRunningScriptsLabel->setVisible(list.isEmpty());
    ui->reloadAllButton->setVisible(!list.isEmpty());
    ui->stopAllButton->setVisible(!list.isEmpty());

    ui->scriptListWidget->updateGeometry();
    setUpdatesEnabled(true);
    Application::processEvents();
    repaint();
}

void RunningScriptsWidget::showEvent(QShowEvent* event) {
    if (!event->spontaneous()) {
        ui->filterLineEdit->setFocus();
    }

    QRect parentGeometry = Application::getInstance()->getDesirableApplicationGeometry();
    int titleBarHeight = UIUtil::getWindowTitleBarHeight(this);
    int topMargin = titleBarHeight;

    setGeometry(parentGeometry.topLeft().x(), parentGeometry.topLeft().y() + topMargin,
                size().width(), parentWidget()->height() - topMargin);

    QWidget::showEvent(event);
}

void RunningScriptsWidget::selectFirstInList() {
    if (_scriptsModelFilter.rowCount() > 0) {
        ui->scriptTreeView->setCurrentIndex(_scriptsModelFilter.index(0, 0));
    }
}

bool RunningScriptsWidget::eventFilter(QObject* sender, QEvent* event) {
    if (sender == ui->filterLineEdit) {
        if (event->type() != QEvent::KeyPress) {
            return false;
        }
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
            QModelIndex selectedIndex = ui->scriptTreeView->currentIndex();
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

void RunningScriptsWidget::allScriptsStopped() {
    Application::getInstance()->stopAllScripts();
}

QVariantList RunningScriptsWidget::getRunning() {
    const int WINDOWS_DRIVE_LETTER_SIZE = 1;
    QVariantList result;
    foreach(const QString& runningScript, Application::getInstance()->getRunningScripts()) {
        QUrl runningScriptURL = QUrl(runningScript);
        if (runningScriptURL.scheme().size() <= WINDOWS_DRIVE_LETTER_SIZE) {
            runningScriptURL = QUrl::fromLocalFile(runningScriptURL.toDisplayString(QUrl::FormattingOptions(QUrl::FullyEncoded)));
        }
        QVariantMap resultNode;
        resultNode.insert("name", runningScriptURL.fileName());
        resultNode.insert("url", runningScriptURL.toDisplayString(QUrl::FormattingOptions(QUrl::FullyEncoded)));
        // The path contains the exact path/URL of the script, which also is used in the stopScript function.
        resultNode.insert("path", runningScript);
        resultNode.insert("local", runningScriptURL.isLocalFile());
        result.append(resultNode);
    }
    return result;
}

QVariantList RunningScriptsWidget::getPublic() {
    return getPublicChildNodes(NULL);
}

QVariantList RunningScriptsWidget::getPublicChildNodes(TreeNodeFolder* parent) {
    QVariantList result;
    QList<TreeNodeBase*> treeNodes = Application::getInstance()->getRunningScriptsWidget()->getScriptsModel()
        ->getFolderNodes(parent);
    for (int i = 0; i < treeNodes.size(); i++) {
        TreeNodeBase* node = treeNodes.at(i);
        if (node->getType() == TREE_NODE_TYPE_FOLDER) {
            TreeNodeFolder* folder = static_cast<TreeNodeFolder*>(node);
            QVariantMap resultNode;
            resultNode.insert("name", node->getName());
            resultNode.insert("type", "folder");
            resultNode.insert("children", getPublicChildNodes(folder));
            result.append(resultNode);
            continue;
        }
        TreeNodeScript* script = static_cast<TreeNodeScript*>(node);
        if (script->getOrigin() == ScriptOrigin::SCRIPT_ORIGIN_LOCAL) {
            continue;
        }
        QVariantMap resultNode;
        resultNode.insert("name", node->getName());
        resultNode.insert("type", "script");
        resultNode.insert("url", script->getFullPath());
        result.append(resultNode);
    }
    return result;
}

QVariantList RunningScriptsWidget::getLocal() {
    QVariantList result;
    QList<TreeNodeBase*> treeNodes = Application::getInstance()->getRunningScriptsWidget()->getScriptsModel()
        ->getFolderNodes(NULL);
    for (int i = 0; i < treeNodes.size(); i++) {
        TreeNodeBase* node = treeNodes.at(i);
        if (node->getType() != TREE_NODE_TYPE_SCRIPT) {
            continue;
        }
        TreeNodeScript* script = static_cast<TreeNodeScript*>(node);
        if (script->getOrigin() != ScriptOrigin::SCRIPT_ORIGIN_LOCAL) {
            continue;
        }
        QVariantMap resultNode;
        resultNode.insert("name", node->getName());
        resultNode.insert("path", script->getFullPath());
        result.append(resultNode);
    }
    return result;
}

bool RunningScriptsWidget::stopScriptByName(const QString& name) {
    foreach (const QString& runningScript, Application::getInstance()->getRunningScripts()) {
        if (QUrl(runningScript).fileName().toLower() == name.trimmed().toLower()) {
            return Application::getInstance()->stopScript(runningScript, false);
        }
    }
    return false;
}

bool RunningScriptsWidget::stopScript(const QString& name, bool restart) {
    return Application::getInstance()->stopScript(name, restart);
}
