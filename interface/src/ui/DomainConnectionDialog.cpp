//
//  DomainConnectionDialog.cpp
//  interface/src/ui
//
//  Created by Stephen Birarda on 05/26/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "DomainConnectionDialog.h"

#include <QtCore/QMetaEnum>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QTableWidget>

#include <NodeList.h>
#include <NumericalConstants.h>

DomainConnectionDialog::DomainConnectionDialog(QWidget* parent) :
    QDialog(parent, Qt::Window | Qt::WindowCloseButtonHint)
{
    setWindowTitle("Domain Connection Timing");
    setAttribute(Qt::WA_DeleteOnClose);

    // setup a QTableWidget so we can populate it with our values
    QTableWidget* timeTable = new QTableWidget;

    timeTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    const QStringList TABLE_HEADERS = QStringList() << "Name" << "Timestamp (ms)" << "Delta (ms)" << "Time elapsed (ms)";

    timeTable->setColumnCount(TABLE_HEADERS.size());

    // ask the NodeList for the current values for connection times
    QMap<quint64, LimitedNodeList::ConnectionStep> times = DependencyManager::get<NodeList>()->getLastConnectionTimes();

    timeTable->setRowCount(times.size());

    timeTable->setHorizontalHeaderLabels(TABLE_HEADERS);

    // setup our data with the values from the NodeList
    quint64 firstStepTime = times.firstKey() / USECS_PER_MSEC;
    quint64 lastStepTime = firstStepTime;

    int thisRow = 0;

    const QMetaObject &nodeListMeta = NodeList::staticMetaObject;
    QMetaEnum stepEnum = nodeListMeta.enumerator(nodeListMeta.indexOfEnumerator("ConnectionStep"));

    foreach(quint64 timestamp, times.keys()) {
        // When did this step occur, how long since the last step, how long since the start?
        quint64 stepTime = timestamp / USECS_PER_MSEC;
        quint64 delta = (stepTime - lastStepTime);
        quint64 elapsed = stepTime - firstStepTime;

        lastStepTime = stepTime;

        // setup the columns for this row in the table
        int stepIndex = (int) times.value(timestamp);

        timeTable->setItem(thisRow, 0, new QTableWidgetItem(stepEnum.valueToKey(stepIndex)));
        timeTable->setItem(thisRow, 1, new QTableWidgetItem(QString::number(stepTime)));
        timeTable->setItem(thisRow, 2, new QTableWidgetItem(QString::number(delta)));
        timeTable->setItem(thisRow, 3, new QTableWidgetItem(QString::number(elapsed)));

        ++thisRow;
    }

    // setup a horizontal box layout
    QHBoxLayout* hBoxLayout = new QHBoxLayout;
    hBoxLayout->addWidget(timeTable);
    hBoxLayout->setSizeConstraint(QLayout::SetMinimumSize);

    // resize the table columns
    timeTable->resizeColumnsToContents();

    // figure out the size of the table so we can size the dialog
    int tableWidth = timeTable->verticalHeader()->sizeHint().width();
    for (int i = 0; i < timeTable->columnCount(); i++) {
        tableWidth += timeTable->columnWidth(i);
    }

    int tableHeight = timeTable->horizontalHeader()->sizeHint().height();
    for (int i = 0; i < timeTable->rowCount(); i++) {
        tableHeight += timeTable->rowHeight(i);
    }

    // set the minimum size of the table to whatever we got
    timeTable->setMinimumSize(tableWidth, tableHeight);

    setLayout(hBoxLayout);
    adjustSize();
}
