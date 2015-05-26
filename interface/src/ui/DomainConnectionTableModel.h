//
//  DomainConnectionTableModel.h
//  interface/src/ui
//
//  Created by Stephen Birarda on 05/26/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_DomainConnectionTableModel_h
#define hifi_DomainConnectionTableModel_h

#pragma once

#include <QtCore/QAbstractTableModel>

class DomainConnectionTableModel: public QAbstractTableModel {
    Q_OBJECT
public:
    DomainConnectionTableModel(QObject* parent = 0);

    const int NUM_COLUMNS = 4; // name, time, delta, since start

    int rowCount(const QModelIndex& parent = QModelIndex()) const { return _numRows; }
    int columnCount(const QModelIndex& parent = QModelIndex()) const { return NUM_COLUMNS; }

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
private:
    int _numRows = 0;

    QVariantList _names;
    QList<quint64> _timestamps;
    QList<quint64> _deltas;
    QList<quint64> _totals;
};


#endif // hifi_DomainConnectionTableModel_h
