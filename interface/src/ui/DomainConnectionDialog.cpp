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

#include <QtCore/QMetaEnum>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QTableWidget>

#include <NodeList.h>
#include <NumericalConstants.h>

#include "DomainConnectionDialog.h"

DomainConnectionDialog::DomainConnectionDialog(QWidget* parent) :
    QDialog(parent, Qt::Window | Qt::WindowCloseButtonHint)
{
    setWindowTitle("Domain Connection Timing");
    setAttribute(Qt::WA_DeleteOnClose);

    // setup a QTableWidget so we can populate it with our values
    QTableWidget* timeTable = new QTableWidget;

    const QStringList TABLE_HEADERS = QStringList() << "Name" << "Timestamp (ms)" << "Delta (ms)" << "Time elapsed (ms)";

    timeTable->setHorizontalHeaderLabels(TABLE_HEADERS);
    timeTable->setColumnCount(TABLE_HEADERS.size());

    // ask the NodeList for the current values for connection times
    QMap<NodeList::ConnectionStep, quint64> times = DependencyManager::get<NodeList>()->getLastConnectionTimes();

    timeTable->setRowCount(times.size());

    // setup our data with the values from the NodeList
    quint64 firstStepTime = times[NodeList::ConnectionStep::LookupAddress] / USECS_PER_MSEC;
    quint64 lastStepTime = firstStepTime;

    int thisRow = 0;

    const QMetaObject &nodeListMeta = NodeList::staticMetaObject;
    QMetaEnum stepEnum = nodeListMeta.enumerator(nodeListMeta.indexOfEnumerator("ConnectionStep"));

    for (int i = 0; i < stepEnum.keyCount(); i++) {
        NodeList::ConnectionStep step = static_cast<NodeList::ConnectionStep>(i);

        if (times.contains(step)) {
            // When did this step occur, how long since the last step, how long since the start?
            quint64 stepTime = times[step] / USECS_PER_MSEC;
            quint64 delta = (stepTime - lastStepTime);
            quint64 elapsed = stepTime - firstStepTime;

            lastStepTime = stepTime;

            // setup the columns for this row in the table
            timeTable->setItem(thisRow, 0, new QTableWidgetItem(stepEnum.valueToKey(i)));
            timeTable->setItem(thisRow, 1, new QTableWidgetItem(QString::number(stepTime)));
            timeTable->setItem(thisRow, 2, new QTableWidgetItem(QString::number(delta)));
            timeTable->setItem(thisRow, 3, new QTableWidgetItem(QString::number(elapsed)));

            ++thisRow;
        }
    }

    QHBoxLayout* hBoxLayout = new QHBoxLayout;
    hBoxLayout->addWidget(timeTable);

    setLayout(hBoxLayout);
}
