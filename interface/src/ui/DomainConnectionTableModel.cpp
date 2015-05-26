//
//  DomainConnectionTableModel.cpp
//  interface/src/ui
//
//  Created by Stephen Birarda on 05/26/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QtCore/QMetaEnum>

#include <NodeList.h>
#include <NumericalConstants.h>

#include "DomainConnectionTableModel.h"

DomainConnectionTableModel::DomainConnectionTableModel(QObject* parent) :
    QAbstractTableModel(parent)
{
    // ask the NodeList for the current values for connection times
    QMap<NodeList::ConnectionStep, quint64> times = DependencyManager::get<NodeList>()->getLastConnectionTimes();

    // setup our data with the returned values

    quint64 totalTime = 0;
    quint64 firstStepTime = times[NodeList::ConnectionStep::LookupAddress] / USECS_PER_MSEC;
    quint64 lastStepTime = firstStepTime;

    const QMetaObject &nodeListMeta = NodeList::staticMetaObject;
    QMetaEnum stepEnum = nodeListMeta.enumerator(nodeListMeta.indexOfEnumerator("ConnectionStep"));

    for (int i = 0; i < stepEnum.keyCount(); i++) {
        NodeList::ConnectionStep step = static_cast<NodeList::ConnectionStep>(i);

        if (times.contains(step)) {
            // When did this step occur, how long since the last step, how long since the start?
            _timestamps[_numRows] = times[step] / USECS_PER_MSEC;
            _deltas[_numRows] = (_timestamps[_numRows] - lastStepTime);
            _totals[_numRows] = _timestamps[_numRows] - firstStepTime;

            // increment the total time by this delta to keep track
            totalTime += _deltas[_numRows];

            lastStepTime = _timestamps[_numRows];

            // increment our counted number of rows
            ++_numRows;
        }
    }
}

QVariant DomainConnectionTableModel::headerData(int section, Qt::Orientation orientation, int role) const {
    switch(section) {
        case 0:
            return QVariant("Name");
        case 1:
            return QVariant("Timestamp (ms)");
        case 2:
            return QVariant("Delta (ms)");
        case 3:
            return QVariant("Total Elapsed (ms)");
        default:
            return QVariant();
    }
}

QVariant DomainConnectionTableModel::data(const QModelIndex& index, int role) const {
    switch(index.column()) {
        case 0:
            return _names[index.row()];
        case 1:
            return QVariant(_timestamps[index.row()]);
        case 2:
            return QVariant(_deltas[index.row()]);
        case 3:
            return QVariant(_totals[index.row()]);
        default:
            return QVariant();
    }
}
