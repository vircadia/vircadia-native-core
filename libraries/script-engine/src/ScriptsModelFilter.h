//
//  ScriptsModelFilter.h
//  interface/src
//
//  Created by Thijs Wenker on 01/11/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ScriptsModelFilter_h
#define hifi_ScriptsModelFilter_h

#include "ScriptsModel.h"
#include <QSortFilterProxyModel>

class ScriptsModelFilter : public QSortFilterProxyModel {
    Q_OBJECT
public:
    ScriptsModelFilter(QObject *parent = NULL);
protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;
    bool lessThan(const QModelIndex& left, const QModelIndex& right) const override;
};

#endif // hifi_ScriptsModelFilter_h
