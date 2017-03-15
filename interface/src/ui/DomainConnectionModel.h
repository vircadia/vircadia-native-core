//
//  DomainConnectionModel.h
//
//  Created by Vlad Stelmahovsky
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#ifndef hifi_DomainConnectionModel_h
#define hifi_DomainConnectionModel_h

#include <QAbstractItemModel>
#include <DependencyManager.h>

class DomainConnectionModel : public QAbstractItemModel, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY
public:
    DomainConnectionModel(QAbstractItemModel* parent = nullptr);
    ~DomainConnectionModel();
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QHash<int, QByteArray> roleNames() const override;

    QModelIndex index(int row, int column, const QModelIndex& parent) const override;
    QModelIndex parent(const QModelIndex& child) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;

    enum Roles {
        DisplayNameRole = Qt::UserRole,
        TimestampRole,
        DeltaRole,
        TimeElapsedRole
    };

public slots:
    void refresh();

protected:

private:
};

#endif // hifi_DomainConnectionModel_h
