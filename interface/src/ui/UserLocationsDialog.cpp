//
//  UserLocationsDialog.cpp
//  interface/src/ui
//
//  Created by Ryan Huffman on 06/24/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QDebug>
#include <QInputDialog>
#include <QPushButton>

#include "Menu.h"
#include "UserLocationsDialog.h"

UserLocationsDialog::UserLocationsDialog(QWidget* parent) :
    QDialog(parent),
    _ui(),
    _proxyModel(this),
    _userLocationsModel(this) {

    _ui.setupUi(this);

    _proxyModel.setSourceModel(&_userLocationsModel);
    _proxyModel.setDynamicSortFilter(true);

    _ui.locationsTreeView->setModel(&_proxyModel);
    _ui.locationsTreeView->setSortingEnabled(true);
    _ui.locationsTreeView->sortByColumn(UserLocationsModel::NameColumn, Qt::AscendingOrder);

    connect(_ui.locationsTreeView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &UserLocationsDialog::updateEnabled);
    connect(&_userLocationsModel, &UserLocationsModel::modelReset, this, &UserLocationsDialog::updateEnabled);
    connect(&_userLocationsModel, &UserLocationsModel::modelReset, &_proxyModel, &QSortFilterProxyModel::invalidate);
    connect(_ui.locationsTreeView, &QTreeView::doubleClicked, this, &UserLocationsDialog::goToModelIndex);

    connect(_ui.deleteButton, &QPushButton::clicked, this, &UserLocationsDialog::deleteSelection);
    connect(_ui.renameButton, &QPushButton::clicked, this, &UserLocationsDialog::renameSelection);
    connect(_ui.refreshButton, &QPushButton::clicked, &_userLocationsModel, &UserLocationsModel::refresh);

    this->setWindowTitle("My Locations");
}

void UserLocationsDialog::updateEnabled() {
    bool enabled = _ui.locationsTreeView->selectionModel()->hasSelection();
    _ui.renameButton->setEnabled(enabled);
    _ui.deleteButton->setEnabled(enabled);
}

void UserLocationsDialog::goToModelIndex(const QModelIndex& index) {
    QVariant location = _proxyModel.data(index.sibling(index.row(), UserLocationsModel::LocationColumn));
    Menu::getInstance()->goToURL(location.toString());
}

void UserLocationsDialog::deleteSelection() {
    QModelIndex selection = _ui.locationsTreeView->selectionModel()->currentIndex();
    selection = _proxyModel.mapToSource(selection);
    if (selection.isValid()) {
        _userLocationsModel.deleteLocation(selection);
    }
}

void UserLocationsDialog::renameSelection() {
    QModelIndex selection = _ui.locationsTreeView->selectionModel()->currentIndex();
    selection = _proxyModel.mapToSource(selection);
    if (selection.isValid()) {
        bool ok;
        QString name = _userLocationsModel.data(selection.sibling(selection.row(), UserLocationsModel::NameColumn)).toString();
        QString newName = QInputDialog::getText(this, "Rename '" + name + "'", "Set name to:", QLineEdit::Normal, name, &ok);
        if (ok && !newName.isEmpty()) {
            _userLocationsModel.renameLocation(selection, newName);
        }
    }
}
