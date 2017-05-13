//
//  ResultsWindow.cpp
//  tools/oven/src/ui
//
//  Created by Stephen Birarda on 4/14/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QtCore/QUrl>
#include <QtGui/QDesktopServices>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QVBoxLayout>

#include "OvenMainWindow.h"

#include "ResultsWindow.h"

ResultsWindow::ResultsWindow(QWidget* parent) :
    QWidget(parent)
{
    // add a title to this window to identify it
    setWindowTitle("High Fidelity Oven - Bake Results");

    // give this dialog the same starting width as the main application window
    resize(FIXED_WINDOW_WIDTH, size().height());

    // have the window delete itself when closed
    setAttribute(Qt::WA_DeleteOnClose);

    setupUI();
}

void ResultsWindow::setupUI() {
    QVBoxLayout* resultsLayout = new QVBoxLayout(this);

    // add a results table to the widget
    _resultsTable = new QTableWidget(0, 2, this);

    // add the header to the table widget
    _resultsTable->setHorizontalHeaderLabels({"File", "Status"});

    // add that table widget to the vertical box layout, so we can make it stretch to the size of the parent
    resultsLayout->insertWidget(0, _resultsTable);

    // make the filename column hold 25% of the total width
    // strech the last column of the table (that holds the results) to fill up the remaining available size
    _resultsTable->horizontalHeader()->resizeSection(0, 0.25 * FIXED_WINDOW_WIDTH);
    _resultsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);

    // make sure we hear about cell clicks so that we can show the output directory for the given row
    connect(_resultsTable, &QTableWidget::cellClicked, this, &ResultsWindow::handleCellClicked);

    // set the layout of this widget to the created layout
    setLayout(resultsLayout);
}

void ResultsWindow::handleCellClicked(int rowIndex, int columnIndex) {
    // make sure this click was on the file/domain being baked
    if (columnIndex == 0) {
        // use QDesktopServices to show the output directory for this row
        auto directory = _outputDirectories[rowIndex];
        QDesktopServices::openUrl(QUrl::fromLocalFile(directory.absolutePath()));
    }
}

int ResultsWindow::addPendingResultRow(const QString& fileName, const QDir& outputDirectory) {
    int rowIndex = _resultsTable->rowCount();

    _resultsTable->insertRow(rowIndex);

    // add a new item for the filename, make it non-editable
    auto fileNameItem = new QTableWidgetItem(fileName);
    fileNameItem->setFlags(fileNameItem->flags() & ~Qt::ItemIsEditable);
    _resultsTable->setItem(rowIndex, 0, fileNameItem);

    auto statusItem = new QTableWidgetItem("Baking...");
    statusItem->setFlags(statusItem->flags() & ~Qt::ItemIsEditable);
    _resultsTable->setItem(rowIndex, 1, statusItem);

    // push an output directory to our list so we can show it if the user clicks on this bake in the results table
    _outputDirectories.push_back(outputDirectory);

    return rowIndex;
}

void ResultsWindow::changeStatusForRow(int rowIndex, const QString& result) {
    const int STATUS_COLUMN = 1;
    auto statusItem = new QTableWidgetItem(result);
    statusItem->setFlags(statusItem->flags() & ~Qt::ItemIsEditable);
    _resultsTable->setItem(rowIndex, STATUS_COLUMN, statusItem);

    // resize the row for the new contents
    _resultsTable->resizeRowToContents(rowIndex);
    // reszie the column for the new contents
    _resultsTable->resizeColumnToContents(STATUS_COLUMN);
}
